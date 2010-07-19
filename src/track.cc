#include "track.h"
#include "album.h"
#include "artist.h"

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
  , album_(NULL)
{
}

Track::~Track() {
  if (track_)
    sp_track_release(track_);
  if (album_)
    delete album_;
}

Handle<Value> Track::New(sp_track *track) {
  HandleScope scope;
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
  HandleScope scope;
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

  return true;
}

Handle<Value> Track::LoadedGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Track *p = Unwrap<Track>(info.This());
  return p->track_
    ? scope.Close(Boolean::New(sp_track_is_loaded(p->track_)))
    : Undefined();
}

Handle<Value> Track::AlbumGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Track *p = Unwrap<Track>(info.This());
  if (!p->track_ || !sp_track_is_loaded(p->track_))
    return Undefined();
  if (!p->album_) {
    Local<Object> instance = Album::New(sp_track_album(p->track_));
    p->album_ = ObjectWrap::Unwrap<Album>(instance);
  }
  return scope.Close(p->album_->handle_);
}

Handle<Value> Track::ArtistsGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Track *p = Unwrap<Track>(info.This());
  if (!p->track_ || !sp_track_is_loaded(p->track_))
    return Undefined();

  int count = sp_track_num_artists(p->track_);
  Local<Array> array = Array::New(count);
  for (int i = 0; i < count; i++) {
    array->Set(Integer::New(i), Artist::New(sp_track_artist(p->track_, i)));
  }

  scope.Close(array);
}

Handle<Value> Track::URIGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Track *p = Unwrap<Track>(info.This());
  if (!p->track_ || !sp_track_is_loaded(p->track_))
    return Undefined();
  char uri_buf[40]; // spotify:track:0EcVDS2dfZR5RytFhFOtYH
  sp_link *link = sp_link_create_from_track(p->track_, 0);
  if (!link)
    return Undefined();
  sp_link_as_string(link, uri_buf, sizeof(uri_buf));
  sp_link_release(link);
  return scope.Close(String::New(uri_buf));
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
  instance_t->SetAccessor(String::New("album"), AlbumGetter);
  instance_t->SetAccessor(String::New("artists"), ArtistsGetter);
  instance_t->SetAccessor(String::New("uri"), URIGetter);

  target->Set(String::New("Track"), constructor_template->GetFunction());
}
