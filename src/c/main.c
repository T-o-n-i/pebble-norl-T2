#include <pebble.h>
#include "glyphs.h"

// Norl für die Pebble Time 2: große Stundenziffer, darunter eine Skala für die Minuten.
// Schütteln oder Antippen (erkannt am Einschalten des Lichts) zeigt für ein paar Sekunden
// Uhrzeit und Datum in groß, dazwischen die Sekunden oder den Regen der nächsten 12 Stunden.
// Den Regen holt das Handy stündlich bei Open-Meteo.
// Bei niedrigem Akku wird die Skala kürzer, rechts daneben steht der Ladezustand als Feld.

// 1 = Detailanzeige mit Testdaten statt Wetter vom Handy, Schütteln schaltet durch Uhrzeiten
// und Anzeigearten
#define RAIN_DEMO 0

#define SETTINGS_KEY 1
#define RAIN_KEY     2

#define MARGIN       6   // Rand um die Stundenziffer
#define METER_H     30   // Höhe der Minutenskala
#define METER_GAP    8   // Abstand zwischen Ziffer und Skala
#define TRIGGER_PAUSE_MS 1000  // Schütteln und Licht kommen oft zusammen, nur das erste zählt
#define LIGHT_DELAY_MS     50  // nach Licht an kurz warten, ob ein Knopf ein Menü öffnet
#define BATTERY_GAP  6   // Abstand zwischen Skala und Akkuanzeige
#define BATTERY_PAD  5   // Innenabstand im Akkufeld
#define ICON_W      12   // Batteriesymbol ohne Pol
#define ICON_H      18
#define ICON_GAP     4   // Abstand zwischen Symbol und Zahl
#define RAIN_HOURS  12   // die volle Breite sind 12 Stunden, der linke Rand ist jetzt
#define RAIN_TOP   112   // Oberkante der Regenbalken in der Detailanzeige
#define RAIN_BAR_H  46   // Höhe der Balken bei 100 % oder voller Menge
#define RAIN_FULL   40   // Regenmenge in 0,1 mm/h, bei der ein Balken ganz oben ankommt
#define RAIN_DATA   24   // so viele Stunden schickt das Handy
#define RAIN_MISSING 255
#define RAIN_REFRESH (60 * 60)      // Wetter höchstens einmal pro Stunde holen
#define RAIN_RETRY   (15 * 60)      // ohne Antwort erst nach 15 Minuten wieder fragen
#define RAIN_STALE   (3 * 60 * 60)  // ältere Daten nicht mehr zeigen

typedef enum {
  HOUR_MODE_WATCH = 0,  // wie in der Uhr eingestellt
  HOUR_MODE_12 = 1,
  HOUR_MODE_24 = 2,
} HourMode;

typedef enum {
  TRIGGER_SHAKE = 0,  // nur Schütteln
  TRIGGER_LIGHT = 1,  // nur Licht an (Antippen, Knopf, Handgelenk drehen)
  TRIGGER_BOTH = 2,
} DetailTrigger;

typedef enum {
  RAIN_OFF = 0,          // Detailanzeige zeigt die Sekunden
  RAIN_PROBABILITY = 1,  // Höhe des Balkens = Regenwahrscheinlichkeit
  RAIN_AMOUNT = 2,       // Höhe des Balkens = Regenmenge
} RainMode;

typedef struct {
  GColor background;
  GColor digits;   // Stundenziffer und Uhrzeit in der Detailanzeige
  GColor accent;   // Minutenskala, Sekunden und Datum
  uint8_t hour_mode;
  uint8_t detail_seconds;
  uint8_t battery_threshold;  // Akkustand anzeigen ab diesem Wert in %
  GColor battery;             // Hintergrund des Akkufelds
  bool battery_show;          // Akkuanzeige eingeschaltet
  uint8_t detail_trigger;     // DetailTrigger
  uint8_t rain_mode;          // RainMode
  uint8_t rain_min_percent;   // Balken erst ab dieser Wahrscheinlichkeit
  uint8_t rain_min_tenths;    // Balken erst ab dieser Menge in 0,1 mm/h
  GColor rain;
} Settings;

// Regen vom Handy, auf der Uhr gespeichert
typedef struct {
  int32_t start;    // Beginn der ersten Stunde (UTC)
  int32_t fetched;  // wann die Uhr die Daten bekommen hat
  uint8_t percent[RAIN_DATA];  // Wahrscheinlichkeit in %
  uint8_t tenths[RAIN_DATA];   // Menge in 0,1 mm/h
} RainData;

static Settings s_settings;
static RainData s_rain;
static time_t s_rain_asked;  // letzte Anfrage ans Handy

static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_detail_timer;
static AppTimer *s_light_timer;
static bool s_detail;
static uint32_t s_last_trigger_ms;
static BatteryChargeState s_battery;

static const char *const WEEKDAYS[] = {
  "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"
};
static const char *const MONTHS[] = {
  "Januar", "Februar", "März", "April", "Mai", "Juni",
  "Juli", "August", "September", "Oktober", "November", "Dezember"
};

// ---------------------------------------------------------------- Einstellungen

static void settings_defaults(void) {
  s_settings.background = GColorBlack;
  s_settings.digits = GColorWhite;
  s_settings.accent = GColorWhite;
  s_settings.hour_mode = HOUR_MODE_WATCH;
  s_settings.detail_seconds = 10;
  s_settings.battery_threshold = 20;
  s_settings.battery = GColorWhite;
  s_settings.battery_show = true;
  s_settings.detail_trigger = TRIGGER_BOTH;
  s_settings.rain_mode = RAIN_PROBABILITY;
  s_settings.rain_min_percent = 30;
  s_settings.rain_min_tenths = 1;
  s_settings.rain = GColorPictonBlue;
}

static void settings_load(void) {
  settings_defaults();
  if (persist_exists(SETTINGS_KEY)) {
    persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
  }
}

static void settings_save(void) {
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static void rain_load(void) {
  if (persist_exists(RAIN_KEY)) {
    persist_read_data(RAIN_KEY, &s_rain, sizeof(s_rain));
  }
}

static bool use_24h(void) {
  switch (s_settings.hour_mode) {
    case HOUR_MODE_12: return false;
    case HOUR_MODE_24: return true;
    default: return clock_is_24h_style();
  }
}

// Stunde 0 bis 23 so, wie sie angezeigt wird
static int clock_hour(int hour) {
  if (use_24h()) {
    return hour;
  }
  int h = hour % 12;
  return h == 0 ? 12 : h;
}

static int display_hour(const struct tm *t) {
  return clock_hour(t->tm_hour);
}

// ---------------------------------------------------------------- Zeichnen

// Minutenskala: gefüllt bis zur aktuellen Minute, Striche alle 5 Minuten
static void draw_meter(GContext *ctx, GRect r, int minute) {
  int16_t fill_w = r.size.w * minute / 60;
  graphics_context_set_fill_color(ctx, s_settings.accent);
  graphics_fill_rect(ctx, GRect(r.origin.x, r.origin.y, fill_w, r.size.h), 0, GCornerNone);

  for (int i = 1; i < 12; i++) {
    int16_t x = r.origin.x + r.size.w * i / 12 - 1;
    int16_t th;
    if (i == 6) {
      th = r.size.h * 3 / 4;
    } else if (i == 3 || i == 9) {
      th = r.size.h / 2;
    } else {
      th = r.size.h / 4;
    }
    bool inside = x + 3 <= r.origin.x + fill_w;
    graphics_context_set_fill_color(ctx, inside ? s_settings.background : s_settings.accent);
    graphics_fill_rect(ctx, GRect(x, r.origin.y + r.size.h - th, 3, th), 0, GCornerNone);
  }
}

// Stundenziffer so groß wie möglich in area, schmale Einsen wie im Original
static void draw_hour(GContext *ctx, GRect area, int hour) {
  char text[4];
  snprintf(text, sizeof(text), "%d", hour);
  int n = strlen(text);
  int ones = 0;
  for (int i = 0; i < n; i++) {
    if (text[i] == '1') ones++;
  }
  int wide = n - ones;

  GlyphStyle st = { .height = area.size.h, .narrow_one = true };
  int16_t s = area.size.h * 18 / 100;
  for (int pass = 0; pass < 3; pass++) {
    st.stroke = s;
    st.gap = s * 2 / 5;  // wie im Original: die 1 steht dicht an der zweiten Ziffer
    if (wide > 0) {
      st.width = (area.size.w - ones * s - (n - 1) * st.gap) / wide;
    } else {
      st.width = area.size.w;
      if (n == 2) {
        st.gap = 2 * s;  // "11": zwei Striche mit doppelter Strichbreite Abstand
      }
    }
    int16_t s2 = glyph_stroke_for(st.height, st.width);
    if (s2 >= s) break;
    s = s2;
  }

  int16_t tw = glyph_text_width(text, &st);
  GPoint origin = GPoint(area.origin.x + (area.size.w - tw) / 2, area.origin.y);
  glyph_draw_text(ctx, origin, text, &st, s_settings.digits, s_settings.background);
}

static bool battery_low(void) {
  return s_settings.battery_show && !s_battery.is_charging &&
         s_battery.charge_percent <= s_settings.battery_threshold;
}

// Senkrechtes Batteriesymbol, innen gefüllt je nach Ladezustand
static void draw_battery_icon(GContext *ctx, GRect r, int percent, GColor c) {
  graphics_context_set_fill_color(ctx, c);
  graphics_fill_rect(ctx, GRect(r.origin.x + 3, r.origin.y, r.size.w - 6, 2), 0, GCornerNone);
  GRect body = GRect(r.origin.x, r.origin.y + 2, r.size.w, r.size.h - 2);
  graphics_context_set_stroke_color(ctx, c);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_round_rect(ctx, GRect(body.origin.x + 1, body.origin.y + 1,
                                      body.size.w - 2, body.size.h - 2), 2);
  int16_t inner = body.size.h - 8;
  int16_t level = (inner * percent + 99) / 100;
  graphics_fill_rect(ctx, GRect(body.origin.x + 4, body.origin.y + 4 + inner - level,
                                body.size.w - 8, level), 0, GCornerNone);
}

// Akkufeld unten rechts: Symbol und Zahl in Hintergrundfarbe auf farbigem Feld.
// Gibt die Breite des Felds zurück.
static int16_t draw_battery(GContext *ctx, GRect b) {
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  // Platz für zwei Ziffern reservieren, damit die Skala nicht springt
  int16_t num_w = graphics_text_layout_get_content_size("00", font, GRect(0, 0, 100, 40),
                                                        GTextOverflowModeWordWrap,
                                                        GTextAlignmentLeft).w;
  int16_t w = BATTERY_PAD + ICON_W + ICON_GAP + num_w + BATTERY_PAD;
  GRect box = GRect(b.size.w - w, b.size.h - METER_H, w, METER_H);
  graphics_context_set_fill_color(ctx, s_settings.battery);
  graphics_fill_rect(ctx, box, 4, GCornerTopLeft | GCornerTopRight);

  char text[4];
  snprintf(text, sizeof(text), "%d", s_battery.charge_percent);
  int16_t tw = graphics_text_layout_get_content_size(text, font, GRect(0, 0, 100, 40),
                                                     GTextOverflowModeWordWrap,
                                                     GTextAlignmentLeft).w;
  // Symbol und Zahl gemeinsam mittig im Feld
  int16_t x = box.origin.x + (w - (ICON_W + ICON_GAP + tw)) / 2;
  int16_t icon_h = ICON_H + 2;
  draw_battery_icon(ctx, GRect(x, box.origin.y + (METER_H - icon_h) / 2, ICON_W, icon_h),
                    s_battery.charge_percent, s_settings.background);
  graphics_context_set_text_color(ctx, s_settings.background);
  graphics_draw_text(ctx, text, font, GRect(x + ICON_W + ICON_GAP, box.origin.y - 3, tw + 4, METER_H + 3),
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  return w;
}

static int16_t text_width(const char *text, GFont font) {
  return graphics_text_layout_get_content_size(text, font, GRect(0, 0, 400, 40),
                                               GTextOverflowModeWordWrap,
                                               GTextAlignmentLeft).w;
}

// ---------------------------------------------------------------- Regen

#if RAIN_DEMO
// Ausgedachter Tag: Regen am Morgen, Gewitter am Nachmittag, Schauer in der Nacht.
// Je Uhrzeit Wahrscheinlichkeit in % und Menge in 0,1 mm/h für die Stunde ab dann.
static const uint8_t DEMO_PERCENT[24] = {
  0, 0, 5, 10, 20, 35, 60, 80, 90, 75, 50, 30, 20, 15, 40, 65, 85, 70, 45, 25, 10, 5, 30, 55
};
static const uint8_t DEMO_TENTHS[24] = {
  0, 0, 0, 0, 0, 1, 3, 8, 15, 6, 2, 1, 0, 0, 1, 4, 25, 10, 2, 0, 0, 0, 1, 3
};
static const uint8_t DEMO_HOURS[] = { 1, 4, 4, 4, 8, 13, 17, 20 };
static const uint8_t DEMO_MINUTES[] = { 35, 0, 20, 40, 35, 10, 50, 35 };
#define DEMO_COUNT (3 * ARRAY_LENGTH(DEMO_HOURS))
static int s_demo;  // Stunde und Anzeigeart, Antippen zählt weiter
#endif

static int isqrt(int n) {
  int r = 0;
  while ((r + 1) * (r + 1) <= n) r++;
  return r;
}

// Wahrscheinlichkeit und Menge für die Stunde, die i Stunden nach der aktuellen beginnt.
// false, wenn dafür nichts bekannt ist.
static bool rain_value(const struct tm *t, int i, int *percent, int *tenths) {
#if RAIN_DEMO
  *percent = DEMO_PERCENT[(t->tm_hour + i) % 24];
  *tenths = DEMO_TENTHS[(t->tm_hour + i) % 24];
  return true;
#else
  time_t now = time(NULL);
  if (now - s_rain.fetched > RAIN_STALE) return false;
  time_t hour = now - t->tm_min * 60 - t->tm_sec + i * SECONDS_PER_HOUR;
  if (hour < s_rain.start) return false;
  int k = (hour - s_rain.start) / SECONDS_PER_HOUR;
  if (k >= RAIN_DATA) return false;
  *percent = s_rain.percent[k];
  *tenths = s_rain.tenths[k];
  return true;
#endif
}

// Regen statt Sekunden in der Detailanzeige: eingeschaltet und nicht zu alt
static bool detail_rain(void) {
  if (s_settings.rain_mode == RAIN_OFF) return false;
#if RAIN_DEMO
  return true;
#else
  return time(NULL) - s_rain.fetched <= RAIN_STALE;
#endif
}

// Höhe des Balkens für die Stunde i, 0 = kein Balken
static int16_t rain_height(const struct tm *t, int i, int16_t full) {
  int percent, tenths;
  if (!rain_value(t, i, &percent, &tenths)) return 0;
  if (s_settings.rain_mode == RAIN_PROBABILITY) {
    if (percent == RAIN_MISSING || percent < s_settings.rain_min_percent) return 0;
    return full * percent / 100;
  }
  if (tenths == RAIN_MISSING || tenths < s_settings.rain_min_tenths || tenths == 0) return 0;
  if (tenths >= RAIN_FULL) return full;
  // Wurzel, damit auch Nieselregen zu sehen ist: 1 mm/h reicht bis zur halben Höhe
  int16_t h = isqrt(full * full * tenths / RAIN_FULL);
  return h < 2 ? 2 : h;
}

// Regenbalken für die nächsten 12 Stunden, darunter eine Skala mit den vollen Stunden.
// Der linke Rand ist jetzt, die Balken wandern mit den Minuten nach links: Links wird die
// laufende Stunde angeschnitten, rechts kommt die dreizehnte herein.
static void draw_rain(GContext *ctx, GRect r, const struct tm *t) {
  const int span = RAIN_HOURS * 60;
  int16_t left = r.origin.x;
  int16_t right = r.origin.x + r.size.w - 1;
  int16_t base = r.origin.y + RAIN_BAR_H;  // Unterkante der Balken

  graphics_context_set_fill_color(ctx, s_settings.rain);
  for (int i = 0; i <= RAIN_HOURS; i++) {
    int16_t h = rain_height(t, i, RAIN_BAR_H);
    if (h <= 0) continue;
    // 2 Pixel Lücke zwischen den Balken, genau über dem Strich der Skala
    int16_t x0 = left + r.size.w * (i * 60 - t->tm_min) / span + 1;
    int16_t x1 = left + r.size.w * ((i + 1) * 60 - t->tm_min) / span - 2;
    if (x0 < left) x0 = left;
    if (x1 > right) x1 = right;
    if (x1 < x0) continue;
    graphics_fill_rect(ctx, GRect(x0, base - h, x1 - x0 + 1, h), 0, GCornerNone);
  }

  graphics_context_set_fill_color(ctx, s_settings.accent);
  graphics_fill_rect(ctx, GRect(left, base + 1, r.size.w, 2), 0, GCornerNone);

  // Striche zu jeder vollen Stunde, alle 3 Stunden länger und mit der Uhrzeit
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  graphics_context_set_text_color(ctx, s_settings.accent);
  for (int i = 0; i <= RAIN_HOURS; i++) {
    int16_t x = left + r.size.w * (i * 60 - t->tm_min) / span - 1;
    if (x < left || x + 1 > right) continue;
    int hour = (t->tm_hour + i) % 24;
    bool labeled = hour % 3 == 0;
    graphics_fill_rect(ctx, GRect(x, base + 3, 2, labeled ? 6 : 3), 0, GCornerNone);
    if (!labeled) continue;
    char text[4];
    snprintf(text, sizeof(text), "%d", clock_hour(hour));
    int16_t tw = text_width(text, font);
    int16_t tx = x + 1 - tw / 2;
    if (tx < left || tx + tw > right + 1) continue;
    graphics_draw_text(ctx, text, font, GRect(tx - 2, base + 4, tw + 4, 22),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
}

static void draw_face(GContext *ctx, GRect b, const struct tm *t) {
  GRect meter = GRect(b.origin.x, b.size.h - METER_H, b.size.w, METER_H);
  GRect digit = GRect(MARGIN, MARGIN, b.size.w - 2 * MARGIN,
                      b.size.h - METER_H - METER_GAP - MARGIN);
  draw_hour(ctx, digit, display_hour(t));

  if (battery_low()) {
    meter.size.w -= draw_battery(ctx, b) + BATTERY_GAP;
  }
  draw_meter(ctx, meter, t->tm_min);
}

static void draw_centered_text(GContext *ctx, const char *text, GFont font, GRect r, GColor c) {
  graphics_context_set_text_color(ctx, c);
  graphics_draw_text(ctx, text, font, r, GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}

static const char *const WEEKDAYS_SHORT[] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" };

// Datumszeile, so ausführlich wie in die Breite passt
static void format_date(char *buf, size_t len, const struct tm *t, GFont font, int16_t max_w) {
  snprintf(buf, len, "%s, %d. %s", WEEKDAYS[t->tm_wday], t->tm_mday, MONTHS[t->tm_mon]);
  if (text_width(buf, font) <= max_w) return;
  snprintf(buf, len, "%s, %d. %s", WEEKDAYS_SHORT[t->tm_wday], t->tm_mday, MONTHS[t->tm_mon]);
  if (text_width(buf, font) <= max_w) return;
  snprintf(buf, len, "%s, %d.%d.", WEEKDAYS_SHORT[t->tm_wday], t->tm_mday, t->tm_mon + 1);
}

// Detailanzeige: Stunden und Minuten, darunter die Sekunden, unten das Datum
static void draw_detail(GContext *ctx, GRect b, const struct tm *t) {
  char hm[8];
  snprintf(hm, sizeof(hm), "%d:%02d", display_hour(t), t->tm_min);
  GlyphStyle big = { .height = 100, .width = 39, .gap = 8, .narrow_one = true };
  big.stroke = glyph_stroke_for(big.height, big.width);
  int16_t tw = glyph_text_width(hm, &big);
  glyph_draw_text(ctx, GPoint((b.size.w - tw) / 2, 6), hm, &big, s_settings.digits,
                  s_settings.background);

  if (detail_rain()) {
    draw_rain(ctx, GRect(MARGIN, RAIN_TOP, b.size.w - 2 * MARGIN, b.size.h - RAIN_TOP), t);
  } else {
    char ss[4];
    snprintf(ss, sizeof(ss), "%02d", t->tm_sec);
    GlyphStyle mid = { .height = 68, .width = 42, .gap = 10, .narrow_one = true };
    mid.stroke = glyph_stroke_for(mid.height, mid.width);
    tw = glyph_text_width(ss, &mid);
    glyph_draw_text(ctx, GPoint((b.size.w - tw) / 2, 118), ss, &mid, s_settings.accent,
                    s_settings.background);
  }

  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  char date[32];
  format_date(date, sizeof(date), t, font, b.size.w - 8);
  draw_centered_text(ctx, date, font, GRect(0, b.size.h - 36, b.size.w, 34), s_settings.accent);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, s_settings.background);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
#if RAIN_DEMO
  struct tm demo = *t;
  demo.tm_hour = DEMO_HOURS[s_demo % ARRAY_LENGTH(DEMO_HOURS)];
  demo.tm_min = DEMO_MINUTES[s_demo % ARRAY_LENGTH(DEMO_HOURS)];
  int kind = s_demo / ARRAY_LENGTH(DEMO_HOURS);
  s_settings.hour_mode = kind == 2 ? HOUR_MODE_12 : HOUR_MODE_24;
  s_settings.rain_mode = kind == 1 ? RAIN_AMOUNT : RAIN_PROBABILITY;
  s_detail = true;
  t = &demo;
#endif
  if (s_detail) {
    draw_detail(ctx, b, t);
  } else {
    draw_face(ctx, b, t);
  }
}

// ---------------------------------------------------------------- Wetter vom Handy

// Fragt das Handy nach neuem Regen, wenn die Daten älter als eine Stunde sind.
// Ohne Verbindung zum Handy passiert nichts, beim Wiederverbinden wird gleich gefragt.
static void rain_request(void) {
  if (RAIN_DEMO || s_settings.rain_mode == RAIN_OFF) return;
  time_t now = time(NULL);
  if (now - s_rain.fetched < RAIN_REFRESH) return;
  if (s_rain_asked && now - s_rain_asked < RAIN_RETRY) return;
  if (!connection_service_peek_pebble_app_connection()) return;
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) return;
  dict_write_uint8(iter, MESSAGE_KEY_RAIN_REQUEST, 1);
  if (app_message_outbox_send() == APP_MSG_OK) {
    s_rain_asked = now;
  }
}

static void rain_received(DictionaryIterator *iter) {
  Tuple *start = dict_find(iter, MESSAGE_KEY_RAIN_START);
  Tuple *percent = dict_find(iter, MESSAGE_KEY_RAIN_PERCENT);
  Tuple *tenths = dict_find(iter, MESSAGE_KEY_RAIN_TENTHS);
  if (!start || !percent || !tenths) return;
  memset(s_rain.percent, RAIN_MISSING, RAIN_DATA);
  memset(s_rain.tenths, RAIN_MISSING, RAIN_DATA);
  memcpy(s_rain.percent, percent->value->data, percent->length < RAIN_DATA ? percent->length : RAIN_DATA);
  memcpy(s_rain.tenths, tenths->value->data, tenths->length < RAIN_DATA ? tenths->length : RAIN_DATA);
  s_rain.start = start->value->int32;
  s_rain.fetched = time(NULL);
  persist_write_data(RAIN_KEY, &s_rain, sizeof(s_rain));
  layer_mark_dirty(s_canvas);
}

static void connection_handler(bool connected) {
  if (connected) {
    s_rain_asked = 0;
    rain_request();
  }
}

// ---------------------------------------------------------------- Zeit und Schütteln

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    rain_request();
  }
  layer_mark_dirty(s_canvas);
}

static void battery_handler(BatteryChargeState state) {
  s_battery = state;
  layer_mark_dirty(s_canvas);
}

static void detail_hide(void *data) {
  s_detail_timer = NULL;
  s_detail = false;
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  layer_mark_dirty(s_canvas);
}

static void detail_show(void) {
  s_detail = true;
  // Sekunden laufen nur, solange die Detailanzeige sie zeigt
  tick_timer_service_subscribe(detail_rain() ? MINUTE_UNIT : SECOND_UNIT, tick_handler);
  uint32_t ms = (uint32_t)s_settings.detail_seconds * 1000;
  if (s_detail_timer) {
    app_timer_reschedule(s_detail_timer, ms);
  } else {
    s_detail_timer = app_timer_register(ms, detail_hide, NULL);
  }
  layer_mark_dirty(s_canvas);
}

static uint32_t now_ms(void) {
  time_t sec;
  uint16_t ms;
  time_ms(&sec, &ms);
  return (uint32_t)sec * 1000 + ms;
}

// true, wenn gerade schon ein anderer Auslöser gewirkt hat. Beim Drehen des Handgelenks
// kommen Schütteln und Licht fast gleichzeitig, sonst würde das zweite die Anzeige gleich
// wieder schließen oder öffnen.
static bool trigger_too_soon(void) {
  uint32_t ms = now_ms();
  if (s_last_trigger_ms && ms - s_last_trigger_ms < TRIGGER_PAUSE_MS) {
    return true;
  }
  s_last_trigger_ms = ms;
  return false;
}

static void light_timer_cancel(void) {
  if (s_light_timer) {
    app_timer_cancel(s_light_timer);
    s_light_timer = NULL;
  }
}

static void light_delayed(void *data) {
  s_light_timer = NULL;
  detail_show();
}

// Licht an öffnet die Detailanzeige oder verlängert sie, schließt sie aber nie.
// Ein Knopfdruck schaltet das Licht ein, bevor das Menü aufgeht. Deshalb erst kurz warten:
// Wird das Watchface in der Zeit beendet oder verdeckt, blitzt die Detailanzeige nicht auf.
static void backlight_handler(bool on) {
  if (!on || trigger_too_soon()) return;
  light_timer_cancel();
  s_light_timer = app_timer_register(LIGHT_DELAY_MS, light_delayed, NULL);
}

static void focus_will_change(bool in_focus) {
  if (!in_focus) {
    light_timer_cancel();
  }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
#if RAIN_DEMO
  s_demo = (s_demo + 1) % DEMO_COUNT;
  layer_mark_dirty(s_canvas);
  return;
#endif
  if (trigger_too_soon()) return;
  if (s_detail) {
    if (s_detail_timer) {
      app_timer_cancel(s_detail_timer);
    }
    detail_hide(NULL);
  } else {
    detail_show();
  }
}

static void triggers_subscribe(void) {
  accel_tap_service_unsubscribe();
  backlight_service_unsubscribe();
  if (s_settings.detail_trigger != TRIGGER_LIGHT) {
    accel_tap_service_subscribe(tap_handler);
  }
  if (s_settings.detail_trigger != TRIGGER_SHAKE) {
    backlight_service_subscribe(backlight_handler);
  }
}

// ---------------------------------------------------------------- Einstellungen vom Handy

static uint8_t tuple_uint(const Tuple *t) {
  if (t->type == TUPLE_CSTRING) {
    return (uint8_t)atoi(t->value->cstring);
  }
  return (uint8_t)t->value->int32;
}

static void settings_received(DictionaryIterator *iter) {
  Tuple *t;
  if ((t = dict_find(iter, MESSAGE_KEY_BACKGROUND))) {
    s_settings.background = GColorFromHEX(t->value->int32);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_DIGITS))) {
    s_settings.digits = GColorFromHEX(t->value->int32);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_ACCENT))) {
    s_settings.accent = GColorFromHEX(t->value->int32);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_HOUR_MODE))) {
    s_settings.hour_mode = tuple_uint(t);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_DETAIL_SECONDS))) {
    uint8_t secs = tuple_uint(t);
    s_settings.detail_seconds = secs > 0 ? secs : 10;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_DETAIL_TRIGGER))) {
    s_settings.detail_trigger = tuple_uint(t);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_BATTERY))) {
    s_settings.battery = GColorFromHEX(t->value->int32);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_BATTERY_SHOW))) {
    s_settings.battery_show = tuple_uint(t) != 0;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_BATTERY_THRESHOLD))) {
    s_settings.battery_threshold = tuple_uint(t);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_RAIN_MODE))) {
    s_settings.rain_mode = tuple_uint(t);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_RAIN_MIN_PERCENT))) {
    s_settings.rain_min_percent = tuple_uint(t);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_RAIN_MIN_TENTHS))) {
    s_settings.rain_min_tenths = tuple_uint(t);
  }
  if ((t = dict_find(iter, MESSAGE_KEY_RAIN))) {
    s_settings.rain = GColorFromHEX(t->value->int32);
  }
  settings_save();
  triggers_subscribe();
  window_set_background_color(s_window, s_settings.background);
  layer_mark_dirty(s_canvas);
  rain_request();  // falls der Regen gerade eingeschaltet wurde
}

// Vom Handy kommen Einstellungen (Clay schickt immer alle Felder), Regen
// oder die Meldung, dass das Handy-Skript bereit ist.
static void inbox_received(DictionaryIterator *iter, void *context) {
  if (dict_find(iter, MESSAGE_KEY_BACKGROUND)) {
    settings_received(iter);
  }
  if (dict_find(iter, MESSAGE_KEY_RAIN_START)) {
    rain_received(iter);
  }
  if (dict_find(iter, MESSAGE_KEY_RAIN_READY)) {
    s_rain_asked = 0;
    rain_request();
  }
}

// ---------------------------------------------------------------- Fenster und App

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
}

static void init(void) {
  settings_load();
  rain_load();

  s_window = window_create();
  window_set_background_color(s_window, s_settings.background);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  triggers_subscribe();
  app_focus_service_subscribe_handlers((AppFocusHandlers) {
    .will_focus = focus_will_change,
  });
  s_battery = battery_state_service_peek();
  battery_state_service_subscribe(battery_handler);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = connection_handler,
  });

  app_message_register_inbox_received(inbox_received);
  app_message_open(256, 64);
}

static void deinit(void) {
  if (s_detail_timer) {
    app_timer_cancel(s_detail_timer);
  }
  light_timer_cancel();
  app_focus_service_unsubscribe();
  accel_tap_service_unsubscribe();
  backlight_service_unsubscribe();
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
