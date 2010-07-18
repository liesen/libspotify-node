var sys = require('sys'),
    spotify = require('../spotify'),
    account = require('../account');

var session = new spotify.Session({
  applicationKey: account.applicationKey
});

session.login(account.username, account.password, function (err) {
  if (err) return sys.error(err.stack || err);
  sys.puts('Logged in as: ' + session.user.displayName+' -- logging out...');
  session.logout(function(){
    sys.puts('Logged out. Bye bye.');
  });
});
