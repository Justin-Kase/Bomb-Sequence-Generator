// Minimal JUCE includes for CLI
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "SequenceGenerator.h"

int main(int argc, char** argv){

    int steps = 16;
    int bars = 2;
    int root = 60;
    float swing = 0.1f;
    float density = 0.7f;
    int octs = 1;
    juce::File outFile = juce::File::getCurrentWorkingDirectory().getChildFile("sequence.mid");

    for (int i = 1; i < argc; ++i) {
        juce::String arg = argv[i];
        if (arg == "--steps" && i+1 < argc) steps = juce::String(argv[++i]).getIntValue();
        else if (arg == "--bars" && i+1 < argc) bars = juce::String(argv[++i]).getIntValue();
        else if (arg == "--root" && i+1 < argc) root = juce::String(argv[++i]).getIntValue();
        else if (arg == "--swing" && i+1 < argc) swing = (float) juce::String(argv[++i]).getDoubleValue();
        else if (arg == "--density" && i+1 < argc) density = (float) juce::String(argv[++i]).getDoubleValue();
        else if (arg == "--octaves" && i+1 < argc) octs = juce::String(argv[++i]).getIntValue();
        else if (arg == "--out" && i+1 < argc) outFile = juce::File(argv[++i]);
    }

    SequenceGenerator gen;
    std::vector<int> major{0,2,4,5,7,9,11};
    gen.configure(480, steps, root, major, swing, density, octs);
    auto evs = gen.generateBars(bars);

    juce::MidiMessageSequence seq;
    for (auto& e : evs) {
        double t = e.startTick / 480.0; // seconds per beat = 1, i.e., 60bpm if exported raw
        seq.addEvent(juce::MidiMessage::noteOn(1, e.midiNote, (juce::uint8) juce::jlimit(1,127,(int)(e.velocity * 127))), t);
        seq.addEvent(juce::MidiMessage::noteOff(1, e.midiNote), (e.startTick + e.lengthTick) / 480.0);
    }

    juce::FileOutputStream fos(outFile);
    if (!fos.openedOk()) { juce::Logger::writeToLog("Failed to open output file"); return 1; }
    juce::MidiFile mf; mf.setTicksPerQuarterNote(480);
    mf.addTrack(seq);
    mf.writeTo(fos);

    juce::Logger::writeToLog("Wrote " + outFile.getFullPathName());
    return 0;
}
