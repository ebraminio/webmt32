project(
    'webmt32', 'cpp',
    version : '0.0.1',
    default_options : ['warning_level=3', 'cpp_std=c++20']
)

fluidsynth_dep = dependency('fluidsynth')
mt32emu_dep = dependency('mt32emu', method : 'builtin')
if target_machine.system() != 'emscripten'
    openal_dep = dependency('openal', method : 'builtin')
    link_args = []
else
    openal_dep = dependency('openal', required : false)
    link_args = [
        '-sNO_EXIT_RUNTIME=1', '-sMODULARIZE=1',
        '--embed-file', '../mt32_ctrl_1_07.rom@mt32_ctrl_1_07.rom',
        '--embed-file', '../mt32_pcm.rom@mt32_pcm.rom',
        '--embed-file', '../SC-55 SoundFont v1.2b.sf2@SC-55 SoundFont v1.2b.sf2',
        '-sEXPORTED_RUNTIME_METHODS=["UTF8ToString"]',
        '-sEXPORTED_FUNCTIONS=["_main", "_playMsg"]'
    ]
endif

webmt32 = executable(
    'webmt32', 'main.cc',
    dependencies : [fluidsynth_dep, mt32emu_dep, openal_dep],
    link_args : link_args,
    install : true
)

test('test', webmt32)
