#include "session.h"
#include "playlistcontainer.h"
#include "playlist.h"
#include "search.h"
#include "track.h"

#include <node.h>
#include <libspotify/api.h>

using namespace v8;
using namespace node;

extern "C" void init(Handle<Object> target) {
  HandleScope scope;
  Session::Initialize(target);
  PlaylistContainer::Initialize(target);
  Playlist::Initialize(target);
  SearchResult::Initialize(target);
  Track::Initialize(target);
  target->Set(String::NewSymbol("version"), Integer::New(SPOTIFY_API_VERSION));
}
