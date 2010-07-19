#include "session.h"
#include "playlistcontainer.h"
#include "playlist.h"

#include <node.h>
#include <libspotify/api.h>

using namespace v8;
using namespace node;

extern "C" void init(Handle<Object> target) {
  HandleScope scope;
  Session::Initialize(target);
  PlaylistContainer::Initialize(target);
  Playlist::Initialize(target);
  target->Set(String::New("version"), Integer::New(SPOTIFY_API_VERSION));
}
