#include "album.h"

using namespace v8;
using namespace node;

#define NS_THROW(_type_, _msg_)\
  return ThrowException(Exception::_type_(String::New(_msg_)))

Persistent<FunctionTemplate> Album::constructor_template;

// -----------------------------------------------------------------------------
// Album implementation

Album::Album(sp_album *album)
  : node::EventEmitter()
  , album_(album)
{
}

Album::~Album() {
  if (album_)
    sp_album_release(album_);
}

Local<Object> Album::New(sp_album *album) {
  HandleScope scope;
  Local<Object> instance = constructor_template->GetFunction()->NewInstance(0, NULL);
  Album *p = ObjectWrap::Unwrap<Album>(instance);
  p->album_ = album;
  if (p->album_) {
    sp_album_add_ref(p->album_);
    p->SetupBackingAlbum();
  }
  return instance;
}

Handle<Value> Album::New(const Arguments& args) {
  HandleScope scope;
  // todo: if called with a string argument, try to parse and load it as a link
  (new Album(NULL))->Wrap(args.This());
  return args.This();
}

void Album::SetupBackingAlbum() {
  if (!album_) return;
  
  // status check
  if (!sp_album_is_loaded(album_)) {
    fprintf(stderr, "todo [%s:%d]: album is not yet loaded\n",__FILE__,__LINE__);
    return;
  }
  
  // todo: symbolize keys
  
  handle_->Set(String::New("name"), String::New(sp_album_name(album_)));
  handle_->Set(String::New("year"), Integer::New(sp_album_year(album_)));
  // todo: symbolize type constants
  Handle<String> type;
  switch (sp_album_type(album_)) {
    case SP_ALBUMTYPE_ALBUM:
      handle_->Set(String::New("type"), String::NewSymbol("album"));
      break;
    case SP_ALBUMTYPE_SINGLE:
      handle_->Set(String::New("type"), String::NewSymbol("single"));
      break;
    case SP_ALBUMTYPE_COMPILATION:
      handle_->Set(String::New("type"), String::NewSymbol("compilation"));
      break;
  }
  // intentional: do not set "type" property for unknown types.

  // todo: lazy getter for artist
}

Handle<Value> Album::LoadedGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Album *p = Unwrap<Album>(info.This());
  return p->album_
    ? scope.Close(Boolean::New(sp_album_is_loaded(p->album_)))
    : Undefined();
}

Handle<Value> Album::ArtistGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Album *p = Unwrap<Album>(info.This());
  if (!p->album_ || !sp_album_is_loaded(p->album_))
    return Undefined();
  Local<Value> artist = Artist::New(sp_album_artist(p->album_));
  scope.Close(artist);
}

void Album::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("Album"));
  constructor_template->Inherit(EventEmitter::constructor_template);
  
  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(String::New("loaded"), LoadedGetter);
  instance_t->SetAccessor(String::New("artist"), ArtistGetter);

  target->Set(String::New("Album"), constructor_template->GetFunction());
}
