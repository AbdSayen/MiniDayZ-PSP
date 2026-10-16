#include "mdaz/platform.hpp"

#include <cstdio>
#include <fstream>

namespace mdaz {

class NullPlatform : public IPlatform {
public:
  NullPlatform() : frames_(0) {}

  bool init(const char* title, int width, int height) {
    (void)width;
    (void)height;
    std::printf("[null-platform] init: %s\n", title ? title : "");
    frames_ = 0;
    return true;
  }
  void shutdown() {}
  bool poll(InputState& out) {
    out = InputState();
    ++frames_;
    return frames_ < 3;
  }
  double time_seconds() { return static_cast<double>(frames_) * (1.0 / 60.0); }
  void begin_frame() {}
  void clear(float, float, float) {}
  void draw_rect(float, float, float, float, float, float, float, float) {}
  void draw_text(float, float, const char* text, float, float, float) {
    if (text) std::puts(text);
  }
  void end_frame() {}

  bool read_file(const char* path, char** out_data, std::size_t* out_size) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    in.seekg(0, std::ios::end);
    const std::streamoff len = in.tellg();
    if (len < 0) return false;
    in.seekg(0, std::ios::beg);
    char* buf = new char[static_cast<std::size_t>(len) + 1];
    in.read(buf, len);
    buf[len] = 0;
    *out_data = buf;
    *out_size = static_cast<std::size_t>(len);
    return true;
  }

  bool write_file(const char* path, const void* data, std::size_t size) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    return static_cast<bool>(out);
  }

private:
  int frames_;
};

IPlatform* create_null_platform() {
  return new NullPlatform();
}

} // namespace mdaz
