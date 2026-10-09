module.exports = [
  {
    "type": "heading",
    "defaultValue": "Norl T2"
  },
  {
    "type": "text",
    "defaultValue": "Große Stundenziffer, darunter eine Skala für die Minuten. Schütteln zeigt die genaue Uhrzeit mit Sekunden und das Datum."
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
        "messageKey": "DETAIL_SECONDS",
        "label": "Detailanzeige nach dem Schütteln",
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
    "type": "submit",
    "defaultValue": "Speichern"
  }
];
