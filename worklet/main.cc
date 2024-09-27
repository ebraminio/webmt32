#include <emscripten/webaudio.h>
#include <emscripten/em_math.h>
#include <cstdio>
#include <cstdint>
#include <cassert>
#define MT32EMU_API_TYPE 3
#include <mt32emu/mt32emu.h>

MT32Emu::Service service;

char lcdMessageBuffer[22] = {};

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
    void showLCDMessage(const char *message) override {
      MAIN_THREAD_ASYNC_EM_ASM({
        console.log(Module.UTF8ToString($0).replace(/\x01/g, '*'));
      }, message);
    }
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

auto outputMode = MT32Emu::AnalogOutputMode_ACCURATE;
float buffer[256];

EM_BOOL ProcessAudio(
  int numInputs, const AudioSampleFrame *inputs, int numOutputs,
  AudioSampleFrame *outputs, int numParams,
  const AudioParamFrame *params, void *userData
) {
  if (!service.isActive()) return EM_FALSE;

  service.renderFloat(buffer, 128);
  for (unsigned i = 0; i < 128; ++i) {
    outputs[0].data[i] = buffer[i * 2];
    outputs[0].data[128 + i] = buffer[i * 2 + 1];
  }
  return EM_TRUE;
}

EMSCRIPTEN_KEEPALIVE
extern "C" void playMsg(uint32_t msg) {
  service.playMsg(msg);
}

void AudioWorkletProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, EM_BOOL success, void *userData) {
  if (!success) return;

  int outputChannelCounts[1] = {2};

  EmscriptenAudioWorkletNodeCreateOptions options = {
    .numberOfInputs = 0,
    .numberOfOutputs = 1,
    .outputChannelCounts = outputChannelCounts
  };

  EMSCRIPTEN_AUDIO_WORKLET_NODE_T wasmAudioWorklet = emscripten_create_wasm_audio_worklet_node(
    audioContext, "webmt32", &options, &ProcessAudio, 0
  );

  EM_ASM({
    const audioContext = emscriptenGetAudioObject($0);
    const audioWorkletNode = emscriptenGetAudioObject($1);
    audioWorkletNode.connect(audioContext.destination);

    const startButton = document.createElement('button');
    startButton.innerHTML = 'Toggle playback';
    document.body.appendChild(startButton);

    startButton.onclick = () => {
      if (audioContext.state !== 'running') audioContext.resume();
      else audioContext.suspend();
    };

    if (navigator.requestMIDIAccess) navigator.requestMIDIAccess().then(midi => {
      midi.inputs.forEach(entry => entry.onmidimessage = event => {
        if (event.data.length === 3) {
          Module._playMsg((event.data[2] << 16) + (event.data[1] << 8) + event.data[0]);
        } else console.log(event.data);
      });
    }).catch(console.error);
  }, audioContext, wasmAudioWorklet);
}

void WebAudioWorkletThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, EM_BOOL success, void *userData) {
  if (!success) return;

  WebAudioWorkletProcessorCreateOptions opts = {
    .name = "webmt32",
  };
  emscripten_create_wasm_audio_worklet_processor_async(audioContext, &opts, AudioWorkletProcessorCreated, 0);

  service.playMsg(0x7f4591);
}

uint8_t wasmAudioWorkletStack[4096];

int main() {
  service.createContext(report_handler);
  service.setAnalogOutputMode(outputMode);
  service.addROMFile("mt32_ctrl_1_07.rom");
  service.addROMFile("mt32_pcm.rom");
  assert(service.openSynth() == MT32EMU_RC_OK);

  EmscriptenWebAudioCreateAttributes attrs = {
    .latencyHint = "interactive",
    .sampleRate = service.getStereoOutputSamplerate(outputMode)
  };

  EMSCRIPTEN_WEBAUDIO_T context = emscripten_create_audio_context(&attrs);

  emscripten_start_wasm_audio_worklet_thread_async(
    context, wasmAudioWorkletStack, sizeof(wasmAudioWorkletStack),
    WebAudioWorkletThreadInitialized, 0
  );
}
