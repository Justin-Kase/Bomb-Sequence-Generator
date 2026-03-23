#include "PluginProcessor.h"
#include "PluginEditor.h"

IllbombSeqGeneratorAudioProcessor::IllbombSeqGeneratorAudioProcessor()
: juce::AudioProcessor (BusesProperties().withOutput("Out", juce::AudioChannelSet::stereo(), true))
{}

void IllbombSeqGeneratorAudioProcessor::prepareToPlay (double sampleRate, int) {
    sampleRate_ = sampleRate;
}

void IllbombSeqGeneratorAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool IllbombSeqGeneratorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() != juce::AudioChannelSet::disabled();
}
#endif

void IllbombSeqGeneratorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                         juce::MidiBuffer& midi) {
    buffer.clear();

    auto* ph = getPlayHead();
    if (!ph) return;

    juce::AudioPlayHead::CurrentPositionInfo info;
    if (!ph->getCurrentPosition(info)) return;
    if (!info.isPlaying || info.bpm <= 0.0) {
        currentPlayStep_.store(-1);
        return;
    }

    const double bpm        = info.bpm;
    const int    ppq        = 480;
    const int    steps      = params_.getRawParameterValue("steps")  ->load();
    const float  swing      = params_.getRawParameterValue("swing")  ->load();
    const float  density    = params_.getRawParameterValue("density")->load();
    const int    root       = (int) params_.getRawParameterValue("root")   ->load();
    const int    octs       = (int) params_.getRawParameterValue("octaves")->load();
    const int    scaleIdx   = (int) params_.getRawParameterValue("scale")  ->load();
    const int    seed       = (int) params_.getRawParameterValue("seed")   ->load();

    const auto& scaleInfo = getScale(scaleIdx);
    generator_.configure(ppq, steps, root, scaleInfo.semitones, swing, density, octs, seed);

    // Update cached pattern for UI
    {
        std::lock_guard<std::mutex> lk(patternMutex_);
        cachedPattern_ = generator_.getStepPattern();
    }

    const int    numSamples    = buffer.getNumSamples();
    const double samplesPerBeat = (sampleRate_ * 60.0) / bpm;
    const double startPPQ       = info.ppqPosition;
    const double endPPQ         = startPPQ + (numSamples / samplesPerBeat);
    const double barLenBeats    = 4.0;   // 4/4

    // Current play-step for display
    const double beatInBar = std::fmod(startPPQ, barLenBeats);
    const double stepsPerBar = (double)steps;
    currentPlayStep_.store((int)std::floor(beatInBar / barLenBeats * stepsPerBar) % steps);

    auto events = generator_.generateBars(1);
    const double ticksPerBeat = (double) ppq;

    for (const auto& ev : events) {
        const double evStartBeat = ev.startTick / ticksPerBeat;
        const double evEndBeat   = (ev.startTick + ev.lengthTick) / ticksPerBeat;
        const double barStart    = std::floor(startPPQ / barLenBeats) * barLenBeats;

        for (int rep = -1; rep <= 1; ++rep) {
            const double absOn  = barStart + rep * barLenBeats + evStartBeat;
            const double absOff = barStart + rep * barLenBeats + evEndBeat;

            if (absOn < endPPQ && absOff > startPPQ) {
                auto clamp = [&](double t) {
                    return (int) juce::jlimit(0, numSamples - 1,
                        (int) std::round((t - startPPQ) * samplesPerBeat));
                };
                int onSample  = clamp(absOn);
                int offSample = clamp(absOff);
                int vel       = juce::jlimit(1, 127, (int)std::round(ev.velocity * 127));

                midi.addEvent(juce::MidiMessage::noteOn (1, ev.midiNote, (juce::uint8)vel), onSample);
                midi.addEvent(juce::MidiMessage::noteOff(1, ev.midiNote),                   offSample);
            }
        }
    }
}

std::vector<StepData> IllbombSeqGeneratorAudioProcessor::getStepPattern() const {
    std::lock_guard<std::mutex> lk(patternMutex_);
    return cachedPattern_;
}

juce::AudioProcessorEditor* IllbombSeqGeneratorAudioProcessor::createEditor() {
    return new IllbombSeqGeneratorAudioProcessorEditor(*this);
}

void IllbombSeqGeneratorAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    auto state = params_.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void IllbombSeqGeneratorAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary(data, sizeInBytes));
    if (xmlState)
        params_.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new IllbombSeqGeneratorAudioProcessor();
}
