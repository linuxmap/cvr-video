// by wangh

#include <algorithm>

#include "muxing/camera_muxer.h"
#include "packet_dispatcher.h"

static void _Dispatch(std::mutex& list_mutex,
                      EncodedPacket* pkt,
                      std::list<CameraMuxer*>* muxers,
                      std::list<CameraMuxer*>* except_muxers) {
  assert(muxers);
  std::lock_guard<std::mutex> lk(list_mutex);
  if (pkt->is_phy_buf && !pkt->release_cb) {
    bool sync = true;
    for (CameraMuxer* muxer : *muxers) {
      if (except_muxers &&
          std::find(except_muxers->begin(), except_muxers->end(), muxer) !=
              except_muxers->end())
        continue;
      if (muxer->use_data_async()) {
        sync = false;
        break;
      }
    }
    if (!sync)
      pkt->copy_av_packet();  // need copy buffer
  }

  for (CameraMuxer* muxer : *muxers) {
    if (except_muxers &&
        std::find(except_muxers->begin(), except_muxers->end(), muxer) !=
            except_muxers->end())
      continue;
    muxer->push_packet(pkt);
  }
}

void PacketDispatcher::Dispatch(EncodedPacket* pkt) {
  _Dispatch(list_mutex, pkt, &handlers, &special_muxers_);
}

void PacketDispatcher::DispatchToSpecial(EncodedPacket* pkt) {
  _Dispatch(list_mutex, pkt, &special_muxers_, nullptr);
}

void PacketDispatcher::AddSpecialMuxer(CameraMuxer* muxer) {
  std::lock_guard<std::mutex> lk(list_mutex);
  special_muxers_.push_back(muxer);
}
void PacketDispatcher::RemoveSpecialMuxer(CameraMuxer* muxer) {
  std::lock_guard<std::mutex> lk(list_mutex);
  special_muxers_.remove(muxer);
}
