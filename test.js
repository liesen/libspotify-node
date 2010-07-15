var spotify = require('./spotify');
var sys = require('sys');

sys.puts('spotify api version: ' + spotify.version);

var config = { 
  applicationKey: 'x',
  cacheLocation: '.cache'
};

var session = new spotify.Session(config);
sys.puts(sys.inspect(session));

session.addListener('log_message', function (message) {
  sys.puts('log_message: ' + message);
});

session.addListener('connection_error', function (message) {
  sys.puts('connection_error: ' + message);
});

session.addListener('logged_in', function (err) {
  sys.puts('!!! logged_in'); 
  sys.puts('status: ' + err);

  if (err != spotify.Error.OK) {
    sys.puts('aaaaaaaaaaaaah');
    return;
  }

  session.logout();
});

session.addListener('logged_out', function () {
  sys.puts('!!! logged_out');
});

session.addListener('message_to_user', function (message) { 
  sys.puts('message_to_user: ' + message);
});

session.login('username', 'password');
