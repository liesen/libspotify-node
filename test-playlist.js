var sys = require('sys'),
    spotify = require('./spotify'),
    account = require('./account');

var session = new spotify.Session({applicationKey: account.applicationKey});
session.addListener('logMessage', sys.print);
session.login(account.username, account.password, function (err) {
  if (err) return sys.error(err.stack || err);
  sys.puts('logged in as: ' + session.user.displayName);
  session.playlists.addListener('playlistAdded', function(playlist, position){
    sys.puts("playlist added at "+position)
    playlist.addListener('updated', function(){
      sys.puts("playlist loaded: "+sys.inspect(this));
    })
    playlist.addListener('tracksAdded', function(count, position){
      sys.puts(count+" tracks added at "+position);
    })
  })
  session.playlists.addListener('playlistRemoved', function(playlist, position){
    sys.puts("playlist removed from "+position+". "+sys.inspect(playlist))
  })
  session.playlists.addListener('load', function(){
    sys.puts(this.length+' playlists loaded');
    //sys.puts(sys.inspect(this))
    //setTimeout(function(){
      session.logout(function(){ sys.puts('!!! logged_out'); });
    //}, 10000);
  })
});
