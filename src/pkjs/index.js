var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var customClay = require('./custom-clay');
var rain = require('./rain');

var clay = new Clay(clayConfig, customClay);

function log(text) {
  console.log('norl: ' + text);
}

function send(dict) {
  Pebble.sendAppMessage(dict, function() {}, function() { log('Senden fehlgeschlagen'); });
}

function fetchRain(lat, lon) {
  var xhr = new XMLHttpRequest();
  xhr.open('GET', rain.url(lat, lon));
  xhr.timeout = 30000;
  xhr.onload = function() {
    if (xhr.status !== 200) {
      log('Open-Meteo HTTP ' + xhr.status);
      return;
    }
    try {
      var m = rain.message(JSON.parse(xhr.responseText));
      send(m);
      log('Regen gesendet, % ab jetzt: ' + m.RAIN_PERCENT.slice(0, 12).join(' '));
    } catch (e) {
      log('Antwort unbrauchbar: ' + e);
    }
  };
  xhr.onerror = function() { log('kein Netz'); };
  xhr.ontimeout = function() { log('Zeitüberschreitung'); };
  xhr.send();
}

// Kein Standort zu bekommen: den letzten nehmen
function requestRain() {
  navigator.geolocation.getCurrentPosition(function(pos) {
    var here = { lat: pos.coords.latitude, lon: pos.coords.longitude };
    localStorage.setItem('location', JSON.stringify(here));
    fetchRain(here.lat, here.lon);
  }, function() {
    var last = null;
    try {
      last = JSON.parse(localStorage.getItem('location'));
    } catch (e) {
      last = null;
    }
    if (last) {
      fetchRain(last.lat, last.lon);
    } else {
      log('kein Standort');
    }
  }, { enableHighAccuracy: false, maximumAge: 30 * 60 * 1000, timeout: 30000 });
}

// Ob neues Wetter nötig ist, entscheidet die Uhr. Sie fragt höchstens einmal pro Stunde.
Pebble.addEventListener('ready', function() {
  send({ RAIN_READY: 1 });
});

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.RAIN_REQUEST !== undefined) {
    requestRain();
  }
});
