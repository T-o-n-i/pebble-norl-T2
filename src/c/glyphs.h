#pragma once
#include <pebble.h>

// Abgerundete Ziffern im Stil von Norl, als Vektoren gezeichnet.
// Jede Ziffer besteht aus Balken mit runden Enden (wie eine 7-Segment-Anzeige),
// die Innenecken werden zusätzlich ausgerundet.

typedef struct {
  int16_t height;   // Höhe einer Ziffer
  int16_t width;    // Breite einer Ziffer (außer der schmalen 1)
  int16_t stroke;   // Strichstärke
  int16_t gap;      // Abstand zwischen zwei Zeichen
  bool narrow_one;  // true: die 1 ist nur einen Strich breit, false: mittig in voller Breite
} GlyphStyle;

// Strichstärke passend zu Höhe und Breite, wie im Original (rund 18 % der Höhe)
int16_t glyph_stroke_for(int16_t height, int16_t width);

// Zeichnet eine Ziffer 0–9 in das Rechteck box
void glyph_draw_digit(GContext *ctx, GRect box, int digit, int16_t stroke,
                      GColor fg, GColor bg);

// Breite einer Zeichenkette aus '0'–'9' und ':'
int16_t glyph_text_width(const char *text, const GlyphStyle *style);

// Zeichnet die Zeichenkette ab der linken oberen Ecke origin
void glyph_draw_text(GContext *ctx, GPoint origin, const char *text,
                     const GlyphStyle *style, GColor fg, GColor bg);
