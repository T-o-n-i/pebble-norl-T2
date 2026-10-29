module.exports = [
  {
    "type": "heading",
    "defaultValue": "Norl T2"
  },
  {
    "type": "text",
    "defaultValue": "Große Stundenziffer, darunter eine Skala für die Minuten. Schütteln oder Antippen zeigt die genaue Uhrzeit, das Datum und dazwischen die Sekunden oder den Regen der nächsten 12 Stunden."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Farben"
      },
      {
        "type": "color",
        "messageKey": "BACKGROUND",
        "defaultValue": "0x000000",
        "label": "Hintergrund"
      },
      {
        "type": "color",
        "messageKey": "DIGITS",
        "defaultValue": "0xFFFFFF",
        "label": "Ziffern"
      },
      {
        "type": "color",
        "messageKey": "ACCENT",
        "defaultValue": "0xFFFFFF",
        "label": "Skalen, Sekunden und Datum"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Anzeige"
      },
      {
        "type": "select",
        "messageKey": "METER_MODE",
        "label": "Minuten",
        "description": "Beim ganzen Hintergrund füllt sich der Bildschirm im Lauf der Stunde von links nach rechts in der Farbe der Skalen. Im gefüllten Teil steht die Stundenziffer in der Hintergrundfarbe, unten bleiben die Striche alle 5 Minuten.",
        "defaultValue": "0",
        "options": [
          { "label": "Skala unten", "value": "0" },
          { "label": "Ganzer Hintergrund", "value": "1" }
        ]
      },
      {
        "type": "select",
        "messageKey": "HOUR_MODE",
        "label": "Stunden",
        "defaultValue": "0",
        "options": [
          { "label": "Wie in der Uhr eingestellt", "value": "0" },
          { "label": "12 Stunden", "value": "1" },
          { "label": "24 Stunden", "value": "2" }
        ]
      },
      {
        "type": "select",
        "messageKey": "DETAIL_TRIGGER",
        "label": "Detailanzeige öffnen",
        "description": "Antippen erkennt das Watchface daran, dass das Licht angeht. Das Licht geht auch bei jedem Knopfdruck und beim Drehen des Handgelenks an.",
        "defaultValue": "2",
        "options": [
          { "label": "Schütteln", "value": "0" },
          { "label": "Antippen", "value": "1" },
          { "label": "Schütteln und Antippen", "value": "2" }
        ]
      },
      {
        "type": "select",
        "messageKey": "DETAIL_SECONDS",
        "label": "Detailanzeige stehen lassen",
        "defaultValue": "10",
        "options": [
          { "label": "5 Sekunden", "value": "5" },
          { "label": "10 Sekunden", "value": "10" },
          { "label": "15 Sekunden", "value": "15" },
          { "label": "30 Sekunden", "value": "30" }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Akkuanzeige"
      },
      {
        "type": "toggle",
        "messageKey": "BATTERY_SHOW",
        "label": "Akkustand anzeigen",
        "description": "Bei niedrigem Akku wird die Minutenskala kürzer, rechts daneben steht ein Feld mit Batteriesymbol und Ladezustand in Prozent. Beim Laden ist es ausgeblendet.",
        "defaultValue": true
      },
      {
        "type": "select",
        "messageKey": "BATTERY_THRESHOLD",
        "label": "Anzeigen",
        "defaultValue": "20",
        "options": [
          { "label": "Bei 10 % und weniger", "value": "10" },
          { "label": "Bei 15 % und weniger", "value": "15" },
          { "label": "Bei 20 % und weniger", "value": "20" },
          { "label": "Bei 25 % und weniger", "value": "25" },
          { "label": "Bei 30 % und weniger", "value": "30" },
          { "label": "Bei 40 % und weniger", "value": "40" },
          { "label": "Bei 50 % und weniger", "value": "50" }
        ]
      },
      {
        "type": "color",
        "messageKey": "BATTERY",
        "defaultValue": "0xFFFFFF",
        "label": "Farbe des Felds",
        "description": "Symbol und Zahl stehen in der Hintergrundfarbe."
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Regen"
      },
      {
        "type": "select",
        "messageKey": "RAIN_MODE",
        "label": "In der Detailanzeige",
        "description": "Statt der Sekunden ein Balken für jede der nächsten 12 Stunden. Der linke Rand ist jetzt, darunter eine Skala mit den vollen Stunden, alle 3 Stunden mit Uhrzeit. Das Wetter kommt stündlich von Open-Meteo für den Standort des Handys. Fehlt es seit mehr als 3 Stunden, kommen wieder die Sekunden.",
        "defaultValue": "1",
        "options": [
          { "label": "Sekunden", "value": "0" },
          { "label": "Regenwahrscheinlichkeit", "value": "1" },
          { "label": "Regenmenge", "value": "2" }
        ]
      },
      {
        "type": "select",
        "messageKey": "RAIN_MIN_PERCENT",
        "label": "Balken ab",
        "description": "Ganz oben ist 100 %.",
        "defaultValue": "30",
        "options": [
          { "label": "10 %", "value": "10" },
          { "label": "20 %", "value": "20" },
          { "label": "30 %", "value": "30" },
          { "label": "40 %", "value": "40" },
          { "label": "50 %", "value": "50" },
          { "label": "60 %", "value": "60" },
          { "label": "70 %", "value": "70" }
        ]
      },
      {
        "type": "select",
        "messageKey": "RAIN_MIN_TENTHS",
        "label": "Balken ab",
        "description": "Bei 1 mm pro Stunde reicht der Balken bis zur halben Höhe, ab 4 mm ganz nach oben.",
        "defaultValue": "1",
        "options": [
          { "label": "0,1 mm pro Stunde", "value": "1" },
          { "label": "0,2 mm pro Stunde", "value": "2" },
          { "label": "0,5 mm pro Stunde", "value": "5" },
          { "label": "1 mm pro Stunde", "value": "10" }
        ]
      },
      {
        "type": "color",
        "messageKey": "RAIN",
        "defaultValue": "0x55AAFF",
        "label": "Farbe der Balken"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Speichern"
  }
];
