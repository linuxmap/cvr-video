#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>

#include <alloca.h>
#include <alsa/asoundlib.h>

#include <libavutil/avstring.h>
#include <libavutil/dict.h>
#include <libavutil/eval.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/parseutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/samplefmt.h>
#ifdef DEBUG
#include <libavutil/avassert.h>
#else
#define av_assert0(...)
#endif
#include <libavcodec/avfft.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>

#include <cvr_ffmpeg_shared.h>
#include <mpp/vpu.h>
#include <mpp/vpu_api.h>
#include "audio/playback/audio_output.h"
#include "rk_rga/rk_rga.h"
#include "ui/cvr/camera_ui_def.h"
#include "ui_resolution.h"
#include "video_ion_alloc.h"
#include "videoplay.h"
#include "vpu.h"

#ifdef DEBUG
#include <execinfo.h>
#include <unwind.h>
#endif
#include <signal.h>

#define STACKSIZE (256 * 1024)

#define USE_MPP_IN_OUTSIDE \
  0  // TODO, need use rga to do copy after decoding one frame completely
#define USE_VPUMEM_DIRECT_RENDER \
  1  // maybe memory dirty display if vpu is writing on it
#define USE_RGA_ROTATION 1

#if USE_MPP_IN_OUTSIDE
#include <mpp/mpp_buffer.h>
#undef USE_VPUMEM_DIRECT_RENDER
#define USE_VPUMEM_DIRECT_RENDER 0
#endif

#if USE_VPUMEM_DIRECT_RENDER
#undef USE_MPP_IN_OUTSIDE
#define USE_MPP_IN_OUTSIDE 0
#endif

#define RK_DEBUG_QUEUE 0
#define RK_DEBUG_FRAME_QUEUE 0
#define RK_DEBUG_CLOCK 0

#if RK_DEBUG_QUEUE
const char video_packet_queue_name[] = "video_packet_queue";
const char audio_packet_queue_name[] = "audio_packet_queue";
#endif

#define MAX_QUEUE_SIZE (1 * 1024 * 1024)
#define MIN_FRAMES 10
#define MAX_FRAMES 60
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

/* Minimum SDL audio buffer size, in samples. */
#define AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio
 * callbacks */
#define AUDIO_MAX_CALLBACKS_PER_SEC 30

#define MIX_MAXVOLUME 128
/* Step size for volume control */
#define VOLUME_STEP (MIX_MAXVOLUME / 50)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to
 * compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 50

/* external clock speed adjustment constants for realtime sources based on
 * buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN 0.900
#define EXTERNAL_CLOCK_SPEED_MAX 1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB 20

/* polls for possible required screen refresh at least this often, should be
 * less than 1/fps */
#define REFRESH_RATE 0.01

typedef int BOOL;

typedef struct MyAVPacketList {
    AVPacket pkt;
    struct MyAVPacketList* next;
    int serial;
} MyAVPacketList;

typedef struct PacketQueue {
    MyAVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    int abort_request;
    int serial;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#if RK_DEBUG_QUEUE
    char* name;  // for debug
#endif
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 3  // >=2
#define SUBPICTURE_QUEUE_SIZE 0     // 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE   \
  FFMAX(SAMPLE_QUEUE_SIZE, \
        FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

#if USE_MPP_IN_OUTSIDE
#define VIDEO_PICTURE_BUFFERD_MPP_SIZE (VIDEO_PICTURE_QUEUE_SIZE + 1)
typedef struct __BufferdMpp {
    MppBuffer buf;
    int bUsing;
} BufferdMpp;
#endif

// typedef struct AudioParams {
//    int freq;
//    int channels;
//    int64_t channel_layout;
//    enum AVSampleFormat fmt;
//    int frame_size;
//    int bytes_per_sec;
//} AudioParams;

typedef struct Clock {
    double pts;       /* clock base */
    double pts_drift; /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial; /* clock is based on a packet with this serial */
    int paused;
    int* queue_serial; /* pointer to the current packet queue serial, used for
                      obsolete clock detection */
#if RK_DEBUG_CLOCK
    char* name;
#endif
} Clock;

#if RK_DEBUG_CLOCK
const char audio_clk_name[] = "audio clock_";
const char video_clk_name[] = "video clock_";
const char ext_clk_name[] = "ext clock_";
void dump_clock(Clock* c)
{
    printf("%s\n", c->name);
    printf("\t\tpts: %f\n", c->pts);
    printf("\t\tpts_drift: %f\n", c->pts_drift);
    printf("\t\tlast_updated: %f\n", c->last_updated);
    printf("\t\tspeed: %f\n", c->speed);
    printf("\t\tserial: %d\n", c->serial);
    printf("\t\tpaused: %d\n", c->paused);
    printf("\t\t*queue_serial: %d\n", *(c->queue_serial));
}
#endif

/* Common struct for handling all types of decoded data and allocated render
 * buffers. */
typedef struct Frame {
    AVFrame* frame;

    int serial;
    double pts;            /* presentation timestamp for the frame */
    double duration;       /* estimated duration of the frame */
    int64_t pos;           /* byte position of the frame in the input file */
    void* vpu_mem_handle;  // vpu mem
    int width;
    int height;
    AVRational sar;
    int frame_drops;  // frame drops early
} Frame;

typedef struct FrameQueue {
    Frame queue[FRAME_QUEUE_SIZE];
    int rindex;
    int windex;
    int size;
    int max_size;
    int keep_last;
    int rindex_shown;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    PacketQueue* pktq;
} FrameQueue;

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct Decoder {
    AVPacket pkt;
    AVPacket pkt_temp;
    PacketQueue* queue;
    AVCodecContext* avctx;
    int pkt_serial;
    int finished;
    int packet_pending;
    pthread_cond_t* empty_queue_cond;
    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    pthread_t decoder_tid;
} Decoder;

typedef struct _rk_Rect {
    int16_t x, y;
    uint16_t w, h;
} rk_Rect;

typedef struct VideoState {
    pthread_t read_tid;
    AVInputFormat* iformat;
    int abort_request;
    int force_refresh;
    int paused;
    int last_paused;
    int queue_attachments_req;
    int seek_req;
    int seek_flags;
    int64_t seek_pos;
    int64_t seek_rel;
    int read_pause_return;
    AVFormatContext* ic;
    int realtime;

    Clock audclk;
    Clock vidclk;
    Clock extclk;

    FrameQueue pictq;
    image_handle pre_pic;   // pre displaying pic
    double pre_queued_pts;  // store the pts pre queued

    FrameQueue sampq;

    Decoder auddec;
    Decoder viddec;

    int viddec_width;
    int viddec_height;

    int audio_stream;

    int av_sync_type;

    double audio_clock;
    int audio_clock_serial;
    double audio_diff_cum; /* used for AV difference average computation */
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    AVStream* audio_st;
    PacketQueue audioq;
    int audio_hw_buf_size;
    uint8_t* audio_buf;
    uint8_t* audio_buf1;
    unsigned int audio_buf_size; /* in bytes */
    unsigned int audio_buf1_size;
    int audio_buf_index; /* in bytes */
    int audio_write_buf_size;
    int audio_volume;
    int muted;
    struct AudioParams audio_src;
    struct AudioParams audio_tgt;
    struct SwrContext* swr_ctx;
    int frame_drops_early;
    int frame_drops_late;

    enum ShowMode {
        SHOW_MODE_NONE = -1,
        SHOW_MODE_VIDEO = 0,
        SHOW_MODE_WAVES,
        SHOW_MODE_RDFT,
        SHOW_MODE_NB
    } show_mode;

    double frame_timer;
    double last_display_time;

    int video_stream;
    AVStream* video_st;
    PacketQueue videoq;
    double max_frame_duration;  // maximum duration of a frame - above this, we
    // consider the jump a timestamp discontinuity

    int eof;

    char* filename;
    int step;

    int last_video_stream, last_audio_stream;

    pthread_cond_t continue_read_thread;
    // snd_pcm_t *pcm_handle;
    WantedAudioParams waparams;
    // uint32_t pcm_samples;
    // snd_pcm_format_t pcm_format;
    pthread_t audio_out_tid;
    uint8_t audio_out_running;

    volatile int pack_read_signaled;
#if USE_MPP_IN_OUTSIDE
    BufferdMpp mpp_bufs[VIDEO_PICTURE_BUFFERD_MPP_SIZE];
    int mpp_buf_index;
    pthread_mutex_t mpp_buf_mutex;
#endif
    int rga_fd;
} VideoState;

static pthread_attr_t global_attr;

/* options specified by the user */
static AVInputFormat* file_iformat = NULL;
static char* input_filename = NULL;
static const char* window_title = NULL;
static int default_width = 640;
static int default_height = 480;
static int audio_disable = 0;
static int video_disable = 0;
static const char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
static int seek_by_bytes = -1;
static int display_disable = 0;
static int show_status = 0;
static int av_sync_type = AV_SYNC_AUDIO_MASTER;
static int64_t start_time = AV_NOPTS_VALUE;
static int64_t duration = AV_NOPTS_VALUE;
static int fast = 0;
static int genpts = 0;
static int lowres = 0;
static int decoder_reorder_pts = -1;
static int autoexit = 1;
static int loop = 1;
static int framedrop = -1;
static int infinite_buffer = -1;
static enum ShowMode show_mode = SHOW_MODE_VIDEO;
static const char* audio_codec_name;
static const char* video_codec_name;
double rdftspeed = 0.02;
int g_max_cache_size = MAX_QUEUE_SIZE;

/* current context */
static int64_t audio_callback_time;
static uint32_t speed = 1, pre_speed = 1;

static AVPacket flush_pkt;

#if USE_MPP_IN_OUTSIDE
static MppBufferGroup g_mpp_buf_group = NULL;
#endif

struct VideoState;
struct sVideoplay_paramate {
    char* filename;
    pthread_t tid;
    char pthread_run;
    struct VideoState* is;
};
struct sVideoplay_paramate* videoplay_paramate = NULL;
void (*videoplay_event_call)(int cmd, void* msg0, void* msg1);

int VIDEOPLAY_RegEventCallback(void (*call)(int cmd, void* msg0, void* msg1))
{
    videoplay_event_call = call;
    return 0;
}

#if USE_MPP_IN_OUTSIDE
static void display(MppBuffer source, size_t buff_size)
{
#else
// static void display(int fd) {
static void display(int rga_fd,
                    int fd,
                    int w,
                    int h,
                    int stride_w,
                    int stride_h,
                    enum AVPixelFormat fmt)
{
#endif
    int ret;
    int out_w, out_h;
    int outdevice;
    struct win* win = NULL;
    int rotate_angle = 0;
    int src_fmt = RGA_FORMAT_YCBCR_420_SP;
    switch (fmt) {
    case AV_PIX_FMT_NV12:
        src_fmt = RGA_FORMAT_YCBCR_420_SP;
        break;
    case AV_PIX_FMT_NV16:
        src_fmt = RGA_FORMAT_YCBCR_422_SP;
        break;
    default:
        printf("%s, unsupport pixel fmt : %d\n", __FILE__, fmt);
        break;
    }
#if USE_MPP_IN_OUTSIDE
    if (!source) {
        return;
    }
#endif
    win = rk_fb_getvideowin();
    if (!win) {
        printf("!rk_fb_getvideowin return NULL\n");
    }
#if USE_MPP_IN_OUTSIDE
#if 0
    if (source) {
        memcpy(win->ion.buffer,
               mpp_buffer_get_ptr(source), buff_size);
    }
#else
    av_assert0(!win->ion.buffer);
    win->buf_ion_fd = mpp_buffer_get_fd(source);
#endif
#else
#if !USE_RGA_ROTATION
    av_assert0(!win->video_ion.buffer);
    win->buf_ion_fd = fd;
    // printf("win: %p, win->buf_ion_fd: %d\n", win, win->buf_ion_fd);
    if (rk_fb_video_disp(win) == -1) {
        printf("rk_fb_video_disp err\n");
    }
#else
    //  printf("display w,h,stridew,strideh [%d, %d, %d, %d]\n",
    //         w, h, stride_w, stride_h);
    outdevice = rk_fb_get_out_device(&out_w, &out_h);
    rotate_angle =
        (((outdevice == OUT_DEVICE_HDMI) || (outdevice == OUT_DEVICE_CVBS_PAL) ||
          (outdevice == OUT_DEVICE_CVBS_NTSC))
         ? 0
         : VIDEO_DISPLAY_ROTATE_ANGLE);

    if (parameter_get_video_flip()) {
        rotate_angle += 180;
        rotate_angle %= 360;
    }

    ret = rk_rga_ionfd_to_ionfd_rotate(rga_fd, fd, w, h, src_fmt, stride_w,
                                       stride_h, win->video_ion.fd, out_w, out_h,
                                       RGA_FORMAT_YCBCR_420_SP, rotate_angle);

    if (0 == ret) {
        if (rk_fb_video_disp(win) == -1) {
            printf("rk_fb_video_disp err\n");
        }
    }
#endif
#endif
}

#if 0
static void realloc_video_win_ion(struct win* win, int width, int height)
{
#if USE_RGA_ROTATION
    int w_align, h_align;
#endif
    if (!win) {
        printf("!!rk_fb_getvideowins return NULL\n");
        return;
    }
    win->buf_width = width;
    win->buf_height = height;
    win->buf_ion_fd = -1;
#if !USE_RGA_ROTATION
    video_ion_free(&win->video_ion);
#else
    w_align = (win->width + 15) & (~15);
    h_align = (win->height + 15) & (~15);
    if (w_align != win->video_ion.width || h_align != win->video_ion.height) {
        video_ion_free(&win->video_ion);
        win->video_ion.client = -1;
        win->video_ion.fd = -1;
        if (video_ion_alloc(&win->video_ion, w_align, h_align)) {
            printf("%s video_ion_alloc fail!\n", __func__);
        }
    }
#endif
}

static void init_video_win(int width, int height)
{
    // struct win* win_write = NULL;
    // struct win* win_disp = NULL;
    // int ret = rk_fb_getvideowins(&win_write, &win_disp);
    // if (!ret) {
    // realloc_video_win_ion(win_write, width, height);
    // realloc_video_win_ion(win_disp, width, height);
    //}
}
#endif

static inline int cmp_audio_fmts(enum AVSampleFormat fmt1,
                                 int64_t channel_count1,
                                 enum AVSampleFormat fmt2,
                                 int64_t channel_count2)
{
    /* If channel count == 1, planar and non-planar formats are the same */
    if (channel_count1 == 1 && channel_count2 == 1)
        return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
    else
        return channel_count1 != channel_count2 || fmt1 != fmt2;
}

static inline int64_t get_valid_channel_layout(int64_t channel_layout,
                                               int channels)
{
    if (channel_layout &&
        av_get_channel_layout_nb_channels(channel_layout) == channels)
        return channel_layout;
    else
        return 0;
}

static void free_picture(Frame* vp);

#if RK_DEBUG_QUEUE
static void dump_packet_queue(PacketQueue* q)
{
    printf("%s:\n", q->name);
    printf("\t\tabort_request\t%d\n", q->abort_request);
    printf("\t\tnb_packets\t%d\n", q->nb_packets);
    printf("\t\tsize\t%d\n", q->size);
    printf("\t\tserials\t%d\n", q->serial);
}
#endif

static int packet_queue_put_private(PacketQueue* q, AVPacket* pkt)
{
    MyAVPacketList* pkt1;

    if (q->abort_request)
        return -1;

    pkt1 = av_malloc(sizeof(MyAVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
    if (pkt == &flush_pkt)
        q->serial++;
    pkt1->serial = q->serial;

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size + sizeof(*pkt1);
#if RK_DEBUG_QUEUE
    printf("packet_queue_put, is_flush_pkt: %d\n", pkt == &flush_pkt);
    dump_packet_queue(q);
#endif
    /* XXX: should duplicate packet data in DV case */
    pthread_cond_signal(&q->cond);
    return 0;
}

static int packet_queue_put(PacketQueue* q, AVPacket* pkt)
{
    int ret;

    pthread_mutex_lock(&q->mutex);
    ret = packet_queue_put_private(q, pkt);
    pthread_mutex_unlock(&q->mutex);

    if (pkt != &flush_pkt && ret < 0)
        av_packet_unref(pkt);

    return ret;
}

static int packet_queue_put_nullpacket(PacketQueue* q, int stream_index)
{
    AVPacket pkt1, *pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = NULL;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return packet_queue_put(q, pkt);
}

/* packet queue handling */
static int packet_queue_init(PacketQueue* q)
{
    int ret = 0;
    memset(q, 0, sizeof(PacketQueue));
    ret = pthread_mutex_init(&q->mutex, NULL);
    // q->mutex = CreateMutex();
    if (ret != 0) {
        av_log(NULL, AV_LOG_FATAL, "CreateMutex(): %s\n", strerror(ret));
        return AVERROR(ENOMEM);
    }
    ret = pthread_cond_init(&q->cond, NULL);
    // q->cond = CreateCond();
    // if (!q->cond) {
    if (0 != ret) {
        av_log(NULL, AV_LOG_FATAL, "CreateCond(): %s\n", strerror(ret));
        return AVERROR(ENOMEM);
    }
    q->abort_request = 1;
    return 0;
}

static void packet_queue_flush(PacketQueue* q)
{
    MyAVPacketList *pkt, *pkt1;

    pthread_mutex_lock(&q->mutex);
    for (pkt = q->first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    pthread_mutex_unlock(&q->mutex);
}

static void packet_queue_destroy(PacketQueue* q)
{
    packet_queue_flush(q);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

static void packet_queue_abort(PacketQueue* q)
{
    pthread_mutex_lock(&q->mutex);

    q->abort_request = 1;

    pthread_cond_signal(&q->cond);

    pthread_mutex_unlock(&q->mutex);
}

static void packet_queue_start(PacketQueue* q)
{
    pthread_mutex_lock(&q->mutex);
    q->abort_request = 0;
    packet_queue_put_private(q, &flush_pkt);
    pthread_mutex_unlock(&q->mutex);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(PacketQueue* q,
                            AVPacket* pkt,
                            int block,
                            int* serial)
{
    MyAVPacketList* pkt1;
    int ret;

    pthread_mutex_lock(&q->mutex);

    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size + sizeof(*pkt1);
            *pkt = pkt1->pkt;
            if (serial)
                *serial = pkt1->serial;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            pthread_cond_wait(&q->cond, &q->mutex);
        }
    }
#if RK_DEBUG_QUEUE
    printf("!packet_queue_get ");
    dump_packet_queue(q);
#endif
    pthread_mutex_unlock(&q->mutex);
    return ret;
}

static void decoder_init(Decoder* d,
                         AVCodecContext* avctx,
                         PacketQueue* queue,
                         pthread_cond_t* empty_queue_cond)
{
    memset(d, 0, sizeof(Decoder));
    d->avctx = avctx;
    d->queue = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts = AV_NOPTS_VALUE;
}

static int decoder_decode_frame(VideoState* is, Decoder* d, AVFrame* frame)
{
    //, AVSubtitle *sub) {
    int got_frame = 0;

    do {
        int ret = -1;
        if (d->queue->abort_request)
            return -1;
        if (!d->packet_pending || d->queue->serial != d->pkt_serial) {
            AVPacket pkt;
            do {
                int nb_packets = is->videoq.nb_packets + is->audioq.nb_packets;
                // if (d->queue->nb_packets == 0) {
                if (nb_packets <= MIN_FRAMES) {
                    if (!__atomic_load_n(&is->pack_read_signaled, __ATOMIC_SEQ_CST) ||
                        nb_packets == 0) {
                        pthread_cond_signal(d->empty_queue_cond);
                        __atomic_store_n(&is->pack_read_signaled, 1, __ATOMIC_SEQ_CST);
                    }
                }

                if (packet_queue_get(d->queue, &pkt, 1, &d->pkt_serial) < 0)
                    return -1;
                if (pkt.data == flush_pkt.data) {
                    avcodec_flush_buffers(d->avctx);
                    d->finished = 0;
                    d->next_pts = d->start_pts;
                    d->next_pts_tb = d->start_pts_tb;
                }
#if RK_DEBUG_QUEUE
                printf(
                    "decoder_decode_frame | %s, d->queue->serial<%d>, "
                    "d->pkt_serial<%d>\n",
                    d->queue->name, d->queue->serial, d->pkt_serial);
#endif
            } while (pkt.data == flush_pkt.data || d->queue->serial != d->pkt_serial);
            av_packet_unref(&d->pkt);
            d->pkt_temp = d->pkt = pkt;
            d->packet_pending = 1;
        }
        switch (d->avctx->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            ret = avcodec_decode_video2(d->avctx, frame, &got_frame, &d->pkt_temp);
#if RK_DEBUG_QUEUE
            printf("avcodec_decode_video2: %d\n", got_frame);
#endif
            if (got_frame) {
                if (decoder_reorder_pts == -1) {
                    frame->pts = av_frame_get_best_effort_timestamp(frame);
                } else if (decoder_reorder_pts) {
                    frame->pts = frame->pkt_pts;
                } else {
                    frame->pts = frame->pkt_dts;
                }
            }
            break;
        case AVMEDIA_TYPE_AUDIO:
            ret = avcodec_decode_audio4(d->avctx, frame, &got_frame, &d->pkt_temp);
            if (got_frame) {
                AVRational tb = (AVRational) {1, frame->sample_rate};
                if (frame->pts != AV_NOPTS_VALUE)
                    frame->pts = av_rescale_q(frame->pts, d->avctx->time_base, tb);
                else if (frame->pkt_pts != AV_NOPTS_VALUE)
                    frame->pts = av_rescale_q(frame->pkt_pts,
                                              av_codec_get_pkt_timebase(d->avctx), tb);
                else if (d->next_pts != AV_NOPTS_VALUE)
                    frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
                if (frame->pts != AV_NOPTS_VALUE) {
                    d->next_pts = frame->pts + frame->nb_samples;
                    d->next_pts_tb = tb;
                }
            }
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            av_assert0(0);
            // ret = avcodec_decode_subtitle2(d->avctx, sub, &got_frame,
            // &d->pkt_temp);
            break;
        default:
            break;
        }

        if (ret < 0) {
            d->packet_pending = 0;
        } else {
            d->pkt_temp.dts = d->pkt_temp.pts = AV_NOPTS_VALUE;
            if (d->pkt_temp.data) {
                if (d->avctx->codec_type != AVMEDIA_TYPE_AUDIO)
                    ret = d->pkt_temp.size;
                d->pkt_temp.data += ret;
                d->pkt_temp.size -= ret;
                if (d->pkt_temp.size <= 0)
                    d->packet_pending = 0;
            } else {
                if (!got_frame) {
                    d->packet_pending = 0;
                    d->finished = d->pkt_serial;
                }
            }
        }
    } while (!got_frame && !d->finished);

    return got_frame;
}

static void decoder_destroy(Decoder* d)
{
    av_packet_unref(&d->pkt);
}

static void frame_queue_unref_item(Frame* vp)
{
    av_frame_unref(vp->frame);
}

static int frame_queue_init(FrameQueue* f,
                            PacketQueue* pktq,
                            int max_size,
                            int keep_last)
{
    int i;
    int ret = 0;
    memset(f, 0, sizeof(FrameQueue));

    ret = pthread_mutex_init(&f->mutex, NULL);
    if (ret != 0) {
        av_log(NULL, AV_LOG_FATAL, "CreateMutex(): %s\n", strerror(ret));
        return AVERROR(ENOMEM);
    }
    ret = pthread_cond_init(&f->cond, NULL);
    if (0 != ret) {
        av_log(NULL, AV_LOG_FATAL, "CreateCond(): %s\n", strerror(ret));
        return AVERROR(ENOMEM);
    }
    f->pktq = pktq;
    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (i = 0; i < f->max_size; i++)
        if (!(f->queue[i].frame = av_frame_alloc()))
            return AVERROR(ENOMEM);
    return 0;
}

static void frame_queue_destory(FrameQueue* f)
{
    int i;
    for (i = 0; i < f->max_size; i++) {
        Frame* vp = &f->queue[i];
        frame_queue_unref_item(vp);
        av_frame_free(&vp->frame);
        free_picture(vp);
    }
    pthread_mutex_destroy(&f->mutex);
    pthread_cond_destroy(&f->cond);
}

static void frame_queue_signal(FrameQueue* f)
{
    pthread_mutex_lock(&f->mutex);
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

static Frame* frame_queue_peek(FrameQueue* f)
{
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static Frame* frame_queue_peek_next(FrameQueue* f)
{
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

static Frame* frame_queue_peek_last(FrameQueue* f)
{
    return &f->queue[f->rindex];
}

static uint8_t frame_queues_is_full(VideoState* is)
{
    FrameQueue* vid_frame_queue = &is->pictq;
    FrameQueue* aud_frame_queue = &is->sampq;
    if ((video_disable || vid_frame_queue->size >= vid_frame_queue->max_size) &&
        (audio_disable || aud_frame_queue->size >= aud_frame_queue->max_size)) {
        return 1;
    }
    return 0;
}

static Frame* frame_queue_peek_writable(FrameQueue* f)
{
#if RK_DEBUG_FRAME_QUEUE
    double now_queue_time = 0;
    double now_wait_time = 0;
    int is_from_wait = 0;
#endif
    /* wait until we have space to put a new frame */
    pthread_mutex_lock(&f->mutex);
#if RK_DEBUG_FRAME_QUEUE
    printf("++peek_writable, f->size: %d, f->max_size: %d\n", f->size,
           f->max_size);
#endif
    while (f->size >= f->max_size && !f->pktq->abort_request) {
#if RK_DEBUG_FRAME_QUEUE
        now_wait_time = av_gettime_relative() / 1000000.0;
        av_log(NULL, AV_LOG_ERROR, "++wait for writable, now time: %f\n",
               now_wait_time);
#endif
        pthread_cond_wait(&f->cond, &f->mutex);
#if RK_DEBUG_FRAME_QUEUE
        is_from_wait = 1;
#endif
    }
    pthread_mutex_unlock(&f->mutex);

    if (f->pktq->abort_request)
        return NULL;
#if RK_DEBUG_FRAME_QUEUE
    now_queue_time = av_gettime_relative() / 1000000.0;
    if (is_from_wait) {
        av_log(NULL, AV_LOG_ERROR,
               "++write index :%d, now time: %f<duration: %f>\n", f->windex,
               now_queue_time, now_queue_time - now_wait_time);
        is_from_wait = 0;
    } else {
        printf("++write index :%d, now time: %f\n", f->windex, now_queue_time);
    }
#endif
    return &f->queue[f->windex];
}

static Frame* frame_queue_peek_readable(FrameQueue* f)
{
#if RK_DEBUG_FRAME_QUEUE
    double now_queue_time = 0;
    double now_wait_time = 0;
    int is_from_wait = 0;
#endif
    /* wait until we have a readable a new frame */
    pthread_mutex_lock(&f->mutex);
#if RK_DEBUG_FRAME_QUEUE
    now_wait_time = av_gettime_relative() / 1000000.0;
    printf("--peek_readable, f->size: %d, f->rindex_shown: %d\n", f->size,
           f->rindex_shown);
#endif
    while (f->size - f->rindex_shown <= 0 && !f->pktq->abort_request) {
#if RK_DEBUG_FRAME_QUEUE
        av_log(NULL, AV_LOG_ERROR, "--wait for readable, now time: %f\n",
               now_wait_time);
#endif
        pthread_cond_wait(&f->cond, &f->mutex);
#if RK_DEBUG_FRAME_QUEUE
        is_from_wait = 1;
#endif
    }
    pthread_mutex_unlock(&f->mutex);

    if (f->pktq->abort_request)
        return NULL;
#if RK_DEBUG_FRAME_QUEUE
    now_queue_time = av_gettime_relative() / 1000000.0;
    if (is_from_wait) {
        av_log(NULL, AV_LOG_ERROR, "++read index :%d, now time: %f<duration: %f>\n",
               (f->rindex + f->rindex_shown) % f->max_size, now_queue_time,
               now_queue_time, now_queue_time - now_wait_time);
        is_from_wait = 0;
    } else {
        printf("++read index :%d, now time: %f\n",
               (f->rindex + f->rindex_shown) % f->max_size, now_queue_time);
    }
#endif
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

static void frame_queue_push(FrameQueue* f)
{
    if (++f->windex == f->max_size)
        f->windex = 0;
    pthread_mutex_lock(&f->mutex);
    f->size++;
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

static void frame_queue_next(FrameQueue* f)
{
    if (f->keep_last && !f->rindex_shown) {
        f->rindex_shown = 1;
        return;
    }
    frame_queue_unref_item(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size)
        f->rindex = 0;
    pthread_mutex_lock(&f->mutex);
    ;
    f->size--;
    pthread_cond_signal(&f->cond);
    pthread_mutex_unlock(&f->mutex);
}

/* jump back to the previous frame if available by resetting rindex_shown */
static int frame_queue_prev(FrameQueue* f)
{
    int ret = f->rindex_shown;
    f->rindex_shown = 0;
    return ret;
}

/* return the number of undisplayed frames in the queue */
static int frame_queue_nb_remaining(FrameQueue* f)
{
    return f->size - f->rindex_shown;
}

/* return last shown position */
static int64_t frame_queue_last_pos(FrameQueue* f)
{
    Frame* fp = &f->queue[f->rindex];
    if (f->rindex_shown && fp->serial == f->pktq->serial)
        return fp->pos;
    else
        return -1;
}

static void decoder_abort(Decoder* d, FrameQueue* fq)
{
    packet_queue_abort(d->queue);
    frame_queue_signal(fq);
    pthread_join(d->decoder_tid, NULL);
    // d->decoder_tid = NULL;
    packet_queue_flush(d->queue);
}

#if USE_VPUMEM_DIRECT_RENDER
static void free_image_from_vpu(void* mem_handle)
{
    image_handle handle = (image_handle)mem_handle;
    if (handle) {
        if (handle->free_func) {
            handle->free_func(handle->rkdec_ctx, &handle->image_buf);
        }
        free(handle);
    }
}
#endif

static void free_picture(Frame* vp)
{
    if (show_status)
        printf("in %s\n", __FUNCTION__);
#if USE_VPUMEM_DIRECT_RENDER
    free_image_from_vpu(vp->vpu_mem_handle);
    vp->vpu_mem_handle = NULL;
#endif
}

static void calculate_display_rect(rk_Rect* rect,
                                   int scr_xleft,
                                   int scr_ytop,
                                   int scr_width,
                                   int scr_height,
                                   int pic_width,
                                   int pic_height,
                                   AVRational pic_sar)
{
    float aspect_ratio;
    int width, height, x, y;

    if (pic_sar.num == 0)
        aspect_ratio = 0;
    else
        aspect_ratio = av_q2d(pic_sar);

    if (aspect_ratio <= 0.0)
        aspect_ratio = 1.0;
    aspect_ratio *= (float)pic_width / (float)pic_height;

    /* XXX: we suppose the screen has a 1.0 pixel ratio */
    height = scr_height;
    width = lrint(height * aspect_ratio) & ~1;
    if (width > scr_width) {
        width = scr_width;
        height = lrint(width / aspect_ratio) & ~1;
    }
    x = (scr_width - width) / 2;
    y = (scr_height - height) / 2;
    rect->x = scr_xleft + x;
    rect->y = scr_ytop + y;
    rect->w = FFMAX(width, 1);
    rect->h = FFMAX(height, 1);
}

static void video_image_display(VideoState* is)
{
    Frame* vp;
    vp = frame_queue_peek(&is->pictq);
    if (show_status) {
        time_t t;
        char tmpBuf[255];
        AVFormatContext* ic = is->ic;
        t = time(0);
        strftime(tmpBuf, 255, "%Y-%m-%d-%H-%M:%S", localtime(&t));
        fprintf(stderr, "in video_image_display, %s\n", tmpBuf);
        // cur duration
        printf("vp->pts = %lf\n", vp->pts);

        // total duration
        if (ic->duration != AV_NOPTS_VALUE) {
            int hours, mins, secs, us;
            int64_t duration = ic->duration + 5000;
            secs = duration / AV_TIME_BASE;
            us = duration % AV_TIME_BASE;
            mins = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;
            fprintf(stderr, "%02d:%02d:%02d.%02d\n", hours, mins, secs,
                    (100 * us) / AV_TIME_BASE);
        }
    }
    if (vp->vpu_mem_handle) {
#if USE_MPP_IN_OUTSIDE
        BufferdMpp* mpp = (BufferdMpp*)vp->vpu_mem_handle;
        av_assert0(mpp->bUsing);
        pthread_mutex_lock(&is->mpp_buf_mutex);
        display(mpp->buf, vp->width * vp->height * 3 / 2);
        mpp->bUsing = 0;
        pthread_mutex_unlock(&is->mpp_buf_mutex);
#else
        image_handle handle = (image_handle)vp->vpu_mem_handle;
        if (handle) {
            if (handle->image_buf.phy_fd > 0) {
                AVCodecContext* v_codec_ctx = is->viddec.avctx;
                display(is->rga_fd, handle->image_buf.phy_fd, vp->width, vp->height,
                        v_codec_ctx->coded_width, v_codec_ctx->coded_height,
                        v_codec_ctx->pix_fmt);
            }
            if (is->pre_pic) {
                free_image_from_vpu(is->pre_pic);
            }
            is->pre_pic = handle;
            vp->vpu_mem_handle = NULL;
        }
#endif
    }
}

static void stream_component_close(VideoState* is, int stream_index)
{
    AVFormatContext* ic = is->ic;
    AVCodecContext* avctx;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return;
    avctx = ic->streams[stream_index]->codec;

    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        is->audio_out_running = 0;
        decoder_abort(&is->auddec, &is->sampq);
        fprintf(stderr, "CloseAudio\n");
        pthread_join(is->audio_out_tid, NULL);
        //        if (is->pcm_handle) {
        //            snd_pcm_drain(is->pcm_handle);
        //            snd_pcm_close(is->pcm_handle);
        //        }
        decoder_destroy(&is->auddec);
        swr_free(&is->swr_ctx);
        av_freep(&is->audio_buf1);
        is->audio_buf1_size = 0;
        is->audio_buf = NULL;
        break;
    case AVMEDIA_TYPE_VIDEO:
        decoder_abort(&is->viddec, &is->pictq);
        decoder_destroy(&is->viddec);
        frame_queue_destory(&is->pictq);
        if (is->pre_pic) {
            free_image_from_vpu(is->pre_pic);
        }
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        av_assert0(0);
        break;
    default:
        break;
    }

    ic->streams[stream_index]->discard = AVDISCARD_ALL;
    avcodec_close(avctx);
    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        is->audio_st = NULL;
        is->audio_stream = -1;
        break;
    case AVMEDIA_TYPE_VIDEO:
        is->video_st = NULL;
        is->video_stream = -1;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        av_assert0(0);
        break;
    default:
        break;
    }
}

static void stream_close(VideoState* is)
{
#if USE_MPP_IN_OUTSIDE
    int i;
#endif

    /* XXX: use a special url_shutdown call to abort parse cleanly */
    is->abort_request = 1;
    pthread_join(is->read_tid, NULL);
    /* close each stream */
    if (is->audio_stream >= 0)
        stream_component_close(is, is->audio_stream);
    if (is->video_stream >= 0)
        stream_component_close(is, is->video_stream);

    packet_queue_destroy(&is->videoq);
    packet_queue_destroy(&is->audioq);
    /* free all pictures */
    // frame_queue_destory(&is->pictq);
    frame_queue_destory(&is->sampq);
    pthread_cond_destroy(&is->continue_read_thread);

    avformat_close_input(&is->ic);

    av_free(is->filename);
#if USE_MPP_IN_OUTSIDE
    for (i = 0; i < VIDEO_PICTURE_BUFFERD_MPP_SIZE; i++) {
        if (is->mpp_bufs[i].buf) {
            mpp_buffer_put(is->mpp_bufs[i].buf);
        }
    }
    pthread_mutex_destroy(&is->mpp_buf_mutex);
#endif
    if (is->rga_fd > 0) {
        rk_rga_close(is->rga_fd);
    }
    av_free(is);
}

static void do_exit(VideoState* is)
{
    if (is) {
        stream_close(is);
    }
#ifdef PLAYING_IN_SEPARATE_PROCESS
    av_lockmgr_register(NULL);
#endif
    if (show_status)
        printf("\n");
    fprintf(stderr, "Quit\n");
}

#if 0
static void sigterm_handler(int sig)
{
    printf("force end video player\n");
    exit(123);
}
#endif

static void set_default_window_size(int width, int height, AVRational sar)
{
    rk_Rect rect;
    calculate_display_rect(&rect, 0, 0, INT_MAX, height, width, height, sar);
    default_width = rect.w;
    default_height = rect.h;
}

/* display the current picture, if any */
static void video_display(VideoState* is)
{
    if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO)
        av_assert0(0);
    // video_audio_display(is);
    else if (is->video_st) {
        is->last_display_time = av_gettime_relative() / 1000000.0;
        video_image_display(is);
    }
}

static double get_clock(Clock* c)
{
    if (!c->queue_serial || *c->queue_serial != c->serial)
        return NAN;
    if (c->paused) {
        return c->pts;
    } else {
        double time = av_gettime_relative() / 1000000.0;
        double value =
            c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
#if RK_DEBUG_CLOCK
        printf("%s: %d \n", __FUNCTION__, __LINE__);
        dump_clock(c);
        printf("\t\tvalue:%f\n", value);
#endif
        return value;
    }
}

static void set_clock_at(Clock* c, double pts, int serial, double time)
{
    c->pts = pts;
    c->last_updated = time;
    c->pts_drift = c->pts - time;
    c->serial = serial;
#if RK_DEBUG_CLOCK
    printf("%s ", __FUNCTION__);
    dump_clock(c);
#endif
}

static void set_clock(Clock* c, double pts, int serial)
{
    double time = av_gettime_relative() / 1000000.0;
    set_clock_at(c, pts, serial, time);
}

static void init_clock(Clock* c, int* queue_serial)
{
    c->speed = 1.0;
    c->paused = 0;
    c->queue_serial = queue_serial;
    set_clock(c, NAN, -1);
}

static void sync_clock_to_slave(Clock* c, Clock* slave)
{
    return;  // TODO
    double clock = get_clock(c);
    double slave_clock = get_clock(slave);
    if (!isnan(slave_clock) &&
        (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
        set_clock(c, slave_clock, slave->serial);
}

static int get_master_sync_type(VideoState* is)
{
    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (is->video_st)
            return AV_SYNC_VIDEO_MASTER;
        else
            return AV_SYNC_AUDIO_MASTER;
    } else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (is->audio_st)
            return AV_SYNC_AUDIO_MASTER;
        if (is->video_st)
            return AV_SYNC_VIDEO_MASTER;
        else
            return AV_SYNC_EXTERNAL_CLOCK;
    } else {
        return AV_SYNC_EXTERNAL_CLOCK;
    }
}

/* get the current master clock value */
static double get_master_clock(VideoState* is)
{
    double val;
    switch (get_master_sync_type(is)) {
    case AV_SYNC_VIDEO_MASTER:
        val = get_clock(&is->vidclk);
        break;
    case AV_SYNC_AUDIO_MASTER:
        val = get_clock(&is->audclk);
        break;
    default:
        val = get_clock(&is->extclk);
        break;
    }
    return val;
}

/* seek in the stream */
static void stream_seek(VideoState* is,
                        int64_t pos,
                        int64_t rel,
                        int seek_by_bytes)
{
    if (!is->seek_req) {
        is->seek_pos = pos;
        is->seek_rel = rel;
        is->seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes)
            is->seek_flags |= AVSEEK_FLAG_BYTE;
        is->seek_req = 1;
        pthread_cond_signal(&is->continue_read_thread);
    }
}

/* pause or resume the video */
static void stream_toggle_pause(VideoState* is)
{
    if (is->paused) {
        is->frame_timer +=
            av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
        if (is->read_pause_return != AVERROR(ENOSYS)) {
            is->vidclk.paused = 0;
        }
        set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
    }
    set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
    is->paused = is->vidclk.paused = is->audclk.paused = is->extclk.paused =
                                                             !is->paused;
}

static void toggle_pause(VideoState* is)
{
    stream_toggle_pause(is);
    is->step = 0;
}

static void step_to_next_frame(VideoState* is)
{
    /* if the stream is paused unpause it, then step */
    if (is->paused)
        stream_toggle_pause(is);
    is->step = 1;
}

static double compute_target_delay(double delay, VideoState* is)
{
    double sync_threshold, diff = 0;

    /* update delay to follow master synchronisation source */
    if (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER) {
        /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
        diff = get_clock(&is->vidclk) - get_master_clock(is);

        /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
        sync_threshold =
            FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));

        // printf("delay: %f, diff: %f, max_frame_duration: %f, sync_threshold:
        // %f\n",
        //       delay, diff, is->max_frame_duration, sync_threshold);

        if (!isnan(diff) && fabs(diff) < is->max_frame_duration) {
            if (diff <= -sync_threshold)
                delay = FFMAX(0, delay + diff);
            else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
                delay = delay + diff;
            else if (diff >= sync_threshold)
                delay = 2 * delay;
        }
    }
    if (show_status)
        av_log(NULL, AV_LOG_ERROR, "video: delay=%0.3f A-V=%f\n", delay, -diff);

    return delay;
}

static double vp_duration(VideoState* is, Frame* vp, Frame* nextvp)
{
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
            return vp->duration;
        else
            return duration;
    } else {
        return 0.0;
    }
}

static void update_video_pts(VideoState* is,
                             double pts,
                             int64_t pos,
                             int serial)
{
    /* update current video pts */
    set_clock(&is->vidclk, pts, serial);
    sync_clock_to_slave(&is->extclk, &is->vidclk);
}

static void free_vp_mem_handle(VideoState* is, Frame* vp)
{
#if USE_MPP_IN_OUTSIDE
    if (vp->vpu_mem_handle) {
        BufferdMpp* mpp = (BufferdMpp*)vp->vpu_mem_handle;
        av_assert0(mpp->bUsing);
        pthread_mutex_lock(&is->mpp_buf_mutex);
        mpp->bUsing = 0;
        pthread_mutex_unlock(&is->mpp_buf_mutex);
    }
#else
    free_picture(vp);
#endif
}

/* called to display each frame */
static int video_refresh(void* opaque, double* remaining_time)
{
    int ret = 0;
    VideoState* is = opaque;
    double time;

    if (is->video_st) {
        int redisplay = 0;
        if (is->force_refresh)
            redisplay = frame_queue_prev(&is->pictq);
    retry:
        if (frame_queue_nb_remaining(&is->pictq) == 0) {
            // nothing to do, no picture to display in the queue
            // if(show_status)
            //    printf("nothing to do, no picture to display in the queue\n");
            ret = -1;
        } else {
            double last_duration, duration, delay;
            Frame *vp, *lastvp;

            /* dequeue the picture */
            lastvp = frame_queue_peek_last(&is->pictq);
            vp = frame_queue_peek(&is->pictq);

            if (vp->serial != is->videoq.serial) {
                free_vp_mem_handle(is, vp);
                frame_queue_next(&is->pictq);
                redisplay = 0;
                goto retry;
            }

            if (lastvp->serial != vp->serial && !redisplay)
                is->frame_timer = av_gettime_relative() / 1000000.0;

            if (is->paused) {
                printf("video doing paused\n");
                goto display;
            }

            /* compute nominal last_duration */
            last_duration = vp_duration(is, lastvp, vp);
            if (redisplay) {
                delay = 0.0;
            } else {
                // if(get_master_sync_type(is)==AV_SYNC_VIDEO_MASTER){
                if (speed != 1) {
                    last_duration /= (vp->frame_drops - lastvp->frame_drops + 1);
                }
                delay = compute_target_delay(last_duration, is);
            }

            time = av_gettime_relative() / 1000000.0;
            if (time < is->frame_timer + delay && !redisplay) {
                *remaining_time =
                    FFMIN(is->frame_timer + delay - time, *remaining_time);
                return ret;
            }
#if 0
            if (is->last_display_time > 0.0) {
                if (delay < time - is->last_display_time) {
                    *remaining_time = 0.0;
                } else {
                    *remaining_time = FFMIN(delay - (time - is->last_display_time),
                                            *remaining_time);
                }
            }
#endif
            is->frame_timer += delay;
            if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
                is->frame_timer = time;

            pthread_mutex_lock(&is->pictq.mutex);
            if (!redisplay && !isnan(vp->pts))
                update_video_pts(is, vp->pts, vp->pos, vp->serial);
            pthread_mutex_unlock(&is->pictq.mutex);

            if (frame_queue_nb_remaining(&is->pictq) > 1) {
                Frame* nextvp = frame_queue_peek_next(&is->pictq);
                duration = vp_duration(is, vp, nextvp);
                // if(get_master_sync_type(is)==AV_SYNC_VIDEO_MASTER){
                if (speed != 1) {
                    duration /= (nextvp->frame_drops - vp->frame_drops + 1);
                }
                if (!is->step &&
                    (redisplay || framedrop > 0 ||
                     (framedrop && (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER ||
                                    speed != 1))) &&
                    time > is->frame_timer + duration) {
                    if (!redisplay) {
                        is->frame_drops_late++;
                        printf("drop 1 video frame\n");
                    }
                    free_vp_mem_handle(is, vp);
                    frame_queue_next(&is->pictq);
                    redisplay = 0;
                    goto retry;
                }
            }
        display:
            /* display picture */
            if (!display_disable && is->show_mode == SHOW_MODE_VIDEO)
                video_display(is);

            frame_queue_next(&is->pictq);

            if (is->step && !is->paused)
                stream_toggle_pause(is);
        }
    } else {
        ret = -1;
    }
    is->force_refresh = 0;
    if (show_status) {
        static int64_t last_time;
        int64_t cur_time;
        int aqsize, vqsize, sqsize;
        double av_diff;

        cur_time = av_gettime_relative();
        if (!last_time || (cur_time - last_time) >= 30000) {
            aqsize = 0;
            vqsize = 0;
            sqsize = 0;
            if (is->audio_st)
                aqsize = is->audioq.size;
            if (is->video_st)
                vqsize = is->videoq.size;
            // if (is->subtitle_st)
            // sqsize = is->subtitleq.size;
            av_diff = 0;
            if (is->audio_st && is->video_st)
                av_diff = get_clock(&is->audclk) - get_clock(&is->vidclk);
            else if (is->video_st)
                av_diff = get_master_clock(is) - get_clock(&is->vidclk);
            else if (is->audio_st)
                av_diff = get_master_clock(is) - get_clock(&is->audclk);
            if (show_status)
                fprintf(
                    stderr,
                    "~~~ %7.2f %s:%7.3f fd=%4d aq=%5dKB vq=%5dKB sq=%5dB f=%" PRId64
                    "/%" PRId64 "   \n",
                    get_master_clock(is),
                    (is->audio_st && is->video_st)
                    ? "A-V"
                    : (is->video_st ? "M-V" : (is->audio_st ? "M-A" : "   ")),
                    av_diff, is->frame_drops_early + is->frame_drops_late,
                    aqsize / 1024, vqsize / 1024, sqsize,
                    is->video_st ? is->video_st->codec->pts_correction_num_faulty_dts
                    : 0,
                    is->video_st ? is->video_st->codec->pts_correction_num_faulty_pts
                    : 0);
            fflush(stdout);
            last_time = cur_time;
            // if(show_status)
            //    printf("%s last_time = %ld\n", __func__, last_time);
        }
    }
    return ret;
}

static int queue_picture(VideoState* is,
                         AVFrame* src_frame,
                         double pts,
                         double duration,
                         int64_t pos,
                         int serial)
{
    Frame* vp;

#if defined(DEBUG_SYNC)
    printf("frame_type=%c pts=%0.3f\n",
           av_get_picture_type_char(src_frame->pict_type), pts);
#endif

    if (!(vp = frame_queue_peek_writable(&is->pictq)))
        return -1;

    vp->sar = src_frame->sample_aspect_ratio;

    vp->width = src_frame->width;
    vp->height = src_frame->height;
    if (is->videoq.abort_request)
        return -1;
    vp->vpu_mem_handle = src_frame->data[3];
    av_assert0(vp->vpu_mem_handle);
    /* if the frame is not skipped, then display it */
    if (vp->vpu_mem_handle) {
#if USE_MPP_IN_OUTSIDE
        src_frame->data[0] = src_frame->data[1] = src_frame->data[2] = 0;
        memset(src_frame->user_reserve_buf, 0, sizeof(src_frame->user_reserve_buf));
        av_assert0(((BufferdMpp*)vp->vpu_mem_handle)->bUsing);
#else
        src_frame->data[3] = 0;
#endif
        vp->pts = pts;
        vp->duration = duration;
        vp->pos = pos;
        vp->serial = serial;
        vp->frame_drops = is->frame_drops_early;
        is->pre_queued_pts = pts;
        // printf("vp->pts: %f, frame_drops: %d\n", pts, vp->frame_drops);
        /* now we can update the picture count */
        frame_queue_push(&is->pictq);
    }
    return 0;
}

// busing=1: get a idle mpp_buf, and set to avframe
// busing=0: set the mpp_buf in avframe to idle
static void set_avframe_using(VideoState* is, AVFrame* frame, int busing)
{
#if USE_MPP_IN_OUTSIDE
    BufferdMpp* mpp_buf = NULL;
    int i = 0;
    if (busing) {
        pdata_handle data_buf = NULL;
        MppBufferInfo info;
        pthread_mutex_lock(&is->mpp_buf_mutex);
        i = is->mpp_buf_index;
        if (i == VIDEO_PICTURE_BUFFERD_MPP_SIZE) {
            i = 0;
        }
        for (; i < VIDEO_PICTURE_BUFFERD_MPP_SIZE; i++) {
            mpp_buf = &is->mpp_bufs[i];
            if (!mpp_buf->bUsing) {
                is->mpp_buf_index = i + 1;
                mpp_buf->bUsing = 1;
                break;
            }
        }
        av_assert0(i < VIDEO_PICTURE_BUFFERD_MPP_SIZE);
        pthread_mutex_unlock(&is->mpp_buf_mutex);
        frame->format = AV_PIX_FMT_NV12;
        frame->width = is->viddec_width;
        frame->height = is->viddec_height;
        frame->linesize[0] = frame->linesize[1] = is->viddec_width;
        frame->linesize[2] = 0;
        frame->user_flags |= VPU_MEM_ALLOC_EXTERNAL;
        if (MPP_OK != mpp_buffer_info_get(mpp_buf->buf, &info)) {
            av_assert0(0);
        }
        frame->data[0] = (uint8_t*)info.ptr;
        frame->data[1] = frame->data[0] + frame->width * frame->height;
        frame->data[2] = (uint8_t*)mpp_buf;
        data_buf = (pdata_handle)frame->user_reserve_buf;
        data_buf->vir_addr = (uint8_t*)info.ptr;
        data_buf->phy_fd = info.fd;
        data_buf->handle = info.hnd;
        data_buf->buf_size = info.size;
    } else {
        mpp_buf = (BufferdMpp*)frame->data[2];
        if (mpp_buf) {
            pthread_mutex_lock(&is->mpp_buf_mutex);
            for (i = 0; i < VIDEO_PICTURE_BUFFERD_MPP_SIZE; i++) {
                if (mpp_buf == &is->mpp_bufs[i]) {
                    is->mpp_buf_index = i;
                    mpp_buf->bUsing = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&is->mpp_buf_mutex);
            frame->data[0] = frame->data[1] = frame->data[2] = 0;
            memset(frame->user_reserve_buf, 0, sizeof(frame->user_reserve_buf));
        }
    }
#else
    image_handle image_buf = NULL;
    if (busing) {
        image_buf = (image_handle)calloc(1, sizeof(DataBuffer_VPU_t));
        if (!image_buf) {
            av_log(NULL, AV_LOG_FATAL, "alloc DataBuffer_VPU_t fail, no memory!!\n");
        }
        frame->format = AV_PIX_FMT_NV12;
        frame->width = is->viddec_width;
        frame->height = is->viddec_height;
        frame->linesize[0] = frame->linesize[1] = is->viddec_width;
        frame->linesize[2] = 0;
        frame->user_flags |= VPU_MEM_ALLOC_EXTERNAL;
        frame->data[0] = frame->data[1] = frame->data[2] = 0;
        av_assert0(frame->data[3] == 0);
        frame->data[3] = (uint8_t*)image_buf;
    } else {
        free_image_from_vpu(frame->data[3]);
        frame->data[3] = 0;
    }
#endif
}

static int get_video_frame(VideoState* is,
                           AVFrame* frame,
                           AVRational frame_rate)
{
    int got_picture;

    if ((got_picture = decoder_decode_frame(is, &is->viddec, frame)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "get video frame return %d\n", got_picture);
        return -1;
    }

    if (got_picture) {
        double dpts = NAN;

        frame->sample_aspect_ratio =
            av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

        is->viddec_width = frame->width;
        is->viddec_height = frame->height;
        if (pre_speed != 1 && speed == 1) {
            frame->user_flags &= (~VPU_SKIP_THIS_FRAME);
        }
        if (framedrop > 0 ||
            (framedrop &&
             (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER || speed != 1))) {
            if (frame->pts != AV_NOPTS_VALUE) {
                double time_base = av_q2d(is->video_st->time_base);
                dpts = time_base * frame->pts;
                double diff;
                if (speed != 1) {
                    diff = dpts - is->pre_queued_pts;
                } else {
                    diff = dpts - get_master_clock(is);
                }
                double interval =
                    (frame_rate.num && frame_rate.den
                ? av_q2d((AVRational) {frame_rate.den, frame_rate.num})
                : time_base);
#if RK_DEBUG_CLOCK
                // printf("diff: %f, frame_last_filter_delay: %f\n", diff,
                // is->frame_last_filter_delay);
                printf("interval: %lf, time_base: %lf, diff: %lf\n", interval,
                       time_base * 1000, diff);
                // printf("viddec.pkt_serial:%d, vidclk.serial:%d\n",
                // is->viddec.pkt_serial, is->vidclk.serial);
                printf("videoq.nb_packets: %d, frame_drops_early: %d\n",
                       is->videoq.nb_packets, is->frame_drops_early);
#endif
                if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
#if CONFIG_AVFILTER
                    diff - is->frame_last_filter_delay < 0 &&
#else
                    // if have slowed one video frame
                    ((get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER &&
                      diff + interval < 0) ||
                     (speed != 1 && diff <= interval * (speed - 1))) &&
#endif
                    is->viddec.pkt_serial == is->vidclk.serial &&
                    is->videoq.nb_packets) {
                    is->frame_drops_early++;
                    av_frame_unref(frame);
#if RK_DEBUG_CLOCK
                    av_log(NULL, AV_LOG_ERROR, "drop early video frames: %d\n",
                           is->frame_drops_early);
#endif
                    frame->user_flags |= VPU_SKIP_THIS_FRAME;
                    printf("drop one video frame-\n");
                    got_picture = 0;
                } else {
#if !USE_MPP_IN_OUTSIDE
                    image_handle handle = (image_handle)frame->data[3];
#endif
                    frame->user_flags &= (~VPU_SKIP_THIS_FRAME);
#if USE_MPP_IN_OUTSIDE
                    if (!frame->data[0] && !frame->data[3]) {
#else
                    if (handle->image_buf.phy_fd <= 0) {
#endif
                        is->frame_drops_early++;
                        av_frame_unref(frame);
                        printf("drop one video frame--\n");
                        got_picture = 0;
                    }
                }
            }
        }
    }

    return got_picture;
}

#if CONFIG_AVFILTER
static int configure_filtergraph(AVFilterGraph* graph,
                                 const char* filtergraph,
                                 AVFilterContext* source_ctx,
                                 AVFilterContext* sink_ctx)
{
    int ret, i;
    int nb_filters = graph->nb_filters;
    AVFilterInOut *outputs = NULL, *inputs = NULL;

    if (filtergraph) {
        outputs = avfilter_inout_alloc();
        inputs = avfilter_inout_alloc();
        if (!outputs || !inputs) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }

        outputs->name = av_strdup("in");
        outputs->filter_ctx = source_ctx;
        outputs->pad_idx = 0;
        outputs->next = NULL;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = sink_ctx;
        inputs->pad_idx = 0;
        inputs->next = NULL;

        if ((ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs,
                                            NULL)) < 0)
            goto fail;
    } else {
        if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0)
            goto fail;
    }

    /* Reorder the filters to ensure that inputs of the custom filters are merged
     * first */
    for (i = 0; i < graph->nb_filters - nb_filters; i++)
        FFSWAP(AVFilterContext*, graph->filters[i], graph->filters[i + nb_filters]);

    ret = avfilter_graph_config(graph, NULL);
fail:
    avfilter_inout_free(&outputs);
    avfilter_inout_free(&inputs);
    return ret;
}

static int configure_video_filters(AVFilterGraph* graph,
                                   VideoState* is,
                                   const char* vfilters,
                                   AVFrame* frame)
{
    static const enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P,
                                                  AV_PIX_FMT_NONE
                                                 };
    char sws_flags_str[512] = "";
    char buffersrc_args[256];
    int ret;
    AVFilterContext *filt_src = NULL, *filt_out = NULL, *last_filter = NULL;
    AVCodecContext* codec = is->video_st->codec;
    AVRational fr = av_guess_frame_rate(is->ic, is->video_st, NULL);
    AVDictionaryEntry* e = NULL;

    while ((e = av_dict_get(sws_dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
        if (!strcmp(e->key, "sws_flags")) {
            av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", "flags",
                        e->value);
        } else
            av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", e->key,
                        e->value);
    }
    if (strlen(sws_flags_str))
        sws_flags_str[strlen(sws_flags_str) - 1] = '\0';

    graph->scale_sws_opts = av_strdup(sws_flags_str);

    snprintf(buffersrc_args, sizeof(buffersrc_args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             frame->width, frame->height, frame->format,
             is->video_st->time_base.num, is->video_st->time_base.den,
             codec->sample_aspect_ratio.num,
             FFMAX(codec->sample_aspect_ratio.den, 1));
    if (fr.num && fr.den)
        av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d",
                    fr.num, fr.den);

    if ((ret = avfilter_graph_create_filter(
                   &filt_src, avfilter_get_by_name("buffer"), "ffplay_buffer",
                   buffersrc_args, NULL, graph)) < 0)
        goto fail;

    ret = avfilter_graph_create_filter(&filt_out,
                                       avfilter_get_by_name("buffersink"),
                                       "ffplay_buffersink", NULL, NULL, graph);
    if (ret < 0)
        goto fail;

    if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts,
                                   AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
        goto fail;

    last_filter = filt_out;

    /* Note: this macro adds a filter before the lastly added filter, so the
     * processing order of the filters is in reverse */
#define INSERT_FILT(name, arg)                                                \
  do {                                                                        \
    AVFilterContext* filt_ctx;                                                \
                                                                              \
    ret = avfilter_graph_create_filter(&filt_ctx, avfilter_get_by_name(name), \
                                       "ffplay_" name, arg, NULL, graph);     \
    if (ret < 0)                                                              \
      goto fail;                                                              \
                                                                              \
    ret = avfilter_link(filt_ctx, 0, last_filter, 0);                         \
    if (ret < 0)                                                              \
      goto fail;                                                              \
                                                                              \
    last_filter = filt_ctx;                                                   \
  } while (0)

    /* SDL YUV code is not handling odd width/height for some driver
     * combinations, therefore we crop the picture to an even width/height. */
    INSERT_FILT("crop", "floor(in_w/2)*2:floor(in_h/2)*2");

    if (autorotate) {
        double theta = get_rotation(is->video_st);

        if (fabs(theta - 90) < 1.0) {
            INSERT_FILT("transpose", "clock");
        } else if (fabs(theta - 180) < 1.0) {
            INSERT_FILT("hflip", NULL);
            INSERT_FILT("vflip", NULL);
        } else if (fabs(theta - 270) < 1.0) {
            INSERT_FILT("transpose", "cclock");
        } else if (fabs(theta) > 1.0) {
            char rotate_buf[64];
            snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
            INSERT_FILT("rotate", rotate_buf);
        }
    }

    if ((ret = configure_filtergraph(graph, vfilters, filt_src, last_filter)) < 0)
        goto fail;

    is->in_video_filter = filt_src;
    is->out_video_filter = filt_out;

fail:
    return ret;
}

static int configure_audio_filters(VideoState* is,
                                   const char* afilters,
                                   int force_output_format)
{
    static const enum AVSampleFormat sample_fmts[] = {AV_SAMPLE_FMT_S16,
                                                      AV_SAMPLE_FMT_NONE
                                                     };
    int sample_rates[2] = {0, -1};
    int64_t channel_layouts[2] = {0, -1};
    int channels[2] = {0, -1};
    AVFilterContext *filt_asrc = NULL, *filt_asink = NULL;
    char aresample_swr_opts[512] = "";
    AVDictionaryEntry* e = NULL;
    char asrc_args[256];
    int ret;

    avfilter_graph_free(&is->agraph);
    if (!(is->agraph = avfilter_graph_alloc()))
        return AVERROR(ENOMEM);

    while ((e = av_dict_get(swr_opts, "", e, AV_DICT_IGNORE_SUFFIX)))
        av_strlcatf(aresample_swr_opts, sizeof(aresample_swr_opts),
                    "%s=%s:", e->key, e->value);
    if (strlen(aresample_swr_opts))
        aresample_swr_opts[strlen(aresample_swr_opts) - 1] = '\0';
    av_opt_set(is->agraph, "aresample_swr_opts", aresample_swr_opts, 0);

    ret = snprintf(asrc_args, sizeof(asrc_args),
                   "sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
                   is->audio_filter_src.freq,
                   av_get_sample_fmt_name(is->audio_filter_src.fmt),
                   is->audio_filter_src.channels, 1, is->audio_filter_src.freq);
    if (is->audio_filter_src.channel_layout)
        snprintf(asrc_args + ret, sizeof(asrc_args) - ret,
                 ":channel_layout=0x%" PRIx64, is->audio_filter_src.channel_layout);

    ret = avfilter_graph_create_filter(
              &filt_asrc, avfilter_get_by_name("abuffer"), "ffplay_abuffer", asrc_args,
              NULL, is->agraph);
    if (ret < 0)
        goto end;

    ret = avfilter_graph_create_filter(
              &filt_asink, avfilter_get_by_name("abuffersink"), "ffplay_abuffersink",
              NULL, NULL, is->agraph);
    if (ret < 0)
        goto end;

    if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts,
                                   AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) <
        0)
        goto end;
    if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1,
                              AV_OPT_SEARCH_CHILDREN)) < 0)
        goto end;

    if (force_output_format) {
        channel_layouts[0] = is->audio_tgt.channel_layout;
        channels[0] = is->audio_tgt.channels;
        sample_rates[0] = is->audio_tgt.freq;
        if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 0,
                                  AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
        if ((ret =
                 av_opt_set_int_list(filt_asink, "channel_layouts", channel_layouts,
                                     -1, AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
        if ((ret = av_opt_set_int_list(filt_asink, "channel_counts", channels, -1,
                                       AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
        if ((ret = av_opt_set_int_list(filt_asink, "sample_rates", sample_rates, -1,
                                       AV_OPT_SEARCH_CHILDREN)) < 0)
            goto end;
    }

    if ((ret = configure_filtergraph(is->agraph, afilters, filt_asrc,
                                     filt_asink)) < 0)
        goto end;

    is->in_audio_filter = filt_asrc;
    is->out_audio_filter = filt_asink;

end:
    if (ret < 0)
        avfilter_graph_free(&is->agraph);
    return ret;
}
#endif /* CONFIG_AVFILTER */

static void* audio_thread(void* arg)
{
    VideoState* is = arg;
    AVFrame* frame = av_frame_alloc();
    Frame* af;
#if CONFIG_AVFILTER
    int last_serial = -1;
    int64_t dec_channel_layout;
    int reconfigure;
#endif
    int got_frame = 0;
    AVRational tb;
    int ret = 0;

    prctl(PR_SET_NAME, "audio_thread", 0, 0, 0);

    if (!frame)
        return (void*)AVERROR(ENOMEM);

    do {
        if ((got_frame = decoder_decode_frame(is, &is->auddec, frame)) < 0)
            goto the_end;

        if (got_frame) {
            tb = (AVRational) {1, frame->sample_rate};

#if CONFIG_AVFILTER
            dec_channel_layout = get_valid_channel_layout(
                                     frame->channel_layout, av_frame_get_channels(frame));

            reconfigure = cmp_audio_fmts(is->audio_filter_src.fmt,
                                         is->audio_filter_src.channels, frame->format,
                                         av_frame_get_channels(frame)) ||
                          is->audio_filter_src.channel_layout != dec_channel_layout ||
                          is->audio_filter_src.freq != frame->sample_rate ||
                          is->auddec.pkt_serial != last_serial;

            if (reconfigure) {
                char buf1[1024], buf2[1024];
                av_get_channel_layout_string(buf1, sizeof(buf1), -1,
                                             is->audio_filter_src.channel_layout);
                av_get_channel_layout_string(buf2, sizeof(buf2), -1,
                                             dec_channel_layout);
                av_log(NULL, AV_LOG_DEBUG,
                       "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s "
                       "serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n",
                       is->audio_filter_src.freq, is->audio_filter_src.channels,
                       av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1,
                       last_serial, frame->sample_rate, av_frame_get_channels(frame),
                       av_get_sample_fmt_name(frame->format), buf2,
                       is->auddec.pkt_serial);

                is->audio_filter_src.fmt = frame->format;
                is->audio_filter_src.channels = av_frame_get_channels(frame);
                is->audio_filter_src.channel_layout = dec_channel_layout;
                is->audio_filter_src.freq = frame->sample_rate;
                last_serial = is->auddec.pkt_serial;

                if ((ret = configure_audio_filters(is, afilters, 1)) < 0)
                    goto the_end;
            }

            if ((ret = av_buffersrc_add_frame(is->in_audio_filter, frame)) < 0)
                goto the_end;

            while ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame,
                                                        0)) >= 0) {
                tb = is->out_audio_filter->inputs[0]->time_base;
#endif
                if (!(af = frame_queue_peek_writable(&is->sampq)))
                    goto the_end;

                af->pts =
                    (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
                af->pos = av_frame_get_pkt_pos(frame);
                af->serial = is->auddec.pkt_serial;
                af->duration =
                av_q2d((AVRational) {frame->nb_samples, frame->sample_rate});

                av_frame_move_ref(af->frame, frame);
                frame_queue_push(&is->sampq);

#if CONFIG_AVFILTER
                if (is->audioq.serial != is->auddec.pkt_serial)
                    break;
            }
            if (ret == AVERROR_EOF)
                is->auddec.finished = is->auddec.pkt_serial;
#endif
        }
    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
the_end:
#if CONFIG_AVFILTER
    avfilter_graph_free(&is->agraph);
#endif
    av_frame_free(&frame);
    printf("%s out\n", __func__);
    return (void*)ret;
}

static int decoder_start(Decoder* d, void* (*fn)(void*), void* arg)
{
    int ret = 0;
    packet_queue_start(d->queue);
    ret = pthread_create(&d->decoder_tid, &global_attr, fn, arg);
    // d->decoder_tid = CreateThread(fn, arg);
    // if (!d->decoder_tid) {
    if (0 != ret) {
        av_log(NULL, AV_LOG_ERROR, "CreateThread(): %s\n", strerror(ret));
        return AVERROR(ENOMEM);
    }
    return 0;
}

static void* video_thread(void* arg)
{
    VideoState* is = arg;
    AVFrame* frame = av_frame_alloc();
    double pts;
    double duration;
    int ret;
    AVRational tb = is->video_st->time_base;
    AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

    prctl(PR_SET_NAME, "video_thread", 0, 0, 0);

#if CONFIG_AVFILTER
    AVFilterGraph* graph = avfilter_graph_alloc();
    AVFilterContext *filt_out = NULL, *filt_in = NULL;
    int last_w = 0;
    int last_h = 0;
    enum AVPixelFormat last_format = -2;
    int last_serial = -1;
    int last_vfilter_idx = 0;
    if (!graph) {
        av_frame_free(&frame);
        return AVERROR(ENOMEM);
    }
#endif

    if (!frame) {
#if CONFIG_AVFILTER
        avfilter_graph_free(&graph);
#endif
        return (void*)AVERROR(ENOMEM);
    }

    for (;;) {
        set_avframe_using(is, frame, 1);
        ret = get_video_frame(is, frame, frame_rate);
        if (ret < 0) {
            goto the_end;
        }
        if (!ret) {
            set_avframe_using(is, frame, 0);
            continue;
        }
#if CONFIG_AVFILTER
        if (last_w != frame->width || last_h != frame->height ||
            last_format != frame->format || last_serial != is->viddec.pkt_serial ||
            last_vfilter_idx != is->vfilter_idx) {
            av_log(
                NULL, AV_LOG_DEBUG,
                "Video frame changed from size:%dx%d format:%s serial:%d to "
                "size:%dx%d format:%s serial:%d\n",
                last_w, last_h,
                (const char*)av_x_if_null(av_get_pix_fmt_name(last_format), "none"),
                last_serial, frame->width, frame->height,
                (const char*)av_x_if_null(av_get_pix_fmt_name(frame->format), "none"),
                is->viddec.pkt_serial);
            avfilter_graph_free(&graph);
            graph = avfilter_graph_alloc();
            if ((ret = configure_video_filters(
                           graph, is, vfilters_list ? vfilters_list[is->vfilter_idx] : NULL,
                           frame)) < 0) {
                Event event;
                event.type = FF_QUIT_EVENT;
                event.user.data1 = is;
                PushEvent(&event);
                goto the_end;
            }
            filt_in = is->in_video_filter;
            filt_out = is->out_video_filter;
            last_w = frame->width;
            last_h = frame->height;
            last_format = frame->format;
            last_serial = is->viddec.pkt_serial;
            last_vfilter_idx = is->vfilter_idx;
            frame_rate = filt_out->inputs[0]->frame_rate;
        }

        ret = av_buffersrc_add_frame(filt_in, frame);
        if (ret < 0)
            goto the_end;

        while (ret >= 0) {
            is->frame_last_returned_time = av_gettime_relative() / 1000000.0;

            ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
            if (ret < 0) {
                if (ret == AVERROR_EOF)
                    is->viddec.finished = is->viddec.pkt_serial;
                ret = 0;
                break;
            }

            is->frame_last_filter_delay =
                av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
            if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
                is->frame_last_filter_delay = 0;
            tb = filt_out->inputs[0]->time_base;
#endif
            duration = (frame_rate.num && frame_rate.den
            ? av_q2d((AVRational) {frame_rate.den, frame_rate.num})
            : 0);
            pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
            ret = queue_picture(is, frame, pts, duration, av_frame_get_pkt_pos(frame),
                                is->viddec.pkt_serial);
            av_frame_unref(frame);
#if CONFIG_AVFILTER
        }
#endif

        if (ret < 0) {
            goto the_end;
        }
    }
the_end:
#if CONFIG_AVFILTER
    avfilter_graph_free(&graph);
#endif
    set_avframe_using(is, frame, 0);
    av_frame_free(&frame);
    printf("%s out\n", __func__);
    return (void*)0;
}

/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
static int synchronize_audio(VideoState* is, int nb_samples)
{
    int wanted_nb_samples = nb_samples;
    // if(get_master_sync_type(is) == AV_SYNC_AUDIO_MASTER){
    if (speed != 1) {
        wanted_nb_samples /=
            speed;  // TODO, should remove audio output when speed playing??
    } else
        /* if not master, then we try to remove or add samples to correct the
           clock */
        if (get_master_sync_type(is) != AV_SYNC_AUDIO_MASTER) {
            double diff, avg_diff;
            int min_nb_samples, max_nb_samples;

            diff = get_clock(&is->audclk) - get_master_clock(is);

            if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
                is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
                if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
                    /* not enough measures to have a correct estimate */
                    is->audio_diff_avg_count++;
                } else {
                    /* estimate the A-V difference */
                    avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

                    if (fabs(avg_diff) >= is->audio_diff_threshold) {
                        wanted_nb_samples = nb_samples + (int)(diff * is->audio_src.sample_rate);
                        // printf("!diff, wanted_nb_samples : %f, %d\n", diff, wanted_nb_samples);
                        min_nb_samples =
                            nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100;
                        max_nb_samples =
                            nb_samples * 100 / (100 - SAMPLE_CORRECTION_PERCENT_MAX);
                        wanted_nb_samples =
                            av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
                        // printf("!wanted_nb_samples: %d\n", wanted_nb_samples);
                    }
#if RK_DEBUG_CLOCK
                    av_log(NULL, AV_LOG_ERROR,
                           "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n", diff,
                           avg_diff, wanted_nb_samples - nb_samples, is->audio_clock,
                           is->audio_diff_threshold);
#endif
                }
            } else {
                /* too big difference : may be initial PTS errors, so
                   reset A-V filter */
                is->audio_diff_avg_count = 0;
                is->audio_diff_cum = 0;
            }
        }

    return wanted_nb_samples;
}

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in is->audio_buf, with size in bytes given by the return
 * value.
 */
static int audio_get_decode_frame(VideoState* is)
{
    int data_size, resampled_data_size = 0;
    int64_t dec_channel_layout;
    av_unused double audio_clock0;
    int wanted_nb_samples;
    Frame* af;

    if (is->paused)
        return -1;

    do {
#if defined(_WIN32)
        while (frame_queue_nb_remaining(&is->sampq) == 0) {
            if ((av_gettime_relative() - audio_callback_time) >
                1000000LL * is->audio_hw_buf_size / is->audio_tgt.bytes_per_sec / 2)
                return -1;
            av_usleep(1000);
        }
#endif
        if (!(af = frame_queue_peek_readable(&is->sampq)))
            return -1;
        frame_queue_next(&is->sampq);
#if RK_DEBUG_QUEUE
        printf(
            "audio_get_uncompressed_frame | af->serial<%d>, "
            "is->audioq.serial<%d>\n",
            af->serial, is->audioq.serial);
#endif
    } while (af->serial != is->audioq.serial);

    data_size =
        av_samples_get_buffer_size(NULL, av_frame_get_channels(af->frame),
                                   af->frame->nb_samples, af->frame->format, 1);

    dec_channel_layout =
        (af->frame->channel_layout &&
         av_frame_get_channels(af->frame) ==
         av_get_channel_layout_nb_channels(af->frame->channel_layout))
        ? af->frame->channel_layout
        : av_get_default_channel_layout(av_frame_get_channels(af->frame));
    wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);
    if (speed == 1 && wanted_nb_samples > af->frame->nb_samples) {
        int64_t delay = (wanted_nb_samples - af->frame->nb_samples) * 1000000LL /
                        af->frame->sample_rate;
        av_usleep(delay);
        wanted_nb_samples = af->frame->nb_samples;
    }

    if (af->frame->format != is->audio_src.fmt ||
        dec_channel_layout != is->audio_src.channel_layout ||
        af->frame->sample_rate != is->audio_src.sample_rate ||
        (wanted_nb_samples != af->frame->nb_samples && !is->swr_ctx)) {
        swr_free(&is->swr_ctx);
        is->swr_ctx = swr_alloc_set_opts(NULL, is->audio_tgt.channel_layout,
                                         is->audio_tgt.fmt, is->audio_tgt.sample_rate,
                                         dec_channel_layout, af->frame->format,
                                         af->frame->sample_rate, 0, NULL);
        if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
            av_log(NULL, AV_LOG_ERROR,
                   "Cannot create sample rate converter for conversion of %d Hz %s "
                   "%d channels to %d Hz %s %d channels!\n",
                   af->frame->sample_rate, av_get_sample_fmt_name(af->frame->format),
                   av_frame_get_channels(af->frame), is->audio_tgt.sample_rate,
                   av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
            swr_free(&is->swr_ctx);
            return -1;
        }
        is->audio_src.channel_layout = dec_channel_layout;
        is->audio_src.channels = av_frame_get_channels(af->frame);
        is->audio_src.sample_rate = af->frame->sample_rate;
        is->audio_src.fmt = af->frame->format;
    }

    if (is->swr_ctx) {
        const uint8_t** in = (const uint8_t**)af->frame->extended_data;
        uint8_t** out = &is->audio_buf1;
        int out_count = (int64_t)wanted_nb_samples * is->audio_tgt.sample_rate /
                        af->frame->sample_rate +
                        256;
        int out_size = av_samples_get_buffer_size(NULL, is->audio_tgt.channels,
                                                  out_count, is->audio_tgt.fmt, 0);
        int len2;
        if (out_size < 0) {
            av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
            return -1;
        }
        if (wanted_nb_samples != af->frame->nb_samples) {
            if (swr_set_compensation(is->swr_ctx,
                                     (wanted_nb_samples - af->frame->nb_samples) *
                                     is->audio_tgt.sample_rate / af->frame->sample_rate,
                                     wanted_nb_samples * is->audio_tgt.sample_rate /
                                     af->frame->sample_rate) < 0) {
                av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                return -1;
            }
        }
        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
        if (!is->audio_buf1)
            return AVERROR(ENOMEM);
        len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
        if (len2 < 0) {
            av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
            return -1;
        }
        if (len2 == out_count) {
            av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
            if (swr_init(is->swr_ctx) < 0)
                swr_free(&is->swr_ctx);
        }
        is->audio_buf = is->audio_buf1;
        resampled_data_size = len2 * is->audio_tgt.channels *
                              av_get_bytes_per_sample(is->audio_tgt.fmt);
    } else {
        is->audio_buf = af->frame->data[0];
        resampled_data_size = data_size;
        assert(af->frame->linesize[0] == resampled_data_size);
    }

    audio_clock0 = is->audio_clock;
    /* update the audio clock with the pts */
    if (!isnan(af->pts))
        is->audio_clock =
            af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;
    else
        is->audio_clock = NAN;
    is->audio_clock_serial = af->serial;
#if 0  // def DEBUG
    {
        static double last_clock;
        printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n",
               is->audio_clock - last_clock,
               is->audio_clock, audio_clock0);
        last_clock = is->audio_clock;
    }
#endif
    return resampled_data_size;
}

static void update_audio_clk(VideoState* is, int size)
{
    if (!isnan(is->audio_clock)) {
        set_clock_at(&is->audclk,
                     is->audio_clock - (double)size / is->audio_tgt.bytes_per_sec,
                     is->audio_clock_serial, av_gettime_relative() / 1000000.0);
        sync_clock_to_slave(&is->extclk, &is->audclk);
    }
}

/* prepare a new audio buffer */
static void audio_callback(void* opaque, uint8_t* stream, int len)
{
    VideoState* is = opaque;
    int audio_size, len1;

    audio_callback_time = av_gettime_relative();

    while (len > 0) {
        if (is->audio_buf_index >= is->audio_buf_size) {
            audio_size = audio_get_decode_frame(is);
            if (audio_size < 0) {
                /* if error, just output silence */
                is->audio_buf = NULL;
                is->audio_buf_size = AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size *
                                     is->audio_tgt.frame_size;
            } else {
                av_assert0(is->show_mode == SHOW_MODE_VIDEO);
                // if (is->show_mode != SHOW_MODE_VIDEO)
                //    update_sample_display(is, (int16_t *)is->audio_buf, audio_size);
                is->audio_buf_size = audio_size;
            }
            is->audio_buf_index = 0;
        }
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len)
            len1 = len;
        if (!is->muted && is->audio_buf && is->audio_volume == MIX_MAXVOLUME) {
            memcpy(stream, (uint8_t*)is->audio_buf + is->audio_buf_index, len1);
        } else {
            memset(stream, 0, len1);
            if (!is->muted && is->audio_buf)
                fprintf(stderr, "MixAudio TODO\n");
            // MixAudio(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1,
            // is->audio_volume);
        }
        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
        update_audio_clk(is, is->audio_hw_buf_size - len);
    }
    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
    update_audio_clk(is, is->audio_hw_buf_size + is->audio_write_buf_size);
}

static void* audio_refresh_thread(void* arg)
{
    snd_pcm_sframes_t frames_left;
    VideoState* is = (VideoState*)arg;
    int len = is->audio_tgt.channels * is->audio_tgt.pcm_samples *
              snd_pcm_format_physical_width(is->audio_tgt.pcm_format) / 8;
    uint8_t* stream_buf = (uint8_t*)av_malloc(len);
    av_assert0(is->audio_hw_buf_size == len);
    printf("audio buf len: %d\n", len);
    if (!stream_buf)
        return NULL;
    frames_left = is->audio_tgt.pcm_samples;

    while (is->audio_out_running) {
        if (!is->paused) {
            uint8_t* stream = stream_buf;
            audio_callback(is, stream, len);
            frames_left = is->audio_tgt.pcm_samples;
            default_push_play_buffer(stream, len, &is->audio_tgt.wanted_params,
                                     frames_left, 1);
        } else {
            av_usleep(10000);
        }
    }

    av_free(stream_buf);
    default_flush_play();
    printf("out of audio_refresh_thread\n");
    return NULL;
}

/* open a given stream. Return 0 if OK */
static int stream_component_open(VideoState* is, int stream_index)
{
    AVFormatContext* ic = is->ic;
    AVCodecContext* avctx;
    AVCodec* codec;
    const char* forced_codec_name = NULL;
    AVDictionary* opts = NULL;
    AVDictionaryEntry* t = NULL;
    int ret = 0;
    int stream_lowres = lowres;

    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return -1;
    avctx = ic->streams[stream_index]->codec;

    codec = avcodec_find_decoder(avctx->codec_id);
    if (codec == NULL)
        return -1;

    printf("codec name: %s, code longname: %s\n", codec->name, codec->long_name);
    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        is->last_audio_stream = stream_index;
        forced_codec_name = audio_codec_name;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        av_assert0(0);
        break;
    // is->last_subtitle_stream = stream_index; forced_codec_name =
    // subtitle_codec_name; break;
    case AVMEDIA_TYPE_VIDEO:
        is->last_video_stream = stream_index;
        forced_codec_name = video_codec_name;
        break;
    default:
        break;
    }
    if (forced_codec_name)
        codec = avcodec_find_decoder_by_name(forced_codec_name);
    if (!codec) {
        if (forced_codec_name)
            av_log(NULL, AV_LOG_WARNING, "No codec could be found with name '%s'\n",
                   forced_codec_name);
        else
            av_log(NULL, AV_LOG_WARNING, "No codec could be found with id %d\n",
                   avctx->codec_id);
        return -1;
    }

    avctx->codec_id = codec->id;
    if (stream_lowres > av_codec_get_max_lowres(codec)) {
        av_log(avctx, AV_LOG_WARNING,
               "The maximum value for lowres supported by the decoder is %d\n",
               av_codec_get_max_lowres(codec));
        stream_lowres = av_codec_get_max_lowres(codec);
    }
    av_codec_set_lowres(avctx, stream_lowres);

#if FF_API_EMU_EDGE
    if (stream_lowres)
        avctx->flags |= CODEC_FLAG_EMU_EDGE;
#endif
    if (fast)
        avctx->flags2 |= AV_CODEC_FLAG2_FAST;
#if FF_API_EMU_EDGE
    if (codec->capabilities & AV_CODEC_CAP_DR1)
        avctx->flags |= CODEC_FLAG_EMU_EDGE;
#endif

    //    opts = filter_codec_opts(codec_opts, avctx->codec_id, ic,
    //    ic->streams[stream_index], codec);
    if (!av_dict_get(opts, "threads", NULL, 0))
        av_dict_set(&opts, "threads", "auto", 0);
    if (stream_lowres)
        av_dict_set_int(&opts, "lowres", stream_lowres, 0);
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO ||
        avctx->codec_type == AVMEDIA_TYPE_AUDIO)
        av_dict_set(&opts, "refcounted_frames", "1", 0);
    if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
        goto fail;
    }
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        ret = AVERROR_OPTION_NOT_FOUND;
        goto fail;
    }

    is->eof = 0;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
#if CONFIG_AVFILTER
    {
        AVFilterLink* link;

        is->audio_filter_src.freq = avctx->sample_rate;
        is->audio_filter_src.channels = avctx->channels;
        is->audio_filter_src.channel_layout =
            get_valid_channel_layout(avctx->channel_layout, avctx->channels);
        is->audio_filter_src.fmt = avctx->sample_fmt;
        if ((ret = configure_audio_filters(is, afilters, 0)) < 0)
            goto fail;
        link = is->out_audio_filter->inputs[0];
        sample_rate = link->sample_rate;
        nb_channels = link->channels;
        channel_layout = link->channel_layout;
    }
#endif

    default_audio_params(&is->audio_src);
    is->audio_hw_buf_size = is->audio_src.audio_hw_buf_size;
    is->audio_tgt = is->audio_src;
    is->audio_buf_size = 0;
    is->audio_buf_index = 0;

    /* init averaging filter */
    is->audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
    is->audio_diff_avg_count = 0;
    /* since we do not have a precise anough audio fifo fullness,
       we correct audio sync only if larger than this threshold */
    is->audio_diff_threshold =
        (double)(is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;
    printf("is->audio_diff_threshold: %f\n", is->audio_diff_threshold);
    is->audio_stream = stream_index;
    is->audio_st = ic->streams[stream_index];

    decoder_init(&is->auddec, avctx, &is->audioq, &is->continue_read_thread);
    if ((is->ic->iformat->flags &
         (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) &&
        !is->ic->iformat->read_seek) {
        is->auddec.start_pts = is->audio_st->start_time;
        is->auddec.start_pts_tb = is->audio_st->time_base;
    }
    if ((ret = decoder_start(&is->auddec, audio_thread, is)) < 0) {
        goto fail;
    }
    {
        is->audio_out_running = 1;
        ret = pthread_create(&is->audio_out_tid, &global_attr,
                             audio_refresh_thread, is);
        if (0 != ret) {
            av_log(NULL, AV_LOG_FATAL, "audio_out CreateThread(): %s\n",
                   strerror(ret));
            goto fail;
        }
    }
    break;
    case AVMEDIA_TYPE_VIDEO:
        is->video_stream = stream_index;
        is->video_st = ic->streams[stream_index];

        is->viddec_width = avctx->width;
        is->viddec_height = avctx->height;

        decoder_init(&is->viddec, avctx, &is->videoq, &is->continue_read_thread);
        if ((ret = decoder_start(&is->viddec, video_thread, is)) < 0)
            goto fail;
        is->queue_attachments_req = 1;
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        av_assert0(0);
        // is->subtitle_stream = stream_index;
        // is->subtitle_st = ic->streams[stream_index];

        // decoder_init(&is->subdec, avctx, &is->subtitleq,
        // is->continue_read_thread);
        // if ((ret = decoder_start(&is->subdec, subtitle_thread, is)) < 0)
        //    goto fail;
        break;
    default:
        break;
    }

fail:
    av_dict_free(&opts);

    return ret;
}

static int decode_interrupt_cb(void* ctx)
{
    VideoState* is = ctx;
    return is->abort_request;
}

static int is_realtime(AVFormatContext* s)
{
    if (!strcmp(s->iformat->name, "rtp") || !strcmp(s->iformat->name, "rtsp") ||
        !strcmp(s->iformat->name, "sdp"))
        return 1;

    if (s->pb &&
        (!strncmp(s->filename, "rtp:", 4) || !strncmp(s->filename, "udp:", 4)))
        return 1;
    return 0;
}

/* this thread gets the stream from the disk or the network */
static void* read_thread(void* arg)
{
    VideoState* is = arg;
    AVFormatContext* ic = NULL;
    int err, i, ret;
    int st_index[AVMEDIA_TYPE_NB];
    AVPacket pkt1, *pkt = &pkt1;
    int64_t stream_start_time;
    int pkt_in_play_range = 0;
    AVDictionaryEntry* t;
    AVDictionary** opts;
    // int orig_nb_streams;
    pthread_mutex_t wait_mutex;
    // int scan_all_pmts_set = 0;
    int64_t pkt_ts;

    int rr = pthread_mutex_init(&wait_mutex, NULL);
    if (0 != rr) {
        av_log(NULL, AV_LOG_FATAL, "CreateMutex(): %s\n", strerror(rr));
        ret = AVERROR(ENOMEM);
        goto fail;
    }

    memset(st_index, -1, sizeof(st_index));
    is->last_video_stream = is->video_stream = -1;
    is->last_audio_stream = is->audio_stream = -1;
    // is->last_subtitle_stream = is->subtitle_stream = -1;
    is->eof = 0;

    ic = avformat_alloc_context();
    if (!ic) {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    ic->interrupt_callback.callback = decode_interrupt_cb;
    ic->interrupt_callback.opaque = is;
    //   if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE))
    //   {
    //       av_dict_set(&format_opts, "scan_all_pmts", "1",
    //       AV_DICT_DONT_OVERWRITE);
    //       scan_all_pmts_set = 1;
    //   }
    err = avformat_open_input(&ic, is->filename, is->iformat,
                              NULL);  //&format_opts);
    if (err < 0) {
        fprintf(stderr, "open %s fail, %s\n", is->filename, av_err2str(err));
        // print_error(is->filename, err);
        ret = -1;
        goto fail;
    }
    // if (scan_all_pmts_set)
    //    av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

    // if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
    //    av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
    //    ret = AVERROR_OPTION_NOT_FOUND;
    //    goto fail;
    //}
    is->ic = ic;

    if (genpts)
        ic->flags |= AVFMT_FLAG_GENPTS;

    // av_format_inject_global_side_data(ic);

    opts = NULL;  // setup_find_stream_info_opts(ic, codec_opts);
    // orig_nb_streams = ic->nb_streams;
    err = avformat_find_stream_info(ic, opts);
    // for (i = 0; i < orig_nb_streams; i++)
    // av_dict_free(&opts[i]);
    // av_freep(&opts);

    if (err < 0) {
        av_log(NULL, AV_LOG_WARNING, "%s: could not find codec parameters\n",
               is->filename);
        ret = -1;
        goto fail;
    }

    if (ic->pb)
        ic->pb->eof_reached = 0;  // FIXME hack, ffplay maybe should not use
    // avio_feof() to test for the end

    if (seek_by_bytes < 0)
        seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) &&
                        strcmp("ogg", ic->iformat->name);

    is->max_frame_duration =
        (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

    if (!window_title && (t = av_dict_get(ic->metadata, "title", NULL, 0)))
        window_title = av_asprintf("%s - %s", t->value, input_filename);

    /* if seeking requested, we execute it */
    if (start_time != AV_NOPTS_VALUE) {
        int64_t timestamp;

        timestamp = start_time;
        /* add the stream start time */
        if (ic->start_time != AV_NOPTS_VALUE)
            timestamp += ic->start_time;
        ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
        if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
                   is->filename, (double)timestamp / AV_TIME_BASE);
        }
    }

    is->realtime = is_realtime(ic);

    if (show_status)
        av_dump_format(ic, 0, is->filename, 0);

    /* cache reading size 1 second */
    if (ic->bit_rate > 0 && g_max_cache_size < ic->bit_rate / 8)
        g_max_cache_size = ic->bit_rate / 8;
    printf("max_cache_size : %dB\n", g_max_cache_size);

    for (i = 0; i < ic->nb_streams; i++) {
        AVStream* st = ic->streams[i];
        enum AVMediaType type = st->codec->codec_type;
        st->discard = AVDISCARD_ALL;
        if (wanted_stream_spec[type] && st_index[type] == -1)
            if (avformat_match_stream_specifier(ic, st, wanted_stream_spec[type]) > 0)
                st_index[type] = i;
    }
    for (i = 0; i < AVMEDIA_TYPE_NB; i++) {
        if (wanted_stream_spec[i] && st_index[i] == -1) {
            av_log(NULL, AV_LOG_ERROR,
                   "Stream specifier %s does not match any %s stream\n",
                   wanted_stream_spec[i], av_get_media_type_string(i));
            st_index[i] = INT_MAX;
        }
    }

    if (!video_disable)
        st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(
                                           ic, AVMEDIA_TYPE_VIDEO, st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
    if (!audio_disable)
        st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(
                                           ic, AVMEDIA_TYPE_AUDIO, st_index[AVMEDIA_TYPE_AUDIO],
                                           st_index[AVMEDIA_TYPE_VIDEO], NULL, 0);

    is->show_mode = show_mode;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        AVStream* st = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
        AVCodecContext* avctx = st->codec;
        AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
        if (avctx->width) {
#if USE_MPP_IN_OUTSIDE
            size_t size = 0;
#endif
            set_default_window_size(avctx->width, avctx->height, sar);
            // printf("default w: %d, h:%d\n", default_width, default_height);
            // init_video_win(default_width, default_height);
            printf("avct_coded w: %d, h:%d\n", avctx->coded_width,
                   avctx->coded_height);
#if USE_MPP_IN_OUTSIDE
            size = ((avctx->coded_width + 15) & (~15)) *
                   ((avctx->coded_height + 15) & (~15))
                   << 1;
            for (i = 0; i < VIDEO_PICTURE_BUFFERD_MPP_SIZE; i++) {
                MPP_RET m_ret =
                    mpp_buffer_get(g_mpp_buf_group, &is->mpp_bufs[i].buf, size);
                if (m_ret != MPP_OK) {
                    av_log(NULL, AV_LOG_FATAL, "Failed to allocate mpp buffer!\n");
                    is->mpp_bufs[i].buf = NULL;
                    ret = AVERROR(ENOMEM);
                    goto fail;
                }
                is->mpp_bufs[i].bUsing = 0;
            }
            is->mpp_buf_index = 0;
#endif
        }
    }

    /* open the streams */
    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
        stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]);
    }

    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
        ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
    }
    if (is->show_mode == SHOW_MODE_NONE)
        is->show_mode = ret >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_RDFT;

    // if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
    //    stream_component_open(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
    //}

    if (is->video_stream < 0 && is->audio_stream < 0) {
        av_log(NULL, AV_LOG_FATAL,
               "Failed to open file '%s' or configure filtergraph\n", is->filename);
        ret = -1;
        goto fail;
    }

    if (infinite_buffer < 0 && is->realtime)
        infinite_buffer = 1;

    for (;;) {
        BOOL force_read = 0;
        if (is->abort_request)
            break;
        if (is->paused != is->last_paused) {
            is->last_paused = is->paused;
            if (is->paused)
                is->read_pause_return = av_read_pause(ic);
            else
                av_read_play(ic);
        }
#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
        if (is->paused && (!strcmp(ic->iformat->name, "rtsp") ||
                           (ic->pb && !strncmp(input_filename, "mmsh:", 5)))) {
            /* wait 10 ms to avoid trying to get another packet */
            /* XXX: horrible */
            Delay(10);
            continue;
        }
#endif
        if (is->seek_req) {
            int64_t seek_target = is->seek_pos;
            int64_t seek_min =
                is->seek_rel > 0 ? seek_target - is->seek_rel + 2 : INT64_MIN;
            int64_t seek_max =
                is->seek_rel < 0 ? seek_target - is->seek_rel - 2 : INT64_MAX;
            // FIXME the +-2 is due to rounding being not done in the correct
            // direction in generation
            //      of the seek_pos/seek_rel variables

            ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max,
                                     is->seek_flags);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n",
                       is->ic->filename);
            } else {
                if (is->audio_stream >= 0) {
                    packet_queue_flush(&is->audioq);
                    packet_queue_put(&is->audioq, &flush_pkt);
                }
                // if (is->subtitle_stream >= 0) {
                //    packet_queue_flush(&is->subtitleq);
                //    packet_queue_put(&is->subtitleq, &flush_pkt);
                //}
                if (is->video_stream >= 0) {
                    packet_queue_flush(&is->videoq);
                    packet_queue_put(&is->videoq, &flush_pkt);
                }
                if (is->seek_flags & AVSEEK_FLAG_BYTE) {
                    set_clock(&is->extclk, NAN, 0);
                } else {
                    set_clock(&is->extclk, seek_target / (double)AV_TIME_BASE, 0);
                }
            }
            is->seek_req = 0;
            is->queue_attachments_req = 1;
            is->eof = 0;
            if (is->paused)
                step_to_next_frame(is);
        }
        if (is->queue_attachments_req) {
            if (is->video_st &&
                is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                AVPacket copy;
                if ((ret = av_copy_packet(&copy, &is->video_st->attached_pic)) < 0)
                    goto fail;
                packet_queue_put(&is->videoq, &copy);
                packet_queue_put_nullpacket(&is->videoq, is->video_stream);
            }
            is->queue_attachments_req = 0;
        }
#if RK_DEBUG_FRAME_QUEUE
        if (frame_queues_is_full(is)) {
            av_log(NULL, AV_LOG_ERROR,
                   "audioq.size: %d, videoq.size: %d,"
                   "all_size: %d, audioq.nb_packets: %d, videoq.nb_packets: %d\n",
                   is->audioq.size, is->videoq.size,
                   is->audioq.size + is->videoq.size, is->audioq.nb_packets,
                   is->videoq.nb_packets);
        }
#endif
        switch (get_master_sync_type(is)) {
        case AV_SYNC_VIDEO_MASTER:
            force_read = (is->videoq.nb_packets < VIDEO_PICTURE_QUEUE_SIZE / 2) &&
                         !is->videoq.abort_request;
            break;
        case AV_SYNC_AUDIO_MASTER:
            force_read = (is->audioq.nb_packets < SAMPLE_QUEUE_SIZE / 2) &&
                         !is->audioq.abort_request;
            break;
        }
        /* if the queue are full, no need to read more */
        if (infinite_buffer < 1 && !force_read &&
            (is->audioq.size + is->videoq.size > g_max_cache_size ||
             frame_queues_is_full(is) ||
             ((is->audioq.nb_packets >= MAX_FRAMES || is->audio_stream < 0 ||
               is->audioq.abort_request) &&
              (is->videoq.nb_packets >= MAX_FRAMES || is->video_stream < 0 ||
               is->videoq.abort_request ||
               (is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC))))) {
            /* wait 120 ms */
            struct timespec to;
            to.tv_sec = time(NULL);
            to.tv_nsec = 120 * 1000000LL;
            pthread_mutex_lock(&wait_mutex);
            __atomic_store_n(&is->pack_read_signaled, 0, __ATOMIC_SEQ_CST);
            pthread_cond_timedwait(&is->continue_read_thread, &wait_mutex, &to);
            pthread_mutex_unlock(&wait_mutex);
            continue;
        }
        if (!is->paused &&
            (!is->audio_st || (is->auddec.finished == is->audioq.serial &&
                               frame_queue_nb_remaining(&is->sampq) == 0)) &&
            (!is->video_st || (is->viddec.finished == is->videoq.serial &&
                               frame_queue_nb_remaining(&is->pictq) == 0))) {
            if (loop != 1 && (!loop || --loop)) {
                stream_seek(is, start_time != AV_NOPTS_VALUE ? start_time : 0, 0, 0);
            } else if (autoexit) {
                ret = AVERROR_EOF;
                goto fail;
            }
        }
        ret = av_read_frame(ic, pkt);
        if (ret < 0) {
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof) {
                if (is->video_stream >= 0)
                    packet_queue_put_nullpacket(&is->videoq, is->video_stream);
                if (is->audio_stream >= 0)
                    packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
                // if (is->subtitle_stream >= 0)
                //    packet_queue_put_nullpacket(&is->subtitleq, is->subtitle_stream);
                is->eof = 1;
            }
            if (ic->pb && ic->pb->error) {
                av_log(NULL, AV_LOG_ERROR, "read files error occurs.\n");
                is->abort_request = 1;
                break;
            }
            {
                struct timespec to;
                to.tv_sec = time(NULL);
                to.tv_nsec = 100 * 1000000LL;
                pthread_mutex_lock(&wait_mutex);
                __atomic_store_n(&is->pack_read_signaled, 0, __ATOMIC_SEQ_CST);
                pthread_cond_timedwait(&is->continue_read_thread, &wait_mutex, &to);
                pthread_mutex_unlock(&wait_mutex);
            }
            continue;
        } else {
            if (show_status) {
                fprintf(stderr, "av read one frame: %d\n", pkt->stream_index);
            }
            is->eof = 0;
        }
        /* check if packet is in play range specified by user, then queue, otherwise
         * discard */
        stream_start_time = ic->streams[pkt->stream_index]->start_time;
        pkt_ts = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
        pkt_in_play_range =
            duration == AV_NOPTS_VALUE ||
            (pkt_ts -
             (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
            av_q2d(ic->streams[pkt->stream_index]->time_base) -
            (double)(start_time != AV_NOPTS_VALUE ? start_time : 0) /
            1000000 <=
            ((double)duration / 1000000);
        if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
            packet_queue_put(&is->audioq, pkt);
        } else if (pkt->stream_index == is->video_stream && pkt_in_play_range &&
                   !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            packet_queue_put(&is->videoq, pkt);
        }  // else if (pkt->stream_index == is->subtitle_stream &&
        // pkt_in_play_range) {
        //  packet_queue_put(&is->subtitleq, pkt);
        //}
        else {
            av_packet_unref(pkt);
        }
    }

    ret = 0;
fail:
    if (ic && !is->ic)
        avformat_close_input(&ic);

    if (ret != 0) {
        fprintf(stderr, "quit event handler TODO\n");
        is->eof = 1;
    }
    pthread_mutex_destroy(&wait_mutex);
    printf("%s out\n", __func__);
    return (void*)0;
}

static VideoState* stream_open(const char* filename, AVInputFormat* iformat)
{
    VideoState* is;
    int ret = 0;

    is = av_mallocz(sizeof(VideoState));
    if (!is)
        return NULL;
    is->filename = av_strdup(filename);
    if (!is->filename)
        goto fail;
    is->iformat = iformat;

    /* start video display */
    if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) <
        0)
        goto fail;
    // if (frame_queue_init(&is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0)
    // < 0)
    //    goto fail;
    if (frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0)
        goto fail;

    if (packet_queue_init(&is->videoq) < 0 ||
        packet_queue_init(&is->audioq) < 0  //||
        // packet_queue_init(&is->subtitleq) < 0
       )
        goto fail;
#if RK_DEBUG_QUEUE
    is->videoq.name = video_packet_queue_name;
    is->audioq.name = audio_packet_queue_name;
#endif
    ret = pthread_cond_init(&is->continue_read_thread, NULL);
    // if (!(is->continue_read_thread = CreateCond())) {
    if (0 != ret) {
        av_log(NULL, AV_LOG_FATAL, "CreateCond(): %s\n", strerror(ret));
        goto fail;
    }

#if RK_DEBUG_CLOCK
    is->vidclk.name = video_clk_name;
    is->audclk.name = audio_clk_name;
    is->extclk.name = ext_clk_name;
#endif
    init_clock(&is->vidclk, &is->videoq.serial);
    init_clock(&is->audclk, &is->audioq.serial);
    init_clock(&is->extclk, &is->extclk.serial);
    is->audio_clock_serial = -1;
    is->audio_volume = MIX_MAXVOLUME;
    is->muted = 0;
    is->av_sync_type = av_sync_type;
    if (av_sync_type == AV_SYNC_AUDIO_MASTER && audio_disable) {
        is->av_sync_type = AV_SYNC_VIDEO_MASTER;
    }
// av_assert0(is->av_sync_type==AV_SYNC_VIDEO_MASTER);
#if USE_MPP_IN_OUTSIDE
    ret = pthread_mutex_init(&is->mpp_buf_mutex, NULL);
    if (0 != ret) {
        av_log(NULL, AV_LOG_FATAL, "mpp_buf_mutex init fail: %s\n", strerror(ret));
        goto fail;
    }
#endif
    ret = pthread_create(&is->read_tid, &global_attr, read_thread, is);
    // is->read_tid     = CreateThread(read_thread, is);
    // if (!is->read_tid) {
    if (0 != ret) {
        av_log(NULL, AV_LOG_FATAL, "CreateThread(): %s\n", strerror(ret));
    fail:
        stream_close(is);
        return NULL;
    }
    return is;
}

#ifdef PLAYING_IN_SEPARATE_PROCESS
static int lockmgr(void** mtx, enum AVLockOp op)
{
    pthread_mutex_t* mtx_t = *mtx;
    int ret = 0;
    av_assert0(mtx != 0);
    switch (op) {
    case AV_LOCK_CREATE:
        mtx_t = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
        if (!mtx_t)
            return 1;
        *mtx = mtx_t;
        //*mtx = CreateMutex();
        ret = pthread_mutex_init(mtx_t, NULL);
        // if(!*mtx) {
        if (0 != ret) {
            av_log(NULL, AV_LOG_FATAL, "CreateMutex(): %s\n", strerror(ret));
            return 1;
        }
        return 0;
    case AV_LOCK_OBTAIN:
        return !!pthread_mutex_lock(*mtx);
    case AV_LOCK_RELEASE:
        return !!pthread_mutex_unlock(*mtx);
    case AV_LOCK_DESTROY:
        if (mtx_t) {
            pthread_mutex_destroy(mtx_t);
            free(mtx_t);
            *mtx = 0;
        }
        return 0;
    }
    return 1;
}
#endif

/* Called from the main */
VideoState* vpu_player_main(char* filepath)
{
    VideoState* is;
    av_register_all();

    // av_log_set_level(AV_LOG_DEBUG);

    input_filename = filepath;

    if (!input_filename) {
        av_log(NULL, AV_LOG_FATAL, "An input file must be specified\n");
        exit(1);
    }

    if (display_disable) {
        video_disable = 1;
    }

#ifdef PLAYING_IN_SEPARATE_PROCESS
    if (av_lockmgr_register(lockmgr)) {
        av_log(NULL, AV_LOG_FATAL, "Could not initialize lock manager!\n");
        do_exit(NULL);
    }
#endif

    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t*)&flush_pkt;

    is = stream_open(input_filename, file_iformat);
    if (!is) {
        av_log(NULL, AV_LOG_FATAL, "Failed to initialize VideoState!\n");
        do_exit(NULL);
    }
    return is;
}

void videoplay_set_fb_black()
{
    int i;
    struct win* win;
    int screen_w, screen_h;
    rk_fb_get_out_device(&screen_w, &screen_h);

    for (i = 0; i < 2; i++) {
        win = rk_fb_getvideowin();
        video_ion_buffer_black(&win->video_ion, screen_w, screen_h);
        if (rk_fb_video_disp(win) == -1) {
            printf("rk_fb_video_disp err\n");
        }
    }
}

void* videoplay_thread(void* arg)
{
    struct sVideoplay_paramate* paramate = (struct sVideoplay_paramate*)arg;

    int sec = 0;
    double remaining_time = 0.0;
    VideoState* is = NULL;
    int rga_fd = -1;
#if 0
    REGISTER_SIGNAL_SIGSEGV;
    signal(SIGINT, sigterm_handler);  /* Interrupt (ANSI).    */
    signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */
#endif
    prctl(PR_SET_NAME, "videoplay_thread", 0, 0, 0);
    rga_fd = rk_rga_open();
    if (rga_fd <= 0) {
        goto out;
    }
    is = vpu_player_main(paramate->filename);
    if (!is) {
        rk_rga_close(rga_fd);
        goto out;
    }
    is->rga_fd = rga_fd;
    paramate->is = is;
    if (videoplay_event_call)
        (*videoplay_event_call)(CMD_VIDEOPLAY_UPDATETIME, (void*)sec, (void*)0);
    while (paramate->pthread_run) {
        int cursec = 0;
        int ret = 0;
        if (remaining_time > 0.0)
            av_usleep((int64_t)(remaining_time * 1000000.0));
        remaining_time = REFRESH_RATE;
        if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh)) {
            ret = video_refresh(is, &remaining_time);
            /**
             *  end thread when all queue is empty and eof==1
             *  eof == 1
             *  nb_packets == 0
             *  frame_queue_nb_remaining(&is->queue) == 0
             */
            if (is->eof == 1 &&
                (!is->viddec.queue || is->viddec.queue->nb_packets <= 0) &&
                (!is->auddec.queue || is->auddec.queue->nb_packets <= 0) &&
                (frame_queue_nb_remaining(&is->sampq) <= 0) && ret == -1) {
                paramate->pthread_run = 0;
            }
            if (show_status)
                fprintf(stderr, "---remaining_time- %lf\n", remaining_time);
        }
        cursec = (int)get_master_clock(is);
        if (sec != cursec) {
            sec = cursec;
            if (videoplay_event_call)
                (*videoplay_event_call)(CMD_VIDEOPLAY_UPDATETIME, (void*)sec, (void*)0);
        }
    }
out:
    if (videoplay_event_call)
        (*videoplay_event_call)(CMD_VIDEOPLAY_EXIT, (void*)0, (void*)0);
    printf("%s out\n", __func__);
    return 0;
}

static void reset_states()
{
    file_iformat = NULL;
    input_filename = NULL;
    window_title = NULL;
    default_width = 640;
    default_height = 480;
    audio_disable = 0;
    video_disable = 0;
    memset(wanted_stream_spec, 0, sizeof(wanted_stream_spec));
    seek_by_bytes = -1;
    display_disable = 0;
    av_sync_type = AV_SYNC_VIDEO_MASTER;  // AV_SYNC_AUDIO_MASTER; //
    start_time = AV_NOPTS_VALUE;
    duration = AV_NOPTS_VALUE;
    fast = 0;
    genpts = 0;
    lowres = 0;
    decoder_reorder_pts = -1;
    autoexit = 1;
    loop = 1;
    framedrop = -1;
    infinite_buffer = -1;
    show_mode = SHOW_MODE_VIDEO;
    rdftspeed = 0.02;
    audio_callback_time = 0;
    speed = 1;
}

int videoplay_init(char* filepath)
{
    int len = 0;
#if USE_MPP_IN_OUTSIDE
    MPP_RET ret;
#endif
    reset_states();
    if (pthread_attr_init(&global_attr)) {
        printf("pthread_attr_init failed!\n");
        return -1;
    }
    if (pthread_attr_setstacksize(&global_attr, STACKSIZE)) {
        printf("pthread_attr_setstacksize failed!\n");
        return -1;
    }
    av_assert0(filepath);
    len = strlen(filepath);
    av_assert0(len > 0);
#if USE_MPP_IN_OUTSIDE
    ret = mpp_buffer_group_get_internal(&g_mpp_buf_group, MPP_BUFFER_TYPE_ION);
    if (MPP_OK != ret) {
        av_log(NULL, AV_LOG_ERROR, "g_mpp_buf_group mpp_buffer_group_get failed\n");
        return -1;
    }
#endif
    if (videoplay_paramate == 0) {
        videoplay_paramate = calloc(1, sizeof(struct sVideoplay_paramate));
        av_assert0(videoplay_paramate);
        if (!videoplay_paramate)
            return -1;
        videoplay_paramate->pthread_run = 1;
        videoplay_paramate->filename = calloc(1, len + 1);
        av_assert0(videoplay_paramate->filename);
        if (!videoplay_paramate->filename) {
            return -1;
        }
        memcpy(videoplay_paramate->filename, filepath, len);
        if (pthread_create(&videoplay_paramate->tid, &global_attr, videoplay_thread,
                           (void*)videoplay_paramate)) {
            printf("%s pthread_create err\n", __func__);
            return -1;
        }
    }
    printf("%s out\n", __func__);
    return 0;
}

int videoplay_exit(void)
{
    if (videoplay_paramate) {
        videoplay_paramate->pthread_run = 0;
    }
    return 0;
}

int videoplay_deinit(void)
{
    printf("%s in\n", __func__);
    if (videoplay_paramate) {
        // videoplay_set_fb_black();
        videoplay_paramate->pthread_run = 0;
        pthread_join(videoplay_paramate->tid, NULL);
        do_exit(videoplay_paramate->is);
        videoplay_paramate->is = NULL;
        if (videoplay_paramate->filename) {
            free(videoplay_paramate->filename);
        }
        free(videoplay_paramate);
        videoplay_paramate = NULL;
    }
#if USE_MPP_IN_OUTSIDE
    if (g_mpp_buf_group != NULL) {
        mpp_buffer_group_put(g_mpp_buf_group);
        g_mpp_buf_group = NULL;
    }
#endif
    if (pthread_attr_destroy(&global_attr))
        printf("pthread_attr_destroy failed!\n");
    printf("%s out\n", __func__);
    return 0;
}

/* show whether videoplay_paramate ready */
static inline BOOL resource_is_ready(struct sVideoplay_paramate* handle)
{
    return handle && handle->is;
}

void videoplay_pause()
{
    if (resource_is_ready(videoplay_paramate)) {
        toggle_pause(videoplay_paramate->is);
    }
}

void videoplay_step_play()
{
    if (resource_is_ready(videoplay_paramate)) {
        speed = 1;
        step_to_next_frame(videoplay_paramate->is);
    }
}

void videoplay_seek_time(double time_step)
{
    if (resource_is_ready(videoplay_paramate)) {
        double pos;
        VideoState* cur_stream = videoplay_paramate->is;
        if (!cur_stream) {
            return;
        }

        if (seek_by_bytes) {
            pos = -1;
            if (pos < 0 && cur_stream->video_stream >= 0)
                pos = frame_queue_last_pos(&cur_stream->pictq);
            if (pos < 0 && cur_stream->audio_stream >= 0)
                pos = frame_queue_last_pos(&cur_stream->sampq);
            if (pos < 0)
                pos = avio_tell(cur_stream->ic->pb);
            if (cur_stream->ic->bit_rate)
                time_step *= cur_stream->ic->bit_rate / 8.0;
            else
                time_step *= 180000.0;
            pos += time_step;
            stream_seek(cur_stream, pos, time_step, 1);
        } else {
            pos = get_master_clock(cur_stream);
            if (isnan(pos))
                pos = (double)cur_stream->seek_pos / AV_TIME_BASE;
            pos += time_step;
            if (cur_stream->ic->start_time != AV_NOPTS_VALUE &&
                pos < cur_stream->ic->start_time / (double)AV_TIME_BASE)
                pos = cur_stream->ic->start_time / (double)AV_TIME_BASE;
            stream_seek(cur_stream, (int64_t)(pos * AV_TIME_BASE),
                        (int64_t)(time_step * AV_TIME_BASE), 0);
        }
    }
}

int videoplay_set_speed(int direction)
{
    if ((direction > 0 && speed >= 8) || (direction < 0 && speed <= 1)) {
        return speed;
    }
    if (resource_is_ready(videoplay_paramate)) {
        // VideoState *is = videoplay_paramate->is;
        pre_speed = speed;
        if (direction > 0) {
            speed = speed << (speed == 1 ? 2 : 1);
        } else {
            speed = speed >> (speed == 4 ? 2 : 1);
        }
        printf("speed: %d\n", speed);
    }
    return speed;
}

/*
static void init_video_win2(int width, int height)
{
    struct win* win;
    int w_align, h_align;
    win = rk_fb_getvideowin();
    if (!win) {
        printf("!!rk_fb_getvideowin return NULL\n");
        return;
    }
    win->buf_width = width;
    win->buf_height = height;
    win->buf_ion_fd = -1;
    w_align = (win->width + 15) & (~15);
    h_align = (win->height + 15) & (~15);
    if (w_align != win->video_ion.width || h_align != win->video_ion.height) {
        video_ion_free(&win->video_ion);
        if (video_ion_alloc(&win->video_ion, w_align, h_align)) {
            printf("%s video_ion_alloc fail!\n", __func__);
        }
    }
}
*/

static void draw_black_to_screen()
{
    int out_w;
    int out_h;

    struct win* video_win = rk_fb_getvideowin();
    rk_fb_get_out_device(&out_w, &out_h);
    video_ion_buffer_black(&video_win->video_ion, out_w, out_h);
    if (rk_fb_video_disp_ex(video_win, out_w, out_h))
        printf("rk_fb_video_disp_ex draw_black_to_screen err.\n");
}

int decode_one_video_frame(char* filepath,
                           image_handle image_buf,
                           int* width,
                           int* height,
                           int* coded_width,
                           int* coded_height,
                           enum AVPixelFormat* pix_fmt);
int videoplay_decode_one_frame(char* filepath)
{
    int ret = 0;
    int w = 0, h = 0;
    int coded_w = 0, coded_h = 0;
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_NV12;
    DataBuffer_VPU_t vpu_data_buf;
    void* ctx = NULL;
    int rga_fd = -1;
    if (!filepath) {
        return -1;
    }
    if ((rga_fd = rk_rga_open()) <= 0) {
        return -1;
    }
    ret = decode_init_context(&ctx, filepath);
    if (ret)
        goto err;
    ret = decode_one_video_frame(ctx, &vpu_data_buf, &w, &h, &coded_w, &coded_h,
                                 &pix_fmt);
    if (!ret) {
        // init_video_win2(w, h);
        display(rga_fd, vpu_data_buf.image_buf.phy_fd, w, h, coded_w, coded_h,
                pix_fmt);
        vpu_data_buf.free_func(vpu_data_buf.rkdec_ctx, &vpu_data_buf.image_buf);
    }
err:
    if (ctx)
        decode_deinit_context(&ctx);
    if (rga_fd > 0) {
        rk_rga_close(rga_fd);
    }
    if (ret)
        draw_black_to_screen();
    return ret;
}

void videoplay_decode_jpeg(char* filepath)
{
    struct vpu_decode* jpeg_decode = NULL;
    struct win* video_win = NULL;
    int width = 0;
    int height = 0;
    char* in_data = NULL;
    int in_size = 0;
    FILE* pInFile = NULL;
    char* pcur = NULL;
    char* plimit = NULL;
    struct video_ion rotate;
    int out_w, out_h;
    int outdevice;
    int src_fmt, dst_fmt;
    int rotate_angle = 0;

    memset(&rotate, 0, sizeof(struct video_ion));
    rotate.client = -1;
    rotate.fd = -1;

    outdevice = rk_fb_get_out_device(&out_w, &out_h);

    jpeg_decode = (struct vpu_decode*)calloc(1, sizeof(struct vpu_decode));
    if (!jpeg_decode) {
        printf("no memory!\n");
        goto exit1;
    }

    pInFile = fopen(filepath, "rb");
    if (!pInFile) {
        printf("JEPG file no exist!\n");
        goto exit1;
    }

    fseek(pInFile, 0L, SEEK_END);
    in_size = ftell(pInFile);
    fseek(pInFile, 0L, SEEK_SET);
    in_data = (char*)malloc(in_size);
    if (!in_data) {
        printf("no memory!\n");
        goto exit1;
    }
    fread(in_data, 1, in_size, pInFile);

    pcur = in_data;
    plimit = in_data + in_size;
    while ((((pcur[0] << 8) | pcur[1]) != 0xffc0) && (pcur < plimit)) {
        pcur++;
    }
    if (pcur < plimit) {
        pcur += 5;
        height = *pcur * 256 + *(pcur + 1);
        pcur += 2;
        width = *pcur * 256 + *(pcur + 1);
    } else {
        printf("SOF0 of JPEG not exist\n");
        goto exit1;
    }

    // WIDTH HEIGHT
    if (vpu_decode_jpeg_init(jpeg_decode, width, height)) {
        printf("%s vpu_decode_jpeg_init fail\n", __func__);
        goto exit1;
    }

    video_win = rk_fb_getvideowin();

    if (video_ion_alloc_rational(&rotate, width, height, 2, 1)) {
        printf("%s video_ion_alloc fail!\n", __func__);
        goto exit1;
    }

    if (vpu_decode_jpeg_doing(jpeg_decode, in_data, in_size, rotate.fd,
                              (char*)rotate.buffer)) {
        printf("vpu_decode_jpeg_doing err\n");
        goto exit1;
    }

    if (MPP_FMT_YUV422SP == jpeg_decode->fmt)
        src_fmt = RGA_FORMAT_YCBCR_422_SP;
    else
        src_fmt = RGA_FORMAT_YCBCR_420_SP;
    dst_fmt = RGA_FORMAT_YCBCR_420_SP;
    rotate_angle =
        (((outdevice == OUT_DEVICE_HDMI) || (outdevice == OUT_DEVICE_CVBS_PAL) ||
          (outdevice == OUT_DEVICE_CVBS_NTSC))
         ? 0
         : VIDEO_DISPLAY_ROTATE_ANGLE);

    if (parameter_get_video_flip()) {
        rotate_angle += 180;
        rotate_angle %= 360;
    }

    rk_rga_ionfd_to_ionfd_rotate_ext(rotate.fd, width, height, src_fmt,
                                     ((width + 15) & ~15), (height + 15) & ~15,
                                     video_win->video_ion.fd, out_w, out_h,
                                     dst_fmt, rotate_angle);

    if (rk_fb_video_disp_ex(video_win, out_w, out_h)) {
        printf("rk_fb_video_disp_ex err\n");
        goto exit1;
    }

    goto exit;

exit1:
    video_win = rk_fb_getvideowin();

    video_ion_buffer_black(&video_win->video_ion, out_w, out_h);

    if (rk_fb_video_disp_ex(video_win, out_w, out_h)) {
        printf("rk_fb_video_disp_ex err\n");
        goto exit;
    }

exit:
    video_ion_free(&rotate);

    vpu_decode_jpeg_done(jpeg_decode);

    if (in_data) {
        free(in_data);
        in_data = NULL;
    }

    if (pInFile) {
        fclose(pInFile);
        pInFile = NULL;
    }

    if (jpeg_decode) {
        free(jpeg_decode);
        jpeg_decode = NULL;
    }
}
