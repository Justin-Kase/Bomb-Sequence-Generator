#pragma once
#include "SequenceGenerator.h"
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>
#include <algorithm>

/**
 * Minimal SMF (Standard MIDI File) writer — Format 0, single track.
 * Shared between the plugin UI export button and the `midigen` CLI.
 */
namespace MidiExporter {

namespace detail {
    inline void writeVarLen(std::vector<uint8_t>& out, uint32_t v) {
        uint8_t buf[4]; int n = 0;
        buf[n++] = v & 0x7F;
        while ((v >>= 7)) buf[n++] = 0x80 | (v & 0x7F);
        for (int i = n - 1; i >= 0; --i) out.push_back(buf[i]);
    }
    inline void writeBE16(std::vector<uint8_t>& out, uint16_t v) {
        out.push_back((v >> 8) & 0xFF); out.push_back(v & 0xFF);
    }
    inline void writeBE32(std::vector<uint8_t>& out, uint32_t v) {
        out.push_back((v >> 24) & 0xFF); out.push_back((v >> 16) & 0xFF);
        out.push_back((v >>  8) & 0xFF); out.push_back(v & 0xFF);
    }
}

/**
 * Write a vector of NoteEvents to a .mid file.
 * @param path      Destination file path
 * @param events    Note events (absolute ticks)
 * @param ppq       Pulses per quarter note (default 480)
 * @param bpm       Tempo in beats per minute (default 120)
 */
inline bool writeMidi(const std::string& path,
                      const std::vector<NoteEvent>& events,
                      int ppq     = 480,
                      double bpm  = 120.0)
{
    using namespace detail;

    const uint32_t tempoUs = (uint32_t)(60'000'000.0 / bpm);

    std::vector<uint8_t> track;

    // Tempo meta-event
    track.insert(track.end(), {0x00, 0xFF, 0x51, 0x03});
    track.push_back((tempoUs >> 16) & 0xFF);
    track.push_back((tempoUs >>  8) & 0xFF);
    track.push_back( tempoUs        & 0xFF);

    // Expand note events into on/off pairs, sorted by tick
    struct Ev { uint32_t tick; bool on; int note; int vel; };
    std::vector<Ev> raw;
    raw.reserve(events.size() * 2);
    for (auto& e : events) {
        raw.push_back({ e.startTick,               true,  e.midiNote, (int)(e.velocity * 127.f) });
        raw.push_back({ e.startTick + e.lengthTick, false, e.midiNote, 0 });
    }
    std::sort(raw.begin(), raw.end(), [](const Ev& a, const Ev& b) {
        return a.tick != b.tick ? a.tick < b.tick : (int)a.on < (int)b.on;
    });

    uint32_t cursor = 0;
    for (auto& ev : raw) {
        writeVarLen(track, ev.tick - cursor);
        cursor = ev.tick;
        if (ev.on) {
            int vel = std::max(1, std::min(127, ev.vel));
            track.push_back(0x90);
            track.push_back((uint8_t)ev.note);
            track.push_back((uint8_t)vel);
        } else {
            track.push_back(0x80);
            track.push_back((uint8_t)ev.note);
            track.push_back(0x00);
        }
    }
    // End of track
    track.insert(track.end(), {0x00, 0xFF, 0x2F, 0x00});

    // Assemble SMF
    std::vector<uint8_t> file;
    file.insert(file.end(), {'M','T','h','d'});
    writeBE32(file, 6);
    writeBE16(file, 0);   // format 0
    writeBE16(file, 1);   // 1 track
    writeBE16(file, (uint16_t)ppq);
    file.insert(file.end(), {'M','T','r','k'});
    writeBE32(file, (uint32_t)track.size());
    file.insert(file.end(), track.begin(), track.end());

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(file.data()), (std::streamsize)file.size());
    return f.good();
}

} // namespace MidiExporter
