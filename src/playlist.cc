#include "playlist.h"

#include <v8.h>
#include <node.h>
#include <libspotify/api.h>


using namespace v8;
using namespace node;

Persistent<FunctionTemplate> Playlist::constructor_template;

Handle<Value> Playlist::New(sp_playlist *playlist) {
  Local<Object> instance = constructor_template->GetFunction()->NewInstance(0, NULL);
  Playlist *pl = ObjectWrap::Unwrap<Playlist>(instance);
  pl->playlist_ = playlist;
  // todo: sp_playlist_add_callbacks here? or maybe someplace else?
  return instance;
}

Handle<Value> Playlist::New(const Arguments& args) {
  (new Playlist(NULL))->Wrap(args.This());
  return args.This();
}

Handle<Value> Playlist::IsLoaded(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  sp_playlist* playlist = ObjectWrap::Unwrap<Playlist>(info.This())->playlist_;
  return scope.Close(Boolean::New(sp_playlist_is_loaded(playlist)));
}

void Playlist::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("Playlist"));
  
  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(String::NewSymbol("isLoaded"), IsLoaded);
  /*instance_t->SetAccessor(NODE_PSYMBOL("length"), LengthGetter);
  instance_t->SetIndexedPropertyHandler(TrackGetter,
                                        TrackSetter,
                                        TrackQuery,
                                        TrackDeleter,
                                        TrackEnumerator);*/

  target->Set(String::New("Playlist"), constructor_template->GetFunction());
}
