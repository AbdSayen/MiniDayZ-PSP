#pragma once
// Shared software mixer implementing the C2 Audio plugin semantics (see mdaz/audio.hpp).
// Platform backends (audio_sdl.cpp, platform/psp/audio_psp.cpp) only provide the output
// thread/callback and the lock; tests drive mix() directly.
#include "mdaz/audio.hpp"

struct stb_vorbis;

namespace mdaz {

class C2Mixer : public IAudio {
public:
  // C2 Audio properties [0,0,0,1,1,600,600,10000,10] (data.js t178): listener Z 600,
  // refDistance 600, maxDistance 10000, rolloff 10, distance model inverse.
  static const int kOutRate = 44100;
  static const int kMaxVoices = 24;
  static const int kMaxStreams = 4;         // concurrently decoded long files (PSP CPU/RAM)
  static const unsigned kStreamOggBytes = 96u * 1024u;
  static const int kMaxClips = 224;

  explicit C2Mixer(unsigned cache_budget_bytes);
  virtual ~C2Mixer();

  // IAudio
  void set_media_dir(const char* path);
  void play(const char* name, bool loop, float db, const char* tag);
  void play_at(const char* name, bool loop, float db, float x, float y, const char* tag);
  void stop(const char* tag);
  void stop_all();
  void set_paused(const char* tag, bool paused);
  void set_muted(const char* tag, bool muted);
  void set_silent(bool s);
  bool silent() const { return silent_; }
  void set_playback_rate(const char* tag, float rate);
  void set_master_db(float db);
  void set_tag_position(const char* tag, float x, float y);
  bool is_tag_playing(const char* tag) const;
  void set_listener(float x, float y);   // main thread, once per frame (also realizes virtual streams)

  // Output: interleaved stereo s16 at kOutRate. Called from the output thread/callback.
  void mix(short* out, int frames);

  // Introspection (tests / boot log).
  int active_voices() const;
  int stream_voices() const;
  int virtual_voices() const;
  unsigned cache_bytes() const { return used_; }
  float voice_gain_for_test(const char* tag, float* left, float* right) const;
  const char* last_error() const { return err_; }

  // Positioned gain/pan (WebAudio PannerNode inverse model + equal-power pan stand-in for HRTF).
  static void positional_gain(float dx, float dy, float* gl, float* gr);

protected:
  virtual void lock() const {}
  virtual void unlock() const {}
  virtual void log(const char* /*msg*/) {}

private:
  struct Clip {
    char file[48];
    short* pcm; int frames, rate;      // decoded mono s16
    unsigned char* ogg; unsigned ogg_len; // streamed file bytes
    bool missing; unsigned last_use; int playing;
  };
  struct Voice {
    bool active, loop, positioned, paused, muted;
    Clip* clip;
    stb_vorbis* vs;       // stream decoder; null for decoded clips or virtual streams
    bool virt;            // stream waiting for a decoder slot (silent, keeps its tag)
    short sbuf[2048]; int sbuf_n;
    unsigned long long pos_fp;   // 32.32 in source frames
    float vol, rate, x, y;
    char tag[40];
    unsigned serial;
  };

  Clip* get_clip(const char* file);
  void start(const char* name, bool loop, float db, bool positioned, float x, float y, const char* tag);
  void kill(Voice& v);
  bool tag_match(const Voice& v, const char* tag) const;
  bool open_stream(Voice& v);
  unsigned char* read_all(const char* file, unsigned& len);
  void evict_for(unsigned need);
  void file_for(const char* name, char* buf, int n) const;

  Voice voices_[kMaxVoices];
  Clip clips_[kMaxClips];
  int nclips_;
  unsigned tick_, used_, budget_, serial_;
  int last_voice_; unsigned last_serial_;
  float lx_, ly_, master_;
  bool silent_;
  int logged_;
  char dir_[256];
  char err_[96];
};

} // namespace mdaz
