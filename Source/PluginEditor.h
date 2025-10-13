#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class MidiSequenceGeneratorAudioProcessor;

class MidiSequenceGeneratorAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    explicit MidiSequenceGeneratorAudioProcessorEditor (MidiSequenceGeneratorAudioProcessor&);
    ~MidiSequenceGeneratorAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MidiSequenceGeneratorAudioProcessor& processorRef_;

    juce::Slider stepsSlider_, swingSlider_, densitySlider_, rootSlider_, octavesSlider_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stepsAtt_, swingAtt_, densityAtt_, rootAtt_, octavesAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSequenceGeneratorAudioProcessorEditor)
};
