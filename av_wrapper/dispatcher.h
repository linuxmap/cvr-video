// by wangh
#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <functional>

#include "video_common.h"

template <class T, typename DATA>
class Dispatcher  // DataDispatcher
{
 private:
  void CallBackIfEmpty() {
    if (handlers.empty()) {
      for (std::function<void(void)>& func : cb_when_empty)
        func();
    }
  }

 protected:
  std::mutex list_mutex;
  std::list<T*> handlers;
  std::list<std::function<void(void)>> cb_when_empty;

 public:
  Dispatcher() {}
  virtual ~Dispatcher() { assert(handlers.size() == 0); }
  void AddHandler(T* handler) {
    std::lock_guard<std::mutex> lk(list_mutex);
    UNUSED(lk);
    handlers.push_back(handler);
  }
  void RemoveHandler(T* handler) {
    std::lock_guard<std::mutex> lk(list_mutex);
    UNUSED(lk);
    handlers.remove(handler);
    CallBackIfEmpty();
  }
  // return if remove successfully
  void RemoveHandler(T* handler, bool& exist) {
    std::lock_guard<std::mutex> lk(list_mutex);
    exist = false;
    for (T* element : handlers) {
      if (element == handler) {
        handlers.remove(handler);
        CallBackIfEmpty();
        exist = true;
        return;
      }
    }
  }
  virtual void Dispatch(DATA* pkt) {}
  bool Empty() {
    std::lock_guard<std::mutex> lk(list_mutex);
    return handlers.empty();
  }
  void RegisterCbWhenEmpty(std::function<void(void)>& func) {
    cb_when_empty.push_back(std::ref(func));
  }
  void UnRegisterCbWhenEmpty() { cb_when_empty.pop_front(); }
};

#endif  // DISPATCHER_H
