mt-32 synthesizer for web.

https://ebraminio.github.io/webmt32

Build:

First put your own `mt32_ctrl_1_07.rom` and `mt32_pcm.rom` at root then

`meson setup wasmbuild --cross-file emscripten-build.ini && ninja -Cwasmbuild`
