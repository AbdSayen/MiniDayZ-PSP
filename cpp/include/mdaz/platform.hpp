#pragma once

#include "mdaz/types.hpp"

namespace mdaz {

struct InputState {
  float move_x;      // -1..1
  float move_y;
  bool attack;
  bool interact;
  bool inventory;
  bool reload;
  bool run;
  bool pause;
  bool confirm;
  bool cancel;

  InputState()
    : move_x(0), move_y(0),
      attack(false), interact(false), inventory(false),
      reload(false), run(false), pause(false),
      confirm(false), cancel(false) {}
};

class IPlatform {
public:
  virtual ~IPlatform() {}
  virtual bool init(const char* title, int width, int height) = 0;
  virtual void shutdown() = 0;
  virtual bool poll(InputState& out) = 0; // false = quit
  virtual double time_seconds() = 0;
  virtual void begin_frame() = 0;
  virtual void clear(float r, float g, float b) = 0;
  virtual void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) = 0;
  virtual void draw_text(float x, float y, const char* text, float r, float g, float b) = 0;
  virtual void end_frame() = 0;
  virtual bool read_file(const char* path, char** out_data, std::size_t* out_size) = 0;
  virtual bool write_file(const char* path, const void* data, std::size_t size) = 0;
};

IPlatform* create_platform(); // desktop SDL or null

} // namespace mdaz
