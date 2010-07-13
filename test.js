var spotify = require('./spotify').spotify,
    sys = require('sys');

sys.puts('spotify => '+sys.inspect(spotify));

var login = spotify.withApplicationKey(); // appkey here later

login("username", "password", function(err) {
  sys.error(err);
}, function(session) {
  sys.puts('session => '+sys.inspect(session));
  session.logout();
});
