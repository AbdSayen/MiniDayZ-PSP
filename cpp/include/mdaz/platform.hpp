#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

struct InputState {
  float move_x;
  float move_y;
  bool attack;          // held
  bool attack_pressed;  // rising edge
  bool interact;
  bool interact_pressed;
  bool inventory;
  bool inventory_pressed;
  bool reload;
  bool reload_pressed;
  bool run;
  bool pause;
  bool pause_pressed;
  bool confirm;
  bool confirm_pressed;
  bool cancel;
  bool save_pressed;
  bool load_pressed;
  bool torch_pressed;
  bool drop_pressed;       // G — drop one from equipped slot
  bool mag_reload_pressed; // V — mag/chamber reload stub
  bool throw_pressed;      // F — throw stick / improvised projectile
  bool bag; // hold Tab to show inventory list
  int slot_digit; // 1..9 pressed this frame, else 0
  int equip_delta; // -1 / +1 from Q/X or mouse wheel

  InputState()
    : move_x(0), move_y(0),
      attack(false), attack_pressed(false),
      interact(false), interact_pressed(false),
      inventory(false), inventory_pressed(false),
      reload(false), reload_pressed(false), run(false),
      pause(false), pause_pressed(false),
      confirm(false), confirm_pressed(false),
      cancel(false), save_pressed(false), load_pressed(false),
      torch_pressed(false), drop_pressed(false), mag_reload_pressed(false),
      throw_pressed(false), bag(false), slot_digit(0), equip_delta(0) {}
};

class IPlatform {
public:
  virtual ~IPlatform() {}
  virtual bool init(const char* title, int width, int height) = 0;
  virtual void shutdown() = 0;
  virtual bool poll(InputState& out) = 0;
  virtual double time_seconds() = 0;
  virtual void begin_frame() = 0;
  virtual void clear(float r, float g, float b) = 0;
  virtual void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) = 0;
  virtual void draw_text(float x, float y, const char* text, float r, float g, float b) = 0;
  virtual void end_frame() = 0;
  virtual bool read_file(const char* path, char** out_data, std::size_t* out_size) = 0;
  virtual bool write_file(const char* path, const void* data, std::size_t size) = 0;
  virtual int screen_w() const { return kPspWidth; }
  virtual int screen_h() const { return kPspHeight; }
};

IPlatform* create_platform();

} // namespace mdaz
