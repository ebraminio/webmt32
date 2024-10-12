#include <AL/al.h>
#include <AL/alc.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <unistd.h>
#endif

void lcdMessage(const char *message) {
#ifdef __EMSCRIPTEN__
    EM_ASM({ window.lcdMessage($0); }, message);
#else
    printf("%s\n", message);
#endif
}

#ifndef __EMSCRIPTEN__
#include "midiparser.hh"
#include "receivers.hh"
#endif
#include "synthesizers.hh"

#define PCM_SIZE 2048
float pcm[PCM_SIZE];
float emptyPcm[PCM_SIZE] = {};
ALuint source = 0;
unsigned sampleRate;
Synth *synth;

static void stream_buffer(const ALuint bufferId) {
    alBufferData(
        bufferId, 0x10011/* AL_FORMAT_STEREO_FLOAT32 */,
        synth->render(pcm, PCM_SIZE) ? pcm : emptyPcm, sizeof pcm, sampleRate
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

extern "C" void playMsg(uint32_t msg) {
    synth->handleShortMessage(msg);
}

[[noreturn]] int main() {
#ifndef __EMSCRIPTEN__
    chdir("..");
#endif
    synth = new MT32Synth();
#ifndef __EMSCRIPTEN__
    synth->handleShortMessage(0x7f4591);
#endif
    sampleRate = synth->sampleRate();

    ALCdevice *device = alcOpenDevice(nullptr);
    assert(device);
    ALCcontext *ctx = alcCreateContext(device, nullptr);
    assert(ctx);
    assert(alcMakeContextCurrent(ctx));
    assert(alIsExtensionPresent("AL_EXT_float32"));

    alGenSources(1, &source); {
        enum { BUFFERS_COUNT = 4 };
        ALuint buffers[BUFFERS_COUNT] = {};
        alGenBuffers(BUFFERS_COUNT, buffers);
        for (const unsigned int buffer: buffers) stream_buffer(buffer);
        alSourceQueueBuffers(source, BUFFERS_COUNT, buffers);
    }
    alSourcePlay(source);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(iter, 0, 0);
    return 0;
#else
    const auto midiParser = new MIDIParser(synth);
    Receiver *receiver = new UDPReceiver(1999);

    while (true) {
        usleep(16);
        iter();
        uint8_t buffer[1000];
        unsigned n = receiver->receive(buffer, sizeof buffer);
        if (n) midiParser->parseMIDIBytes(buffer, n);
    }
#endif
}
