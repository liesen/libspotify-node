/*
  This program dumps a JSON array to stdout containing all URIs and names of
  your current playlists. Sort of like a snapshot.
  
  Since playlists are stochastically loaded we must first wait for all initial
  "updated" events -- which are _not_ preceeded by "updating" events when loading
  the first time -- then finally iterate playlists to get the actual sequence.
*/
var sys = require('sys'),
    spotify = require('./spotify'),
    account = require('./account');

var session = new spotify.Session({applicationKey: account.applicationKey});
session.addListener('logMessage', function(m){ sys.error(m.substr(0,m.length-1)); });
var p;

session.login(account.username, account.password, function (err) {
  if (err) return sys.error(err.stack || err);
  //p = session.playlists;
  function finalize() {
    var playlists = [];
    for (var i=0; i<session.playlists.length; i++) {
      playlists.push({uri: session.playlists[i].uri, name: session.playlists[i].name})
    }
    sys.puts(JSON.stringify(playlists));
    session.logout();
  }
  var refcount = 1; // 1 for "load" event
  session.playlists.addListener('playlistAdded', function(playlist, position){
    refcount++;
    playlist.addListener('updating', function(){
      //refcount++;
    })
    playlist.addListener('updated', function(){
      if (--refcount === 0) finalize();
    })
  })
  session.playlists.addListener('load', function(){
    sys.error('Waiting for '+session.playlists.length+' playlists to load...');
    if (--refcount === 0) finalize();
  });
});
