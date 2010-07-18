var sys = require('sys'),
    spotify = require('../spotify'),
    account = require('../account');

// Create a new session
var session = new spotify.Session({ applicationKey: account.applicationKey });

// Log messages from libspotify printed straight to stdout
session.addListener('logMessage', sys.print);

// Sign in to Spotify (as the user defined in account.js)
session.login(account.username, account.password, function (err) {
  // If an error occured, output details and bail out
  if (err) return sys.error(err.stack || err);
  
  // You're logged in and could do something useful here...
  sys.puts('Logged in as: ' + session.user.displayName+' -- logging out...');
  
  // We need to log out since a live session object will emit events and "live"
  // on, causing your program never to end, unless logging out.
  session.logout(function(){
    // Logging out might take a moment since settings etc. are synced.
    // This optional callback is called when the logout process has completed.
    sys.puts('Logged out. Bye bye.');
  });
});
