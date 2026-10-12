// Läuft auf der Einstellungsseite: Schwelle und Farbe der Akkuanzeige
// nur zeigen, wenn die Anzeige eingeschaltet ist.
module.exports = function() {
  var clayConfig = this;

  function updateBattery() {
    var show = this.get();
    ['BATTERY_THRESHOLD', 'BATTERY'].forEach(function(key) {
      var item = clayConfig.getItemByMessageKey(key);
      if (show) {
        item.show();
      } else {
        item.hide();
      }
    });
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var toggle = clayConfig.getItemByMessageKey('BATTERY_SHOW');
    updateBattery.call(toggle);
    toggle.on('change', updateBattery);
  });
};
