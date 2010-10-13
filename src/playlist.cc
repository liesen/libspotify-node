#include "playlist.h"
#include "playlistcontainer.h"
#include "session.h"
#include "track.h"

#include <list>
#include <map>
#include <vector>

Persistent<FunctionTemplate> Playlist::constructor_template;

typedef std::map<sp_playlist*, v8::Persistent<v8::Object> > PlaylistMap;

static PlaylistMap playlist_cache_;

const int kUriBufferLength = 256;

// -----------------------------------------------------------------------------
// libspotify callbacks

static void TracksAdded(sp_playlist *playlist, sp_track *const *tracks,
                        int count, int position, void *userdata) {
  // called on the main thread
  Playlist* pl = static_cast<Playlist*>(userdata);
  Local<Array> tracks_array = Array::New(count);

  for (int i = 0; i < count; i++) {
    tracks_array->Set(i, Track::New(tracks[i]));
  }

  Handle<Value> argv[] = { Integer::New(position), tracks_array };
  pl->Emit(String::New("tracksAdded"), 2, argv);
}

static void TracksRemoved(sp_playlist *playlist, const int *tracks, int count,
                          void *userdata) {
  // called on the main thread
  Playlist* pl = static_cast<Playlist*>(userdata);
  Local<Array> array = Array::New(count);
  for (int i = 0; i < count; i++) {
    array->Set(Integer::New(i), Integer::New(tracks[i]));
  }
  Handle<Value> argv[] = { array };
  pl->Emit(String::New("tracksRemoved"), 1, argv);
}

static void TracksMoved(sp_playlist *playlist, const int *tracks, int count,
                        int new_position, void *userdata) {
  // called on the main thread
  Playlist* pl = static_cast<Playlist*>(userdata);
  Local<Array> array = Array::New(count);

  for (int i = 0; i < count; i++) {
    array->Set(i, Integer::New(tracks[i]));
  }

  Handle<Value> argv[] = { array, Integer::New(new_position) };
  pl->Emit(String::New("tracksMoved"), 2, argv);
}

static void PlaylistRenamed(sp_playlist *playlist, void *userdata) {
  // called on the main thread
  Playlist* pl = static_cast<Playlist*>(userdata);
  pl->Emit(String::New("renamed"), 0, NULL);
}

static void PlaylistStateChanged(sp_playlist *playlist, void *userdata) {
  // called on the main thread
  // The "state" in this case are the flags like collaborative or pending.
  //printf("[%s] stateChanged -- colab: %d, pending: %d\n",
  //       _PlaylistURI(playlist),
  //       sp_playlist_is_collaborative(playlist),
  //       sp_playlist_has_pending_changes(playlist));
  Playlist* pl = static_cast<Playlist*>(userdata);
  pl->Emit(String::New("stateChanged"), 0, NULL);
}

static void PlaylistUpdateInProgress(sp_playlist *playlist, bool done,
                                     void *userdata) {
  // called on the main thread
  //printf("[%s] updateInProgress -- done: %s\n",
  //  _PlaylistURI(playlist), done ? "true" : "false");
  Playlist* pl = static_cast<Playlist*>(userdata);
  pl->Emit(String::New(done ? "updated" : "updating"), 0, NULL);
}

static void PlaylistMetadataUpdated(sp_playlist *playlist, void *userdata) {
  // Called when metadata for one or more tracks in a playlist has been updated.
  //printf("[%s] metadataUpdated\n", _PlaylistURI(playlist));
  Playlist* pl = static_cast<Playlist*>(userdata);
  pl->Emit(String::New("metadataUpdated"), 0, NULL);
}

static sp_playlist_callbacks callbacks = {
  /* tracks_added */ TracksAdded,
  /* tracks_removed */ TracksRemoved,
  /* tracks_moved */ TracksMoved,
  /* playlist_renamed */ PlaylistRenamed,
  /* playlist_state_changed */ PlaylistStateChanged,
  /* playlist_update_in_progress */ PlaylistUpdateInProgress,
  /* playlist_metadata_updated */ PlaylistMetadataUpdated,
};

// -----------------------------------------------------------------------------
// Playlist implementation

Playlist::Playlist(sp_playlist* playlist)
    : EventEmitter()
    , playlist_(playlist) {
  if (playlist_) {
    sp_playlist_add_ref(playlist_);
    sp_playlist_add_callbacks(playlist_, &callbacks, this);
  }
}

Playlist::~Playlist() {
  if (playlist_) {
    PlaylistMap::iterator it = playlist_cache_.find(playlist_);

    if (it != playlist_cache_.end()) {
      it->second.Dispose();
      playlist_cache_.erase(it);
    }

    sp_playlist_release(playlist_);
  }
}

/**
 * Creates a new JavaScript playlist object wrapper, or a cached if one exists.
 */
Handle<Value> Playlist::New(sp_playlist *playlist) {
  // Try to find playlist in cache
  PlaylistMap::iterator it = playlist_cache_.find(playlist);

  if (it != playlist_cache_.end())
    return it->second;

  // Create new object
  HandleScope scope;
  Persistent<Object> instance = Persistent<Object>::New(
      constructor_template->GetFunction()->NewInstance(0, NULL));
  Playlist* pl = new Playlist(playlist);
  pl->Wrap(instance);
  playlist_cache_[playlist] = instance;
  return scope.Close(instance);
}

Handle<Value> Playlist::HasPendingChangesGetter(Local<String> property,
                                                const AccessorInfo& info) {
  HandleScope scope;
  Playlist* p = Unwrap<Playlist>(info.This());
  return scope.Close(Boolean::New(p->HasPendingChanges()));
}


Handle<Value> Playlist::LengthGetter(Local<String> property,
                                     const AccessorInfo& info) {
  HandleScope scope;
  Playlist* p = Unwrap<Playlist>(info.This());
  int num_tracks = sp_playlist_num_tracks(p->playlist_);
  return scope.Close(Integer::New(num_tracks));
}

Handle<Value> Playlist::TrackGetter(uint32_t index,
                                    const AccessorInfo& info) {
  HandleScope scope;
  Playlist* p = Unwrap<Playlist>(info.This());

  if (!p->IsLoaded())
    return Undefined();

  sp_track* track = sp_playlist_track(p->playlist_, index);

  if (track == NULL)
    return Undefined();

  return scope.Close(Track::New(track));
}

Handle<Value> Playlist::TrackSetter(uint32_t index,
                                    Local<Value> value,
                                    const AccessorInfo& info) {
  return Undefined();
}

Handle<Boolean> Playlist::TrackDeleter(uint32_t index,
                                       const AccessorInfo& info) {
  return False();
}

Handle<Integer> Playlist::TrackQuery(uint32_t index,
                                     const AccessorInfo& info) {
  HandleScope scope;
  Playlist* p = Unwrap<Playlist>(info.This());

  if (!p->IsLoaded())
    return Handle<Integer>();

  int num_tracks = sp_playlist_num_tracks(p->playlist_);

  if (index >= num_tracks)
    return Handle<Integer>();

  return scope.Close(Integer::New(None));
}

Handle<Array> Playlist::TrackEnumerator(const AccessorInfo& info) {
  HandleScope scope;
  Playlist* p = Unwrap<Playlist>(info.This());

  if (p == NULL)
    JS_THROW(Error, "no tracks for you!");

  if (!p->IsLoaded())
    return scope.Close(Array::New());

  int num_tracks = p->NumTracks();
  Local<Array> tracks = Array::New(num_tracks);

  for (int i = 0; i < num_tracks; i++) {
    sp_track* track = sp_playlist_track(p->playlist_, i);
    tracks->Set(i, Track::New(track));
  }

  return scope.Close(tracks);
}

Handle<Value> Playlist::Push(const Arguments& args) {
  HandleScope scope;
  Playlist* p = Unwrap<Playlist>(args.This());

  if (p == NULL)
    return scope.Close(Integer::New(0));

  int num_tracks = p->NumTracks();

  if (args.Length() < 1 || !p->IsLoaded())
    return scope.Close(Integer::New(num_tracks));

  if (!args[0]->IsObject())
    return JS_THROW(TypeError, "first argument must be the session");

  Session* s = Unwrap<Session>(args[0]->ToObject());

  if (s == NULL)
    return JS_THROW(TypeError, "first argument must be the session");

  // TODO(liesen): mnfg...
  int num_new_tracks = 0;
  int tracks_size = 0;
  std::vector<sp_track*> tracks(args.Length() - 1);

  for (int i = 1; i < args.Length(); i++) {
    if (!args[i]->IsObject()) continue;
    Track* t = Unwrap<Track>(args[i]->ToObject());
    if (t == NULL) continue;
    tracks[num_new_tracks++] = t->track_;
    tracks_size += sizeof(t->track_);
  }

  const sp_track* trax = static_cast<const sp_track*>(malloc(tracks_size));
  trax = tracks[0];
  sp_error error = sp_playlist_add_tracks(p->playlist_, &trax,
                                          num_new_tracks, num_tracks,
                                          s->session_);

  if (error != SP_ERROR_OK)
    return JS_THROW(Error, sp_error_message(error));

  return scope.Close(Integer::New(num_tracks + num_new_tracks));
}

Handle<Value> Playlist::LoadedGetter(Local<String> property,
                                     const AccessorInfo& info) {
  HandleScope scope;
  sp_playlist* playlist = ObjectWrap::Unwrap<Playlist>(info.This())->playlist_;
  if (!playlist) return Undefined();
  return scope.Close(Boolean::New(sp_playlist_is_loaded(playlist)));
}

Handle<Value> Playlist::NameGetter(Local<String> property,
                                   const AccessorInfo& info) {
  HandleScope scope;
  sp_playlist* playlist = ObjectWrap::Unwrap<Playlist>(info.This())->playlist_;
  if (!playlist || !sp_playlist_is_loaded(playlist))
    return Undefined();
  return scope.Close(String::New(sp_playlist_name(playlist)));
}

Handle<Value> Playlist::UriGetter(Local<String> property,
                                  const AccessorInfo& info) {
  HandleScope scope;
  Playlist *p = Unwrap<Playlist>(info.This());

  if (!p->playlist_ || !p->IsLoaded())
    return Undefined();

  sp_link* link = sp_link_create_from_playlist(p->playlist_);

  if (!link)
    return Undefined();

  char uri_buf[kUriBufferLength]; // probably enough
  int uri_length = sp_link_as_string(link, uri_buf, kUriBufferLength);
  sp_link_release(link);
  return scope.Close(String::New(uri_buf, uri_length));
}

void Playlist::Initialize(Handle<Object> target) {
  HandleScope scope;
  Local<FunctionTemplate> t = FunctionTemplate::New();
  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->SetClassName(String::NewSymbol("Playlist"));
  constructor_template->Inherit(EventEmitter::constructor_template);

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "push", Push);

  Local<ObjectTemplate> instance_t = constructor_template->InstanceTemplate();
  instance_t->SetInternalFieldCount(1);
  instance_t->SetAccessor(String::NewSymbol("hasPendingChanges"),
                          HasPendingChangesGetter);
  instance_t->SetAccessor(String::NewSymbol("loaded"), LoadedGetter);
  instance_t->SetAccessor(String::NewSymbol("name"), NameGetter);
  instance_t->SetAccessor(String::NewSymbol("uri"), UriGetter);
  instance_t->SetAccessor(NODE_PSYMBOL("length"), LengthGetter);
  instance_t->SetIndexedPropertyHandler(TrackGetter,
                                        TrackSetter,
                                        TrackQuery,
                                        TrackDeleter,
                                        TrackEnumerator);

  target->Set(String::New("Playlist"), constructor_template->GetFunction());
}
