#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SequenceGenerator.h"
#include <atomic>
#include <vector>
#include <mutex>

class IllbombSeqGeneratorAudioProcessor : public juce::AudioProcessor {
public:
    IllbombSeqGeneratorAudioProcessor();
    ~IllbombSeqGeneratorAudioProcessor() override = default;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout&) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return JucePlugin_WantsMidiInput;    }
    bool producesMidi() const override { return JucePlugin_ProducesMidiOutput; }
    bool isMidiEffect() const override { return JucePlugin_IsMidiEffect;       }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()                        override { return 1; }
    int  getCurrentProgram()                     override { return 0; }
    void setCurrentProgram (int)                 override {}
    const juce::String getProgramName (int)      override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& parameters() { return params_; }

    // Called by the editor to get current step pattern for display
    std::vector<StepData> getStepPattern() const;

    // Current playback step index (-1 = not playing)
    int getCurrentPlayStep() const { return currentPlayStep_.load(); }

private:
    juce::AudioProcessorValueTreeState params_ {
        *this, nullptr, "Params",
        juce::AudioProcessorValueTreeState::ParameterLayout {
            std::make_unique<juce::AudioParameterInt>  ("steps",   "Steps",   4,  32,  16),
            std::make_unique<juce::AudioParameterFloat>("swing",   "Swing",
                juce::NormalisableRange<float>{0.f, 0.5f}, 0.1f),
            std::make_unique<juce::AudioParameterFloat>("density", "Density",
                juce::NormalisableRange<float>{0.f, 1.f},  0.7f),
            std::make_unique<juce::AudioParameterInt>  ("root",    "Root",    36, 84,  60),
            std::make_unique<juce::AudioParameterInt>  ("octaves", "Octaves", 0,  3,   1),
            std::make_unique<juce::AudioParameterInt>  ("scale",   "Scale",   0,
                (int)getAllScales().size() - 1, 0),
            std::make_unique<juce::AudioParameterInt>  ("seed",    "Seed",    0,  999, 42),
        }
    };

    SequenceGenerator generator_;
    double sampleRate_   = 44100.0;

    std::atomic<int> currentPlayStep_ { -1 };

    // Cached step pattern for UI (updated in processBlock)
    mutable std::mutex patternMutex_;
    std::vector<StepData> cachedPattern_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IllbombSeqGeneratorAudioProcessor)
};
