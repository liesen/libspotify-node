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

  static Handle<Value> LoadedGetter(Local<String> property,
                                    const AccessorInfo& info);
  static Handle<Value> NameGetter(Local<String> property,
                                  const AccessorInfo& info);
  static Handle<Value> URIGetter(Local<String> property,
                                 const AccessorInfo& info);

 protected:
  sp_playlist* playlist_;
  static Persistent<FunctionTemplate> constructor_template;
};

#endif
