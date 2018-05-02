// by wangh
// mussy codes, TO BE REWRITTEN
#ifndef CAMERA_MUXER_H
#define CAMERA_MUXER_H

#include "../../video_common.h"
#include "../../message_queue.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <atomic>

class BaseEncoder;

typedef struct {
    AVFormatContext* av_format_context;
    AVStream* audio_avstream;
    AVStream* video_avstream;
#if USE_GPS_MOVTEXT
    AVStream* subtitle_avstream;
#endif
    struct timeval v_first_timeval;
    int64_t pre_video_pts;
    uint64_t audio_first_index;
    bool got_first_video;
    bool got_first_audio;
} MuxProcessor;

class CameraMuxer // comsumer
{
public:
    typedef enum {
        MUXER_START = 1,
        /* exit muxer thread as soon as possible without completing all packets in
           list and tailer */
        MUXER_NORMAL_EXIT,
        /* exit muxer thread until completing all packets in list and tailer */
        MUXER_FLUSH_EXIT,
        /* exit muxer thread immediately without any operation */
        MUXER_IMMEDIATE_EXIT
    } MUXER_MSG_ID;
    virtual bool init_subtitle_stream(AVStream *stream) { return false; };

private:
    BaseEncoder* v_encoder;
    BaseEncoder* a_encoder;
    pthread_t write_tid; // thread of writing
    std::list<pthread_t> w_tid_list; // threads of writing, size shoule be never more than 2
    std::mutex w_tidlist_mutex;

    int video_packet_num;
    std::list<EncodedPacket*>* packet_list; // normally 30fps, max_size a
    // little more than
    // second*30*20k=600k*second
    std::mutex packet_list_mutex;
    std::condition_variable packet_list_cond;

    MessageQueue<MUXER_MSG_ID> msg_queue;

#if 0//def DEBUG
    int packet_total_size;
#endif
protected:
    Runnable run_func;
    char format[16];
    bool direct_piece; // write directly with piece
    int video_sample_rate;
    MUXER_MSG_ID exit_id;
    int max_video_num;
    bool no_async;
    MuxProcessor global_processor; // used when no_async=true
    std::atomic_bool processor_ready;
    std::condition_variable global_processor_cond;
    virtual int init();
    void free_packet_list();
    EncodedPacket* pop_packet();

public:
#ifdef DEBUG
    char class_name[32]; // debug
#endif
    volatile bool write_enable; // when moving enable writefile or explicitly enable
    EncodedPacket empty_packet;
    CameraMuxer();
    virtual ~CameraMuxer();
    virtual void push_packet(EncodedPacket* pkt);
    EncodedPacket* wait_pop_packet(bool wait = true);

    // single muxer
    virtual int init_muxer_processor(MuxProcessor* process);
    virtual void deinit_muxer_processor(AVFormatContext* oc);
    virtual int muxer_write_header(AVFormatContext* oc, char* url);
    virtual int muxer_write_tailer(AVFormatContext* oc);
    virtual int muxer_write_free_packet(MuxProcessor* process, EncodedPacket* pkt);
    virtual int muxer_write_packet(AVFormatContext* ctx, AVStream* st, AVPacket* pkt);

    virtual int init_uri(char* uri, int rate);
    int start_new_job(pthread_attr_t* attr);
    void remove_wtid(pthread_t tid);
    virtual void stop_current_job();
    void sync_jobs_complete(); // wait all threads finish
    inline virtual void set_encoder(BaseEncoder* video_encoder,
                                    BaseEncoder* audio_encoder)
    {
        v_encoder = video_encoder;
        a_encoder = audio_encoder;
    }
    inline BaseEncoder* get_video_encoder()
    {
        assert(v_encoder);
        return v_encoder;
    }
    inline BaseEncoder* get_audio_encoder()
    {
        return a_encoder;
    }
    inline char* get_out_format()
    {
        return format;
    }
    void set_out_format(const char* fmt)
    {
        snprintf(format, sizeof(format), fmt);
    }
    Message<MUXER_MSG_ID>* getMessage();
    /* new thread, new list */
    void set_packet_list(std::list<EncodedPacket*>* list,
                         std::list<EncodedPacket*>** backup);
    int getVSampleRate()
    {
        return video_sample_rate;
    }

    void setMaxVideoNum(int max_vn)
    {
        max_video_num = max_vn;
    }

    inline bool use_data_async()
    {
        return !no_async;
    }
    inline void set_async(bool async)
    {
        no_async = !async;
    }
    inline void set_exit_id(MUXER_MSG_ID id)
    {
        exit_id = id;
    }
    void clear_packet_list();
};

class FFContext;
class MPPH264Encoder;
int copy_extradata(FFContext& ff_ctx, void* extra_data, size_t extra_data_size);

#endif // CAMERA_MUXER_H
