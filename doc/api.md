# node-spotify API

## spotify

The module.

    var spotify = require('spotify');

### spotify.version -> string

Module version. I.e. `"0.1"`

### spotify.spotifyAPIVersion -> int

Which Spotify API version this module is compatible with. I.e. `2`.

## spotify.Session(configuration)

Represents a session. Can be compared with a regular logged in (running) Spotify desktop client.

    var session = new spotify.Session({
      applicationKey: appkey,    // [required] array of ints
      userAgent: "my app",       // [optional] defaults to "node-spotify"
      cacheLocation: ".cache",   // [optional] defaults to ".spotify-cache"
      settingsLocation: ".conf"  // [optional] defaults to ".spotify-settings"
    });

> **Note:** Even though this API allows for virtually unlimited number of concurrent Session objects, it is still considered experimental because libspotify has not been extensively tested with session concurrency (i.e. there might be thread sync bugs lurking in the depts of libspotify).

All methods except `login` require the session to be in a logged in state when called, or weird and highly unexpected things will happen.

### spotify.Session.prototype.login(username, password, callback(err))

Sign in to Spotify. The required callback is invoked when you are logged in or an error occurred (e.g. wrong username or password).

    session.login("johndoe", "foobar", function(err) {
      if (err) throw err;
      sys.puts("Logged in");
      session.logout();
    });

### spotify.Session.prototype.logout([callback()])

Sign out of Spotify. The optional callback is called once all finalization code has been run (e.g. writing settings and syncing with Spotify servers).

    session.login("johndoe", "foobar", function(err) {
      if (err) throw err;
      session.logout(function(){
        sys.puts("Logged out. Bye bye.");
      });
    });

### spotify.Session.prototype.search(query, callback(err, result))

Search the Spotify catalouge.

`query` can be either a string (search query) or an object:

    var query = {
      query: "album:belle", // [required] query string
      trackOffset:  0,      // [optional] defaults to 0
      trackCount:  10,      // [optional] defaults to 10
      albumOffset:  0,      // [optional] defaults to 0
      albumCount:   5,      // [optional] defaults to 10
      artistOffset: 0,      // [optional] defaults to 0
      artistCount:  3       // [optional] defaults to 10
    };
    session.search(query, function(err, result){
      if (err) throw err;
      sys.puts(sys.inspect(result.tracks));
    });

### spotify.Session.prototype.getTrackByLink(link[, callback(err, track)]) -> track

Look up a track from a track link (URI or URL). If a callback function is passed as the 2nd argument, that callback will be called when the track's info has loaded:

    var link = 'spotify:track:01gCUID7bHTcp6JzeTfpIe';
    session.getTrackByLink(link, function(err, track) {
      if (err) throw err;
      sys.puts('track info:\n'+sys.inspect(track));
    });

If no callback is passed errors like malformed link are thrown (rather than passed to a callback):

    var link = 'spotify:track:01gCUID7bHTcp6JzeTfpIe';
    try {
      var track = session.getTrackByLink(link);
    } catch (e) {
      sys.error(e.stack || e);
    }

Note that the track object returned from this function is not guaranteed to be in a usable (i.e. `track.loaded == true`) state.

### spotify.Session.prototype.user -> User

The currently logged in user.

### spotify.Session.prototype.playlists -> PlaylistContainer

An array-like object representing all playlists you are subscribing to (all the playlists in the left hand side column in the official Spotify desktop client).

### spotify.Session.prototype.connectionState -> string

The state of the session (e.g. "logged_in" or "disconnected"). See `spotify/index.js` for details.



