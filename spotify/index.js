var binding = require('./binding');
Object.keys(binding).forEach(function(k){ exports[k] = binding[k]; });

// helper to define immutable <constants> on <obj> -- see below for examples
function defineConstants(obj, constants) {
  var m = {};
  for (var name in constants) {
    m[name] = {
      value: constants[name],
      writable: false,
      enumerable: true,
      configurable: false
    };
  }
  Object.defineProperties(obj, m);
}

// connection state constants
defineConstants(binding.Session, {
  STATE_LOGGED_OUT:   0, // User not yet logged in
  STATE_LOGGED_IN:    1, // Logged in against a Spotify access point
  STATE_DISCONNECTED: 2, // Was logged in, has been disconnected
  STATE_UNDEFINED:    3, // The connection state is undefined
});

// connection state getter
Object.defineProperty(binding.Session.prototype, "connectionState", {
  enumerable: true,
  get: function(){
    switch (this._connectionState) {
      case binding.Album.STATE_LOGGED_OUT: return "logged_out";
      case binding.Album.STATE_LOGGED_IN: return "logged_in";
      case binding.Album.STATE_DISCONNECTED: return "disconnected";
    }
  },
});

// album type constants
defineConstants(binding.Album, {
  TYPE_ALBUM:       0, // Normal album
  TYPE_SINGLE:      1, // Single
  TYPE_COMPILATION: 2, // Compilation
});

// album type getter
Object.defineProperty(binding.Album.prototype, "type", {
  enumerable: true,
  get: function(){
    switch (this._type) {
      case binding.Album.TYPE_ALBUM: return "album";
      case binding.Album.TYPE_SINGLE: return "single";
      case binding.Album.TYPE_COMPILATION: return "compilation";
    }
    return "unknown";
  },
});
