#ifndef SPOTIFY_PLAYLIST_H_
#define SPOTIFY_PLAYLIST_H_

#include "index.h"

class Playlist : public EventEmitter {
 public:
  static void Initialize(Handle<Object> target);

  explicit Playlist(sp_playlist* playlist);
  ~Playlist();

  static Handle<Value> New(const Arguments& args);
  static Handle<Value> New(sp_playlist *playlist);

  static v8::Handle<v8::Value> LengthGetter(v8::Local<v8::String> property,
                                            const v8::AccessorInfo& info);

  static Handle<Value> LoadedGetter(Local<String> property,
                                    const AccessorInfo& info);

  static Handle<Value> NameGetter(Local<String> property,
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

  static Handle<Value> UriGetter(Local<String> property,
                                 const AccessorInfo& info);
 protected:
  sp_playlist* playlist_;
  static Persistent<FunctionTemplate> constructor_template;
};

#endif
