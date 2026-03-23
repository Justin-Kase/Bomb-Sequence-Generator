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
#include <string>
#include "SequenceGenerator.h"
#include "MidiExporter.h"

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

    MidiExporter::writeMidi(outFile, events, 480);
    std::cout << "Written " << events.size() << " notes to " << outFile << "\n";
    return 0;
}
