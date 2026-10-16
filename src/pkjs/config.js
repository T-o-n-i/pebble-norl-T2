module.exports = [
  {
    "type": "heading",
    "defaultValue": "Norl T2"
  },
  {
    "type": "text",
    "defaultValue": "Große Stundenziffer, darunter eine Skala für die Minuten. Schütteln oder Antippen zeigt die genaue Uhrzeit mit Sekunden und das Datum."
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
        "label": "Minutenskala, Sekunden und Datum"
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
    "type": "submit",
    "defaultValue": "Speichern"
  }
];
