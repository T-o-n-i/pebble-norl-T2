// Regen für die nächsten Stunden von Open-Meteo, als Nachricht für die Uhr.
// Ohne Pebble-Abhängigkeiten, damit es sich in Node testen lässt: tools/test/rain_test.js
var HOURS = 24;     // so viele Stunden ab der aktuellen bekommt die Uhr
var MISSING = 255;  // Wert fehlt

function url(lat, lon) {
  return 'https://api.open-meteo.com/v1/forecast?latitude=' + lat.toFixed(2) +
    '&longitude=' + lon.toFixed(2) +
    '&hourly=precipitation,precipitation_probability&timeformat=unixtime&timezone=auto' +
    '&forecast_hours=' + (HOURS + 1);
}

function toByte(v, factor) {
  if (v === null || v === undefined || isNaN(v)) return MISSING;
  return Math.max(0, Math.min(254, Math.round(v * factor)));
}

// Die Liste beginnt mit der aktuellen Stunde. Menge und Wahrscheinlichkeit gelten bei
// Open-Meteo jeweils für die Stunde VOR dem Zeitpunkt, die Stunde ab time[k] steht also bei k + 1.
// RAIN_PERCENT in %, RAIN_TENTHS in 0,1 mm/h, RAIN_START = Beginn der ersten Stunde (UTC).
function message(j) {
  var h = j && j.hourly;
  if (!h || !h.time || !h.time.length) throw new Error('keine Stunden in der Antwort');
  var percent = [], tenths = [];
  for (var k = 0; k < HOURS; k++) {
    percent.push(toByte(h.precipitation_probability && h.precipitation_probability[k + 1], 1));
    tenths.push(toByte(h.precipitation && h.precipitation[k + 1], 10));
  }
  return { RAIN_START: h.time[0], RAIN_PERCENT: percent, RAIN_TENTHS: tenths };
}

module.exports = { url: url, message: message, HOURS: HOURS, MISSING: MISSING };
