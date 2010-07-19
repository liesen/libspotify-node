#include "playlistcontainer.h"

#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <libspotify/api.h>

using namespace v8;
using namespace node;

Persistent<FunctionTemplate> PlaylistContainer::constructor_template;

// -----------------------------------------------------------------------------
// libspotify callbacks

static void PlaylistAdded(sp_playlistcontainer *pc,
                          sp_playlist *playlist,
                          int position,
                          void *userdata)
{
  // this is called on the main thread
  fprintf(stderr, "sp: playlist added, %d: %s\n", position, sp_playlist_name(playlist));
  // Emit("playlist_added", 0, NULL);
}

static void PlaylistRemoved(sp_playlistcontainer *pc,
                            sp_playlist *playlist,
                            int position,
                            void *userdata)
{
  // this is called on the main thread
  fprintf(stderr, "sp: playlist removed%d\n", position);
}

static void PlaylistContainerLoaded(sp_playlistcontainer* pc, void* userdata) {
  // this is called on the main thread
  PlaylistContainer* p = static_cast<PlaylistContainer*>(userdata);
  p->Emit(String::New("load"), 0, NULL);
}

// -----------------------------------------------------------------------------
// PlaylistContainer implementation

static sp_playlistcontainer_callbacks callbacks = {
  PlaylistAdded,
  PlaylistRemoved,
  NULL,
  PlaylistContainerLoaded
};

PlaylistContainer::PlaylistContainer(sp_playlistcontainer* playlist_container)
  : node::EventEmitter()
  , playlist_container_(playlist_container)
{
  this->playlist_container_ = playlist_container_;
  if (this->playlist_container_)
    sp_playlistcontainer_add_callbacks(this->playlist_container_, &callbacks, this);
}

PlaylistContainer *PlaylistContainer::New(sp_playlistcontainer *playlist_container) {
  Local<Object> instance = constructor_template->GetFunction()->NewInstance(0, NULL);
  PlaylistContainer *pc = ObjectWrap::Unwrap<PlaylistContainer>(instance);
  pc->playlist_container_ = playlist_container;
  sp_playlistcontainer_add_callbacks(pc->playlist_container_, &callbacks, pc);
  return pc;
}

Handle<Value> PlaylistContainer::New(const Arguments& args) {
  PlaylistContainer* pc = new PlaylistContainer(
    (args.Length() && args[0]->IsExternal())
      ? (sp_playlistcontainer *)External::Unwrap(args[0])
      : NULL );
  pc->Wrap(args.This());
  return args.This();
}

Handle<Value> PlaylistContainer::StartLoading(const Arguments& args) {
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(args.This());
  sp_playlistcontainer_callbacks callbacks = {
    PlaylistAdded,
    PlaylistRemoved,
    NULL,
    PlaylistContainerLoaded 
  };

  sp_playlistcontainer_add_callbacks(pc->playlist_container_, &callbacks, pc);
  return Undefined();
}

Handle<Value> PlaylistContainer::LengthGetter(Local<String> property,
                                              const AccessorInfo& info) {
  HandleScope scope;
  sp_playlistcontainer* pc = Unwrap<PlaylistContainer>(info.This())->playlist_container_;
  int num_playlists = sp_playlistcontainer_num_playlists(pc);
  return scope.Close(Integer::New(num_playlists));
}

Handle<Value> PlaylistContainer::PlaylistGetter(uint32_t index, const AccessorInfo& info) {
  HandleScope scope;
  sp_playlistcontainer* pc = Unwrap<PlaylistContainer>(info.This())->playlist_container_;
  sp_playlist* playlist = sp_playlistcontainer_playlist(pc, index);
  if (!playlist)
    return Undefined();
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

void PlaylistContainer::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("PlaylistContainer"));
  constructor_template->Inherit(EventEmitter::constructor_template);

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "startLoading", StartLoading);

  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(NODE_PSYMBOL("length"), LengthGetter);
  instance_t->SetIndexedPropertyHandler(PlaylistGetter,
                                        PlaylistSetter,
                                        PlaylistQuery,
                                        PlaylistDeleter,
                                        PlaylistEnumerator);

  target->Set(String::New("PlaylistContainer"), constructor_template->GetFunction());
}
