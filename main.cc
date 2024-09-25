#include <cstdio>
#include <cstdint>
#include <cassert>
#include <vector>
#define MT32EMU_API_TYPE 3
#include <mt32emu/mt32emu.h>

MT32Emu::Service service;

#ifdef __EMSCRIPTEN__
extern "C" {
    extern void lcdMessage(const char *message);
    extern void play(unsigned bufferSize, const float *channel0, const float *channel1, float timeOffset);
}
#else
void lcdMessage(const char *message) { printf("%s\n", message); }

void play(unsigned bufferSize, const float *channel0, const float *channel1, float timeOffset) {
}
#endif

char lcdMessageBuffer[22] = {};

class ReportHandler : public MT32Emu::IReportHandlerV1 {
public:
    virtual ~ReportHandler() = default;

    void printDebug(const char *fmt, va_list list) override { vprintf(fmt, list); }

    void onLCDStateUpdated() override {
        service.getDisplayState(lcdMessageBuffer, false);
        showLCDMessage(lcdMessageBuffer);
    }

    void onErrorControlROM() override {
    }

    void onErrorPCMROM() override {
    }

    void showLCDMessage(const char *message) override { lcdMessage(message); }

    void onMIDIMessagePlayed() override {
    }

    bool onMIDIQueueOverflow() override { return false; }

    void onMIDISystemRealtime(MT32Emu::Bit8u system_realtime) override {
    }

    void onDeviceReset() override {
    }

    void onDeviceReconfig() override {
    }

    void onNewReverbMode(MT32Emu::Bit8u mode) override {
    }

    void onNewReverbTime(MT32Emu::Bit8u time) override {
    }

    void onNewReverbLevel(MT32Emu::Bit8u level) override {
    }

    void onPolyStateChanged(MT32Emu::Bit8u part_num) override {
    }

    void onProgramChanged(MT32Emu::Bit8u part_num, const char *sound_group_name, const char *patch_name) override {
    }

    void onMidiMessageLEDStateUpdated(bool ledState) override {
    }
} report_handler;

auto outputMode = MT32Emu::AnalogOutputMode_ACCURATE;

extern "C" unsigned sampleRate() {
    return service.getStereoOutputSamplerate(outputMode);
}

float *buffer, *channel0, *channel1;

extern "C" void init() {
    service.createContext(report_handler);
    service.setAnalogOutputMode(outputMode);
#ifdef __EMSCRIPTEN__
    service.addROMFile("/rom/mt32_ctrl_1_07.rom");
    service.addROMFile("/rom/mt32_pcm.rom");
#else
    service.addROMFile("../mt32_ctrl_1_07.rom");
    service.addROMFile("../mt32_pcm.rom");
#endif
    assert(service.openSynth() == MT32EMU_RC_OK);
    report_handler.onLCDStateUpdated();

    const float samplerate = sampleRate();
    buffer = new float[samplerate * 2];
    channel0 = new float[samplerate];
    channel1 = new float[samplerate];
}

extern "C" void playMsg(uint32_t msg) {
    service.playMsg(msg);
}

extern "C" void render() {
    const unsigned samplerate = sampleRate();
    float timeOffset = .0f;
    while (service.isActive()) {
        service.renderFloat(buffer, samplerate);
        for (unsigned i = 0; i < samplerate; ++i) {
            channel0[i] = buffer[i * 2];
            channel1[i] = buffer[i * 2 + 1];
        }
        play(samplerate, channel0, channel1, timeOffset);
        ++timeOffset;
    }
}

int main() {
#ifndef __EMSCRIPTEN__
    init();
    playMsg(0x007f4591);
    render();
#endif
    return 0;
}
