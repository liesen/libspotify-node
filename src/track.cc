#include "track.h"

using namespace v8;
using namespace node;

#define NS_THROW(_type_, _msg_)\
  return ThrowException(Exception::_type_(String::New(_msg_)))

Persistent<FunctionTemplate> Track::constructor_template;

// -----------------------------------------------------------------------------
// Track implementation

Track::Track(sp_track *track)
  : node::EventEmitter()
  , track_(track)
{
}

Track::~Track() {
  if (track_)
    sp_track_release(track_);
}

Handle<Value> Track::New(sp_track *track) {
  Local<Object> instance = constructor_template->GetFunction()->NewInstance(0, NULL);
  Track *p = ObjectWrap::Unwrap<Track>(instance);
  p->track_ = track;
  if (p->track_) {
    sp_track_add_ref(p->track_);
    if (!p->SetupBackingTrack())
      NS_THROW(Error, sp_error_message(sp_track_error(p->track_)));
  }
  return instance;
}

Handle<Value> Track::New(const Arguments& args) {
  // todo: if called with a string argument, try to parse and load it as a link
  (new Track(NULL))->Wrap(args.This());
  return args.This();
}

bool Track::SetupBackingTrack() {
  if (!track_) return true;
  
  // status check
  switch (sp_track_error(track_)) {
   case SP_ERROR_OK:
    break;
   case SP_ERROR_IS_LOADING:
    // possible solution: nextTick -> check, .. until loaded
    fprintf(stderr, "todo [%s:%d]: track is not yet loaded\n",__FILE__,__LINE__);
    return true;
   default:
    return false;
  }
  
  // todo: symbolize keys
  
  handle_->Set(String::New("name"), String::New(sp_track_name(track_)));
  handle_->Set(String::New("available"), Boolean::New(sp_track_is_available(track_)));
  handle_->Set(String::New("duration"), Integer::New(sp_track_duration(track_)));
  handle_->Set(String::New("popularity"),
    Number::New( (float)sp_track_popularity(track_)/100.0 ));

  // only available for browse results:
  int d = sp_track_index(track_);
  if (d) handle_->Set(String::New("discIndex"), Integer::New(d));
  d = sp_track_disc(track_);
  if (d) handle_->Set(String::New("disc"), Integer::New(d));

  // todo: lazy getters for artists and album
  // [sp_track_num_artists & sp_track_artist ...]
  // sp_track_album

  return true;
}

Handle<Value> Track::LoadedGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Track *p = Unwrap<Track>(info.This());
  return scope.Close(Boolean::New(sp_track_is_loaded(p->track_)));
}

void Track::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("Track"));
  constructor_template->Inherit(EventEmitter::constructor_template);
  
  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(String::New("loaded"), LoadedGetter);

  target->Set(String::New("Track"), constructor_template->GetFunction());
}
