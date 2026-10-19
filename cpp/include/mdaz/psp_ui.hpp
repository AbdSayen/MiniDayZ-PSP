#pragma once
// PSP-first UI toolkit (logical 480x272). Shared by desktop and PSP builds.
// Font: 5x7 bitmap, 6 px advance, 1 logical px per font pixel on both platforms.
// No mouse/touch: every widget is driven by focus + PSP buttons (see BUILDING.md).

#include "mdaz/platform.hpp"

namespace mdaz {
namespace ui {

static const float kScreenW = 480.f;
static const float kScreenH = 272.f;
static const float kSafe = 6.f;        // safe margin on all sides
static const float kCharW = 6.f;       // glyph advance
static const float kCharH = 7.f;       // glyph height
static const float kLineH = 10.f;      // line pitch
static const float kGlyphSize = 11.f;  // PSP button glyph box
static const float kHintBarH = 15.f;   // bottom hint strip height
static const float kHintBarY = kScreenH - kSafe - kHintBarH + 2.f;

enum Btn {
  Btn_None = 0,
  Btn_Cross,
  Btn_Circle,
  Btn_Triangle,
  Btn_Square,
  Btn_L,
  Btn_R,
  Btn_Start,
  Btn_Select,
  Btn_DPad,
  Btn_Analog
};

// One on-screen hint: [mod +] btn  label.  desk_key is shown instead when
// keyboard hints are enabled on desktop (--keys), e.g. "F" for R+Cross (throw).
struct Hint {
  Btn mod;
  Btn btn;
  const char* desk_key;
  const char* label;
};

// Desktop may opt into keyboard-cap hints; PSP always shows PSP glyphs.
void set_keyboard_hints(bool on);
bool keyboard_hints();

int text_w(const char* s);
void text(IPlatform& p, float x, float y, const char* s, float r, float g, float b);
void text_center(IPlatform& p, float cx, float y, const char* s, float r, float g, float b);
void text_right(IPlatform& p, float rx, float y, const char* s, float r, float g, float b);
// Truncates with ".." so the string never exceeds max_w pixels.
void text_fit(IPlatform& p, float x, float y, float max_w, const char* s,
              float r, float g, float b);
// Word-wrap into at most max_lines lines; returns number of lines drawn.
int text_wrap(IPlatform& p, float x, float y, float max_w, int max_lines, const char* s,
              float r, float g, float b);

void outline(IPlatform& p, float x, float y, float w, float h,
             float r, float g, float b, float a);
// Dark C2-style panel (brown border, near-black fill).
void panel(IPlatform& p, float x, float y, float w, float h, float alpha = 0.92f);
// Title strip on top of a panel.
void panel_title(IPlatform& p, float x, float y, float w, const char* title);
// Pulsing focus frame (t = any running clock).
void focus_frame(IPlatform& p, float x, float y, float w, float h, float t);

// Draw one button glyph; returns its width.
float glyph(IPlatform& p, float x, float y, Btn b);
// Draw a full hint (mod + btn + label); returns total width.
float hint(IPlatform& p, float x, float y, const Hint& h);
float hint_width(const Hint& h);
// Bottom strip with hints laid out left->right inside the safe area.
void hint_bar(IPlatform& p, const Hint* hints, int n, float alpha = 0.78f);
// Row of hints right-aligned at (rx, y), no background.
void hint_row_right(IPlatform& p, float rx, float y, const Hint* hints, int n);

// Simple vertical list menu with focus. Returns nothing; caller owns index.
struct ListStyle {
  float x, y, w, row_h;
  bool wood; // use menu_btn_wood-sheet0 rows (main menu look)
};
void list_menu(IPlatform& p, const ListStyle& st, const char* const* labels, int n,
               int focus, float t, const bool* disabled = 0);

} // namespace ui
} // namespace mdaz
