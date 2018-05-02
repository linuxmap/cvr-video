// by wangh
#ifndef PACKETLIST_H
#define PACKETLIST_H

#include "../dispatcher.h"

class CameraMuxer;

class PacketDispatcher : public Dispatcher<CameraMuxer, EncodedPacket> {
 public:
  virtual ~PacketDispatcher() { assert(special_muxers_.empty()); }
  virtual void Dispatch(EncodedPacket* pkt) override;
  void DispatchToSpecial(EncodedPacket* pkt);
  void AddSpecialMuxer(CameraMuxer* muxer);
  void RemoveSpecialMuxer(CameraMuxer* muxer);
  bool HasSpecialMuxer() { return !special_muxers_.empty(); }

 private:
  std::list<CameraMuxer*> special_muxers_;
};

#endif  // PACKETLIST_H
