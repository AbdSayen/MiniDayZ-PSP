// PSP audio backend: original media/*.ogg (data/media), software mixer on one stereo
// sceAudio channel at 44100 Hz.
// C2 Audio plugin (data.js type t178, properties [0,0,0,1,1,600,600,10000,10]):
//   timescale off, panning HRTF, distance model inverse, listener Z 600 (above the layout
//   plane), refDistance 600, maxDistance 10000, rolloffFactor 10.
//   Positioned sounds: WebAudio PannerNode inverse model
//     gain = ref / (ref + rolloff * (clamp(d, ref, max) - ref)), d = |listener - source| in 3D.
//   HRTF panning is approximated with equal-power panning by azimuth (KNOWN_ISSUES).
//   Volume: dbToLinear capped to [0, 1] (the port passes linear gains).
#include "mdaz/audio.hpp"

#if defined(MDAZ_PSP) && defined(__PSP__)
#include <pspkernel.h>
#include <pspaudio.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#define STB_VORBIS_NO_PUSHDATA_API
#include "../../third_party/stb/stb_vorbis.c"

extern "C" void mdaz_boot_log(const char* fmt, ...);

namespace mdaz {
namespace {

struct TagFile { const char* tag; const char* file; };
static const TagFile kTagFiles[] = {
#include "../../src/platform/audio_names.inc"
};

const int kOutRate = 44100;
const int kFrames = 1024;                 // per sceAudioOutputBlocking
const int kMaxVoices = 16;
const int kMaxStreams = 3;                // long loops / ambience decoded on the fly
const unsigned kStreamOggBytes = 96 * 1024;   // ogg files above this size are streamed
const unsigned kCacheBudget = 4u * 1024u * 1024u;  // decoded PCM for short clips
const float kRef = 600.f, kMaxD = 10000.f, kRolloff = 10.f, kListenerZ = 600.f;

struct Clip {
  char file[48];
  short* pcm;          // mono s16 (short clips)
  int frames, rate;
  unsigned char* ogg;  // whole file (streamed clips)
  unsigned ogg_len;
  bool missing;
  unsigned last_use;
  int playing;
};

struct Voice {
  bool active;
  Clip* clip;
  stb_vorbis* vs;      // stream decoder (null for decoded clips)
  short sbuf[2048];
  int sbuf_n, sbuf_pos;
  unsigned pos_fp;     // 16.16 position in source frames (decoded clips) / sbuf
  unsigned step_fp;
  int vol_l, vol_r;    // 0..32768
  char tag[32];
};

class PspAudio : public IAudio {
public:
  PspAudio() : ch_(-1), sema_(-1), thread_(-1), nclips_(0), tick_(0), used_(0),
               lx_(0.f), ly_(0.f), quit_(false), plays_(0) {
    std::strcpy(dir_, "data/media");
    std::memset(voices_, 0, sizeof(voices_));
    std::memset(clips_, 0, sizeof(clips_));
    ch_ = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, kFrames, PSP_AUDIO_FORMAT_STEREO);
    if (ch_ < 0) { mdaz_boot_log("audio: sceAudioChReserve failed %d", ch_); return; }
    sema_ = sceKernelCreateSema("mdaz_audio", 0, 1, 1, 0);
    thread_ = sceKernelCreateThread("mdaz_audio", &PspAudio::thread_entry, 0x12, 0x40000,   // 256 KB: stb_vorbis decodes with alloca temp buffers
                                   
                                    PSP_THREAD_ATTR_USER, 0);
    if (thread_ >= 0) { PspAudio* self = this; sceKernelStartThread(thread_, sizeof(self), &self); }
    mdaz_boot_log("audio: channel %d, mixer thread %d, %d Hz stereo", ch_, thread_, kOutRate);
  }

  void set_media_dir(const char* path) {
    if (path && path[0]) { std::strncpy(dir_, path, sizeof(dir_) - 1); dir_[sizeof(dir_) - 1] = 0; }
  }
  void set_listener(float x, float y) { lx_ = x; ly_ = y; }

  void play(const char* name, float volume) { start(name, volume, false, 0.f, 0.f); }
  void play_at(const char* name, float x, float y, float volume) { start(name, volume, true, x, y); }

  void stop_tag(const char* tag) {
    if (!tag) return;
    lock();
    for (int i = 0; i < kMaxVoices; ++i)
      if (voices_[i].active && std::strcmp(voices_[i].tag, tag) == 0) kill(voices_[i]);
    unlock();
  }

private:
  static int thread_entry(SceSize, void* argp) {
    PspAudio* self = *(PspAudio**)argp;
    self->run();
    return 0;
  }
  void lock() { if (sema_ >= 0) sceKernelWaitSema(sema_, 1, 0); }
  void unlock() { if (sema_ >= 0) sceKernelSignalSema(sema_, 1); }

  const char* file_for(const char* name, char* buf, int n) {
    for (unsigned i = 0; i < sizeof(kTagFiles) / sizeof(kTagFiles[0]); ++i)
      if (std::strcmp(kTagFiles[i].tag, name) == 0) return kTagFiles[i].file;
    // C2 plays media by file name: tag == file stem (case-insensitive on the web server).
    int k = 0;
    for (; name[k] && k < n - 5; ++k) buf[k] = (char)std::tolower((unsigned char)name[k]);
    std::strcpy(buf + k, ".ogg");
    return buf;
  }

  unsigned char* read_all(const char* file, unsigned& len) {
    char path[160];
    std::snprintf(path, sizeof(path), "%s/%s", dir_, file);
    FILE* f = std::fopen(path, "rb");
    if (!f) return 0;
    std::fseek(f, 0, SEEK_END);
    const long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    unsigned char* b = sz > 0 ? (unsigned char*)std::malloc((size_t)sz) : 0;
    if (b && std::fread(b, 1, (size_t)sz, f) != (size_t)sz) { std::free(b); b = 0; }
    std::fclose(f);
    len = b ? (unsigned)sz : 0;
    return b;
  }

  void evict_for(unsigned need) {
    while (used_ + need > kCacheBudget) {
      int best = -1;
      for (int i = 0; i < nclips_; ++i)
        if (clips_[i].pcm && clips_[i].playing == 0 && (best < 0 || clips_[i].last_use < clips_[best].last_use)) best = i;
      if (best < 0) return;
      used_ -= (unsigned)clips_[best].frames * 2u;
      std::free(clips_[best].pcm); clips_[best].pcm = 0; clips_[best].frames = 0;
    }
  }

  // Main thread: find / load a clip (lock not held; the mixer only touches clips via voices).
  Clip* get_clip(const char* file) {
    Clip* c = 0;
    for (int i = 0; i < nclips_; ++i) if (std::strcmp(clips_[i].file, file) == 0) { c = &clips_[i]; break; }
    if (!c) {
      if (nclips_ >= kMaxClips) return 0;
      c = &clips_[nclips_++];
      std::memset(c, 0, sizeof(*c));
      std::strncpy(c->file, file, sizeof(c->file) - 1);
    }
    c->last_use = ++tick_;
    if (c->missing) return 0;
    if (c->pcm || c->ogg) return c;
    unsigned len = 0;
    unsigned char* data = read_all(file, len);
    if (!data) { c->missing = true; mdaz_boot_log("audio: missing %s/%s", dir_, file); return 0; }
    if (len > kStreamOggBytes) {
      c->ogg = data; c->ogg_len = len;
      int err = 0;
      stb_vorbis* v = stb_vorbis_open_memory(data, (int)len, &err, 0);
      if (!v) { c->missing = true; std::free(data); c->ogg = 0; return 0; }
      c->rate = (int)stb_vorbis_get_info(v).sample_rate;
      stb_vorbis_close(v);
      if (logged_ < 40) { ++logged_; mdaz_boot_log("audio: stream %s (%u bytes ogg, %d Hz)", file, len, c->rate); }
      return c;
    }
    int chn = 0, rate = 0;
    short* pcm = 0;
    const int frames = stb_vorbis_decode_memory(data, (int)len, &chn, &rate, &pcm);
    std::free(data);
    if (frames <= 0 || !pcm || chn <= 0) { if (pcm) std::free(pcm); c->missing = true; return 0; }
    if (chn > 1) {   // downmix to mono in place
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
    unlock();
    c->pcm = pcm; c->frames = frames; c->rate = rate;
    used_ += (unsigned)frames * 2u;
    if (logged_ < 40) { ++logged_; mdaz_boot_log("audio: decoded %s (%d frames, %d Hz, cache %u KB)", file, frames, rate, used_ / 1024u); }
    return c;
  }

  void start(const char* name, float volume, bool positioned, float x, float y) {
    if (ch_ < 0 || !name || !name[0]) return;
    char buf[64];
    Clip* c = get_clip(file_for(name, buf, (int)sizeof(buf)));
    if (!c) return;
    if (volume < 0.f) volume = 0.f;
    if (volume > 1.f) volume = 1.f;
    float gl = volume, gr = volume;
    if (positioned) {
      const float dx = x - lx_, dy = y - ly_;
      float d = std::sqrt(dx * dx + dy * dy + kListenerZ * kListenerZ);
      if (d < kRef) d = kRef;
      if (d > kMaxD) d = kMaxD;
      const float g = kRef / (kRef + kRolloff * (d - kRef));
      // Equal-power pan by azimuth (stand-in for HRTF): p in [-1, 1].
      const float p = dx / std::sqrt(dx * dx + kListenerZ * kListenerZ);
      const float a = (p + 1.f) * 0.785398163f;
      gl = volume * g * std::cos(a);
      gr = volume * g * std::sin(a);
    }
    stb_vorbis* vs = 0;
    if (c->ogg) {
      int streams = 0;
      for (int i = 0; i < kMaxVoices; ++i) if (voices_[i].active && voices_[i].vs) ++streams;
      if (streams >= kMaxStreams) return;
      int err = 0;
      vs = stb_vorbis_open_memory(c->ogg, (int)c->ogg_len, &err, 0);
      if (!vs) return;
    }
    lock();
    int vi = -1;
    for (int i = 0; i < kMaxVoices; ++i) if (!voices_[i].active) { vi = i; break; }
    if (vi < 0) {   // steal the oldest decoded-clip voice
      for (int i = 0; i < kMaxVoices; ++i) if (!voices_[i].vs) { kill(voices_[i]); vi = i; break; }
    }
    if (vi < 0) { unlock(); if (vs) stb_vorbis_close(vs); return; }
    Voice& v = voices_[vi];
    v.clip = c; v.vs = vs; v.sbuf_n = 0; v.sbuf_pos = 0; v.pos_fp = 0;
    v.step_fp = (unsigned)(((unsigned long long)c->rate << 16) / kOutRate);
    v.vol_l = (int)(gl * 32768.f); v.vol_r = (int)(gr * 32768.f);
    std::strncpy(v.tag, name, sizeof(v.tag) - 1); v.tag[sizeof(v.tag) - 1] = 0;
    ++c->playing;
    v.active = true;
    ++plays_;
    unlock();
  }

  void kill(Voice& v) {
    if (!v.active) return;
    if (v.vs) { stb_vorbis_close(v.vs); v.vs = 0; }
    if (v.clip && v.clip->playing > 0) --v.clip->playing;
    v.active = false;
  }

  // Mixer thread.
  void run() {
    static short out[2][kFrames * 2];
    static int acc[kFrames * 2];
    int cur = 0;
    while (!quit_) {
      std::memset(acc, 0, sizeof(acc));
      lock();
      for (int i = 0; i < kMaxVoices; ++i) {
        Voice& v = voices_[i];
        if (!v.active) continue;
        for (int f = 0; f < kFrames; ++f) {
          int s;
          if (v.vs) {
            int idx = (int)(v.pos_fp >> 16);
            while (idx >= v.sbuf_n) {
              v.pos_fp -= (unsigned)v.sbuf_n << 16;
              short* bp[1] = { v.sbuf };   // non-interleaved API downmixes stereo to 1 channel
              v.sbuf_n = stb_vorbis_get_samples_short(v.vs, 1, bp, 2048);
              if (v.sbuf_n <= 0) break;
              idx = (int)(v.pos_fp >> 16);
            }
            if (v.sbuf_n <= 0) { kill(v); break; }
            s = v.sbuf[idx];
          } else {
            const int idx = (int)(v.pos_fp >> 16);
            if (idx >= v.clip->frames) { kill(v); break; }
            s = v.clip->pcm[idx];
          }
          acc[f * 2] += (s * v.vol_l) >> 15;
          acc[f * 2 + 1] += (s * v.vol_r) >> 15;
          v.pos_fp += v.step_fp;
        }
      }
      unlock();
      short* o = out[cur];
      for (int k = 0; k < kFrames * 2; ++k) {
        int s = acc[k];
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        o[k] = (short)s;
      }
      sceAudioOutputPannedBlocking(ch_, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, o);
      cur ^= 1;
    }
  }

  static const int kMaxClips = 224;
  int ch_;
  SceUID sema_, thread_;
  Voice voices_[kMaxVoices];
  Clip clips_[kMaxClips];
  int nclips_;
  unsigned tick_, used_;
  float lx_, ly_;
  volatile bool quit_;
  unsigned plays_;
  int logged_ = 0;
  char dir_[128];
};

} // namespace

IAudio* create_psp_audio() { return new PspAudio(); }

} // namespace mdaz
#else
namespace mdaz { IAudio* create_psp_audio() { return new NullAudio(); } }
#endif
