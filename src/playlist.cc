#include "playlist.h"

#include <v8.h>
#include <node.h>
#include <libspotify/api.h>

namespace spotify {

using namespace v8;
using namespace node;

Handle<Value> Playlist::New(sp_playlist *playlist) {
  HandleScope scope;
  Local<ObjectTemplate> t = ObjectTemplate::New();
  t->SetInternalFieldCount(1);

  t->SetAccessor(String::NewSymbol("isLoaded"), IsLoaded);

  Handle<Object> instance = t->NewInstance();
  Playlist *p = new Playlist(playlist);
  p->Wrap(instance);
  return scope.Close(instance);
}

Handle<Value> Playlist::IsLoaded(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  sp_playlist* playlist = ObjectWrap::Unwrap<Playlist>(info.This())->playlist_;
  return scope.Close(Boolean::New(sp_playlist_is_loaded(playlist)));
}

}

