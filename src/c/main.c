#include <pebble.h>
#include "glyphs.h"

// Norl für die Pebble Time 2: große Stundenziffer, darunter eine Skala für die Minuten.
// Schütteln zeigt für ein paar Sekunden Uhrzeit mit Sekunden und Datum in groß.

#define SETTINGS_KEY 1

#define MARGIN       6   // Rand um die Stundenziffer
#define METER_H     30   // Höhe der Minutenskala
#define METER_GAP    8   // Abstand zwischen Ziffer und Skala

typedef enum {
  HOUR_MODE_WATCH = 0,  // wie in der Uhr eingestellt
  HOUR_MODE_12 = 1,
  HOUR_MODE_24 = 2,
} HourMode;

typedef struct {
  GColor background;
  GColor digits;   // Stundenziffer und Uhrzeit in der Detailanzeige
  GColor accent;   // Minutenskala, Sekunden und Datum
  uint8_t hour_mode;
  uint8_t detail_seconds;
} Settings;

static Settings s_settings;

static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_detail_timer;
static bool s_detail;

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

static bool use_24h(void) {
  switch (s_settings.hour_mode) {
    case HOUR_MODE_12: return false;
    case HOUR_MODE_24: return true;
    default: return clock_is_24h_style();
  }
}

static int display_hour(const struct tm *t) {
  if (use_24h()) {
    return t->tm_hour;
  }
  int h = t->tm_hour % 12;
  return h == 0 ? 12 : h;
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

static void draw_face(GContext *ctx, GRect b, const struct tm *t) {
  GRect meter = GRect(b.origin.x, b.size.h - METER_H, b.size.w, METER_H);
  GRect digit = GRect(MARGIN, MARGIN, b.size.w - 2 * MARGIN,
                      b.size.h - METER_H - METER_GAP - MARGIN);
  draw_hour(ctx, digit, display_hour(t));
  draw_meter(ctx, meter, t->tm_min);
}

static void draw_centered_text(GContext *ctx, const char *text, GFont font, GRect r, GColor c) {
  graphics_context_set_text_color(ctx, c);
  graphics_draw_text(ctx, text, font, r, GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}

static const char *const WEEKDAYS_SHORT[] = { "So", "Mo", "Di", "Mi", "Do", "Fr", "Sa" };

static int16_t text_width(const char *text, GFont font) {
  return graphics_text_layout_get_content_size(text, font, GRect(0, 0, 400, 40),
                                               GTextOverflowModeWordWrap,
                                               GTextAlignmentLeft).w;
}

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

  char ss[4];
  snprintf(ss, sizeof(ss), "%02d", t->tm_sec);
  GlyphStyle mid = { .height = 68, .width = 42, .gap = 10, .narrow_one = true };
  mid.stroke = glyph_stroke_for(mid.height, mid.width);
  tw = glyph_text_width(ss, &mid);
  glyph_draw_text(ctx, GPoint((b.size.w - tw) / 2, 118), ss, &mid, s_settings.accent,
                  s_settings.background);

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
  if (s_detail) {
    draw_detail(ctx, b, t);
  } else {
    draw_face(ctx, b, t);
  }
}

// ---------------------------------------------------------------- Zeit und Schütteln

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
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
  // Sekunden laufen nur, solange die Detailanzeige zu sehen ist
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  uint32_t ms = (uint32_t)s_settings.detail_seconds * 1000;
  if (s_detail_timer) {
    app_timer_reschedule(s_detail_timer, ms);
  } else {
    s_detail_timer = app_timer_register(ms, detail_hide, NULL);
  }
  layer_mark_dirty(s_canvas);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_detail) {
    if (s_detail_timer) {
      app_timer_cancel(s_detail_timer);
    }
    detail_hide(NULL);
  } else {
    detail_show();
  }
}

// ---------------------------------------------------------------- Einstellungen vom Handy

static uint8_t tuple_uint(const Tuple *t) {
  if (t->type == TUPLE_CSTRING) {
    return (uint8_t)atoi(t->value->cstring);
  }
  return (uint8_t)t->value->int32;
}

static void inbox_received(DictionaryIterator *iter, void *context) {
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
  settings_save();
  window_set_background_color(s_window, s_settings.background);
  layer_mark_dirty(s_canvas);
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

  s_window = window_create();
  window_set_background_color(s_window, s_settings.background);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  accel_tap_service_subscribe(tap_handler);

  app_message_register_inbox_received(inbox_received);
  app_message_open(128, 64);
}

static void deinit(void) {
  if (s_detail_timer) {
    app_timer_cancel(s_detail_timer);
  }
  accel_tap_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
