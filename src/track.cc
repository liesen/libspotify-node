#include "track.h"
#include "album.h"
#include "artist.h"

Persistent<FunctionTemplate> Track::constructor_template;

// -----------------------------------------------------------------------------
// Track implementation

Track::Track(sp_track *track) : track_(track), album_(NULL) {
  if (track_)
    sp_track_add_ref(track_);
}

Track::~Track() {
  if (track_)
    sp_track_release(track_);

  if (album_)
    delete album_;
}

Handle<Value> Track::New(sp_track *track) {
  HandleScope scope;
  Local<Function> constructor = constructor_template->GetFunction();
  Local<Object> instance = constructor->NewInstance(0, NULL);
  Track *p = new Track(track);
  p->Wrap(instance);

  if (p->track_ && !p->SetupBackingTrack())
    return JS_THROW(Error, sp_error_message(sp_track_error(p->track_)));

  return scope.Close(instance);
}

Handle<Value> Track::New(const Arguments& args) {
  return args.This();
}

/**
 * Sets all permanent properties on a track.
 */
bool Track::SetupBackingTrack() {
  if (!track_) return true;

  // status check
  switch (sp_track_error(track_)) {
  case SP_ERROR_OK:
    break;
  case SP_ERROR_IS_LOADING:
    return true;
  default:
    return false;
  }

  // todo: symbolize keys

  handle_->Set(String::New("name"), String::New(sp_track_name(track_)));
  handle_->Set(String::New("available"),
               Boolean::New(sp_track_is_available(track_)));
  handle_->Set(String::New("duration"),
               Integer::New(sp_track_duration(track_)));
  handle_->Set(String::New("popularity"),
               Integer::New(sp_track_popularity(track_)));

  // Set properties only available for browse results
  int track_index = sp_track_index(track_);

  if (track_index)
    handle_->Set(String::New("discIndex"), Integer::New(track_index));

  int disc = sp_track_disc(track_);

  if (disc)
    handle_->Set(String::New("disc"), Integer::New(disc));

  return true;
}

GETTER_C(Track::LoadedGetter) {
  HandleScope scope;
  Track *p = Unwrap<Track>(info.This());
  return p->track_
    ? scope.Close(Boolean::New(sp_track_is_loaded(p->track_)))
    : Undefined();
}

GETTER_C(Track::AlbumGetter) {
  HandleScope scope;
  Track *p = Unwrap<Track>(info.This());

  if (!p->track_ || !sp_track_is_loaded(p->track_))
    return Undefined();

  if (!p->album_) {
    Local<Object> instance = Album::New(sp_track_album(p->track_));
    // p->album_ = ObjectWrap::Unwrap<Album>(instance);
    return scope.Close(instance);
  }

  return scope.Close(p->album_->handle_);
}

GETTER_C(Track::ArtistsGetter) {
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

/**
 * Gets the URI of a track. Replaces getter with the URI (string constant) once
 * it has been created.
 */
Handle<Value> Track::UriGetter(Local<String> property,
                               const AccessorInfo& info) {
  HandleScope scope;
  Track *p = Unwrap<Track>(info.This());

  if (!p->track_ || !sp_track_is_loaded(p->track_))
    return Undefined();

  sp_link *link = sp_link_create_from_track(p->track_, 0);

  if (!link)
    return Undefined();

  const int kUriBufferLen = 40;  // spotify:track:0EcVDS2dfZR5RytFhFOtYH
  char uri_buf[kUriBufferLen];
  int uri_len = sp_link_as_string(link, uri_buf, kUriBufferLen);
  sp_link_release(link);
  return scope.Close(String::New(uri_buf, uri_len));
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
  instance_t->SetAccessor(String::New("uri"), UriGetter);

  target->Set(String::New("Track"), constructor_template->GetFunction());
}
