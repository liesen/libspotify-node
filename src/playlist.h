#ifndef SPOTIFY_PLAYLIST_H_
#define SPOTIFY_PLAYLIST_H_

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

namespace spotify {

using namespace v8;
using namespace node;

class Playlist : public ObjectWrap {
 public:
  static Handle<Value> New(sp_playlist* playlist);

 protected:
  Playlist(sp_playlist* playlist) : playlist_(playlist) {
  }

  static Handle<Value> IsLoaded(Local<String> property, 
                                const AccessorInfo& info);

 private:
  sp_playlist* playlist_;
 
};

}

#endif
