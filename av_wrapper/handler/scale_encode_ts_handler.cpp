/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: hertz.wang hertz.wong@rock-chips.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "scale_encode_ts_handler.h"
#include "encoder_muxing/encoder/ffmpeg_wrapper/ff_audio_encoder.h"

#include <poll.h>
#include <unistd.h>
#include <sys/prctl.h>

#include <atomic>

class LiveState
{
public:
    enum { READY = 0, NONE, RUN, PAUSE, RERUN };
#ifdef DEBUG
    static const char* kStateInfo[RERUN + 1];
#endif
    LiveState() : live_state_(NONE) {}
    bool SetState(int state, int& cur_state);

private:
    std::atomic_int live_state_;
};

#ifdef DEBUG
const char* LiveState::kStateInfo[RERUN + 1] = {"READY", "NONE", "RUN", "PAUSE",
                                                "RERUN"
                                               };
#endif

bool LiveState::SetState(int next_state, int& cur_state)
{
    PRINTF("- next_state: %d\n", next_state);
    int expect = -1;
    bool ret = false;
    switch (next_state) {
    case READY:
        expect = NONE;
        break;
    case RUN:
        expect = READY;
        ret = live_state_.compare_exchange_strong(expect, next_state);
        if (!ret) {
            expect = PAUSE;
            ret = live_state_.compare_exchange_strong(expect, next_state);
        }
        if (!ret)
            expect = RERUN;
        else
            goto out;
        break;
    case PAUSE:
        expect = RUN;
        break;
    case NONE:
        expect = PAUSE;
        ret = live_state_.compare_exchange_strong(expect, next_state);
        if (!ret) {
            if (expect == NONE)
                goto out;
            printf(
                "Warning: live destroy when not pausing, set state to recreate "
                "if running\n");
            expect = RUN;
            next_state = RERUN;
        } else {
            goto out;
        }
        break;
    }
    if (expect >= 0)
        ret = live_state_.compare_exchange_strong(expect, next_state);

out:
    cur_state = expect;
#ifdef DEBUG
    if (!ret)
        printf("can not change state [%s] to [%s]\n", kStateInfo[expect],
               kStateInfo[next_state]);
#endif
    PRINTF("-expect: %d, next_state: %d\n", expect, next_state);
    return ret;
}

static LiveState g_live_state;

#define CMDSTARTSTREAM "live:start_stream"
#define CMDSTOPSTREAM "live:stop_stream"

// For pipe rtsp, communicate with live555.
static void* _commu_routine(void* arg)
{
    ScaleEncodeTSHandler* ts_handler = static_cast<ScaleEncodeTSHandler*>(arg);

    prctl(PR_SET_NAME, "_commu_routine", 0, 0, 0);

    while (ts_handler->Valid()) {
        CameraTs* ts = ts_handler->GetTsMuxer();
        int fd = ts_handler->GetCommuFd();
        if (fd < 0)
            continue;
        struct pollfd clientfds;
        clientfds.fd = fd;
        clientfds.events = POLLIN | POLLERR;
        clientfds.revents = 0;
        int p_ret = poll(&clientfds, 1, 50);
        if (p_ret < 0 || !(clientfds.revents & POLLIN))
            continue;
        char str[128];
        ssize_t ret = read(fd, str, sizeof str);
        if (ret <= 0) {
            fprintf(stderr, "client read from commu fd failed, errno: %d\n", errno);
            continue;
        }
        fprintf(stderr, "got request string: %s\n", str);
        PacketDispatcher* dispatcher = ts_handler->GetPacketDispatcher();
        int cur_live_state = -1;
        if (!strcmp(str, CMDSTARTSTREAM) &&
            g_live_state.SetState(LiveState::RUN, cur_live_state)) {
            BaseVideoEncoder* ve = (BaseVideoEncoder*)ts_handler->GetVideoEncoder();
            ve->SetForceIdrFrame();
            ts->SetRunning();
            dispatcher->AddHandler(ts);
        } else if (!strcmp(str, CMDSTOPSTREAM)) {
            dispatcher->RemoveHandler(ts);
            BaseVideoEncoder* ve = (BaseVideoEncoder*)ts_handler->GetVideoEncoder();
            ve->SetForceIdrFrame();
            ts->SetPausing();
            ts->clear_packet_list();
            g_live_state.SetState(LiveState::PAUSE, cur_live_state);
        } else
            fprintf(stderr, "unknown request string: %s\n", str);
    }
    pthread_exit(nullptr);
}

const char* ScaleEncodeTSHandler::kCommuPath = "/tmp/live_commu_fifo";

ScaleEncodeTSHandler::ScaleEncodeTSHandler(MediaConfig& config)
    : ts_(nullptr),
      scale_config_(config),
      time_val_{0, 0},
      commu_fd_(-1),
      commu_tid_(0),
      valid_(false) {}

ScaleEncodeTSHandler::~ScaleEncodeTSHandler()
{
    assert(!ts_);
    DeInit();
}

int ScaleEncodeTSHandler::Init(MediaConfig src_config)
{
    if (ScaleEncodeHandler<MPPH264Encoder>::Init(
            src_config, 3, 2, scale_config_.video_config.width,
            scale_config_.video_config.height))
        return -1;
    if (ff_ctx_.InitConfig(src_config.video_config, false))
        return -1;
    void* extra_data = nullptr;
    size_t extra_data_size = 0;
    encoder_->GetExtraData(extra_data, extra_data_size);
    if (copy_extradata(ff_ctx_, extra_data, extra_data_size))
        return -1;
    encoder_->SetHelpContext(&ff_ctx_);
    return 0;
}

void ScaleEncodeTSHandler::DeInit()
{
    ScaleEncodeHandler<MPPH264Encoder>::DeInit();
    StopTransferStream();
}

void ScaleEncodeTSHandler::GetSrcConfig(const MediaConfig& src_config,
                                        int& src_w,
                                        int& src_h,
                                        PixelFormat& src_fmt)
{
    src_w = src_config.video_config.width;
    src_h = src_config.video_config.height;
    src_fmt = src_config.video_config.fmt;
}

int ScaleEncodeTSHandler::PrepareBuffers(MediaConfig& src_config,
                                         const int dst_numerator,
                                         const int dst_denominator,
                                         int& dst_w,
                                         int& dst_h)
{
    int ret = ScaleEncodeHandler<MPPH264Encoder>::PrepareBuffers(
                  src_config, dst_numerator, dst_denominator, dst_w, dst_h);
    if (ret)
        return ret;
    VideoConfig& vconfig = src_config.video_config;
    vconfig = scale_config_.video_config;
    vconfig.fmt = kCacheFmt;
    vconfig.width = dst_w;
    vconfig.height = dst_h;
    return 0;
}

void ScaleEncodeTSHandler::Work()
{
    if (!ts_)
        return;
    EncodedPacket* pkt = new EncodedPacket();
    if (!pkt) {
        printf("alloc EncodedPacket failed\n");
        assert(0);
        return;
    }
    Buffer src(yuv_data_);
    Buffer dst(dst_data_);
    int ret = encoder_->EncodeOneFrame(&src, &dst, nullptr);
    if (!ret) {
        pkt->type = VIDEO_PACKET;
        pkt->is_phy_buf = false;
        ret = FFContext::PackEncodedDataToAVPacket(dst, pkt->av_pkt,
                                                   !pkt->is_phy_buf);
        if (!ret) {
            if (dst.GetUserFlag() & MPP_PACKET_FLAG_INTRA)
                pkt->av_pkt.flags |= AV_PKT_FLAG_KEY;
            // rk guarantee that return only one slice, and start with 00 00 01
            pkt->av_pkt.flags |= AV_PKT_FLAG_ONE_NAL;
            pkt->time_val = time_val_;
            // ts_.push_packet(pkt);
            pkt_dispatcher_.Dispatch(pkt);
        }
    }
    pkt->unref();
}

void ScaleEncodeTSHandler::Process(int src_fd,
                                   const VideoConfig& src_config,
                                   const struct timeval& time)
{
    if (!ts_)
        return;
    std::lock_guard<std::mutex> _lk(mtx_);
    if (ts_ && !ts_->Running())
        return;
    if (ts_ && ts_->Error() < 0) {
        mtx_.unlock();
        StopTransferStream();
        mtx_.lock();
    }
    // AutoPrintInterval hh("ScaleEncodeTSHandler::Process");
    time_val_ = time;
    ScaleEncodeHandler<MPPH264Encoder>::Process(src_fd, src_config);
}

int ScaleEncodeTSHandler::StartTransferStream(char* uri,
                                              pthread_attr_t* global_attr,
                                              FFAudioEncoder* audio_enc)
{
    std::lock_guard<std::mutex> _lk(mtx_);
    int cur_live_state = -1;
    if (ts_)
        return -1;
    ts_ = CameraTs::create();
    if (!ts_) {
        printf("create camerats failed\n");
        return -1;
    }
    char uri_str[256];
    snprintf(uri_str, sizeof(uri_str), "%s", uri);
    VideoConfig& vconfig = encoder_->GetVideoConfig();
    printf("ts: vconfig w/h -> (%d/%d)\n", vconfig.width, vconfig.height);
    if (ts_->init_uri(uri_str, vconfig.frame_rate) < 0) {
        printf("ts init %s failed\n", uri_str);
        goto err;
    }
    ts_->set_encoder(encoder_, audio_enc);
    if (ts_->start_new_job(global_attr) < 0) {
        goto err;
    }
    valid_ = true;
    g_live_state.SetState(LiveState::READY, cur_live_state);
    if (ts_->StartByOutside()) {
        commu_fd_ = CameraTs::OpenPipe(kCommuPath);
        if (commu_fd_ < 0 || ::StartThread(commu_tid_, _commu_routine, this))
            goto err;
        if (cur_live_state == LiveState::RERUN &&
            write(commu_fd_, CMDSTARTSTREAM, sizeof(CMDSTARTSTREAM)) !=
            sizeof(CMDSTARTSTREAM))
            printf("write rerun %s failed, errno : %d\n", CMDSTARTSTREAM, errno);
    } else {
        g_live_state.SetState(LiveState::RUN, cur_live_state);
        pkt_dispatcher_.AddHandler(ts_);
    }
    return 0;

err:
    CameraTs::ClosePipe(commu_fd_);
    valid_ = false;
    ::StopThread(commu_tid_);
    delete ts_;
    ts_ = nullptr;
    g_live_state.SetState(LiveState::NONE, cur_live_state);
    return -1;
}

int ScaleEncodeTSHandler::StopTransferStream()
{
    std::lock_guard<std::mutex> _lk(mtx_);
    int cur_live_state = -1;
    if (!ts_)
        return -1;
    pkt_dispatcher_.RemoveHandler(ts_);
    if (!ts_->StartByOutside())
        g_live_state.SetState(LiveState::PAUSE, cur_live_state);
    ts_->stop_current_job();
    CameraTs::ClosePipe(commu_fd_);
    valid_ = false;
    ::StopThread(commu_tid_);
    delete ts_;
    ts_ = nullptr;
    g_live_state.SetState(LiveState::NONE, cur_live_state);
    return 0;
}
