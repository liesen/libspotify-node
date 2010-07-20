# node-spotify API

## spotify

The module.

    var spotify = require('spotify');

### spotify.version -> string

Module version. I.e. `"0.1"`

### spotify.spotifyAPIVersion -> int

Which Spotify API version this module is compatible with. I.e. `2`.

## spotify.Session

Represents a session. Can be compared with a regular logged in (running) Spotify desktop client.

> **Note:** Even though this API allows for virtually unlimited number of concurrent Session objects, it is still considered experimental because libspotify has not been extensively tested with session concurrency (i.e. there might be thread sync bugs lurking in the depts of libspotify).

    var session = new spotify.Session({
      applicationKey: account.applicationKey,
    });
    

