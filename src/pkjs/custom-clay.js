// Läuft auf der Einstellungsseite: Felder nur zeigen, wenn sie gerade etwas bewirken.
module.exports = function() {
  var clayConfig = this;

  function showIf(key, show) {
    var item = clayConfig.getItemByMessageKey(key);
    if (show) {
      item.show();
    } else {
      item.hide();
    }
  }

  // Schwelle und Farbe der Akkuanzeige nur, wenn die Anzeige eingeschaltet ist
  function updateBattery() {
    var show = clayConfig.getItemByMessageKey('BATTERY_SHOW').get();
    showIf('BATTERY_THRESHOLD', show);
    showIf('BATTERY', show);
  }

  // Beim Regen die Schwelle passend zur Anzeigeart
  function updateRain() {
    var mode = clayConfig.getItemByMessageKey('RAIN_MODE').get();
    showIf('RAIN_MIN_PERCENT', mode === '1');
    showIf('RAIN_MIN_TENTHS', mode === '2');
    showIf('RAIN', mode !== '0');
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    updateBattery();
    clayConfig.getItemByMessageKey('BATTERY_SHOW').on('change', updateBattery);
    updateRain();
    clayConfig.getItemByMessageKey('RAIN_MODE').on('change', updateRain);
  });
};
