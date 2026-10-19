// C2 Audio semantics (c2runtime Audio plugin) on the shared mixer + data.js sound tables.
#include "../src/platform/audio_mixer.hpp"
#include "mdaz/c2_sfx.hpp"
#include "mdaz/rng.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
int g_fail = 0;
void check(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "OK  " : "FAIL", what);
  if (!ok) ++g_fail;
}
struct TestMixer : mdaz::C2Mixer { TestMixer() : mdaz::C2Mixer(16u * 1024u * 1024u) {} };
float peak(TestMixer& m, int frames) {
  static short buf[2 * 4096];
  float p = 0.f;
  while (frames > 0) {
    const int n = frames > 4096 ? 4096 : frames;
    m.mix(buf, n);
    for (int i = 0; i < 2 * n; ++i) { const float a = std::fabs((float)buf[i]); if (a > p) p = a; }
    frames -= n;
  }
  return p;
}
} // namespace

int test_audio() {
  g_fail = 0;
  // dbToLinear = 10^(db/20) clamped to [0,1]
  check(std::fabs(mdaz::c2_db_to_linear(0.f) - 1.f) < 1e-5f, "dB 0 -> 1");
  check(std::fabs(mdaz::c2_db_to_linear(-10.f) - 0.316227766f) < 1e-4f, "dB -10 -> 0.316");
  check(mdaz::c2_db_to_linear(25.f) == 1.f, "dB +25 clamps to 1 (c2runtime)");
  // inverse distance model: at the listener (dx=dy=0, listener Z 600 = ref) full gain.
  { float l, r; mdaz::C2Mixer::positional_gain(0.f, 0.f, &l, &r);
    check(std::fabs(l * l + r * r - 1.f) < 1e-3f, "positional gain at listener = 1 (equal power)");
    float l2, r2; mdaz::C2Mixer::positional_gain(3000.f, 0.f, &l2, &r2);
    check(r2 > l2 && (l2 * l2 + r2 * r2) < 1.f, "positional: right pan + attenuation"); }

  // Tables (data.js)
  mdaz::Rng rng(3);
  { const mdaz::C2Sfx s = mdaz::c2_bandit_fire_sfx(8, rng);
    check(s.name && !std::strcmp(s.name, "akm_5") && s.db == 25.f && !std::strcmp(s.tag, "akm"), "bandit IV11 8 akm_5 +25 [akm]"); }
  { const mdaz::C2Sfx s = mdaz::c2_bandit_fire_sfx(4, rng);
    check(s.name && !std::strncmp(s.name, "mosin_", 6), "bandit IV11 4 mosin"); }
  { const mdaz::C2Sfx s = mdaz::c2_friend_fire_sfx(2, rng);
    check(s.name && !std::strcmp(s.name, "magnum_single_1") && !std::strcmp(s.tag, "357"), "friend IV14 2 magnum [357]"); }
  { const mdaz::C2Sfx s = mdaz::c2_item_activate_sfx(8);
    check(s.name && !std::strcmp(s.name, "DrinkSoda_0") && s.db == -10.f, "item_activate 8 DrinkSoda_0 -10"); }
  check(!std::strcmp(mdaz::c2_day_loop_file(0, false), "MD_winter_amb"), "Day&Night CurrentLevel 0 -> winter (literal C2)");
  check(!std::strncmp(mdaz::c2_footstep_sfx(1, rng), "forest_run", 10), "footstep forest");

  // Mixer semantics with real media (skipped when the media dir is absent).
  const char* dir = std::getenv("MDAZ_MEDIA_DIR");
  if (!dir) dir = "C:/Users/abdur/Desktop/MiniDayZ/original_docs/media";
  TestMixer m;
  m.set_media_dir(dir);
  m.set_listener(0.f, 0.f);
  m.play("pick1", false, 0.f, "");
  if (m.active_voices() == 0) {
    std::printf("SKIP mixer media tests (no media at %s)\n", dir);
    return g_fail;
  }
  check(peak(m, 44100 * 3) > 0.f && m.active_voices() == 0, "one-shot plays then ends");
  m.play("rain_loop", true, -5.f, "Rain_Loop");
  check(m.is_tag_playing("rain_loop"), "tags case-insensitive");
  peak(m, 44100 * 30);
  m.set_listener(0.f, 0.f);
  check(m.is_tag_playing("rain_loop"), "streamed loop still playing after 30 s (loops)");
  m.set_paused("rain_loop", true);
  check(!m.is_tag_playing("rain_loop"), "IsTagPlaying ignores paused");
  check(peak(m, 4096) == 0.f, "paused voice is silent");
  m.set_paused("rain_loop", false);
  check(m.is_tag_playing("rain_loop") && peak(m, 8192) > 0.f, "resume");
  m.stop("RAIN_LOOP");
  check(!m.is_tag_playing("rain_loop"), "stop by tag");
  m.play("pick1", true, 0.f, "a");
  m.play("pick2", true, 0.f, "b");
  m.stop_all();
  check(m.active_voices() == 0, "StopAll");
  m.set_silent(true);
  m.play("pick1", false, 0.f, "s");
  check(m.active_voices() == 0, "Play is ignored while silent");
  m.set_silent(false);
  m.play("pick1", false, -10.f, "g");
  float l = 0.f, r = 0.f;
  m.voice_gain_for_test("g", &l, &r);
  check(std::fabs(l - 0.316227766f) < 1e-3f, "non-positioned voice volume = dbToLinear");
  m.play("", false, 0.f, "");   // missing file: no crash
  return g_fail;
}
