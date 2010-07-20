var sys = require('sys'),
    spotify = require('../spotify'),
    account = require('../account');

var session = new spotify.Session({ applicationKey: account.applicationKey });
session.addListener('logMessage', sys.print);
session.login(account.username, account.password, function (err) {
  if (err) return sys.error(err.stack || err);
  session.search('album:belle', function(err, result){
    sys.puts(sys.inspect(result));
    session.logout();
  })
});
