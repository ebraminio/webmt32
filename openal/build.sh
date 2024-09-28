#!/bin/bash
emcc \
  munt/mt32emu/src/srchelper/InternalResampler.cpp \
  munt/mt32emu/src/sha1/sha1.cpp \
  munt/mt32emu/src/Analog.cpp \
  munt/mt32emu/src/File.cpp \
  munt/mt32emu/src/srchelper/srctools/src/LinearResampler.cpp \
  munt/mt32emu/src/srchelper/srctools/src/IIR2xResampler.cpp \
  munt/mt32emu/src/Display.cpp \
  munt/mt32emu/src/srchelper/srctools/src/ResamplerModel.cpp \
  munt/mt32emu/src/srchelper/srctools/src/FIRResampler.cpp \
  munt/mt32emu/src/srchelper/srctools/src/SincResampler.cpp \
  munt/mt32emu/src/BReverbModel.cpp \
  munt/mt32emu/src/LA32WaveGenerator.cpp \
  munt/mt32emu/src/LA32Ramp.cpp \
  munt/mt32emu/src/MidiStreamParser.cpp \
  munt/mt32emu/src/PartialManager.cpp \
  munt/mt32emu/src/SampleRateConverter.cpp \
  munt/mt32emu/src/ROMInfo.cpp \
  munt/mt32emu/src/Poly.cpp \
  munt/mt32emu/src/Partial.cpp \
  munt/mt32emu/src/LA32FloatWaveGenerator.cpp \
  munt/mt32emu/src/TVF.cpp \
  munt/mt32emu/src/Part.cpp \
  munt/mt32emu/src/VersionTagging.cpp \
  munt/mt32emu/src/TVA.cpp \
  munt/mt32emu/src/TVP.cpp \
  munt/mt32emu/src/Tables.cpp \
  munt/mt32emu/src/Synth.cpp \
  munt/mt32emu/src/FileStream.cpp \
  munt/mt32emu/src/c_interface/c_interface.cpp \
  main.cc \
  --embed-file ../mt32_ctrl_1_07.rom@mt32_ctrl_1_07.rom \
  --embed-file ../mt32_pcm.rom@mt32_pcm.rom \
  -Imunt/mt32emu/src \
  -I. \
  -sEXPORTED_RUNTIME_METHODS=["UTF8ToString"] \
  -o index.html
