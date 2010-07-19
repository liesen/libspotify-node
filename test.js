var sys = require('sys'),
    spotify = require('./spotify'),
    account = require('./account');

sys.puts('spotify api version: ' + spotify.version);

var config = {
  applicationKey: account.applicationKey,
};

var session = new spotify.Session(config);
sys.puts(sys.inspect(session));

session.addListener('logMessage', function (message) {
  sys.puts('logMessage: ' + message.substr(0,message.length-1));
});

session.addListener('connection_error', function (message) {
  sys.puts('connection_error: ' + message);
});

session.addListener('message_to_user', function (message) { 
  sys.puts('message_to_user: ' + message);
});

session.login(account.username, account.password, function (err) {
  sys.puts('!!! logged_in'); 
  if (err) return sys.error(err.stack || err);
  sys.puts('logged in as: ' + session.user.displayName+' -- logging out...');
  session.logout(function(){
    sys.puts('!!! logged_out');
  });
});
