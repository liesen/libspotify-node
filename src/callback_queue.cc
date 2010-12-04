#include "callback_queue.h"
#include "track.h"
#include "album.h"
#include "artist.h"

void CallbackQueue::push(const Local<Value> &callback, int type /*= 0*/,
                         void *obj /*= NULL*/) {
  CallbackQueueEntry *entry = new CallbackQueueEntry;
  entry->callback = cb_persist(callback);
  entry->type = type;
  entry->obj = obj;
  TAILQ_INSERT_TAIL(&queue_, entry, link);
}

void CallbackQueue::push(const Local<Value> &callback, sp_track *track) {
  assert(sp_track_error(track) == SP_ERROR_IS_LOADING);
  sp_track_add_ref(track);
  push(callback, kMetadataUpdateTypeTrack, reinterpret_cast<void*>(track));
}

void CallbackQueue::remove(CallbackQueueEntry *entry) {
  if (entry->type == kMetadataUpdateTypeTrack && entry->obj)
    sp_track_release(reinterpret_cast<sp_track*>(entry->obj));
  if (entry->callback)
    cb_destroy(entry->callback);
  TAILQ_REMOVE(&queue_, entry, link);
  delete entry;
}

int CallbackQueue::process(sp_session* session,
                           const Handle<Object> &context,
                           bool once /*= false*/) {
  int hits = 0;
  CallbackQueueEntry *entry;
  TAILQ_FOREACH(entry, &queue_, link) {
    sp_error status;

    if (entry->type == kMetadataUpdateTypeTrack) {
      sp_track *t = reinterpret_cast<sp_track *>(entry->obj);
      status = sp_track_error(t);
      if (status != SP_ERROR_IS_LOADING) {
        // state has changed -- invoke callback
        Handle<Value> err = Undefined();
        if (status == SP_ERROR_OK) {
          // loaded
          Handle<Value> argv[] = { Undefined(), Track::New(session, t) };
          (*entry->callback)->Call(context, 2, argv);
        } else {
          // an error occured
          Handle<Value> argv[] = {
            Exception::Error(String::New(sp_error_message(status))) };
          (*entry->callback)->Call(context, 1, argv);
        }
      }
    } // kMetadataUpdateTypeTrack

    // are we done with this entry?
    if (status != SP_ERROR_IS_LOADING) {
      remove(entry);
      hits++;
      if (once) return hits;
    }
  }
  return hits;
}
