#include "mdaz/psp_ui.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace mdaz {
namespace ui {

namespace {

bool g_keyboard_hints = false;

// 7x7 PSP face-button symbols (bit 6 = leftmost column).
const unsigned char kCross[7] = {0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41};
const unsigned char kCircle[7] = {0x1C, 0x22, 0x41, 0x41, 0x41, 0x22, 0x1C};
const unsigned char kTriangle[7] = {0x08, 0x08, 0x14, 0x14, 0x22, 0x22, 0x7F};
const unsigned char kSquare[7] = {0x7F, 0x41, 0x41, 0x41, 0x41, 0x41, 0x7F};
const unsigned char kDPad[7] = {0x1C, 0x1C, 0x7F, 0x77, 0x7F, 0x1C, 0x1C};
const unsigned char kAnalog[7] = {0x1C, 0x3E, 0x77, 0x63, 0x77, 0x3E, 0x1C};

void bits7(IPlatform& p, float x, float y, const unsigned char* rows,
           float r, float g, float b) {
  for (int row = 0; row < 7; ++row)
    for (int col = 0; col < 7; ++col)
      if (rows[row] & (0x40 >> col)) p.draw_rect(x + col, y + row, 1.f, 1.f, r, g, b, 1.f);
}

const char* cap_label(Btn b) {
  switch (b) {
  case Btn_L: return "L";
  case Btn_R: return "R";
  case Btn_Start: return "START";
  case Btn_Select: return "SELECT";
  default: return 0;
  }
}

float key_cap(IPlatform& p, float x, float y, const char* label) {
  const float w = static_cast<float>(text_w(label)) + 5.f;
  p.draw_rect(x, y, w, kGlyphSize, 0.82f, 0.8f, 0.72f, 1.f);
  p.draw_rect(x + 1.f, y + 1.f, w - 2.f, kGlyphSize - 2.f, 0.16f, 0.15f, 0.13f, 1.f);
  p.draw_text(x + 3.f, y + 2.f, label, 0.95f, 0.93f, 0.85f);
  return w;
}

} // namespace

void set_keyboard_hints(bool on) { g_keyboard_hints = on; }
bool keyboard_hints() {
#if defined(MDAZ_PSP)
  return false;
#else
  return g_keyboard_hints;
#endif
}

int text_w(const char* s) {
  if (!s) return 0;
  const int n = static_cast<int>(std::strlen(s));
  return n > 0 ? n * 6 - 1 : 0;
}

void text(IPlatform& p, float x, float y, const char* s, float r, float g, float b) {
  if (s) p.draw_text(std::floor(x), std::floor(y), s, r, g, b);
}

void text_center(IPlatform& p, float cx, float y, const char* s, float r, float g, float b) {
  text(p, cx - text_w(s) * 0.5f, y, s, r, g, b);
}

void text_right(IPlatform& p, float rx, float y, const char* s, float r, float g, float b) {
  text(p, rx - text_w(s), y, s, r, g, b);
}

void text_fit(IPlatform& p, float x, float y, float max_w, const char* s,
              float r, float g, float b) {
  if (!s) return;
  const int max_chars = static_cast<int>((max_w + 1.f) / kCharW);
  const int n = static_cast<int>(std::strlen(s));
  if (n <= max_chars) { text(p, x, y, s, r, g, b); return; }
  if (max_chars <= 2) return;
  char buf[128];
  int keep = max_chars - 2;
  if (keep > static_cast<int>(sizeof(buf)) - 3) keep = static_cast<int>(sizeof(buf)) - 3;
  std::memcpy(buf, s, static_cast<size_t>(keep));
  buf[keep] = '.';
  buf[keep + 1] = '.';
  buf[keep + 2] = 0;
  text(p, x, y, buf, r, g, b);
}

int text_wrap(IPlatform& p, float x, float y, float max_w, int max_lines, const char* s,
              float r, float g, float b) {
  if (!s || max_lines <= 0) return 0;
  const int max_chars = static_cast<int>((max_w + 1.f) / kCharW);
  if (max_chars <= 0) return 0;
  int lines = 0;
  const char* cur = s;
  while (*cur && lines < max_lines) {
    while (*cur == ' ') ++cur;
    if (!*cur) break;
    int len = static_cast<int>(std::strlen(cur));
    int take = len;
    if (len > max_chars) {
      take = max_chars;
      int sp = take;
      while (sp > 0 && cur[sp] != ' ') --sp;
      if (sp > 0) take = sp;
    }
    char buf[128];
    if (take > static_cast<int>(sizeof(buf)) - 1) take = static_cast<int>(sizeof(buf)) - 1;
    std::memcpy(buf, cur, static_cast<size_t>(take));
    buf[take] = 0;
    const bool last = (lines == max_lines - 1) && cur[take] != 0;
    if (last) text_fit(p, x, y + lines * kLineH, max_w, cur, r, g, b);
    else text(p, x, y + lines * kLineH, buf, r, g, b);
    cur += take;
    ++lines;
  }
  return lines;
}

void outline(IPlatform& p, float x, float y, float w, float h,
             float r, float g, float b, float a) {
  p.draw_rect(x, y, w, 1.f, r, g, b, a);
  p.draw_rect(x, y + h - 1.f, w, 1.f, r, g, b, a);
  p.draw_rect(x, y + 1.f, 1.f, h - 2.f, r, g, b, a);
  p.draw_rect(x + w - 1.f, y + 1.f, 1.f, h - 2.f, r, g, b, a);
}

void panel(IPlatform& p, float x, float y, float w, float h, float alpha) {
  p.draw_rect(x, y, w, h, 0.05f, 0.045f, 0.035f, alpha);
  outline(p, x, y, w, h, 0.47f, 0.39f, 0.25f, 1.f);
  outline(p, x + 1.f, y + 1.f, w - 2.f, h - 2.f, 0.16f, 0.13f, 0.09f, 1.f);
}

void panel_title(IPlatform& p, float x, float y, float w, const char* title) {
  p.draw_rect(x + 2.f, y + 2.f, w - 4.f, 12.f, 0.3f, 0.24f, 0.15f, 1.f);
  text(p, x + 6.f, y + 4.f, title, 1.f, 0.93f, 0.7f);
}

void focus_frame(IPlatform& p, float x, float y, float w, float h, float t) {
  const float k = 0.75f + 0.25f * std::sin(t * 7.f);
  outline(p, x - 1.f, y - 1.f, w + 2.f, h + 2.f, 1.f * k, 0.86f * k, 0.35f * k, 1.f);
  outline(p, x, y, w, h, 1.f * k, 0.78f * k, 0.25f * k, 1.f);
}

float glyph(IPlatform& p, float x, float y, Btn b) {
  x = std::floor(x);
  y = std::floor(y);
  const char* cap = cap_label(b);
  if (cap) return key_cap(p, x, y, cap);
  const unsigned char* bits = 0;
  float r = 1.f, g = 1.f, bl = 1.f;
  switch (b) {
  case Btn_Cross: bits = kCross; r = 0.55f; g = 0.72f; bl = 1.f; break;
  case Btn_Circle: bits = kCircle; r = 1.f; g = 0.45f; bl = 0.45f; break;
  case Btn_Triangle: bits = kTriangle; r = 0.35f; g = 0.95f; bl = 0.65f; break;
  case Btn_Square: bits = kSquare; r = 1.f; g = 0.55f; bl = 0.85f; break;
  case Btn_DPad: bits = kDPad; r = 0.9f; g = 0.88f; bl = 0.8f; break;
  case Btn_Analog: bits = kAnalog; r = 0.9f; g = 0.88f; bl = 0.8f; break;
  default: return 0.f;
  }
  p.draw_rect(x, y, kGlyphSize, kGlyphSize, 0.6f, 0.58f, 0.52f, 1.f);
  p.draw_rect(x + 1.f, y + 1.f, kGlyphSize - 2.f, kGlyphSize - 2.f, 0.1f, 0.1f, 0.1f, 1.f);
  bits7(p, x + 2.f, y + 2.f, bits, r, g, bl);
  return kGlyphSize;
}

float hint_width(const Hint& h) {
  float w = 0.f;
  if (keyboard_hints() && h.desk_key) {
    w = static_cast<float>(text_w(h.desk_key)) + 5.f;
  } else {
    if (h.mod != Btn_None) {
      const char* c = cap_label(h.mod);
      w += (c ? text_w(c) + 5.f : kGlyphSize) + 7.f;
    }
    const char* c = cap_label(h.btn);
    w += c ? text_w(c) + 5.f : kGlyphSize;
  }
  if (h.label && h.label[0]) w += 3.f + text_w(h.label);
  return w;
}

float hint(IPlatform& p, float x, float y, const Hint& h) {
  const float x0 = x;
  if (keyboard_hints() && h.desk_key) {
    x += key_cap(p, x, y, h.desk_key);
  } else {
    if (h.mod != Btn_None) {
      x += glyph(p, x, y, h.mod);
      text(p, x + 1.f, y + 2.f, "+", 0.9f, 0.88f, 0.8f);
      x += 7.f;
    }
    x += glyph(p, x, y, h.btn);
  }
  if (h.label && h.label[0]) {
    text(p, x + 3.f, y + 2.f, h.label, 0.93f, 0.9f, 0.8f);
    x += 3.f + text_w(h.label);
  }
  return x - x0;
}

void hint_bar(IPlatform& p, const Hint* hints, int n, float alpha) {
  if (!hints || n <= 0) return;
  const float y = kHintBarY;
  p.draw_rect(0.f, y - 2.f, kScreenW, kHintBarH, 0.f, 0.f, 0.f, alpha);
  float x = kSafe;
  const float max_x = kScreenW - kSafe;
  for (int i = 0; i < n; ++i) {
    const float w = hint_width(hints[i]);
    if (x + w > max_x) break;
    hint(p, x, y, hints[i]);
    x += w + 10.f;
  }
}

void hint_row_right(IPlatform& p, float rx, float y, const Hint* hints, int n) {
  float total = 0.f;
  for (int i = 0; i < n; ++i) total += hint_width(hints[i]) + (i ? 8.f : 0.f);
  float x = rx - total;
  for (int i = 0; i < n; ++i) {
    x += hint(p, x, y, hints[i]) + 8.f;
  }
}

void list_menu(IPlatform& p, const ListStyle& st, const char* const* labels, int n,
               int focus, float t, const bool* disabled) {
  for (int i = 0; i < n; ++i) {
    const float y = st.y + i * st.row_h;
    const float h = st.row_h - 4.f;
    const bool f = (i == focus);
    const bool dis = disabled && disabled[i];
    if (st.wood && p.has_sprite("menu_btn_wood-sheet0")) {
      // Frames 160x39: default y=1, green y=42 (focus).
      p.draw_sprite_frame("menu_btn_wood-sheet0", st.x, y, st.w, h, 1, f ? 42 : 1, 160, 39,
                          0.f, dis ? 0.55f : 1.f);
    } else {
      p.draw_rect(st.x, y, st.w, h, f ? 0.3f : 0.12f, f ? 0.25f : 0.1f, f ? 0.14f : 0.08f, 0.95f);
      outline(p, st.x, y, st.w, h, 0.47f, 0.39f, 0.25f, 1.f);
    }
    float cr = 0.95f, cg = 0.92f, cb = 0.82f;
    if (dis) { cr = 0.5f; cg = 0.48f; cb = 0.44f; }
    else if (f) { cr = 1.f; cg = 1.f; cb = 0.75f; }
    text_center(p, st.x + st.w * 0.5f, y + (h - kCharH) * 0.5f, labels[i], cr, cg, cb);
    if (f) focus_frame(p, st.x, y, st.w, h, t);
  }
}

} // namespace ui
} // namespace mdaz
