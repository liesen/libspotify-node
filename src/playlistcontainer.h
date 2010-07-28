#ifndef SPOTIFY_PLAYLISTCONTAINER_H_
#define SPOTIFY_PLAYLISTCONTAINER_H_

#include "index.h"

#include <map>
#include <list>

class PlaylistContainer : public node::EventEmitter {
 public:
  explicit PlaylistContainer(sp_playlistcontainer* playlist_container);
  ~PlaylistContainer();

  static void Initialize(v8::Handle<v8::Object> target);

  static v8::Handle<v8::Value> New(sp_playlistcontainer* playlist_container);

  static v8::Handle<v8::Value> LengthGetter(v8::Local<v8::String> property,
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

  // create('playlist name', function (playlist) { ...  Creates and adds a
  // playlist. Callback invoked when playlist is /added/ to container
  static v8::Handle<v8::Value> Create(const v8::Arguments& args);

  // remove(playlist)  Removes a playlist
  static v8::Handle<v8::Value> Remove(const v8::Arguments& args);

  sp_playlistcontainer* playlist_container_;

  std::list<std::pair<sp_playlist*, Persistent<Function>*> > create_callback_queue_;

  int NumPlaylists();

  static v8::Persistent<v8::FunctionTemplate> constructor_template;
};

#endif  // SPOTIFY_PLAYLISTCONTAINER_H_
