// Läuft auf der Einstellungsseite: Felder nur zeigen, wenn sie gerade etwas bewirken,
// und oben eine Vorschau des Watchfaces mit den gewählten Einstellungen zeichnen.
// Clay fügt diese Funktion als Text in die Seite ein, deshalb steht alles darin.
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

  // ---------------------------------------------------------------- Vorschau

  // Nachbau der Zeichnung aus main.c und glyphs.c, gleiche Maße wie auf der Uhr.
  // Beispiel: 10:38:47 Uhr, Akku bei 15 %, Regen wie bei RAIN_DEMO in main.c.
  var W = 200;
  var H = 228;
  var MARGIN = 6;
  var METER_H = 30;
  var METER_H_FLAT = 20;
  var METER_GAP = 8;
  var BATTERY_PAD = 5;
  var BATTERY_GAP = 6;
  var ICON_W = 12;
  var ICON_H = 18;
  var ICON_GAP = 4;
  var ICON_W_FLAT = 9;
  var ICON_H_FLAT = 14;
  var RAIN_HOURS = 12;
  var RAIN_TOP = 112;
  var RAIN_BAR_H = 46;
  var RAIN_FULL = 40;
  var HOUR = 10;
  var MINUTE = 38;
  var SECOND = 47;
  var BATTERY_PERCENT = 15;
  var DEMO_PERCENT = [0, 0, 5, 10, 20, 35, 60, 80, 90, 75, 50, 30,
                      20, 15, 40, 65, 85, 70, 45, 25, 10, 5, 30, 55];
  var DEMO_TENTHS = [0, 0, 0, 0, 0, 1, 3, 8, 15, 6, 2, 1,
                     0, 0, 1, 4, 25, 10, 2, 0, 0, 0, 1, 3];
  var WEEKDAYS = ['Sonntag', 'Montag', 'Dienstag', 'Mittwoch', 'Donnerstag', 'Freitag', 'Samstag'];
  var WEEKDAYS_SHORT = ['So', 'Mo', 'Di', 'Mi', 'Do', 'Fr', 'Sa'];
  var MONTHS = ['Januar', 'Februar', 'März', 'April', 'Mai', 'Juni',
                'Juli', 'August', 'September', 'Oktober', 'November', 'Dezember'];
  var FONT = '"Helvetica Neue", Helvetica, Arial, sans-serif';

  // Segmente A bis G als Bits 0 bis 6, wie in glyphs.c
  var SEG_A = 0x01, SEG_B = 0x02, SEG_C = 0x04, SEG_D = 0x08, SEG_E = 0x10, SEG_F = 0x20,
      SEG_G = 0x40;
  var SEGMENTS = [0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f];

  // Dieselbe Farbkorrektur wie im Farbwähler von Clay, damit die Vorschau zu den
  // Farbfeldern passt
  var SUNLIGHT = {
    '000000': '000000', '000055': '001e41', '0000aa': '004387', '0000ff': '0068ca',
    '005500': '2b4a2c', '005555': '27514f', '0055aa': '16638d', '0055ff': '007dce',
    '00aa00': '5e9860', '00aa55': '5c9b72', '00aaaa': '57a5a2', '00aaff': '4cb4db',
    '00ff00': '8ee391', '00ff55': '8ee69e', '00ffaa': '8aebc0', '00ffff': '84f5f1',
    '550000': '4a161b', '550055': '482748', '5500aa': '40488a', '5500ff': '2f6bcc',
    '555500': '564e36', '555555': '545454', '5555aa': '4f6790', '5555ff': '4180d0',
    '55aa00': '759a64', '55aa55': '759d76', '55aaaa': '71a6a4', '55aaff': '69b5dd',
    '55ff00': '9ee594', '55ff55': '9de7a0', '55ffaa': '9becc2', '55ffff': '95f6f2',
    'aa0000': '99353f', 'aa0055': '983e5a', 'aa00aa': '955694', 'aa00ff': '8f74d2',
    'aa5500': '9d5b4d', 'aa5555': '9d6064', 'aa55aa': '9a7099', 'aa55ff': '9587d5',
    'aaaa00': 'afa072', 'aaaa55': 'aea382', 'aaaaaa': 'ababab', 'ffffff': 'ffffff',
    'aaaaff': 'a7bae2', 'aaff00': 'c9e89d', 'aaff55': 'c9eaa7', 'aaffaa': 'c7f0c8',
    'aaffff': 'c3f9f7', 'ff0000': 'e35462', 'ff0055': 'e25874', 'ff00aa': 'e16aa3',
    'ff00ff': 'de83dc', 'ff5500': 'e66e6b', 'ff5555': 'e6727c', 'ff55aa': 'e37fa7',
    'ff55ff': 'e194df', 'ffaa00': 'f1aa86', 'ffaa55': 'f1ad93', 'ffaaaa': 'efb5b8',
    'ffaaff': 'ecc3eb', 'ffff00': 'ffeeab', 'ffff55': 'fff1b5', 'ffffaa': 'fff6d3'
  };

  // Ganzzahlige Division wie in C
  function div(a, b) {
    return (a / b) | 0;
  }

  function value(key) {
    return clayConfig.getItemByMessageKey(key).get();
  }

  function cssColor(number) {
    var hex = ('000000' + Number(number).toString(16)).slice(-6).toLowerCase();
    return '#' + (SUNLIGHT[hex] || hex);
  }

  function color(key) {
    return cssColor(value(key));
  }

  // Rechteck mit abgerundeten Ecken, Radien oben links, oben rechts, unten rechts, unten links
  function fillRound(ctx, x, y, w, h, tl, tr, br, bl) {
    ctx.beginPath();
    ctx.moveTo(x + tl, y);
    ctx.arcTo(x + w, y, x + w, y + h, tr);
    ctx.arcTo(x + w, y + h, x, y + h, br);
    ctx.arcTo(x, y + h, x, y, bl);
    ctx.arcTo(x, y, x + w, y, tl);
    ctx.closePath();
    ctx.fill();
  }

  function fillRect(ctx, x, y, w, h, c) {
    ctx.fillStyle = c;
    ctx.fillRect(x, y, w, h);
  }

  function fillPill(ctx, x, y, w, h) {
    var r = div(Math.min(w, h), 2);
    fillRound(ctx, x, y, w, h, r, r, r, r);
  }

  function fillet(ctx, cx, cy, dx, dy, r, fg, bg) {
    if (r < 2) return;
    fillRect(ctx, dx > 0 ? cx : cx - r, dy > 0 ? cy : cy - r, r, r, fg);
    ctx.fillStyle = bg;
    ctx.beginPath();
    ctx.arc((dx > 0 ? cx + r : cx - 1 - r) + 0.5, (dy > 0 ? cy + r : cy - 1 - r) + 0.5, r,
            0, 2 * Math.PI);
    ctx.fill();
  }

  function strokeFor(height, width) {
    var s = Math.min(div(height * 18, 100), div(width * 28, 100));
    return s < 2 ? 2 : s;
  }

  function drawDigit(ctx, x0, y0, w, h, digit, s, fg, bg) {
    var x1 = x0 + w;
    var y1 = y0 + h;
    ctx.fillStyle = fg;
    if (digit === 1) {
      fillPill(ctx, x0 + div(w - s, 2), y0, s, h);
      return;
    }
    var seg = SEGMENTS[digit];
    var holeH = div(h - 3 * s, 2);
    var ym = y0 + s + holeH;

    if ((seg & SEG_F) && (seg & SEG_E)) {
      fillPill(ctx, x0, y0, s, h);
    } else if (seg & SEG_F) {
      fillPill(ctx, x0, y0, s, ym + s - y0);
    } else if (seg & SEG_E) {
      fillPill(ctx, x0, ym, s, y1 - ym);
    }
    if ((seg & SEG_B) && (seg & SEG_C)) {
      fillPill(ctx, x1 - s, y0, s, h);
    } else if (seg & SEG_B) {
      fillPill(ctx, x1 - s, y0, s, ym + s - y0);
    } else if (seg & SEG_C) {
      fillPill(ctx, x1 - s, ym, s, y1 - ym);
    }
    if (seg & SEG_A) fillPill(ctx, x0, y0, w, s);
    if (seg & SEG_G) fillPill(ctx, x0, ym, w, s);
    if (seg & SEG_D) fillPill(ctx, x0, y1 - s, w, s);

    var r = Math.min(div(s, 3), div(Math.min(w - 2 * s, holeH), 2));
    var hx0 = x0 + s;
    var hx1 = x1 - s;
    var uy0 = y0 + s;
    var ly0 = ym + s;
    var ly1 = y1 - s;
    if ((seg & SEG_A) && (seg & SEG_F)) fillet(ctx, hx0, uy0, 1, 1, r, fg, bg);
    if ((seg & SEG_A) && (seg & SEG_B)) fillet(ctx, hx1, uy0, -1, 1, r, fg, bg);
    if ((seg & SEG_G) && (seg & SEG_F)) fillet(ctx, hx0, ym, 1, -1, r, fg, bg);
    if ((seg & SEG_G) && (seg & SEG_B)) fillet(ctx, hx1, ym, -1, -1, r, fg, bg);
    if ((seg & SEG_G) && (seg & SEG_E)) fillet(ctx, hx0, ly0, 1, 1, r, fg, bg);
    if ((seg & SEG_G) && (seg & SEG_C)) fillet(ctx, hx1, ly0, -1, 1, r, fg, bg);
    if ((seg & SEG_D) && (seg & SEG_E)) fillet(ctx, hx0, ly1, 1, -1, r, fg, bg);
    if ((seg & SEG_D) && (seg & SEG_C)) fillet(ctx, hx1, ly1, -1, -1, r, fg, bg);
  }

  function charWidth(ch, st) {
    return ch === ':' || (ch === '1' && st.narrowOne) ? st.stroke : st.width;
  }

  function glyphTextWidth(text, st) {
    var total = 0;
    for (var i = 0; i < text.length; i++) {
      total += (i > 0 ? st.gap : 0) + charWidth(text[i], st);
    }
    return total;
  }

  function drawGlyphs(ctx, x, y, text, st, fg, bg) {
    for (var i = 0; i < text.length; i++) {
      var ch = text[i];
      var cw = charWidth(ch, st);
      var s = st.stroke;
      if (ch === ':') {
        var holeH = div(st.height - 3 * s, 2);
        ctx.fillStyle = fg;
        fillPill(ctx, x, y + s + div(holeH, 2) - div(s, 2), s, s);
        fillPill(ctx, x, y + 2 * s + holeH + div(holeH, 2) - div(s, 2), s, s);
      } else {
        drawDigit(ctx, x, y, cw, st.height, Number(ch), s, fg, bg);
      }
      x += cw + st.gap;
    }
  }

  function textWidth(ctx, text, font) {
    ctx.font = font;
    return ctx.measureText(text).width;
  }

  function use24h() {
    return value('HOUR_MODE') !== '1';  // „wie in der Uhr“: hier 24 Stunden
  }

  function clockHour(hour) {
    if (use24h()) return hour;
    var h = hour % 12;
    return h === 0 ? 12 : h;
  }

  // Wie draw_hour in main.c: so groß wie möglich, schmale Einsen
  function drawHour(ctx, ax, ay, aw, ah, hour, c) {
    var text = String(hour);
    var n = text.length;
    var ones = text.split('1').length - 1;
    var wide = n - ones;
    var st = { height: ah, narrowOne: true };
    var s = div(ah * 18, 100);
    for (var pass = 0; pass < 3; pass++) {
      st.stroke = s;
      st.gap = div(s * 2, 5);
      if (wide > 0) {
        st.width = div(aw - ones * s - (n - 1) * st.gap, wide);
      } else {
        st.width = aw;
        if (n === 2) st.gap = 2 * s;
      }
      var s2 = strokeFor(st.height, st.width);
      if (s2 >= s) break;
      s = s2;
    }
    var tw = glyphTextWidth(text, st);
    drawGlyphs(ctx, ax + div(aw - tw, 2), ay, text, st, c.digits, c.background);
  }

  function drawMeter(ctx, x, y, w, h, minute, c, top) {
    var fillW = div(w * minute, 60);
    fillRect(ctx, x, y, fillW, h, c.accent);
    for (var i = 1; i < 12; i++) {
      var tx = x + div(w * i, 12) - 1;
      var th = i === 6 ? div(h * 3, 4) : (i === 3 || i === 9 ? div(h, 2) : div(h, 4));
      fillRect(ctx, tx, top ? y : y + h - th, 3, th, tx + 3 <= x + fillW ? c.background : c.accent);
    }
  }

  function drawBatteryIcon(ctx, x, y, w, h, percent, c) {
    fillRect(ctx, x + 3, y, w - 6, 2, c);
    var by = y + 2;
    var bh = h - 2;
    ctx.strokeStyle = c;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(x + 4, by + 1);
    ctx.arcTo(x + w - 1, by + 1, x + w - 1, by + bh - 1, 2);
    ctx.arcTo(x + w - 1, by + bh - 1, x + 1, by + bh - 1, 2);
    ctx.arcTo(x + 1, by + bh - 1, x + 1, by + 1, 2);
    ctx.arcTo(x + 1, by + 1, x + w - 1, by + 1, 2);
    ctx.closePath();
    ctx.stroke();
    var inner = bh - 8;
    var level = div(inner * percent + 99, 100);
    fillRect(ctx, x + 4, by + 4 + inner - level, w - 8, level, c);
  }

  // Akkufeld rechts im Bereich der Skala ab y mit Höhe h, gibt die Breite zurück
  function drawBattery(ctx, y, h, top) {
    var flat = h < METER_H;
    var font = 'bold ' + (flat ? 15 : 22) + 'px ' + FONT;
    var iconW = flat ? ICON_W_FLAT : ICON_W;
    var iconH = (flat ? ICON_H_FLAT : ICON_H) + 2;
    var numW = textWidth(ctx, '00', font);
    var w = Math.round(BATTERY_PAD + iconW + ICON_GAP + numW + BATTERY_PAD);
    var bx = W - w;
    ctx.fillStyle = color('BATTERY');
    if (top) {
      fillRound(ctx, bx, y, w, h, 0, 0, 4, 4);
    } else {
      fillRound(ctx, bx, y, w, h, 4, 4, 0, 0);
    }
    var text = String(BATTERY_PERCENT);
    var tw = textWidth(ctx, text, font);
    var x = bx + (w - (iconW + ICON_GAP + tw)) / 2;
    drawBatteryIcon(ctx, x, y + div(h - iconH, 2), iconW, iconH, BATTERY_PERCENT,
                    color('BACKGROUND'));
    ctx.fillStyle = color('BACKGROUND');
    ctx.textBaseline = 'middle';
    ctx.textAlign = 'left';
    ctx.fillText(text, x + iconW + ICON_GAP, y + h / 2 + 1);
    return w;
  }

  function drawFace(ctx, filled) {
    var c = { background: color('BACKGROUND'), digits: color('DIGITS'), accent: color('ACCENT') };
    if (filled) {
      c = { background: c.accent, digits: c.background, accent: c.background };
    }
    fillRect(ctx, 0, 0, W, H, c.background);
    var pos = value('METER_POSITION');
    var top = pos !== '0';
    var bottom = pos !== '1';
    var mh = pos === '2' ? METER_H_FLAT : METER_H;
    var y0 = top ? mh + METER_GAP : MARGIN;
    var y1 = H - (bottom ? mh + METER_GAP : MARGIN);
    drawHour(ctx, MARGIN, y0, W - 2 * MARGIN, y1 - y0, HOUR, c);

    // Akkufeld unten rechts, ohne Skala unten oben rechts
    var batteryY = bottom ? H - mh : 0;
    var battery = value('BATTERY_SHOW');
    var fill = value('METER_MODE') === '1';
    var meterW = W;
    if (battery && !fill) {
      meterW -= drawBattery(ctx, batteryY, mh, !bottom) + BATTERY_GAP;
    }
    var minute = fill ? 0 : MINUTE;
    if (top) drawMeter(ctx, 0, 0, meterW, mh, minute, c, true);
    if (bottom) drawMeter(ctx, 0, H - mh, meterW, mh, minute, c, false);
    if (battery && fill) {
      drawBattery(ctx, batteryY, mh, !bottom);
    }
  }

  function isqrt(n) {
    var r = 0;
    while ((r + 1) * (r + 1) <= n) r++;
    return r;
  }

  function rainHeight(i, full) {
    var mode = value('RAIN_MODE');
    var percent = DEMO_PERCENT[(HOUR + i) % 24];
    var tenths = DEMO_TENTHS[(HOUR + i) % 24];
    if (mode === '1') {
      return percent < Number(value('RAIN_MIN_PERCENT')) ? 0 : div(full * percent, 100);
    }
    if (tenths < Number(value('RAIN_MIN_TENTHS')) || tenths === 0) return 0;
    if (tenths >= RAIN_FULL) return full;
    var h = isqrt(div(full * full * tenths, RAIN_FULL));
    return h < 2 ? 2 : h;
  }

  function drawRain(ctx, x, y, w) {
    var span = RAIN_HOURS * 60;
    var right = x + w - 1;
    var base = y + RAIN_BAR_H;
    var i;
    for (i = 0; i <= RAIN_HOURS; i++) {
      var h = rainHeight(i, RAIN_BAR_H);
      if (h <= 0) continue;
      var x0 = Math.max(x, x + div(w * (i * 60 - MINUTE), span) + 1);
      var x1 = Math.min(right, x + div(w * ((i + 1) * 60 - MINUTE), span) - 2);
      if (x1 < x0) continue;
      fillRect(ctx, x0, base - h, x1 - x0 + 1, h, color('RAIN'));
    }
    var accent = color('ACCENT');
    fillRect(ctx, x, base + 1, w, 2, accent);
    var font = 'bold 15px ' + FONT;
    ctx.font = font;
    ctx.textBaseline = 'top';
    ctx.textAlign = 'center';
    for (i = 0; i <= RAIN_HOURS; i++) {
      var tx = x + div(w * (i * 60 - MINUTE), span) - 1;
      if (tx < x || tx + 1 > right) continue;
      var hour = (HOUR + i) % 24;
      var labeled = hour % 3 === 0;
      fillRect(ctx, tx, base + 3, 2, labeled ? 6 : 3, accent);
      if (!labeled) continue;
      var text = String(clockHour(hour));
      var tw = ctx.measureText(text).width;
      if (tx + 1 - tw / 2 < x || tx + 1 + tw / 2 > right + 1) continue;
      ctx.fillStyle = accent;
      ctx.fillText(text, tx + 1, base + 10);
    }
  }

  function dateText(ctx, font, maxW) {
    var d = new Date();
    var text = WEEKDAYS[d.getDay()] + ', ' + d.getDate() + '. ' + MONTHS[d.getMonth()];
    if (textWidth(ctx, text, font) <= maxW) return text;
    text = WEEKDAYS_SHORT[d.getDay()] + ', ' + d.getDate() + '. ' + MONTHS[d.getMonth()];
    if (textWidth(ctx, text, font) <= maxW) return text;
    return WEEKDAYS_SHORT[d.getDay()] + ', ' + d.getDate() + '.' + (d.getMonth() + 1) + '.';
  }

  function drawDetail(ctx) {
    var bg = color('BACKGROUND');
    var accent = color('ACCENT');
    fillRect(ctx, 0, 0, W, H, bg);
    var hm = clockHour(HOUR) + ':' + (MINUTE < 10 ? '0' : '') + MINUTE;
    var big = { height: 100, width: 39, gap: 8, narrowOne: true };
    big.stroke = strokeFor(big.height, big.width);
    drawGlyphs(ctx, div(W - glyphTextWidth(hm, big), 2), 6, hm, big, color('DIGITS'), bg);

    if (value('RAIN_MODE') !== '0') {
      drawRain(ctx, MARGIN, RAIN_TOP, W - 2 * MARGIN);
    } else {
      var ss = String(SECOND);
      var mid = { height: 68, width: 42, gap: 10, narrowOne: true };
      mid.stroke = strokeFor(mid.height, mid.width);
      drawGlyphs(ctx, div(W - glyphTextWidth(ss, mid), 2), 118, ss, mid, accent, bg);
    }

    var font = 'bold 22px ' + FONT;
    ctx.font = font;
    ctx.fillStyle = accent;
    ctx.textBaseline = 'middle';
    ctx.textAlign = 'center';
    ctx.fillText(dateText(ctx, font, W - 8), W / 2, H - 19);
  }

  // Zeichenfläche in voller Auflösung des Handys, Koordinaten wie auf der Uhr
  function canvasContext(id) {
    var canvas = document.getElementById(id);
    var scale = window.devicePixelRatio || 1;
    canvas.width = W * scale;
    canvas.height = H * scale;
    var ctx = canvas.getContext('2d');
    ctx.setTransform(scale, 0, 0, scale, 0, 0);
    return ctx;
  }

  function drawPreview() {
    var face = canvasContext('norl-face');
    drawFace(face, false);
    if (value('METER_MODE') === '1') {
      // gefüllter Teil mit vertauschten Farben, rechts abgeschnitten
      face.save();
      face.beginPath();
      face.rect(0, 0, div(W * MINUTE, 60), H);
      face.clip();
      drawFace(face, true);
      face.restore();
    }
    drawDetail(canvasContext('norl-detail'));
  }

  // Beide Ansichten nebeneinander, beim Blättern bleibt die Vorschau oben stehen
  function setupPreview() {
    var box = document.getElementById('norl-preview');
    box.style.display = 'flex';
    box.style.justifyContent = 'center';
    box.style.gap = '12px';
    [].forEach.call(box.getElementsByTagName('canvas'), function(canvas) {
      canvas.style.width = '42%';
      canvas.style.maxWidth = '200px';
      canvas.style.aspectRatio = W + ' / ' + H;
      canvas.style.borderRadius = '6px';
      canvas.style.boxShadow = '0 0 0 1px rgba(255, 255, 255, 0.25)';
    });
    var note = document.getElementById('norl-preview-note');
    note.style.display = 'block';
    note.style.textAlign = 'center';
    note.style.fontSize = '12px';
    note.style.opacity = '0.6';
    note.style.marginTop = '4px';
    var component = box.parentNode.parentNode;
    component.style.position = '-webkit-sticky';
    component.style.position = 'sticky';
    component.style.top = '0';
    component.style.zIndex = '1';
    component.style.background = window.getComputedStyle(document.body).backgroundColor;

    drawPreview();
    ['BACKGROUND', 'DIGITS', 'ACCENT', 'METER_MODE', 'METER_POSITION', 'HOUR_MODE',
     'BATTERY_SHOW', 'BATTERY',
     'RAIN_MODE', 'RAIN_MIN_PERCENT', 'RAIN_MIN_TENTHS', 'RAIN'].forEach(function(key) {
      clayConfig.getItemByMessageKey(key).on('change', drawPreview);
    });
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    updateBattery();
    clayConfig.getItemByMessageKey('BATTERY_SHOW').on('change', updateBattery);
    updateRain();
    clayConfig.getItemByMessageKey('RAIN_MODE').on('change', updateRain);
    setupPreview();
  });
};
