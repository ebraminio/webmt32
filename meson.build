project(
    'webmt32', 'cpp',
    version : '0.0.1',
    default_options : ['warning_level=3', 'cpp_std=c++20']
)

null_dep = dependency('', required : false)

if target_machine.system() == 'emscripten'
    fluidsynth_dep = dependency('fluidsynth', method : 'builtin')
    mt32emu_dep = dependency('mt32emu', method : 'builtin')
    openal_dep = null_dep
    minifb_dep = null_dep
    cairo_dep = null_dep
    link_args = [
        '-sNO_EXIT_RUNTIME=1', '-sMODULARIZE=1',
        '--embed-file', '../mt32_ctrl_1_07.rom@mt32_ctrl_1_07.rom',
        '--embed-file', '../mt32_pcm.rom@mt32_pcm.rom',
        '--embed-file', '../SC-55 SoundFont v1.2b.sf2@SC-55 SoundFont v1.2b.sf2',
        '-sEXPORTED_RUNTIME_METHODS=["UTF8ToString"]',
        '-sEXPORTED_FUNCTIONS=["_main", "_playMsg"]'
    ]
else
    fluidsynth_dep = dependency('fluidsynth')
    mt32emu_dep = dependency('mt32emu')
    minifb_dep = import('cmake').subproject('minifb').dependency('minifb')
    cairo_dep = dependency('cairo')
    if target_machine.system() == 'darwin'
        minifb_dep = [
            minifb_dep,
            dependency(
                'appleframeworks',
                modules : ['Foundation', 'AppKit', 'Metal', 'MetalKit'],
                required : true
            ),
        ]
        # macOS's builtin OpenAL isn't bad just is a bit different
        openal_dep = dependency('openal', method : 'builtin')
    else
        openal_dep = dependency('openal')
    endif
    link_args = []
endif

webmt32 = executable(
    'webmt32', 'src/main.cc',
    dependencies : [fluidsynth_dep, mt32emu_dep, openal_dep, minifb_dep, cairo_dep],
    link_args : link_args,
    install : true
)

test('test', webmt32)
