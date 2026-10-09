#include "glyphs.h"

// Segmente wie bei einer 7-Segment-Anzeige
#define SEG_A  0x01  // oben
#define SEG_B  0x02  // rechts oben
#define SEG_C  0x04  // rechts unten
#define SEG_D  0x08  // unten
#define SEG_E  0x10  // links unten
#define SEG_F  0x20  // links oben
#define SEG_G  0x40  // Mitte

static const uint8_t DIGIT_SEGMENTS[10] = {
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,          // 0
  SEG_B | SEG_C,                                          // 1 (wird gesondert gezeichnet)
  SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,                  // 2
  SEG_A | SEG_B | SEG_G | SEG_C | SEG_D,                  // 3
  SEG_F | SEG_G | SEG_B | SEG_C,                          // 4
  SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,                  // 5
  SEG_A | SEG_F | SEG_G | SEG_E | SEG_D | SEG_C,          // 6
  SEG_A | SEG_B | SEG_C,                                  // 7
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,  // 8
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,          // 9
};

int16_t glyph_stroke_for(int16_t height, int16_t width) {
  int16_t by_height = height * 18 / 100;
  int16_t by_width = width * 28 / 100;
  int16_t s = by_height < by_width ? by_height : by_width;
  return s < 2 ? 2 : s;
}

// Balken mit vollrunden Enden
static void fill_pill(GContext *ctx, int16_t x, int16_t y, int16_t w, int16_t h) {
  int16_t r = (w < h ? w : h) / 2;
  graphics_fill_rect(ctx, GRect(x, y, w, h), r, GCornersAll);
}

// Rundet eine Innenecke aus. (cx, cy) ist die Ecke des Lochs,
// dx/dy zeigen ins Loch hinein (+1 oder -1).
static void fillet(GContext *ctx, int16_t cx, int16_t cy, int dx, int dy, int16_t r,
                   GColor fg, GColor bg) {
  if (r < 2) {
    return;
  }
  int16_t rx = dx > 0 ? cx : cx - r;
  int16_t ry = dy > 0 ? cy : cy - r;
  graphics_context_set_fill_color(ctx, fg);
  graphics_fill_rect(ctx, GRect(rx, ry, r, r), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, bg);
  int16_t mx = dx > 0 ? cx + r : cx - 1 - r;
  int16_t my = dy > 0 ? cy + r : cy - 1 - r;
  graphics_fill_circle(ctx, GPoint(mx, my), r);
}

void glyph_draw_digit(GContext *ctx, GRect box, int digit, int16_t s, GColor fg, GColor bg) {
  if (digit < 0 || digit > 9) {
    return;
  }
  int16_t x0 = box.origin.x;
  int16_t y0 = box.origin.y;
  int16_t w = box.size.w;
  int16_t h = box.size.h;
  int16_t x1 = x0 + w;
  int16_t y1 = y0 + h;

  graphics_context_set_fill_color(ctx, fg);

  if (digit == 1) {
    fill_pill(ctx, x0 + (w - s) / 2, y0, s, h);
    return;
  }

  uint8_t seg = DIGIT_SEGMENTS[digit];
  int16_t hole_h = (h - 3 * s) / 2;
  int16_t ym = y0 + s + hole_h;  // Oberkante des Mittelbalkens

  // Senkrechte Balken; zusammenhängende Segmente werden ein Balken
  if ((seg & SEG_F) && (seg & SEG_E)) {
    fill_pill(ctx, x0, y0, s, h);
  } else if (seg & SEG_F) {
    fill_pill(ctx, x0, y0, s, ym + s - y0);
  } else if (seg & SEG_E) {
    fill_pill(ctx, x0, ym, s, y1 - ym);
  }
  if ((seg & SEG_B) && (seg & SEG_C)) {
    fill_pill(ctx, x1 - s, y0, s, h);
  } else if (seg & SEG_B) {
    fill_pill(ctx, x1 - s, y0, s, ym + s - y0);
  } else if (seg & SEG_C) {
    fill_pill(ctx, x1 - s, ym, s, y1 - ym);
  }

  // Waagerechte Balken über die volle Breite
  if (seg & SEG_A) {
    fill_pill(ctx, x0, y0, w, s);
  }
  if (seg & SEG_G) {
    fill_pill(ctx, x0, ym, w, s);
  }
  if (seg & SEG_D) {
    fill_pill(ctx, x0, y1 - s, w, s);
  }

  // Innenecken ausrunden
  int16_t hole_w = w - 2 * s;
  int16_t r = s / 3;
  int16_t limit = (hole_w < hole_h ? hole_w : hole_h) / 2;
  if (r > limit) {
    r = limit;
  }
  int16_t hx0 = x0 + s;
  int16_t hx1 = x1 - s;
  // oberes Loch
  int16_t uy0 = y0 + s;
  int16_t uy1 = ym;
  if ((seg & SEG_A) && (seg & SEG_F)) fillet(ctx, hx0, uy0, 1, 1, r, fg, bg);
  if ((seg & SEG_A) && (seg & SEG_B)) fillet(ctx, hx1, uy0, -1, 1, r, fg, bg);
  if ((seg & SEG_G) && (seg & SEG_F)) fillet(ctx, hx0, uy1, 1, -1, r, fg, bg);
  if ((seg & SEG_G) && (seg & SEG_B)) fillet(ctx, hx1, uy1, -1, -1, r, fg, bg);
  // unteres Loch
  int16_t ly0 = ym + s;
  int16_t ly1 = y1 - s;
  if ((seg & SEG_G) && (seg & SEG_E)) fillet(ctx, hx0, ly0, 1, 1, r, fg, bg);
  if ((seg & SEG_G) && (seg & SEG_C)) fillet(ctx, hx1, ly0, -1, 1, r, fg, bg);
  if ((seg & SEG_D) && (seg & SEG_E)) fillet(ctx, hx0, ly1, 1, -1, r, fg, bg);
  if ((seg & SEG_D) && (seg & SEG_C)) fillet(ctx, hx1, ly1, -1, -1, r, fg, bg);
}

static int16_t char_width(char c, const GlyphStyle *st) {
  if (c == ':') {
    return st->stroke;
  }
  if (c == '1' && st->narrow_one) {
    return st->stroke;
  }
  return st->width;
}

int16_t glyph_text_width(const char *text, const GlyphStyle *st) {
  int16_t total = 0;
  for (const char *p = text; *p; p++) {
    if (p != text) {
      total += st->gap;
    }
    total += char_width(*p, st);
  }
  return total;
}

void glyph_draw_text(GContext *ctx, GPoint origin, const char *text, const GlyphStyle *st,
                     GColor fg, GColor bg) {
  int16_t x = origin.x;
  for (const char *p = text; *p; p++) {
    int16_t cw = char_width(*p, st);
    if (*p == ':') {
      // zwei runde Punkte auf Höhe der beiden Löcher
      int16_t s = st->stroke;
      int16_t hole_h = (st->height - 3 * s) / 2;
      graphics_context_set_fill_color(ctx, fg);
      graphics_fill_rect(ctx, GRect(x, origin.y + s + hole_h / 2 - s / 2, s, s), s / 2, GCornersAll);
      graphics_fill_rect(ctx, GRect(x, origin.y + 2 * s + hole_h + hole_h / 2 - s / 2, s, s),
                         s / 2, GCornersAll);
    } else if (*p >= '0' && *p <= '9') {
      glyph_draw_digit(ctx, GRect(x, origin.y, cw, st->height), *p - '0', st->stroke, fg, bg);
    }
    x += cw + st->gap;
  }
}
