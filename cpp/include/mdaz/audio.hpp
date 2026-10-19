#pragma once

namespace mdaz {

// C2 Audio plugin (data.js object type t178) semantics, 1:1 with c2runtime.js Acts:
//   PlayByName / Play                -> play()      (panner off)
//   PlayAtObject(ByName)/AtPosition  -> play_at()   (panner on; positions tracked via set_tag_position)
//   Stop / StopAll / SetPaused / SetMuted / SetSilent / SetPlaybackRate / SetMasterVolume / IsTagPlaying.
// name: media file stem as written in data.js (lowercased + ".ogg", like c2runtime PlayByName).
// db:   decibels, dbToLinear() capped to [0, 1] exactly as c2runtime (so +10 / +25 dB play at 1.0).
// tag:  case-insensitive; "" addresses the last played sound (getAudioByTag with empty tag).
class IAudio {
public:
  virtual ~IAudio() {}
  virtual void set_media_dir(const char* /*path*/) {}
  virtual void play(const char* name, bool loop, float db, const char* tag) = 0;
  virtual void play_at(const char* name, bool loop, float db, float x, float y, const char* tag) = 0;
  virtual void stop(const char* tag) = 0;
  virtual void stop_all() = 0;
  // SetPaused: C2 state 0 = pause, 1 = resume -> paused = (state == 0).
  virtual void set_paused(const char* /*tag*/, bool /*paused*/) {}
  // SetMuted: C2 combo 0 = mute -> muted = (state == 0).
  virtual void set_muted(const char* /*tag*/, bool /*muted*/) {}
  // SetSilent: C2 combo 0 = silent, 1 = not silent. While silent, Play* actions are ignored.
  virtual void set_silent(bool /*silent*/) {}
  virtual bool silent() const { return false; }
  virtual void set_playback_rate(const char* /*tag*/, float /*rate*/) {}
  virtual void set_master_db(float /*db*/) {}
  // Object tracker: PlayAtObject sounds follow their object; the port moves them by tag.
  virtual void set_tag_position(const char* /*tag*/, float /*x*/, float /*y*/) {}
  virtual bool is_tag_playing(const char* /*tag*/) const { return false; }
  // SetListenerObject (sid9183183159438440: listener = player_base t216 chain).
  virtual void set_listener(float /*x*/, float /*y*/) {}
};

class NullAudio : public IAudio {
public:
  void play(const char*, bool, float, const char*) {}
  void play_at(const char*, bool, float, float, float, const char*) {}
  void stop(const char*) {}
  void stop_all() {}
};

// c2runtime dbToLinear: 10^(db/20) clamped to [0, 1] (NaN -> 0).
float c2_db_to_linear(float db);

// Desktop: SDL2 output of the shared C2 mixer (src/platform/audio_mixer.cpp). Null without SDL.
IAudio* create_sdl_audio();
// PSP: shared C2 mixer on one sceAudio channel (platform/psp/audio_psp.cpp).
IAudio* create_psp_audio();

} // namespace mdaz
