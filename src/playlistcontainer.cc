#include "playlistcontainer.h"

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

using namespace v8;
using namespace node;

static void PlaylistAdded(sp_playlistcontainer *pc,
                          sp_playlist *playlist,
                          int position,
                          void *userdata) {
  fprintf(stderr, "sp: playlist added, %d: %s\n", position, sp_playlist_name(playlist));
  // Emit("playlist_added", 0, NULL);
}

static void PlaylistRemoved(sp_playlistcontainer *pc,
                            sp_playlist *playlist,
                            int position,
                            void *userdata) {
  fprintf(stderr, "sp: playlist removed%d\n", position);
}

static void PlaylistContainerLoaded(sp_playlistcontainer* pc,
                                    void* userdata) {
  fprintf(stderr, "sp: playlist container loaded\n");
  // PlaylistContainer* p = static_cast<PlaylistContainer*>(userdata);
  // p->Emit(String::New("container_loaded"), 0, NULL);
}

Handle<Object> PlaylistContainer::New(
    sp_playlistcontainer *playlist_container) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New();
//  t->Inherit(EventEmitter::constructor_template);

  Local<ObjectTemplate> instance_t = t->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(NODE_PSYMBOL("length"), NumPlaylists);
  instance_t->SetIndexedPropertyHandler(PlaylistGetter,
                                        PlaylistSetter,
                                        PlaylistQuery,
                                        PlaylistDeleter,
                                        PlaylistEnumerator);

  Local<Object> instance = instance_t->NewInstance();
  PlaylistContainer* pc = new PlaylistContainer(playlist_container);
  pc->Wrap(instance);

  sp_playlistcontainer_callbacks callbacks = {
    PlaylistAdded,
    PlaylistRemoved,
    NULL,
    PlaylistContainerLoaded 
  };

  sp_playlistcontainer_add_callbacks(pc->playlist_container_, &callbacks, pc);
  return scope.Close(instance);
}

Handle<Value> PlaylistContainer::NumPlaylists(Local<String> property,
                                              const AccessorInfo& info) {
  HandleScope scope;
  sp_playlistcontainer* pc = Unwrap<PlaylistContainer>(info.This())->playlist_container_;
  int num_playlists = sp_playlistcontainer_num_playlists(pc);
  return scope.Close(Integer::New(num_playlists));
}

Handle<Value> PlaylistContainer::PlaylistGetter(uint32_t index, 
                                                const AccessorInfo& info) {
  HandleScope scope;
  sp_playlistcontainer* pc = Unwrap<PlaylistContainer>(info.This())->playlist_container_;
  sp_playlist* playlist = sp_playlistcontainer_playlist(pc, index);
  const char* playlist_name = sp_playlist_name(playlist);
  return scope.Close(String::New(playlist_name));
}

Handle<Value> PlaylistContainer::PlaylistSetter(uint32_t index,
                                                Local<Value> value,
                                                const AccessorInfo& info) {
  return Handle<Value>();
}

Handle<Boolean> PlaylistContainer::PlaylistDeleter(uint32_t index,
                                                   const AccessorInfo& info) {
  return Handle<Boolean>();
}


Handle<Boolean> PlaylistContainer::PlaylistQuery(uint32_t index,
                                                 const AccessorInfo& info) {
  HandleScope scope;
  sp_playlistcontainer* pc = Unwrap<PlaylistContainer>(info.This())->playlist_container_;
  int num_playlists = sp_playlistcontainer_num_playlists(pc);
  return scope.Close(Boolean::New(index < num_playlists));
}

Handle<Array> PlaylistContainer::PlaylistEnumerator(const AccessorInfo& info) {
  HandleScope scope;
  sp_playlistcontainer* pc = Unwrap<PlaylistContainer>(info.This())->playlist_container_;
  int num_playlists = sp_playlistcontainer_num_playlists(pc);
  fprintf(stderr, "sp: playlist enumerator, %d playlists\n", num_playlists);
  return scope.Close(Array::New(num_playlists));
}

