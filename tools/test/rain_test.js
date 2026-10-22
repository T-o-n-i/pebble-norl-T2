// Prüft, was das Handy aus einer gespeicherten Open-Meteo-Antwort an die Uhr schickt.
// Aufruf: node tools/test/rain_test.js
var assert = require('assert');
var path = require('path');
var rain = require('../../src/pkjs/rain');
var json = require('./open-meteo-frankfurt-2026-10-22.json');

var m = rain.message(json);
assert.strictEqual(m.RAIN_START, json.hourly.time[0]);
assert.strictEqual(m.RAIN_PERCENT.length, rain.HOURS);
assert.strictEqual(m.RAIN_TENTHS.length, rain.HOURS);
// Die Stunde ab time[0] steht bei Open-Meteo unter time[1]
assert.strictEqual(m.RAIN_PERCENT[0], json.hourly.precipitation_probability[1]);
assert.strictEqual(m.RAIN_TENTHS[1], Math.round(json.hourly.precipitation[2] * 10));
m.RAIN_PERCENT.concat(m.RAIN_TENTHS).forEach(function(v) {
  assert.ok(v >= 0 && v <= 255 && v % 1 === 0, 'kein Byte: ' + v);
});

// Fehlende und zu große Werte
var odd = { hourly: { time: [0, 3600, 7200],
                      precipitation: [0, null, 99],
                      precipitation_probability: [0, 50] } };
m = rain.message(odd);
assert.strictEqual(m.RAIN_TENTHS[0], rain.MISSING);
assert.strictEqual(m.RAIN_TENTHS[1], 254);
assert.strictEqual(m.RAIN_PERCENT[0], 50);
assert.strictEqual(m.RAIN_PERCENT[1], rain.MISSING);
assert.strictEqual(m.RAIN_PERCENT[23], rain.MISSING);

assert.throws(function() { rain.message({}); });
assert.ok(/forecast_hours=25/.test(rain.url(50.1, 8.68)));

console.log('ok: ' + path.basename(__filename));
