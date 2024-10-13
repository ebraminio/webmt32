#pragma once

#include <cstdint>
#include <fluidsynth.h>
#define MT32EMU_API_TYPE 3
#include <mt32emu/mt32emu.h>

struct Synth {
    virtual ~Synth() = default;

    virtual bool render(
        [[maybe_unused]] float *buffer,
        [[maybe_unused]] unsigned size
    ) { return false; }

    virtual void handleShortMessage([[maybe_unused]] uint32_t message) {
    }

    virtual void handleSysEx(
        [[maybe_unused]] const uint8_t *data,
        [[maybe_unused]] size_t size
    ) {
    }

    virtual unsigned sampleRate() {
        return 0;
    }
};

struct FluidSynth final : Synth {
    fluid_settings_t *settings;
    fluid_synth_t *synth;

    ~FluidSynth() override {
        delete_fluid_settings(settings);
        delete_fluid_synth(synth);
    }

    FluidSynth() {
        settings = new_fluid_settings();
        fluid_settings_setstr(settings, "player.timing-source", "sample");
        fluid_settings_setint(settings, "synth.lock-memory", 0);

        synth = new_fluid_synth(settings);
        fluid_synth_sfload(synth, "SC-55 SoundFont v1.2b.sf2", true);
    }

    unsigned sampleRate() override {
        double sample_rate;
        fluid_settings_getnum(settings, "synth.sample-rate", &sample_rate);
        return static_cast<unsigned>(sample_rate);
    }

    void handleShortMessage(const uint32_t message) override {
        // This part is brought from https://github.com/dwhinham/mt32-pi/blob/ee645df6/src/synth/soundfontsynth.cpp#L206
        const uint8_t status = message & 0xFF;
        const uint8_t channel = message & 0x0F;
        const uint8_t data1 = message >> 8 & 0xFF;
        const uint8_t data2 = message >> 16 & 0xFF;

        // Handle system real-time messages
        if (status == 0xFF) {
            fluid_synth_system_reset(synth);
            return;
        }

        // Handle channel messages
        switch (status & 0xF0) {
            // Note off
            case 0x80:
                fluid_synth_noteoff(synth, channel, data1);
                break;

            // Note on
            case 0x90:
                fluid_synth_noteon(synth, channel, data1, data2);
                break;

            // Polyphonic key pressure/aftertouch
            case 0xA0:
                fluid_synth_key_pressure(synth, channel, data1, data2);
                break;

            // Control change
            case 0xB0:
                fluid_synth_cc(synth, channel, data1, data2);
                break;

            // Program change
            case 0xC0:
                fluid_synth_program_change(synth, channel, data1);
                break;

            // Channel pressure/aftertouch
            case 0xD0:
                fluid_synth_channel_pressure(synth, channel, data1);
                break;

            // Pitch bend
            case 0xE0:
                fluid_synth_pitch_bend(synth, channel, data2 << 7 | data1);
                break;

            default: ;
        }
    }

    void handleSysEx(const uint8_t *data, const size_t size) override {
        fluid_synth_sysex(
            synth, reinterpret_cast<const char *>(data), size,
            nullptr, nullptr, nullptr, 0
        );
    }

    bool render(float *buffer, const unsigned size) override {
        if (fluid_synth_get_active_voice_count(synth) < 1) return false;
        fluid_synth_write_float(synth, size / 2, buffer, 0, 2, buffer, 1, 2);
        return true;
    }
};

class ReportHandler final : public MT32Emu::IReportHandlerV1 {
    MT32Emu::Service &service;
    char lcdMessageBuffer[22]{};

public:
    explicit ReportHandler(MT32Emu::Service &service) : IReportHandlerV1(), service(service) {
    }

    virtual ~ReportHandler() = default;

    void printDebug(const char *fmt, const va_list list) override { vprintf(fmt, list); }

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

    void onMIDISystemRealtime(uint8_t system_realtime) override {
    }

    void onDeviceReset() override {
    }

    void onDeviceReconfig() override {
    }

    void onNewReverbMode(uint8_t mode) override {
    }

    void onNewReverbTime(uint8_t time) override {
    }

    void onNewReverbLevel(uint8_t level) override {
    }

    void onPolyStateChanged(uint8_t part_num) override {
    }

    void onProgramChanged(uint8_t part_num, const char *sound_group_name, const char *patch_name) override {
    }

    void onMidiMessageLEDStateUpdated(bool ledState) override {
    }
};

struct MT32Synth final : Synth {
    MT32Emu::Service service;
    ReportHandler *report_handler;

    ~MT32Synth() override {
        delete report_handler;
        service.freeContext();
        service.closeSynth();
    }

    MT32Synth() {
        report_handler = new ReportHandler(service);
        service.createContext(*report_handler);
        service.setAnalogOutputMode(MT32Emu::AnalogOutputMode_ACCURATE);
        service.addROMFile("mt32_ctrl_1_07.rom");
        service.addROMFile("mt32_pcm.rom");
        assert(service.openSynth() == MT32EMU_RC_OK);
        report_handler->onLCDStateUpdated();
    }

    unsigned sampleRate() override {
        return service.getStereoOutputSamplerate(MT32Emu::AnalogOutputMode_ACCURATE);
    }

    void handleShortMessage(const uint32_t message) override {
        service.playMsg(message);
    }

    void handleSysEx(const uint8_t *data, const size_t size) override {
        service.playSysex(data, size);
    }

    bool render(float *buffer, const unsigned size) override {
        if (!service.isActive()) return false;
        service.renderFloat(buffer, size / 2);
        return true;
    }
};
