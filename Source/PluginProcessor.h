#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SequenceGenerator.h"

class MidiSequenceGeneratorAudioProcessor : public juce::AudioProcessor {
public:
    MidiSequenceGeneratorAudioProcessor();
    ~MidiSequenceGeneratorAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return JucePlugin_WantsMidiInput; }
    bool producesMidi() const override { return JucePlugin_ProducesMidiOutput; }
    bool isMidiEffect() const override { return JucePlugin_IsMidiEffect; }

    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& parameters() { return params_; }

private:
    juce::AudioProcessorValueTreeState params_ {
        *this, nullptr, "Params", {
            std::make_unique<juce::AudioParameterInt>("steps", "Steps", 4, 32, 16),
            std::make_unique<juce::AudioParameterFloat>("swing", "Swing", juce::NormalisableRange<float>{0.f, 0.5f}, 0.1f),
            std::make_unique<juce::AudioParameterFloat>("density", "Density", juce::NormalisableRange<float>{0.f, 1.f}, 0.7f),
            std::make_unique<juce::AudioParameterInt>("root", "Root", 36, 84, 60),
            std::make_unique<juce::AudioParameterInt>("octaves", "Octaves", 0, 3, 1)
        }
    };

    SequenceGenerator generator_;
    // transport
    double sampleRate_ = 44100.0;
    double lastPosPPQ_ = 0.0; // host position

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSequenceGeneratorAudioProcessor)
};
