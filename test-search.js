var sys = require('sys'),
    spotify = require('./spotify'),
    account = require('./account');

var session = new spotify.Session({applicationKey: account.applicationKey});
session.addListener('logMessage', sys.print);
session.login(account.username, account.password, function (err) {
  if (err) return sys.error(err.stack || err);
  sys.puts('logged in as: ' + session.user.displayName);
  session.search('artist:belle', function(err, result){
    if (err) return sys.error(err.stack || err);
    sys.puts('search completed. '+result.tracks.length+' tracks');
    sys.puts('tracks: '+sys.inspect(result.tracks));
    sys.puts('logging out...');
    session.logout();
  })
});
