// Shared C2 Audio mixer — see audio_mixer.hpp / mdaz/audio.hpp.
#include "audio_mixer.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif
#define STB_VORBIS_NO_PUSHDATA_API
#include "../../third_party/stb/stb_vorbis.c"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace mdaz {

float c2_db_to_linear(float db) {
  float v = std::pow(10.f, db / 20.f);
  if (!(v == v) || v < 0.f) v = 0.f;   // !isFinite / NaN -> 0
  if (v > 1.f) v = 1.f;
  return v;
}

namespace {
const float kRef = 600.f, kMaxD = 10000.f, kRolloff = 10.f, kListenerZ = 600.f;

void lower_copy(char* dst, int n, const char* src) {
  int k = 0;
  if (src) for (; src[k] && k < n - 1; ++k) dst[k] = (char)std::tolower((unsigned char)src[k]);
  dst[k] = 0;
}
} // namespace

void C2Mixer::positional_gain(float dx, float dy, float* gl, float* gr) {
  float d = std::sqrt(dx * dx + dy * dy + kListenerZ * kListenerZ);
  if (d < kRef) d = kRef;
  if (d > kMaxD) d = kMaxD;
  const float g = kRef / (kRef + kRolloff * (d - kRef));
  const float p = dx / std::sqrt(dx * dx + kListenerZ * kListenerZ);
  const float a = (p + 1.f) * 0.785398163f;
  *gl = g * std::cos(a);
  *gr = g * std::sin(a);
}

C2Mixer::C2Mixer(unsigned cache_budget_bytes)
  : nclips_(0), tick_(0), used_(0), budget_(cache_budget_bytes), serial_(0),
    last_voice_(-1), last_serial_(0), lx_(0.f), ly_(0.f), master_(1.f), silent_(false), logged_(0) {
  std::memset(voices_, 0, sizeof(voices_));
  std::memset(clips_, 0, sizeof(clips_));
  std::strcpy(dir_, "data/media");
  err_[0] = 0;
}

C2Mixer::~C2Mixer() {
  for (int i = 0; i < kMaxVoices; ++i) if (voices_[i].vs) stb_vorbis_close(voices_[i].vs);
  for (int i = 0; i < nclips_; ++i) { std::free(clips_[i].pcm); std::free(clips_[i].ogg); }
}

void C2Mixer::set_media_dir(const char* path) {
  if (path && path[0]) { std::strncpy(dir_, path, sizeof(dir_) - 1); dir_[sizeof(dir_) - 1] = 0; }
}

void C2Mixer::file_for(const char* name, char* buf, int n) const {
  // c2runtime PlayByName: files_subfolder + filename.toLowerCase() + ".ogg"
  lower_copy(buf, n - 4, name);
  std::strcat(buf, ".ogg");
}

unsigned char* C2Mixer::read_all(const char* file, unsigned& len) {
  char path[320];
  std::snprintf(path, sizeof(path), "%s/%s", dir_, file);
  FILE* f = std::fopen(path, "rb");
  len = 0;
  if (!f) return 0;
  std::fseek(f, 0, SEEK_END);
  const long sz = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  unsigned char* b = sz > 0 ? (unsigned char*)std::malloc((size_t)sz) : 0;
  if (b && std::fread(b, 1, (size_t)sz, f) != (size_t)sz) { std::free(b); b = 0; }
  std::fclose(f);
  if (b) len = (unsigned)sz;
  return b;
}

void C2Mixer::evict_for(unsigned need) {
  while (used_ + need > budget_) {
    int best = -1;
    for (int i = 0; i < nclips_; ++i)
      if (clips_[i].pcm && clips_[i].playing == 0 &&
          (best < 0 || clips_[i].last_use < clips_[best].last_use)) best = i;
    if (best < 0) return;
    used_ -= (unsigned)clips_[best].frames * 2u;
    std::free(clips_[best].pcm); clips_[best].pcm = 0; clips_[best].frames = 0;
  }
}

C2Mixer::Clip* C2Mixer::get_clip(const char* file) {
  Clip* c = 0;
  for (int i = 0; i < nclips_; ++i) if (std::strcmp(clips_[i].file, file) == 0) { c = &clips_[i]; break; }
  if (!c) {
    if (nclips_ >= kMaxClips) return 0;
    lock();
    c = &clips_[nclips_];
    std::memset(c, 0, sizeof(*c));
    std::strncpy(c->file, file, sizeof(c->file) - 1);
    ++nclips_;
    unlock();
  }
  c->last_use = ++tick_;
  if (c->missing) return 0;
  if (c->pcm || c->ogg) return c;
  unsigned len = 0;
  unsigned char* data = read_all(file, len);
  char msg[160];
  if (!data) {
    c->missing = true;
    std::snprintf(err_, sizeof(err_), "missing %s", file);
    std::snprintf(msg, sizeof(msg), "audio: missing %s/%s", dir_, file); log(msg);
    return 0;
  }
  if (len > kStreamOggBytes) {
    int err = 0;
    stb_vorbis* v = stb_vorbis_open_memory(data, (int)len, &err, 0);
    if (!v) { c->missing = true; std::free(data); return 0; }
    c->rate = (int)stb_vorbis_get_info(v).sample_rate;
    stb_vorbis_close(v);
    c->ogg_len = len;
    c->ogg = data;
    if (logged_ < 60) { ++logged_; std::snprintf(msg, sizeof(msg), "audio: stream %s (%u bytes ogg, %d Hz)", file, len, c->rate); log(msg); }
    return c;
  }
  int chn = 0, rate = 0;
  short* pcm = 0;
  const int frames = stb_vorbis_decode_memory(data, (int)len, &chn, &rate, &pcm);
  std::free(data);
  if (frames <= 0 || !pcm || chn <= 0) { std::free(pcm); c->missing = true; return 0; }
  if (chn > 1) {
    for (int i = 0; i < frames; ++i) {
      int s = 0;
      for (int k = 0; k < chn; ++k) s += pcm[i * chn + k];
      pcm[i] = (short)(s / chn);
    }
    short* shrunk = (short*)std::realloc(pcm, (size_t)frames * 2u);
    if (shrunk) pcm = shrunk;
  }
  lock();
  evict_for((unsigned)frames * 2u);
  c->frames = frames; c->rate = rate;
  c->pcm = pcm;
  used_ += (unsigned)frames * 2u;
  unlock();
  if (logged_ < 60) { ++logged_; std::snprintf(msg, sizeof(msg), "audio: decoded %s (%d frames, %d Hz, cache %u KB)", file, frames, rate, used_ / 1024u); log(msg); }
  return c;
}

bool C2Mixer::open_stream(Voice& v) {
  int err = 0;
  v.vs = stb_vorbis_open_memory(v.clip->ogg, (int)v.clip->ogg_len, &err, 0);
  v.sbuf_n = 0;
  v.pos_fp = 0;
  return v.vs != 0;
}

void C2Mixer::start(const char* name, bool loop, float db, bool positioned, float x, float y, const char* tag) {
  if (silent_ || !name || !name[0]) return;     // c2runtime Play*: if (silent) return;
  char file[64];
  file_for(name, file, (int)sizeof(file));
  Clip* c = get_clip(file);
  if (!c) return;
  stb_vorbis* vs = 0;
  bool virt = false;
  if (c->ogg) {
    if (stream_voices() >= kMaxStreams) virt = true;
    else {
      int err = 0;
      vs = stb_vorbis_open_memory(c->ogg, (int)c->ogg_len, &err, 0);
      if (!vs) return;
    }
  }
  lock();
  int vi = -1;
  for (int i = 0; i < kMaxVoices; ++i) if (!voices_[i].active) { vi = i; break; }
  if (vi < 0) {   // steal the oldest non-looping decoded-clip voice
    unsigned best = 0xffffffffu;
    for (int i = 0; i < kMaxVoices; ++i)
      if (!voices_[i].loop && !voices_[i].clip->ogg && voices_[i].serial < best) { best = voices_[i].serial; vi = i; }
    if (vi >= 0) kill(voices_[vi]);
  }
  if (vi < 0) { unlock(); if (vs) stb_vorbis_close(vs); return; }
  Voice& v = voices_[vi];
  v.clip = c; v.vs = vs; v.virt = virt; v.sbuf_n = 0; v.pos_fp = 0;
  v.loop = loop; v.positioned = positioned; v.paused = false; v.muted = false;
  v.vol = c2_db_to_linear(db); v.rate = 1.f; v.x = x; v.y = y;
  lower_copy(v.tag, (int)sizeof(v.tag), tag ? tag : "");
  v.serial = ++serial_;
  ++c->playing;
  v.active = true;
  last_voice_ = vi; last_serial_ = v.serial;
  unlock();
}

void C2Mixer::play(const char* name, bool loop, float db, const char* tag) {
  start(name, loop, db, false, 0.f, 0.f, tag);
}
void C2Mixer::play_at(const char* name, bool loop, float db, float x, float y, const char* tag) {
  start(name, loop, db, true, x, y, tag);
}

void C2Mixer::kill(Voice& v) {
  if (!v.active) return;
  if (v.vs) { stb_vorbis_close(v.vs); v.vs = 0; }
  if (v.clip && v.clip->playing > 0) --v.clip->playing;
  v.active = false;
  v.virt = false;
}

bool C2Mixer::tag_match(const Voice& v, const char* tag) const {
  if (!v.active) return false;
  if (!tag || !tag[0]) {   // getAudioByTag(""): the last audio, if not ended
    return last_voice_ >= 0 && &voices_[last_voice_] == &v && v.serial == last_serial_;
  }
  char t[40];
  lower_copy(t, (int)sizeof(t), tag);
  return std::strcmp(v.tag, t) == 0;
}

void C2Mixer::stop(const char* tag) {
  lock();
  for (int i = 0; i < kMaxVoices; ++i) if (tag_match(voices_[i], tag)) kill(voices_[i]);
  unlock();
}
void C2Mixer::stop_all() {
  lock();
  for (int i = 0; i < kMaxVoices; ++i) kill(voices_[i]);
  unlock();
}
void C2Mixer::set_paused(const char* tag, bool paused) {
  lock();
  for (int i = 0; i < kMaxVoices; ++i) if (tag_match(voices_[i], tag)) voices_[i].paused = paused;
  unlock();
}
void C2Mixer::set_muted(const char* tag, bool muted) {
  lock();
  for (int i = 0; i < kMaxVoices; ++i) if (tag_match(voices_[i], tag)) voices_[i].muted = muted;
  unlock();
}
void C2Mixer::set_silent(bool s) { silent_ = s; }   // instances: doSetMuted(is_muted || is_silent)
void C2Mixer::set_playback_rate(const char* tag, float rate) {
  if (rate < 0.f) rate = 0.f;
  lock();
  for (int i = 0; i < kMaxVoices; ++i) if (tag_match(voices_[i], tag)) voices_[i].rate = rate;
  unlock();
}
void C2Mixer::set_master_db(float db) { master_ = c2_db_to_linear(db); }
void C2Mixer::set_tag_position(const char* tag, float x, float y) {
  lock();
  for (int i = 0; i < kMaxVoices; ++i) if (tag_match(voices_[i], tag)) { voices_[i].x = x; voices_[i].y = y; }
  unlock();
}
bool C2Mixer::is_tag_playing(const char* tag) const {
  bool r = false;
  lock();
  for (int i = 0; i < kMaxVoices && !r; ++i) if (tag_match(voices_[i], tag) && !voices_[i].paused) r = true;
  unlock();
  return r;
}

void C2Mixer::set_listener(float x, float y) {
  lx_ = x; ly_ = y;
  // Realize virtual streams (nearest first) when a decoder slot is free. Main thread: the
  // stb_vorbis decoder allocation stays out of the output thread.
  for (;;) {
    if (stream_voices() >= kMaxStreams) break;
    int best = -1; float bd = 0.f;
    lock();
    for (int i = 0; i < kMaxVoices; ++i) {
      const Voice& v = voices_[i];
      if (!v.active || !v.virt) continue;
      const float d = v.positioned ? (v.x - x) * (v.x - x) + (v.y - y) * (v.y - y) : 0.f;
      if (best < 0 || d < bd) { best = i; bd = d; }
    }
    unlock();
    if (best < 0) break;
    int err = 0;
    stb_vorbis* vs = stb_vorbis_open_memory(voices_[best].clip->ogg, (int)voices_[best].clip->ogg_len, &err, 0);
    lock();
    Voice& v = voices_[best];
    if (v.active && v.virt && vs) { v.vs = vs; v.virt = false; v.sbuf_n = 0; v.pos_fp = 0; vs = 0; }
    unlock();
    if (vs) { stb_vorbis_close(vs); break; }
  }
}

int C2Mixer::active_voices() const { int n = 0; for (int i = 0; i < kMaxVoices; ++i) if (voices_[i].active) ++n; return n; }
int C2Mixer::stream_voices() const { int n = 0; for (int i = 0; i < kMaxVoices; ++i) if (voices_[i].active && voices_[i].vs) ++n; return n; }
int C2Mixer::virtual_voices() const { int n = 0; for (int i = 0; i < kMaxVoices; ++i) if (voices_[i].active && voices_[i].virt) ++n; return n; }

float C2Mixer::voice_gain_for_test(const char* tag, float* left, float* right) const {
  for (int i = 0; i < kMaxVoices; ++i) {
    const Voice& v = voices_[i];
    if (!tag_match(v, tag)) continue;
    float gl = 1.f, gr = 1.f;
    if (v.positioned) positional_gain(v.x - lx_, v.y - ly_, &gl, &gr);
    const float m = (v.muted || silent_) ? 0.f : v.vol * master_;
    if (left) *left = gl * m;
    if (right) *right = gr * m;
    return m;
  }
  if (left) *left = 0.f;
  if (right) *right = 0.f;
  return -1.f;
}

void C2Mixer::mix(short* out, int frames) {
  static int acc[4096 * 2];
  if (frames > 4096) frames = 4096;
  std::memset(acc, 0, sizeof(int) * (size_t)frames * 2u);
  lock();
  for (int i = 0; i < kMaxVoices; ++i) {
    Voice& v = voices_[i];
    if (!v.active || v.virt || v.paused) continue;
    float gl = 1.f, gr = 1.f;
    if (v.positioned) positional_gain(v.x - lx_, v.y - ly_, &gl, &gr);
    const float m = (v.muted || silent_) ? 0.f : v.vol * master_;
    const int vl = (int)(gl * m * 32768.f), vr = (int)(gr * m * 32768.f);
    const unsigned long long step =
        (unsigned long long)((double)v.clip->rate * (double)v.rate / (double)kOutRate * 4294967296.0);
    for (int f = 0; f < frames; ++f) {
      int s;
      if (v.vs) {
        unsigned idx = (unsigned)(v.pos_fp >> 32);
        while ((int)idx >= v.sbuf_n) {
          v.pos_fp -= (unsigned long long)(unsigned)v.sbuf_n << 32;
          short* bp[1] = { v.sbuf };   // non-interleaved API: downmixes to the 1 requested channel
          v.sbuf_n = stb_vorbis_get_samples_short(v.vs, 1, bp, 2048);
          if (v.sbuf_n <= 0) {
            if (v.loop && stb_vorbis_seek_start(v.vs)) {
              v.sbuf_n = stb_vorbis_get_samples_short(v.vs, 1, bp, 2048);
            }
            if (v.sbuf_n <= 0) break;
          }
          idx = (unsigned)(v.pos_fp >> 32);
        }
        if (v.sbuf_n <= 0) { kill(v); break; }
        s = v.sbuf[idx];
      } else {
        unsigned idx = (unsigned)(v.pos_fp >> 32);
        if ((int)idx >= v.clip->frames) {
          if (!v.loop || v.clip->frames <= 0) { kill(v); break; }
          v.pos_fp -= (unsigned long long)(unsigned)v.clip->frames << 32;
          idx = (unsigned)(v.pos_fp >> 32);
          if ((int)idx >= v.clip->frames) { v.pos_fp = 0; idx = 0; }
        }
        s = v.clip->pcm[idx];
      }
      acc[f * 2] += (s * vl) >> 15;
      acc[f * 2 + 1] += (s * vr) >> 15;
      v.pos_fp += step;
    }
  }
  unlock();
  for (int k = 0; k < frames * 2; ++k) {
    int s = acc[k];
    if (s > 32767) s = 32767;
    if (s < -32768) s = -32768;
    out[k] = (short)s;
  }
}

} // namespace mdaz
