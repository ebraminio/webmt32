#pragma once
// This is derived from https://github.com/dwhinham/mt32-pi/blob/52b85b7/src/midiparser.cpp
// The original file was published under terms of GPLv3,
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <cassert>
#include <cstdint>
#include <cstdio>
#include "synthesizers.hh"

class MIDIParser {
public:
    explicit MIDIParser(Synth *synth) : synth(synth) {
    }

    void parseMIDIBytes(const uint8_t *data, const size_t size, const bool isIgnoreNoteOns = false) {
        // Process MIDI messages
        // See: https://www.midi.org/specifications/item/table-1-summary-of-midi-message
        for (size_t i = 0; i < size; ++i) {
            uint8_t byte = data[i];

            // System Real-Time message - single byte, handle immediately
            // Can appear anywhere in the stream, even in between status/data bytes
            if (byte >= 0xF8) {
                // Ignore undefined System Real-Time
                if (byte != 0xF9 && byte != 0xFD)
                    onShortMessage(byte);

                continue;
            }

            switch (state) {
                // Expecting a status byte
                case State::StatusByte:
                    parseStatusByte(byte);
                    break;

                // Expecting a data byte
                case State::DataByte:
                    // Expected a data byte, but received a status
                    if (byte & 0x80) {
                        onUnexpectedStatus();
                        resetState(true);
                        parseStatusByte(byte);
                        break;
                    }

                    messageBuffer[messageLength++] = byte;
                    checkCompleteShortMessage(isIgnoreNoteOns);
                    break;

                // Expecting a SysEx data byte or EOX
                case State::SysExByte:
                    // Received a status that wasn't EOX
                    if (byte & 0x80 && byte != 0xF7) {
                        onUnexpectedStatus();
                        resetState(true);
                        parseStatusByte(byte);
                        break;
                    }

                // Buffer overflow
                    if (messageLength == sizeof messageBuffer) {
                        onSysExOverflow();
                        resetState(true);
                        parseStatusByte(byte);
                        break;
                    }

                    messageBuffer[messageLength++] = byte;

                // End of SysEx
                    if (byte == 0xF7) {
                        onSysExMessage(messageBuffer, messageLength);
                        resetState(true);
                    }

                    break;
            }
        }
    }

private:
    void onShortMessage(const uint32_t message) const {
        synth->handleShortMessage(message);
    }

    void onSysExMessage(const uint8_t *data, const size_t size) const {
        synth->handleSysEx(data, size);
    }

    void onUnexpectedStatus() const {
        if (state == State::SysExByte)
            puts("Received illegal status byte during SysEx message; SysEx ignored");
        else
            puts("Received illegal status byte when data expected");
    }

    static void onSysExOverflow() {
        puts("Buffer overrun when receiving SysEx message; SysEx ignored");
    }

    enum class State {
        StatusByte,
        DataByte,
        SysExByte
    };

    void parseStatusByte(const uint8_t byte) {
        // Is it a status byte?
        if (byte & 0x80) {
            switch (byte) {
                // Invalid End of SysEx or undefined System Common message; ignore and clear running status
                case 0xF4:
                case 0xF5:
                case 0xF7:
                    messageBuffer[0] = 0;
                    return;

                // Start of SysEx message
                case 0xF0:
                    state = State::SysExByte;
                    break;

                // Tune Request - single byte, handle immediately and clear running status
                case 0xF6:
                    onShortMessage(byte);
                    messageBuffer[0] = 0;
                    break;

                // Channel or System Common message
                default:
                    state = State::DataByte;
                    break;
            }

            messageBuffer[messageLength++] = byte;
        }

        // Data byte, use Running Status if we've stored a status byte
        else if (messageBuffer[0]) {
            messageBuffer[1] = byte;
            messageLength = 2;

            // We could have a complete 2-byte message, otherwise wait for third byte
            if (!checkCompleteShortMessage())
                state = State::DataByte;
        }
    }

    bool checkCompleteShortMessage(const bool isIgnoreNoteOns = false) {
        const uint8_t status = messageBuffer[0];

        // MIDI message is complete if we receive 3 bytes,
        // or 2 bytes if it's a Program Change, Channel Pressure/Aftertouch, Time Code Quarter Frame, or Song Select
        if (messageLength == 3 ||
            (messageLength == 2 && ((status >= 0xC0 && status <= 0xDF) || status == 0xF1 || status == 0xF3))) {
            const bool isNoteOn = (status & 0xF0) == 0x90;

            if (!(isNoteOn && isIgnoreNoteOns))
                onShortMessage(prepareShortMessage());

            // Clear running status if System Common
            resetState(status >= 0xF1 && status <= 0xF7);
            return true;
        }

        return false;
    }

    [[nodiscard]] uint32_t prepareShortMessage() const {
        assert(messageLength == 2 || messageLength == 3);

        uint32_t message = 0;
        for (size_t i = 0; i < messageLength; ++i)
            message |= messageBuffer[i] << 8 * i;

        return message;
    }

    void resetState(const bool isClearStatusByte) {
        if (isClearStatusByte)
            messageBuffer[0] = 0;

        messageLength = 0;
        state = State::StatusByte;
    }

    Synth *synth;
    State state = State::StatusByte;
    // Matches mt32emu's SysEx buffer size
    uint8_t messageBuffer[1000] = {};
    size_t messageLength = 0;
};
