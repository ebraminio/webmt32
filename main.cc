#include <AL/al.h>
#include <AL/alc.h>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#define MT32EMU_API_TYPE 3
#include <mt32emu/mt32emu.h>
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

char lcdMessageBuffer[22] = {};
MT32Emu::Service service;
#define PCM_SIZE 2048
float pcm[PCM_SIZE];
float emptyPcm[PCM_SIZE] = {};
ALuint source = 0;
unsigned sampleRate;
auto outputMode = MT32Emu::AnalogOutputMode_ACCURATE;

class ReportHandler : public MT32Emu::IReportHandlerV1 {
public:
    virtual ~ReportHandler() = default;
    void printDebug(const char *fmt, va_list list) override { vprintf(fmt, list); }
    void onLCDStateUpdated() override {
        service.getDisplayState(lcdMessageBuffer, false);
        showLCDMessage(lcdMessageBuffer);
    }
    void onErrorControlROM() override {}
    void onErrorPCMROM() override {}
    void showLCDMessage(const char *message) override { lcdMessage(message); }
    void onMIDIMessagePlayed() override {}
    bool onMIDIQueueOverflow() override { return false; }
    void onMIDISystemRealtime(MT32Emu::Bit8u system_realtime) override {}
    void onDeviceReset() override {}
    void onDeviceReconfig() override {}
    void onNewReverbMode(MT32Emu::Bit8u mode) override {}
    void onNewReverbTime(MT32Emu::Bit8u time) override {}
    void onNewReverbLevel(MT32Emu::Bit8u level) override {}
    void onPolyStateChanged(MT32Emu::Bit8u part_num) override {}
    void onProgramChanged(MT32Emu::Bit8u part_num, const char *sound_group_name, const char *patch_name) override {}
    void onMidiMessageLEDStateUpdated(bool ledState) override {}
} report_handler;


static void stream_buffer(ALuint bufferId) {
    bool isActive = service.isActive();
    if (isActive) service.renderFloat(pcm, PCM_SIZE / 2);
    alBufferData(
      bufferId, 0x10011/* AL_FORMAT_STEREO_FLOAT32 */, isActive ? pcm : emptyPcm,
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

extern "C" void playMsg(uint32_t msg) { service.playMsg(msg); }

[[noreturn]] int main() {
    service.createContext(report_handler);
    service.setAnalogOutputMode(outputMode);
#ifdef __EMSCRIPTEN__
    service.addROMFile("mt32_ctrl_1_07.rom");
    service.addROMFile("mt32_pcm.rom");
#else
     service.addROMFile("../mt32_ctrl_1_07.rom");
     service.addROMFile("../mt32_pcm.rom");
#endif
    assert(service.openSynth() == MT32EMU_RC_OK);
    report_handler.onLCDStateUpdated();
    sampleRate = service.getStereoOutputSamplerate(outputMode);
#ifndef __EMSCRIPTEN__
    service.playMsg(0x7f4591);
#endif

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
    for (const unsigned int buffer : buffers) stream_buffer(buffer);
    alSourceQueueBuffers(source, BUFFERS_COUNT, buffers);
#undef BUFFERS_COUNT
    alSourcePlay(source);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(iter, 0, 0);
#else
    while (true) {
        usleep(16);
        iter();
    }
#endif
    return 0;
}
