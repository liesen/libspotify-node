#include "artist.h"

Persistent<FunctionTemplate> Artist::constructor_template;

// -----------------------------------------------------------------------------
// Artist implementation

Artist::Artist(sp_artist *artist)
  : node::EventEmitter()
  , artist_(artist)
{
}

Artist::~Artist() {
  if (artist_)
    sp_artist_release(artist_);
}

Local<Object> Artist::New(sp_artist *artist) {
  HandleScope scope;
  Local<Object> instance =
    constructor_template->GetFunction()->NewInstance(0, NULL);
  Artist *p = ObjectWrap::Unwrap<Artist>(instance);
  p->artist_ = artist;
  if (p->artist_) {
    sp_artist_add_ref(p->artist_);
    p->SetupBackingArtist();
  }
  return instance;
}

Handle<Value> Artist::New(const Arguments& args) {
  HandleScope scope;
  // todo: if called with a string argument, try to parse and load it as a link
  (new Artist(NULL))->Wrap(args.This());
  return args.This();
}

void Artist::SetupBackingArtist() {
  if (!artist_) return;

  // status check
  if (!sp_artist_is_loaded(artist_)) {
    TODO("artist is not yet loaded");
    return;
  }

  // todo: symbolize keys
  handle_->Set(String::New("name"), String::New(sp_artist_name(artist_)));
}

Handle<Value> Artist::LoadedGetter(Local<String> property,
                                   const AccessorInfo& info) {
  HandleScope scope;
  Artist *p = Unwrap<Artist>(info.This());
  return p->artist_
    ? scope.Close(Boolean::New(sp_artist_is_loaded(p->artist_)))
    : Undefined();
}

Handle<Value> Artist::URIGetter(Local<String> property,
                                const AccessorInfo& info) {
  HandleScope scope;
  Artist *p = Unwrap<Artist>(info.This());
  if (!p->artist_ || !sp_artist_is_loaded(p->artist_))
    return Undefined();
  char uri_buf[41]; // spotify:artist:0EcVDS2dfZR5RytFhFOtYH
  sp_link *link = sp_link_create_from_artist(p->artist_);
  if (!link)
    return Undefined();
  sp_link_as_string(link, uri_buf, sizeof(uri_buf));
  sp_link_release(link);
  return scope.Close(String::New(uri_buf));
}

void Artist::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("Artist"));
  constructor_template->Inherit(EventEmitter::constructor_template);

  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(String::New("loaded"), LoadedGetter);
  instance_t->SetAccessor(String::New("uri"), URIGetter);

  target->Set(String::New("Artist"), constructor_template->GetFunction());
}
