// PSP audio backend: the shared C2 mixer (src/platform/audio_mixer.cpp) on one stereo
// sceAudio channel at 44100 Hz, fed by a dedicated thread.
// Memory: the decoded-clip cache (4 MB budget) and the stream decoders are malloc'd from the
// newlib heap (PSP_HEAP_SIZE_KB(-1024) = all user RAM minus 1 MB), not from the ~1 MB kernel
// partition remainder that sceKernelTotalFreeMemSize reports. Only the mixer thread stack
// (64 KB) comes from that remainder.
#include "mdaz/audio.hpp"

#if defined(MDAZ_PSP) && defined(__PSP__)
#include <pspkernel.h>
#include <pspaudio.h>
#include <cstdio>
#include "../../src/platform/audio_mixer.hpp"

extern "C" void mdaz_boot_log(const char* fmt, ...);

namespace mdaz {
namespace {

const int kFrames = 1024;   // per sceAudioOutputPannedBlocking
// Mixer thread stack: stb_vorbis decodes with alloca temp buffers. Measured: every one of the
// 207 media files decodes (open + full get_samples_short loop) on a 20 KB stack (fails at 16 KB,
// max frame size 1024) -> 64 KB keeps >3x headroom and frees 192 KB vs the previous 256 KB.
const int kStack = 0x10000;

class PspAudio : public C2Mixer {
public:
  PspAudio() : C2Mixer(4u * 1024u * 1024u), ch_(-1), sema_(-1), thread_(-1), quit_(false) {
    ch_ = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, kFrames, PSP_AUDIO_FORMAT_STEREO);
    if (ch_ < 0) { mdaz_boot_log("audio: sceAudioChReserve failed %d", ch_); return; }
    sema_ = sceKernelCreateSema("mdaz_audio", 0, 1, 1, 0);
    thread_ = sceKernelCreateThread("mdaz_audio", &PspAudio::thread_entry, 0x12, kStack,
                                    PSP_THREAD_ATTR_USER, 0);
    if (thread_ >= 0) { PspAudio* self = this; sceKernelStartThread(thread_, sizeof(self), &self); }
    mdaz_boot_log("audio: channel %d, mixer thread %d (stack %d KB), %d Hz stereo, cache %u KB",
                  ch_, thread_, kStack / 1024, kOutRate, 4096u);
  }

protected:
  void lock() const { if (sema_ >= 0) sceKernelWaitSema(sema_, 1, 0); }
  void unlock() const { if (sema_ >= 0) sceKernelSignalSema(sema_, 1); }
  void log(const char* msg) { mdaz_boot_log("%s", msg); }

private:
  static int thread_entry(SceSize, void* argp) {
    PspAudio* self = *(PspAudio**)argp;
    static short out[2][kFrames * 2];
    int cur = 0;
    while (!self->quit_) {
      self->mix(out[cur], kFrames);
      sceAudioOutputPannedBlocking(self->ch_, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, out[cur]);
      cur ^= 1;
    }
    return 0;
  }
  int ch_;
  SceUID sema_, thread_;
  volatile bool quit_;
};

} // namespace

IAudio* create_psp_audio() { return new PspAudio(); }

} // namespace mdaz
#else
namespace mdaz { IAudio* create_psp_audio() { return new NullAudio(); } }
#endif
