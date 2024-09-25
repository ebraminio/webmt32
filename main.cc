#include <cstdio>
#include <cstdint>
#include <cassert>
#define MT32EMU_API_TYPE 3
#include <mt32emu/mt32emu.h>

MT32Emu::Service service;

#ifdef __EMSCRIPTEN__
extern "C" {
    extern void lcdMessage(const char *message);
}
#else
void lcdMessage(const char *message) { printf("%s\n", message); }
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
}

extern "C" unsigned sampleRate() {
    return service.getStereoOutputSamplerate(outputMode);
}

extern "C" void playMsg(uint32_t msg) {
    service.playMsg(msg);
}

extern "C" void render() {
    FILE *f0 = fopen("result0.pcm", "wb");
    FILE *f1 = fopen("result1.pcm", "wb");
    const unsigned samplerate = sampleRate();
    auto *buffer = new float[samplerate];
    while (service.isActive()) {
        service.renderFloat(buffer, samplerate / 2);
        for (unsigned i = 0; i < samplerate; i += 2) {
            fwrite(&buffer[i], sizeof(float), 1, f0);
            fwrite(&buffer[i + 1], sizeof(float), 1, f1);
        }
    }
    delete[] buffer;
    fclose(f0);
    fclose(f1);
}

int main() {
#ifndef __EMSCRIPTEN__
    init();
    render(0x007f4591);
#endif
    return 0;
}
