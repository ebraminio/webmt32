addToLibrary({
    lcdMessage: function (message) {
        Module.lcdMessage(Module.UTF8ToString(message));
    },
    play: function (bufferSize, channel0, channel1, timeOffset) {
        Module.play(bufferSize, channel0 / 4, channel1 / 4, timeOffset);
    },
});
