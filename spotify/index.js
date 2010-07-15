var binding = require('./binding');
Object.keys(binding).forEach(function(k){ exports[k] = binding[k]; });

exports.Error = {
  OK: 0
};
