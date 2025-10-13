#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <type_traits>

MidiSequenceGeneratorAudioProcessor::MidiSequenceGeneratorAudioProcessor()
: juce::AudioProcessor (BusesProperties().withOutput("Out", juce::AudioChannelSet::stereo(), true))
{}

void MidiSequenceGeneratorAudioProcessor::prepareToPlay (double sampleRate, int) {
    sampleRate_ = sampleRate;
}

void MidiSequenceGeneratorAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MidiSequenceGeneratorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() != juce::AudioChannelSet::disabled();
}
#endif

void MidiSequenceGeneratorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    buffer.clear();

    if (auto* ph = getPlayHead()) {
        juce::AudioPlayHead::CurrentPositionInfo info;
        if (ph->getCurrentPosition(info)) {
            if (info.isPlaying && info.bpm > 0.0) {
                const double bpm = info.bpm;
                const int ppq = 480; // internal grid
                const int steps = params_.getRawParameterValue("steps")->load();
                const float swing = params_.getRawParameterValue("swing")->load();
                const float density = params_.getRawParameterValue("density")->load();
                const int root = (int) params_.getRawParameterValue("root")->load();
                const int octs = (int) params_.getRawParameterValue("octaves")->load();

                std::vector<int> major{0,2,4,5,7,9,11};
                generator_.configure(ppq, steps, root, major, swing, density, octs);

                const int numSamples = buffer.getNumSamples();
                const double samplesPerBeat = (sampleRate_ * 60.0) / bpm;
                const double startPPQ = info.ppqPosition; // at start of buffer (beats)
                const double endPPQ = startPPQ + (numSamples / samplesPerBeat); // beats

                // Generate one bar of pattern in PPQ ticks, then wrap per bar
                auto events = generator_.generateBars(1);
                const double ticksPerBeat = (double) ppq;

                for (const auto& ev : events) {
                    const double evStartBeat = (double) ev.startTick / ticksPerBeat; // beats into bar
                    const double evEndBeat = (double) (ev.startTick + ev.lengthTick) / ticksPerBeat;

                    const double barLenBeats = 4.0; // 4/4
                    const double barStart = std::floor(startPPQ / barLenBeats) * barLenBeats;
                    for (int rep = -1; rep <= 1; ++rep) {
                        const double absOn = barStart + rep * barLenBeats + evStartBeat;
                        const double absOff = barStart + rep * barLenBeats + evEndBeat;

                        if (absOn < endPPQ && absOff > startPPQ) {
                            const int onSample = (int) juce::jlimit(0, numSamples - 1, (int) std::round((absOn - startPPQ) * samplesPerBeat));
                            const int offSample = (int) juce::jlimit(0, numSamples - 1, (int) std::round((absOff - startPPQ) * samplesPerBeat));

                            if (onSample >= 0 && onSample < numSamples)
                                midi.addEvent(juce::MidiMessage::noteOn(1, ev.midiNote, (juce::uint8) juce::jlimit(1, 127, (int)std::round(ev.velocity * 127))), onSample);
                            if (offSample >= 0 && offSample < numSamples)
                                midi.addEvent(juce::MidiMessage::noteOff(1, ev.midiNote), offSample);
                        }
                    }
                }
            }
        }
    }
}

juce::AudioProcessorEditor* MidiSequenceGeneratorAudioProcessor::createEditor() {
    return new MidiSequenceGeneratorAudioProcessorEditor(*this);
}

void MidiSequenceGeneratorAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    auto state = params_.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void MidiSequenceGeneratorAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState)
        params_.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// This factory function is required by JUCE plugin clients
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new MidiSequenceGeneratorAudioProcessor();
}
