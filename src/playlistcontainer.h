#ifndef SPOTIFY_PLAYLISTCONTAINER_H_
#define SPOTIFY_PLAYLISTCONTAINER_H_

#include <libspotify/api.h>
#include <node.h>
#include <node_events.h>
#include <v8.h>


namespace spotify {

class PlaylistContainer : public node::EventEmitter {
 public:
  PlaylistContainer(sp_playlistcontainer* playlist_container) : 
      node::EventEmitter(), playlist_container_(playlist_container) {
  }

  sp_playlistcontainer* playlist_container_;

  static v8::Handle<v8::Object> New(sp_playlistcontainer* playlist_container);

  static v8::Handle<v8::Value> NumPlaylists(v8::Local<v8::String> property,
                                            const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> PlaylistGetter(uint32_t index,
                                              const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> PlaylistSetter(uint32_t index,
                                              v8::Local<v8::Value> value,
                                              const v8::AccessorInfo& info);

  static v8::Handle<v8::Boolean> PlaylistDeleter(uint32_t index,
                                                 const v8::AccessorInfo& info);

  static v8::Handle<v8::Boolean> PlaylistQuery(uint32_t index,
                                               const v8::AccessorInfo& info);

  static v8::Handle<v8::Array> PlaylistEnumerator(const v8::AccessorInfo& info);
};
}

#endif  // SPOTIFY_PLAYLISTCONTAINER_H_
