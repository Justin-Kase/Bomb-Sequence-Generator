/**
 * midigen CLI — export MIDI files without a DAW
 *
 * Usage:
 *   midigen --steps 16 --bars 4 --root 60 --swing 0.1 --density 0.7
 *           --octaves 1 --scale 0 --seed 42 --out sequence.mid
 *
 * Scales (--scale N):
 *   0=Major  1=Minor  2=Pent.Major  3=Pent.Minor  4=Blues
 *   5=Dorian  6=Phrygian  7=Lydian  8=Mixolydian  9=Locrian
 */
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include "SequenceGenerator.h"

// ─── Minimal MIDI writer ─────────────────────────────────────────────────────
static void writeVarLen(std::vector<uint8_t>& out, uint32_t v) {
    uint8_t buf[4]; int n = 0;
    buf[n++] = v & 0x7F;
    while ((v >>= 7)) { buf[n++] = 0x80 | (v & 0x7F); }
    for (int i = n - 1; i >= 0; --i) out.push_back(buf[i]);
}

static void writeBE16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back((v >> 8) & 0xFF);
    out.push_back(v & 0xFF);
}

static void writeBE32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back((v >> 24) & 0xFF); out.push_back((v >> 16) & 0xFF);
    out.push_back((v >>  8) & 0xFF); out.push_back(v & 0xFF);
}

static void writeMidi(const std::string& path, const std::vector<NoteEvent>& events,
                       int ppq, int tempo_us = 500000) {
    std::vector<uint8_t> track;
    // Tempo event
    track.insert(track.end(), {0x00, 0xFF, 0x51, 0x03});
    track.push_back((tempo_us >> 16) & 0xFF);
    track.push_back((tempo_us >>  8) & 0xFF);
    track.push_back( tempo_us        & 0xFF);

    struct Ev { uint32_t tick; bool on; int note; int vel; };
    std::vector<Ev> raw;
    for (auto& e : events) {
        raw.push_back({e.startTick,              true,  e.midiNote, (int)(e.velocity * 127)});
        raw.push_back({e.startTick + e.lengthTick, false, e.midiNote, 0});
    }
    std::sort(raw.begin(), raw.end(), [](const Ev& a, const Ev& b){ return a.tick < b.tick; });

    uint32_t cursor = 0;
    for (auto& ev : raw) {
        writeVarLen(track, ev.tick - cursor);
        cursor = ev.tick;
        if (ev.on) {
            track.push_back(0x90); track.push_back((uint8_t)ev.note); track.push_back((uint8_t)ev.vel);
        } else {
            track.push_back(0x80); track.push_back((uint8_t)ev.note); track.push_back(0x00);
        }
    }
    // End of track
    track.insert(track.end(), {0x00, 0xFF, 0x2F, 0x00});

    std::vector<uint8_t> file;
    // Header chunk
    file.insert(file.end(), {'M','T','h','d'});
    writeBE32(file, 6); writeBE16(file, 0); writeBE16(file, 1); writeBE16(file, (uint16_t)ppq);
    // Track chunk
    file.insert(file.end(), {'M','T','r','k'});
    writeBE32(file, (uint32_t)track.size());
    file.insert(file.end(), track.begin(), track.end());

    std::ofstream f(path, std::ios::binary);
    f.write((char*)file.data(), (std::streamsize)file.size());
}

// ─── main ────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    int         steps    = 16;
    int         bars     = 4;
    int         root     = 60;
    float       swing    = 0.0f;
    float       density  = 0.7f;
    int         octaves  = 1;
    int         scaleIdx = 0;
    int         seed     = 42;
    std::string outFile  = "sequence.mid";

    for (int i = 1; i < argc - 1; ++i) {
        std::string a = argv[i];
        if      (a == "--steps"  ) steps    = std::stoi(argv[++i]);
        else if (a == "--bars"   ) bars     = std::stoi(argv[++i]);
        else if (a == "--root"   ) root     = std::stoi(argv[++i]);
        else if (a == "--swing"  ) swing    = std::stof(argv[++i]);
        else if (a == "--density") density  = std::stof(argv[++i]);
        else if (a == "--octaves") octaves  = std::stoi(argv[++i]);
        else if (a == "--scale"  ) scaleIdx = std::stoi(argv[++i]);
        else if (a == "--seed"   ) seed     = std::stoi(argv[++i]);
        else if (a == "--out"    ) outFile  = argv[++i];
    }

    const auto& scaleInfo = getScale(scaleIdx);
    std::cout << "Scale: " << scaleInfo.name << " | Seed: " << seed << " | Steps: " << steps
              << " | Root: " << root << " | Density: " << density << "\n";

    SequenceGenerator gen;
    gen.configure(480, steps, root, scaleInfo.semitones, swing, density, octaves, seed);
    auto events = gen.generateBars(bars);

    writeMidi(outFile, events, 480);
    std::cout << "Written " << events.size() << " notes to " << outFile << "\n";
    return 0;
}
