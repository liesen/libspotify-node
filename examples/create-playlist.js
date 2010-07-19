/*
  This example will create a new playlist called "Test playlists.create" to
  your list of playlists.
*/
var sys = require('sys'),
    spotify = require('./spotify'),
    account = require('./account');

var session = new spotify.Session({applicationKey: account.applicationKey});
session.addListener('logMessage', sys.print);
session.login(account.username, account.password, function (err) {
  if (err) return sys.error(err.stack || err);
  sys.puts('logged in as: ' + session.user.displayName);

  // Creating a new playlist involve two steps:
  //
  //  1. The actual creation of a new playlist entitity
  //
  //  2. Confirmation the new playlists has been securely registered in
  //     session.playlists
  //
  // create() will complete step 1 immediately and return a new playlist object.
  // The (optional) callback will be invoked as soon as step 2 has completed.
  //
  // Note: although step 2 has completed that doesn't mean the changes have been
  //       written to the upstream Spotify server. You need to listen for a
  //       subsequent "load" event on session.playlists.
  //
  session.playlists.create('Test playlists.create', function(err, playlist){
    if (err) return sys.error(err.stack || err);
    sys.puts("created and saved "+playlist.uri+" ('"+playlist.name+"')");
    // Note that new playlists are always added to your list of playlists
    // (playlistcontainer in Spotify speak) -- you will need to remove it after
    // it has been added (i.e. in this callback) if you do not wish to keep it
    // in your list of playlists.
    session.logout();
  });
});
