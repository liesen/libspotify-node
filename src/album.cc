#include "album.h"
#include "artist.h"

using namespace v8;
using namespace node;

#define NS_THROW(_type_, _msg_)\
  return ThrowException(Exception::_type_(String::New(_msg_)))

Persistent<FunctionTemplate> Album::constructor_template;

// -----------------------------------------------------------------------------
// Album implementation

Album::Album(sp_album *album)
  : node::EventEmitter()
  , album_(album) {
}

Album::~Album() {
  if (album_)
    sp_album_release(album_);
}

Local<Object> Album::New(sp_album *album) {
  HandleScope scope;
  Local<Object> instance =
    constructor_template->GetFunction()->NewInstance(0, NULL);
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
  // Type is only available when browsing albums, so e.g. will be
  // SP_ALBUMTYPE_UNKNOWN for search results. There's a "type" getter defined
  // in index.js which returns a string rep, based on this value.
  handle_->Set(String::New("_type"), Integer::New(sp_album_type(album_)));
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

Handle<Value> Album::URIGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  Album *p = Unwrap<Album>(info.This());
  if (!p->album_ || !sp_album_is_loaded(p->album_))
    return Undefined();
  char uri_buf[40]; // spotify:album:0EcVDS2dfZR5RytFhFOtYH
  sp_link *link = sp_link_create_from_album(p->album_);
  if (!link)
    return Undefined();
  sp_link_as_string(link, uri_buf, sizeof(uri_buf));
  sp_link_release(link);
  return scope.Close(String::New(uri_buf));
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
  instance_t->SetAccessor(String::New("uri"), URIGetter);

  target->Set(String::New("Album"), constructor_template->GetFunction());
}
