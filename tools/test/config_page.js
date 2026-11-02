// Erzeugt die Einstellungsseite als build/config.html, um sie samt Vorschau im Browser
// anzusehen. Die Seite muss über http laufen, etwa mit
//   python3 -m http.server -d build 8765
// und dann http://localhost:8765/config.html in Handygröße öffnen.
var fs = require('fs');
var path = require('path');
var Module = require('module');

var root = path.join(__dirname, '..', '..');
var store = {};
global.localStorage = {
  getItem: function(k) { return store[k] || null; },
  setItem: function(k, v) { store[k] = v; }
};
var watch = { platform: 'emery', model: 'pebble_time_2', language: 'de_DE',
              firmware: { major: 4, minor: 9, patch: 0, suffix: '' } };
global.Pebble = {
  platform: 'ios',
  addEventListener: function() {},
  getActiveWatchInfo: function() { return watch; },
  getAccountToken: function() { return ''; },
  getWatchToken: function() { return ''; }
};

// Clay erwartet das Modul message_keys, das sonst der Pebble-Build erzeugt
var keys = {};
require(path.join(root, 'package.json')).pebble.messageKeys.forEach(function(name, i) {
  keys[name] = 10000 + i;
});
var resolve = Module._resolveFilename;
Module._resolveFilename = function(request) {
  if (request === 'message_keys') return 'message_keys';
  return resolve.apply(this, arguments);
};
require.cache.message_keys = { id: 'message_keys', filename: 'message_keys', loaded: true,
                               exports: keys };

var Clay = require(path.join(root, 'node_modules/@rebble/clay/dist/js/index.js'));
var clay = new Clay(require(path.join(root, 'src/pkjs/config')),
                    require(path.join(root, 'src/pkjs/custom-clay')),
                    { autoHandleEvents: false });
clay.meta.activeWatchInfo = watch;
var url = clay.generateUrl();
var out = path.join(root, 'build', 'config.html');
fs.mkdirSync(path.dirname(out), { recursive: true });
fs.writeFileSync(out, decodeURIComponent(url.replace('data:text/html;charset=utf-8,', '')));
console.log('geschrieben: ' + out);
