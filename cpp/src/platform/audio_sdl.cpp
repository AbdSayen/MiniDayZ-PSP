// Desktop audio: SDL2 callback driving the shared C2 mixer (audio_mixer.cpp).
#include "mdaz/audio.hpp"
#include "audio_mixer.hpp"

#include <cstdio>

#if defined(MDAZ_HAS_SDL2) && MDAZ_HAS_SDL2
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

namespace mdaz {

class SdlAudio : public C2Mixer {
public:
  SdlAudio() : C2Mixer(64u * 1024u * 1024u), device_(0) {
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0 && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return;
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = kOutRate;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = &SdlAudio::callback;
    want.userdata = this;
    device_ = SDL_OpenAudioDevice(0, 0, &want, &have, 0);
    if (device_) SDL_PauseAudioDevice(device_, 0);
  }
  ~SdlAudio() { if (device_) SDL_CloseAudioDevice(device_); }

protected:
  void lock() const { if (device_ && !in_cb_) SDL_LockAudioDevice(device_); }
  void unlock() const { if (device_ && !in_cb_) SDL_UnlockAudioDevice(device_); }
  void log(const char* msg) { std::fprintf(stderr, "[mdaz] %s\n", msg); }

private:
  static void SDLCALL callback(void* ud, Uint8* stream, int len) {
    SdlAudio* self = static_cast<SdlAudio*>(ud);
    // Called with the device lock held by SDL: mix without re-locking.
    self->in_cb_ = true;
    self->mix(reinterpret_cast<short*>(stream), len / 4);
    self->in_cb_ = false;
  }
  SDL_AudioDeviceID device_;
  volatile bool in_cb_ = false;
};

IAudio* create_sdl_audio() { return new SdlAudio(); }

} // namespace mdaz

#else

namespace mdaz {
IAudio* create_sdl_audio() { return new NullAudio(); }
}

#endif
