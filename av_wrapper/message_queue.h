#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H

// by wangh
#include <condition_variable>
#include <list>
#include <mutex>

typedef enum { HIGH_PRIORITY, MID_PRIORITY, LOW_PRIORITY } MessagePriority;

template <typename T>
class Message {
 private:
  T m_id;
  MessagePriority m_priority;
  Message() {}

 public:
  void* arg1;  // be careful for its lifetime
  void* arg2;
  void* arg3;
  static Message<T>* create(T id, MessagePriority priority = MID_PRIORITY) {
    Message<T>* m = new Message<T>();
    if (!m)
      return nullptr;
    m->m_id = id;
    m->m_priority = priority;
    m->arg1 = NULL;
    m->arg2 = NULL;
    m->arg3 = NULL;
    return m;
  }
  inline T GetID() { return m_id; }
  inline MessagePriority GetPriority() { return m_priority; }
  bool PriorityHighThan(Message& msg) { return m_priority < msg.GetPriority(); }
  bool PriorityLowOrEqual(Message& msg) {
    return m_priority >= msg.GetPriority();
  }
};

template <typename T>
class MessageQueue {
 private:
  std::mutex mtx;
  std::condition_variable cond;
  std::list<Message<T>*> msg_list;

 public:
  MessageQueue() {}
  ~MessageQueue() { assert(msg_list.empty()); }
  void Push(Message<T>* msg, bool signal = false) {
    std::unique_lock<std::mutex> lk(mtx);
    msg_list.push_back(msg);
    if (signal) {
      cond.notify_one();
    }
  }
  Message<T>* PopFront() {
    std::unique_lock<std::mutex> lk(mtx);
    assert(!msg_list.empty());
    Message<T>* msg = msg_list.front();
    msg_list.pop_front();
    return msg;
  }
  void PopMessageOfId(T id, std::list<Message<T>*>& store_list) {
    std::unique_lock<std::mutex> lk(mtx);
    if (msg_list.empty())
      return;
    auto it = msg_list.begin();
    for (; it != msg_list.end();) {
      Message<T>* m = *it;
      if (m->GetID() == id) {
        it = msg_list.erase(it);
        store_list.push_back(m);
        continue;
      }
      it++;
    }
  }
  bool Empty() {
    std::unique_lock<std::mutex> lk(mtx);
    return msg_list.empty();
  }
  int size() {
    std::unique_lock<std::mutex> lk(mtx);
    return msg_list.size();
  }
  void WaitMessage() {
    std::unique_lock<std::mutex> lk(mtx);
    cond.wait(lk);
  }
  void clear() {
    std::unique_lock<std::mutex> lk(mtx);
    for (Message<T>* m : msg_list)
      delete m;
    msg_list.clear();
  }
};

#endif  // MESSAGEQUEUE_H
