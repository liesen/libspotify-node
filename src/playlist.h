#ifndef SPOTIFY_PLAYLIST_H_
#define SPOTIFY_PLAYLIST_H_

#include "index.h"

class Playlist : public node::EventEmitter {
 public:
  explicit Playlist(sp_playlist* playlist);

  ~Playlist();

  static void Initialize(v8::Handle<v8::Object> target);

  static v8::Handle<v8::Value> New(sp_playlist *playlist);

  static v8::Handle<v8::Value> LengthGetter(v8::Local<v8::String> property,
                                            const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> LoadedGetter(v8::Local<v8::String> property,
                                            const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> NameGetter(v8::Local<v8::String> property,
                                          const AccessorInfo& info);

  static v8::Handle<v8::Value> TrackGetter(uint32_t index,
                                           const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> TrackSetter(uint32_t index,
                                           v8::Local<v8::Value> value,
                                           const v8::AccessorInfo& info);

  static v8::Handle<v8::Boolean> TrackDeleter(uint32_t index,
                                              const v8::AccessorInfo& info);

  static v8::Handle<v8::Boolean> TrackQuery(uint32_t index,
                                            const v8::AccessorInfo& info);

  static v8::Handle<v8::Array> TrackEnumerator(const v8::AccessorInfo& info);

  static v8::Handle<v8::Value> Push(const v8::Arguments& args);

  static v8::Handle<v8::Value> HasPendingChangesGetter(
      v8::Local<v8::String> property,
      const v8::AccessorInfo& info);

  static Handle<Value> UriGetter(Local<String> property,
                                 const AccessorInfo& info);

  bool IsLoaded() { return sp_playlist_is_loaded(playlist_); }

  bool HasPendingChanges() {
    return sp_playlist_has_pending_changes(playlist_);
  }

  int NumTracks() { return sp_playlist_num_tracks(playlist_); }

  sp_playlist* playlist_;

  static Persistent<FunctionTemplate> constructor_template;
};

#endif
