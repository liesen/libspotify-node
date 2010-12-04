#ifndef SPOTIFY_CALLBACK_QUEUE_H_
#define SPOTIFY_CALLBACK_QUEUE_H_

#include "index.h"
#include "queue.h"

#define CallbackQueueForEach(entry, q) TAILQ_FOREACH(entry, &(q)->queue_, link)

// queue for various callbacks waiting on a particular state
typedef struct CallbackQueueEntry {
  // previous and next siblings in the doubly linked-list
  TAILQ_ENTRY(CallbackQueueEntry) link;
  // callback to be invoked
  Persistent<Function> *callback;
  // optional type (e.g. "it's a track waiting for load event")
  int type;
  // optional object/userdata (e.g. a sp_track ref)
  void *obj;
} CallbackQueueEntry;

class CallbackQueue {
 public:
  TAILQ_HEAD(, CallbackQueueEntry) queue_;
  CallbackQueue() { TAILQ_INIT(&queue_); }
  void push(const Local<Value> &callback, int type = 0, void *obj = NULL);
  void push(const Local<Value> &callback, sp_track *track);
  void remove(CallbackQueueEntry *entry);
  int process(sp_session* session, const Handle<Object> &context, bool once = false);
};

#endif // SPOTIFY_CALLBACK_QUEUE_H_
