project(
    'webmt32', 'cpp',
    version: '0.0.1',
    default_options: ['warning_level=3', 'cpp_std=c++20']
)

mt32emu_dep = dependency('mt32emu', method: 'builtin')
openal_dep = dependency('openal', required: false)

webmt32 = executable(
    'webmt32', 'main.cc',
    dependencies: [mt32emu_dep, openal_dep],
    install: true
)

test('test', webmt32)
