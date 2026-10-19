#include "mdaz/audio.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#if defined(MDAZ_HAS_SDL2) && MDAZ_HAS_SDL2
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>

// Single-file OGG decoder (original_docs/media/*.ogg)
#include "../../third_party/stb/stb_vorbis.c"

namespace mdaz {

namespace {

struct Clip {
  std::vector<float> samples; // mono float -1..1 @ device rate
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

Clip load_ogg_mono(const char* path, int out_rate) {
  Clip c;
  int channels = 0, rate = 0;
  short* pcm = 0;
  const int nframes = stb_vorbis_decode_filename(path, &channels, &rate, &pcm);
  if (nframes <= 0 || !pcm || channels <= 0 || rate <= 0) {
    if (pcm) free(pcm);
    return c;
  }
  // Mix to mono float then naive resample to out_rate
  std::vector<float> mono(static_cast<std::size_t>(nframes));
  for (int i = 0; i < nframes; ++i) {
    int sum = 0;
    for (int ch = 0; ch < channels; ++ch)
      sum += pcm[i * channels + ch];
    mono[static_cast<std::size_t>(i)] =
      static_cast<float>(sum) / static_cast<float>(channels) / 32768.f;
  }
  free(pcm);
  if (rate == out_rate) {
    c.samples.swap(mono);
    return c;
  }
  const double ratio = static_cast<double>(rate) / static_cast<double>(out_rate);
  const int out_n = static_cast<int>(nframes / ratio);
  if (out_n <= 0) return c;
  c.samples.resize(static_cast<std::size_t>(out_n));
  for (int i = 0; i < out_n; ++i) {
    const double src = static_cast<double>(i) * ratio;
    const int i0 = static_cast<int>(src);
    const int i1 = i0 + 1 < nframes ? i0 + 1 : i0;
    const float t = static_cast<float>(src - i0);
    c.samples[static_cast<std::size_t>(i)] =
      mono[static_cast<std::size_t>(i0)] * (1.f - t) +
      mono[static_cast<std::size_t>(i1)] * t;
  }
  return c;
}

} // namespace

class SdlAudio : public IAudio {
public:
  SdlAudio() : device_(0), muted_(false), out_rate_(22050), media_loaded_(false) {
    media_dir_[0] = 0;
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
    out_rate_ = have_.freq > 0 ? have_.freq : 22050;
    // Procedural fallbacks until media dir is set
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

  void set_media_dir(const char* path) {
    if (!path || !path[0]) return;
    if (media_dir_[0] && std::strcmp(media_dir_, path) == 0) return;
    // Only bind once (first successful path wins)
    if (media_loaded_) return;
    std::strncpy(media_dir_, path, sizeof(media_dir_) - 1);
    media_dir_[sizeof(media_dir_) - 1] = 0;
    // Map game tags → original Construct media/*.ogg
    load_replace("gunshot", "akm_1.ogg");
    load_replace("gunshot_silent", "ak_silent_1.ogg");
    load_replace("gun_akm", "akm_1.ogg");
    load_replace("gun_ak74", "ak74_1.ogg");
    load_replace("gun_colt", "colt_1.ogg");
    load_replace("gun_fnx", "fnx_1.ogg");
    load_replace("gun_bow", "bow_1.ogg");
    load_replace("gun_shotgun", "izh_1.ogg");
    load_replace("gun_mosin", "mosin_1.ogg");
    load_replace("gun_m4", "m4_single_1.ogg");
    load_replace("m4_single_1", "m4_single_1.ogg");
    load_replace("m4_single_2", "m4_single_2.ogg");
    load_replace("fn_fal_single_1", "fn_fal_single_1.ogg");
    load_replace("fn_fal_single_2", "fn_fal_single_2.ogg");
    load_replace("gun_glock", "glock19_single_1.ogg");
    load_replace("glock19_single_1", "glock19_single_1.ogg");
    load_replace("glock19_single_2", "glock19_single_2.ogg");
    load_replace("gun_mp5", "mp5k_single_0.ogg");
    load_replace("mp5k_single_0", "mp5k_single_0.ogg");
    load_replace("mp5k_single_1", "mp5k_single_1.ogg");
    load_replace("gun_svd", "svd_single_0.ogg");
    load_replace("svd_single_0", "svd_single_0.ogg");
    load_replace("svd_single_1", "svd_single_1.ogg");
    load_replace("gun_saiga", "saiga12.ogg");
    load_replace("saiga12", "saiga12.ogg");
    load_replace("akm_3", "akm_1.ogg");
    load_replace("akm_5", "akm_1.ogg");
    load_replace("Ak74_1", "ak74_1.ogg");
    load_replace("Ak74_3", "ak74_1.ogg");
    load_replace("FNX_1", "fnx_1.ogg");
    load_replace("FNX_2", "fnx_1.ogg");
    load_replace("colt_1", "colt_1.ogg");
    load_replace("colt_2", "colt_1.ogg");
    load_replace("Bow_1", "bow_1.ogg");
    load_replace("izh_1", "izh_1.ogg");
    load_replace("izh_2", "izh_1.ogg");
    load_replace("mosin_1", "mosin_1.ogg");
    load_replace("mosin_2", "mosin_2.ogg");
    load_replace("mac10_1", "mac10_1.ogg");
    load_replace("mac10_2", "mac10_2.ogg");
    load_replace("pm_shot", "pm_shot.ogg");
    load_replace("pb_shot_1", "pb_shot_1.ogg");
    load_replace("pb_shot_2", "pb_shot_2.ogg");
    load_replace("groza_shot_1", "groza_shot_1.ogg");
    load_replace("groza_shot_2", "groza_shot_2.ogg");
    load_replace("vss_shot_3", "vss_shot_3.ogg");
    load_replace("madsen_shot", "madsen_shot.ogg");
    load_replace("bizon_shot_1", "bizon_shot_1.ogg");
    load_replace("attack_8", "attack_8.ogg");
    load_replace("attack_10", "attack_10.ogg");
    load_replace("attack_11", "attack_11.ogg");
    load_replace("attack_12", "attack_12.ogg");
    load_replace("attack_13", "attack_13.ogg");
    load_replace("attack_9", "attack_9.ogg");
    load_replace("melee_swing", "attack_0.ogg");
    load_replace("attack_0", "attack_0.ogg");
    load_replace("attack_1", "attack_1.ogg");
    // Keep procedural fallback for silent if file missing
    if (clips_.find("gunshot_silent") == clips_.end() || clips_["gunshot_silent"].samples.empty())
      clips_["gunshot_silent"] = make_noise_burst(0.1f, 220.f, 0.25f);
    load_replace("hit", "body_1.ogg");
    load_replace("body_1", "body_1.ogg");
    load_replace("body_2", "body_2.ogg");
    load_replace("pickup", "gearpick.ogg");
    load_replace("craft", "crafting.ogg");
    load_replace("bunker", "bunker_door.ogg");
    load_replace("death", "tutor_death.ogg");
    load_replace("airdrop", "drop1.ogg");
    load_replace("howl", "wolf_spot.ogg");
    load_replace("wolf_attack", "wolf_attack.ogg");
    load_replace("wolf_attack_2", "wolf_attack_2.ogg");
    load_replace("wolf_attack_3", "wolf_attack_3.ogg");
    load_replace("cough", "empty_click.ogg");
    load_replace("rain", "rain_loop.ogg");
    load_replace("thunder", "car_crash_1.ogg");
    load_replace("car_crash_1", "car_crash_1.ogg");
    load_replace("car_crash_2", "car_crash_2.ogg");
    load_replace("car_crash_3", "car_crash_3.ogg");
    load_replace("click", "empty_click.ogg");
    load_replace("wind", "blizzard.ogg");
    load_replace("ambient", "ambient.ogg");
    load_replace("fire", "fireplace_loop.ogg");
    load_replace("eat", "eatingsoft_0.ogg");
    load_replace("drink", "drinksoda_0.ogg");
    load_replace("reload", "fnx45_reload.ogg");
    load_replace("reload_pistol", "reload_pistol.ogg");
    load_replace("reload_rifle", "reload_rifle.ogg");
    load_replace("Reload_rifle", "reload_rifle.ogg");
    load_replace("explode", "f1_explode.ogg");
    load_replace("bush", "bushsearch.ogg");
    load_replace("step_forest", "forest_run_1.ogg");
    load_replace("step_sand", "sand_run_1.ogg");
    load_replace("step_concrete", "concrete_run_1.ogg");
    load_replace("step_run", "run_1.ogg");
    load_replace("crow", "crow_1.ogg");
    load_replace("deer", "deerrun.ogg");
    load_replace("flare_throw", "flare_throw.ogg");
    load_replace("Whiskey", "whiskey.ogg");
    load_replace("DrinkSoda_0", "drinksoda_0.ogg");
    load_replace("SKS_close_0", "sks_close_0.ogg");
    load_replace("repeater", "repeater.ogg");
    load_replace("sporter_22", "sporter_22.ogg");
    load_replace("l85", "l85.ogg");
    load_replace("ak74_2", "ak74_2.ogg");
    load_replace("ak74_3", "ak74_3.ogg");
    load_replace("akm_2", "akm_2.ogg");
    load_replace("akm_4", "akm_4.ogg");
    load_replace("mosin_3", "mosin_3.ogg");
    load_replace("izh_3", "izh_3.ogg");
    load_replace("remington_1", "remington_1.ogg");
    load_replace("remington_2", "remington_2.ogg");
    load_replace("ump45_single_1", "ump45_single_1.ogg");
    load_replace("ump45_single_2", "ump45_single_2.ogg");
    load_replace("m4_silent_1", "m4_silent_1.ogg");
    load_replace("m4_silent_2", "m4_silent_2.ogg");
    load_replace("magnum_single_1", "magnum_single_1.ogg");
    load_replace("steyraug_shot_0", "steyraug_shot_0.ogg");
    load_replace("steyraug_shot_1", "steyraug_shot_1.ogg");
    load_replace("sv98_shot_1", "sv98_shot_1.ogg");
    load_replace("sv98_shot_2", "sv98_shot_2.ogg");
    load_replace("wolf_spot_2", "wolf_spot_2.ogg");
    load_replace("zed_tank_1", "zed_tank_1.ogg");
    load_replace("zed_tank_2", "zed_tank_2.ogg");
    load_replace("zed_tank_3", "zed_tank_3.ogg");
    load_replace("zed_tank_scream", "zed_tank_scream.ogg");
    load_replace("spotted_0", "spotted_0.ogg");
    load_replace("spotted_1", "spotted_1.ogg");
    load_replace("spotted_2", "spotted_2.ogg");
    load_replace("spotted_3", "spotted_3.ogg");
    load_replace("spotted_4", "spotted_4.ogg");
    load_replace("spotted_5", "spotted_5.ogg");
    load_replace("spotted_6", "spotted_6.ogg");
    load_replace("rainroof", "rainroof.ogg");
    load_replace("night_loop", "night_loop.ogg");
    load_replace("bandage", "bandage.ogg");
    load_replace("vitamins", "vitamins.ogg");
    load_replace("hard_ground_1", "hard_ground_1.ogg");
    load_replace("hard_ground_2", "hard_ground_2.ogg");
    load_replace("md_achievment unlocked", "md_achievment unlocked.ogg");
    media_loaded_ = true;
    std::fprintf(stderr, "[mdaz] media dir: %s\n", media_dir_);
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
    std::vector<float> scaled(c.samples.size());
    for (std::size_t i = 0; i < c.samples.size(); ++i)
      scaled[i] = c.samples[i] * volume;
    SDL_QueueAudio(device_, &scaled[0],
                   static_cast<Uint32>(scaled.size() * sizeof(float)));
  }

  void stop_tag(const char*) {
    if (device_) SDL_ClearQueuedAudio(device_);
  }

private:
  void load_replace(const char* tag, const char* file) {
    if (!media_dir_[0] || !tag || !file) return;
    char path[768];
    std::snprintf(path, sizeof(path), "%s/%s", media_dir_, file);
    Clip c = load_ogg_mono(path, out_rate_);
    if (!c.samples.empty()) {
      clips_[tag] = c;
      std::fprintf(stderr, "[mdaz] audio OK: %s <- %s (%u samples)\n",
                   tag, file, static_cast<unsigned>(c.samples.size()));
    }
  }

  SDL_AudioDeviceID device_;
  SDL_AudioSpec have_;
  bool muted_;
  int out_rate_;
  bool media_loaded_;
  char media_dir_[512];
  std::map<std::string, Clip> clips_;
};

IAudio* create_sdl_audio() { return new SdlAudio(); }

} // namespace mdaz

#else

namespace mdaz {
IAudio* create_sdl_audio() { return new NullAudio(); }
}

#endif
