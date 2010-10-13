#include "playlistcontainer.h"
#include "playlist.h"

#include <map>
#include <list>

Persistent<FunctionTemplate> PlaylistContainer::constructor_template;

typedef std::pair<sp_playlist*, Persistent<Function>*> PlaylistAddedCallback;
typedef std::list<PlaylistAddedCallback> PlaylistAddedCallbacks;

// -----------------------------------------------------------------------------
// libspotify callbacks

static void PlaylistAdded(sp_playlistcontainer *playlist_container,
                          sp_playlist *playlist,
                          int position,
                          void *userdata) {
  HandleScope scope;
  // this is called on the main thread
  PlaylistContainer* pc = static_cast<PlaylistContainer*>(userdata);

  // First, trigger any queued callback from Create
  for (PlaylistAddedCallbacks::iterator it = pc->create_callback_queue_.begin();
       it != pc->create_callback_queue_.end();
       ++it) {
    if (it->first == playlist) {
      Handle<Value> argv[] = { Playlist::New(playlist) };
      *(*it->second)->Call(pc->handle_, 1, argv);
      sp_playlist_release(playlist);
      cb_destroy(it->second);
      pc->create_callback_queue_.erase(it);
    }
  }

  Handle<Value> argv[] = { Playlist::New(playlist), Integer::New(position) };
  pc->Emit(String::New("playlistAdded"), 2, argv);
}

static void PlaylistRemoved(sp_playlistcontainer *playlist_container,
                            sp_playlist *playlist,
                            int old_position,
                            void *userdata) {
  // this is called on the main thread
  PlaylistContainer* pc = static_cast<PlaylistContainer*>(userdata);
  Handle<Value> argv[] = {
    Playlist::New(playlist),
    Integer::New(old_position)
  };
  pc->Emit(String::New("playlistRemoved"), 2, argv);
}

static void PlaylistMoved(sp_playlistcontainer *playlist_container,
                          sp_playlist *playlist,
                          int old_position,
                          int new_position,
                          void *userdata) {
  // this is called on the main thread
  PlaylistContainer* pc = static_cast<PlaylistContainer*>(userdata);
  Handle<Value> argv[] = {
    Playlist::New(playlist),
    Integer::New(old_position),
    Integer::New(new_position)
  };
  pc->Emit(String::New("playlistMoved"), 3, argv);
}

static void PlaylistContainerLoaded(sp_playlistcontainer* playlist_container,
                                    void* userdata) {
  // this is called on the main thread
  PlaylistContainer* pc = static_cast<PlaylistContainer*>(userdata);
  pc->Emit(String::New("loaded"), 0, NULL);
}

// -----------------------------------------------------------------------------
// PlaylistContainer implementation

static sp_playlistcontainer_callbacks callbacks = {
  PlaylistAdded,
  PlaylistRemoved,
  PlaylistMoved,
  PlaylistContainerLoaded
};

PlaylistContainer::PlaylistContainer(sp_playlistcontainer* playlist_container)
    : EventEmitter(),
      playlist_container_(playlist_container) {
  if (playlist_container_)
    sp_playlistcontainer_add_callbacks(playlist_container_, &callbacks, this);
}

PlaylistContainer::~PlaylistContainer() {
  sp_playlistcontainer_remove_callbacks(playlist_container_, &callbacks, this);

  // Clear the create_callback_queue_
  for (PlaylistAddedCallbacks::iterator it = create_callback_queue_.begin();
       it != create_callback_queue_.end();
       ++it) {
    Local<Value> argv[] = { Exception::Error(String::New("aborted")) };
    *(*it->second)->Call(handle_, 1, argv);
    sp_playlist_release(it->first);
    cb_destroy(it->second);
  }

  create_callback_queue_.clear();
}

Handle<Value> PlaylistContainer::New(sp_playlistcontainer *playlist_container) {
  HandleScope scope;
  Local<Object> instance =
    constructor_template->GetFunction()->NewInstance(0, NULL);
  PlaylistContainer* pc = new PlaylistContainer(playlist_container);
  pc->Wrap(instance);
  return scope.Close(instance);
}

Handle<Value> PlaylistContainer::LengthGetter(Local<String> property,
                                              const AccessorInfo& info) {
  HandleScope scope;
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(info.This());
  return scope.Close(Integer::New(
    sp_playlistcontainer_num_playlists(pc->playlist_container_)));
}

Handle<Value> PlaylistContainer::PlaylistGetter(uint32_t index,
                                                const AccessorInfo& info) {
  HandleScope scope;
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(info.This());
  sp_playlist* playlist = sp_playlistcontainer_playlist(
      pc->playlist_container_, index);
  if (!playlist) return Undefined();
  return scope.Close(Playlist::New(playlist));
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


Handle<Integer> PlaylistContainer::PlaylistQuery(uint32_t index,
                                                 const AccessorInfo& info) {
  HandleScope scope;
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(info.This());
  int num_playlists = pc->NumPlaylists();

  if (index < num_playlists)
    return scope.Close(Integer::New(None));

  return Handle<Integer>();
}

Handle<Array> PlaylistContainer::PlaylistEnumerator(const AccessorInfo& info) {
  HandleScope scope;
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(info.This());
  int num_playlists = pc->NumPlaylists();
  Local<Array> playlists = Array::New(num_playlists);

  for (int i = 0; i < num_playlists; i++) {
    sp_playlist* playlist = sp_playlistcontainer_playlist(
        pc->playlist_container_, i);
    playlists->Set(i, Playlist::New(playlist));
  }

  return scope.Close(playlists);
}

Handle<Value> PlaylistContainer::Create(const Arguments& args) {
  HandleScope scope;

  if (args.Length() < 1)
    return JS_THROW(TypeError, "search takes at least 1 argument");
  if (!args[0]->IsString())
    return JS_THROW(TypeError, "first argument must be a string");
  if (args.Length() > 1 && !args[1]->IsFunction())
    return JS_THROW(TypeError, "last argument must be a function");

  PlaylistContainer* pc = Unwrap<PlaylistContainer>(args.This());
  String::Utf8Value name(args[0]);
  sp_playlist *playlist = sp_playlistcontainer_add_new_playlist(
      pc->playlist_container_,
      *name);

  if (!playlist)
    return JS_THROW(Error, "failed to create new playlist");

  // queue callback
  if (args.Length() > 1) {
    sp_playlist_add_ref(playlist);
    PlaylistAddedCallback callback_entry(playlist, cb_persist(args[1]));
    pc->create_callback_queue_.push_back(callback_entry);
  }

  return scope.Close(Playlist::New(playlist));
}

Handle<Value> PlaylistContainer::Remove(const Arguments& args) {
  HandleScope scope;
  PlaylistContainer* pc = Unwrap<PlaylistContainer>(args.This());

  if (args.Length() < 1)
    return Undefined();

  if (!args[0]->IsObject())
    return JS_THROW(TypeError, "first argument must be an object");

  Playlist* p = Unwrap<Playlist>(args[0]->ToObject());

  // OK, thanks for letting me delete a playlist without knowing its index...
  for (int i = 0; i < pc->NumPlaylists(); i++) {
    sp_playlist* playlist = sp_playlistcontainer_playlist(
        pc->playlist_container_,
        i);

    if (p->playlist_ == playlist) {
      sp_error error = sp_playlistcontainer_remove_playlist(
          pc->playlist_container_,
          i);

      if (error != SP_ERROR_OK)
        return scope.Close(JS_THROW(Error, sp_error_message(error)));

      return Undefined();
    }
  }

  return scope.Close(
      JS_THROW(Error, "Playlist not found within the current container"));
}

void PlaylistContainer::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New();
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("PlaylistContainer"));
  constructor_template->Inherit(EventEmitter::constructor_template);

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "create", Create);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "remove", Remove);

  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(String::NewSymbol("length"), LengthGetter);
  instance_t->SetIndexedPropertyHandler(PlaylistGetter,
                                        PlaylistSetter,
                                        PlaylistQuery,
                                        PlaylistDeleter,
                                        PlaylistEnumerator);

  target->Set(String::New("PlaylistContainer"),
              constructor_template->GetFunction());
}

int PlaylistContainer::NumPlaylists() {
  return sp_playlistcontainer_num_playlists(playlist_container_);
}
