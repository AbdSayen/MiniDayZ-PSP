#include "mdaz/audio.hpp"

#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#if defined(MDAZ_HAS_SDL2) && MDAZ_HAS_SDL2
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

namespace mdaz {

namespace {

struct Clip {
  std::vector<float> samples; // mono float -1..1
};

Clip make_noise_burst(float seconds, float pitch_hz, float volume) {
  Clip c;
  const int rate = 22050;
  const int n = static_cast<int>(seconds * rate);
  c.samples.resize(static_cast<std::size_t>(n));
  unsigned seed = 12345u + static_cast<unsigned>(pitch_hz * 10.f);
  for (int i = 0; i < n; ++i) {
    seed = seed * 1103515245u + 12345u;
    const float noise = (static_cast<float>(seed & 0xFFFF) / 65535.f) * 2.f - 1.f;
    const float t = static_cast<float>(i) / static_cast<float>(rate);
    const float env = 1.f - t / seconds;
    const float tone = std::sin(6.2831853f * pitch_hz * t);
    float s = (noise * 0.65f + tone * 0.35f) * env * volume;
    if (s > 1.f) s = 1.f;
    if (s < -1.f) s = -1.f;
    c.samples[static_cast<std::size_t>(i)] = s;
  }
  return c;
}

Clip make_thud(float seconds, float volume) {
  Clip c;
  const int rate = 22050;
  const int n = static_cast<int>(seconds * rate);
  c.samples.resize(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(rate);
    const float env = std::exp(-t * 12.f);
    float s = std::sin(6.2831853f * 90.f * t) * env * volume;
    c.samples[static_cast<std::size_t>(i)] = s;
  }
  return c;
}

} // namespace

class SdlAudio : public IAudio {
public:
  SdlAudio() : device_(0), muted_(false) {
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
      if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        muted_ = true;
        return;
      }
    }
    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = 22050;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = 0;
    device_ = SDL_OpenAudioDevice(0, 0, &want, &have_, 0);
    if (!device_) {
      muted_ = true;
      return;
    }
    clips_["gunshot"] = make_noise_burst(0.12f, 180.f, 0.55f);
    clips_["melee_swing"] = make_noise_burst(0.08f, 420.f, 0.35f);
    clips_["hit"] = make_thud(0.15f, 0.5f);
    clips_["pickup"] = make_noise_burst(0.05f, 880.f, 0.25f);
    clips_["craft"] = make_noise_burst(0.1f, 660.f, 0.3f);
    clips_["bunker"] = make_thud(0.25f, 0.4f);
    clips_["death"] = make_thud(0.5f, 0.6f);
    clips_["airdrop"] = make_noise_burst(0.35f, 95.f, 0.45f);
    clips_["howl"] = make_noise_burst(0.55f, 140.f, 0.4f);
    clips_["cough"] = make_noise_burst(0.18f, 220.f, 0.3f);
    clips_["rain"] = make_noise_burst(0.4f, 60.f, 0.22f);
    clips_["thunder"] = make_thud(0.7f, 0.65f);
    clips_["click"] = make_noise_burst(0.04f, 1200.f, 0.2f);
    clips_["wind"] = make_noise_burst(0.5f, 80.f, 0.18f);
    SDL_PauseAudioDevice(device_, 0);
  }

  ~SdlAudio() {
    if (device_) SDL_CloseAudioDevice(device_);
  }

  void play(const char* name, float volume) {
    play_at(name, 0.f, 0.f, volume);
  }

  void play_at(const char* name, float /*x*/, float /*y*/, float volume) {
    if (muted_ || !device_ || !name) return;
    std::map<std::string, Clip>::iterator it = clips_.find(name);
    if (it == clips_.end()) return;
    const Clip& c = it->second;
    if (c.samples.empty()) return;
    if (volume < 0.f) volume = 0.f;
    if (volume > 1.f) volume = 1.f;
    if (volume >= 0.99f) {
      SDL_QueueAudio(device_, &c.samples[0],
                     static_cast<Uint32>(c.samples.size() * sizeof(float)));
      return;
    }
    // Scaled copy for quieter plays
    std::vector<float> scaled(c.samples.size());
    for (std::size_t i = 0; i < c.samples.size(); ++i) {
      scaled[i] = c.samples[i] * volume;
    }
    SDL_QueueAudio(device_, &scaled[0],
                   static_cast<Uint32>(scaled.size() * sizeof(float)));
  }

  void stop_tag(const char*) {
    if (device_) SDL_ClearQueuedAudio(device_);
  }

private:
  SDL_AudioDeviceID device_;
  SDL_AudioSpec have_;
  bool muted_;
  std::map<std::string, Clip> clips_;
};

IAudio* create_sdl_audio() { return new SdlAudio(); }

} // namespace mdaz

#else

namespace mdaz {
IAudio* create_sdl_audio() { return new NullAudio(); }
}

#endif
