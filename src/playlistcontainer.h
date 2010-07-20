#ifndef SPOTIFY_PLAYLISTCONTAINER_H_
#define SPOTIFY_PLAYLISTCONTAINER_H_

#include "index.h"
#include "queue.h"

typedef struct create_callback_entry {
	TAILQ_ENTRY(create_callback_entry) link;
	sp_playlist *playlist;
	v8::Persistent<v8::Function> *callback;
} create_callback_entry_t;
TAILQ_HEAD(create_callback_queue, create_callback_entry);
typedef struct create_callback_queue create_callback_queue_t;


class PlaylistContainer : public node::EventEmitter {
 public:
  PlaylistContainer(sp_playlistcontainer* playlist_container);
  ~PlaylistContainer();

  static void Initialize(v8::Handle<v8::Object> target);

  static v8::Handle<v8::Value> New(sp_playlistcontainer* playlist_container);
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  
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
  
  
  static v8::Handle<v8::Value> Create(const v8::Arguments& args);
  
  sp_playlistcontainer* playlist_container_;
  create_callback_queue_t create_callback_queue_;
  
  static v8::Persistent<v8::FunctionTemplate> constructor_template;
};

#endif  // SPOTIFY_PLAYLISTCONTAINER_H_
