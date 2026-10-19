#pragma once

namespace mdaz {

// Audio scaffold — original uses Construct Audio plugin with named clips.
// Desktop/PSP backends will implement; null is silent.
class IAudio {
public:
  virtual ~IAudio() {}
  // Optional: load original Construct media/*.ogg (no-op on NullAudio).
  virtual void set_media_dir(const char* /*path*/) {}
  virtual void play(const char* name, float volume = 1.f) = 0;
  virtual void play_at(const char* name, float x, float y, float volume = 1.f) = 0;
  virtual void stop_tag(const char* tag) = 0;
  // C2 Audio listener (positioned sounds); the port keeps it on the player.
  virtual void set_listener(float /*x*/, float /*y*/) {}
};

class NullAudio : public IAudio {
public:
  void set_media_dir(const char*) {}
  void play(const char*, float) {}
  void play_at(const char*, float, float, float) {}
  void stop_tag(const char*) {}
};

// Desktop: procedural SDL audio. Null when SDL unavailable.
IAudio* create_sdl_audio();
// PSP: original media/*.ogg mixed on one sceAudio channel (platform/psp/audio_psp.cpp).
IAudio* create_psp_audio();

} // namespace mdaz
