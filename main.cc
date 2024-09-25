#include <cstdio>
#include <cstdint>
#include <cassert>
#define MT32EMU_API_TYPE 3
#include <mt32emu/mt32emu.h>

class ReportHandler : public MT32Emu::IReportHandlerV1 {
    MT32Emu::Service *service;

public:
    virtual ~ReportHandler() = default;

    explicit ReportHandler(MT32Emu::Service *service): IReportHandlerV1(), service(service) {
    }

    void printDebug(const char *fmt, va_list list) override { vprintf(fmt, list); }

    void onLCDStateUpdated() override {
        char targetBuffer[32] = {};
        service->getDisplayState(targetBuffer, false);
        printf("%s\n", targetBuffer);
    }

    void onErrorControlROM() override {
    }

    void onErrorPCMROM() override {
    }

    void showLCDMessage(const char *message) override {
        printf("%s\n", message);
    }

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
};

MT32Emu::Service service;
ReportHandler report_handler{&service};

extern "C" void init() {
    service.createContext(report_handler);
    service.setAnalogOutputMode(MT32Emu::AnalogOutputMode_ACCURATE);
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

extern "C" void render(uint32_t msg) {
    service.playMsg(msg);

    FILE *f0 = fopen("result0.pcm", "wb");
    FILE *f1 = fopen("result1.pcm", "wb");
    const unsigned samplerate = service.getStereoOutputSamplerate(MT32Emu::AnalogOutputMode_ACCURATE);
    auto *buffer = new float[samplerate];
    while (service.isActive()) {
        service.renderFloat(buffer, samplerate / 2);
        for (unsigned i = 0; i < samplerate; i += 2) {
            fwrite(&buffer[i], sizeof(float), 1, f0);
            fwrite(&buffer[i + 1], sizeof(float), 1, f1);
        }
    }
    printf("Sample rate is: %d\n", samplerate);
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
