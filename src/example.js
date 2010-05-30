log(log)
log(spotify)

function test(obj, prop) {
  var ok = prop in obj;
  log(obj + '.' + prop + ': ' + ok);
  return ok;
}

test(spotify, 'withApplicationKey')

if (test(spotify, 'login')) {
  log(spotify.login)
  login("username", "password", onError, onSession);

  function onError(message) {
    log(this)
    log("onError: " + message)
  }

  function onSession(session) {
    log(this);
    log('got session');

    function testSession(prop) {
      return test(session, prop);
    }

    testSession('connectionState');
    testSession('playlistContainer');
    
    if (testSession('user')) {
      user = session.user;
      log('user: '+ user);
      log(user.isLoaded);
      user.displayName = 'whatever';
      log(user.displayName);
    }

    if (testSession('logout')) {
      session.logout();
    }
  }
}

