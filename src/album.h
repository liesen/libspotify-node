#ifndef SPOTIFY_ALBUM_H_
#define SPOTIFY_ALBUM_H_

#include "index.h"

class Album : public EventEmitter {
 public:
  static Persistent<FunctionTemplate> constructor_template;
  static void Initialize(Handle<Object> target);
  explicit Album(sp_album *album);
  ~Album();
  static Handle<Value> New(const Arguments& args);
  static Local<Object> New(sp_album *album);

  static Handle<Value> LoadedGetter(Local<String> property,
                                    const AccessorInfo& info);
  static Handle<Value> ArtistGetter(Local<String> property,
                                    const AccessorInfo& info);
  static Handle<Value> URIGetter(Local<String> property,
                                 const AccessorInfo& info);
 protected:
  void SetupBackingAlbum();
  sp_album* album_;
};

#endif
