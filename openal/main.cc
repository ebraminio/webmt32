#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <AL/al.h>
#include <AL/alc.h>
#define MT32EMU_API_TYPE 3
#include <mt32emu/mt32emu.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <unistd.h>
#endif

MT32Emu::Service service;
#define PCM_SIZE 1024
float pcm[PCM_SIZE];
ALuint source = 0;
unsigned sampleRate;
auto outputMode = MT32Emu::AnalogOutputMode_ACCURATE;

static void stream_buffer(ALuint bufferId) {
  if (!service.isActive()) return;
  service.renderFloat(pcm, PCM_SIZE / 2);
  alBufferData(
    bufferId, 0x10011/* AL_FORMAT_STEREO_FLOAT32 */, pcm,
    sizeof (pcm), sampleRate
  );
}

static void iter() {
  ALint processed, state;
  alGetSourcei(source, AL_SOURCE_STATE, &state);
  alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
  assert(alGetError() == AL_NO_ERROR);

  while (processed > 0) {
    ALuint buffer;
    alSourceUnqueueBuffers(source, 1, &buffer);
    processed--;
    stream_buffer(buffer);
    alSourceQueueBuffers(source, 1, &buffer);
  }

  if (state != AL_PLAYING && state != AL_PAUSED) {
    fprintf(stderr, "Buffer underrun, restart the play\n");
    alSourcePlay(source);
  }
}

int main() {
  service.createContext();
  service.setAnalogOutputMode(outputMode);
  service.addROMFile("mt32_ctrl_1_07.rom");
  service.addROMFile("mt32_pcm.rom");
  assert(service.openSynth() == MT32EMU_RC_OK);
  sampleRate = service.getStereoOutputSamplerate(outputMode);
  service.playMsg(0x7f4591);

  ALCdevice *device = alcOpenDevice(nullptr);
  assert(device);
  ALCcontext *ctx = alcCreateContext(device, nullptr);
  assert(ctx);
  assert(alcMakeContextCurrent(ctx));
  assert(alIsExtensionPresent("AL_EXT_float32"));

  alGenSources(1, &source);
  #define BUFFERS_COUNT 4
  ALuint buffers[BUFFERS_COUNT] = {};
  alGenBuffers(BUFFERS_COUNT, buffers);
  for (unsigned i = 0; i < BUFFERS_COUNT; ++i) stream_buffer(buffers[i]);
  alSourceQueueBuffers(source, BUFFERS_COUNT, buffers);
  #undef BUFFERS_COUNT
  alSourcePlay(source);

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(iter, 0, 0);
#else
  while (1) {
    usleep(16);
    iter();
  }
#endif
  return 0;
}
