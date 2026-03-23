#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <BinaryData.h>
#include "SequenceGenerator.h"

class MidiSequenceGeneratorAudioProcessor;

// ─── Custom look-and-feel ─────────────────────────────────────────────────────
class SeqLookAndFeel : public juce::LookAndFeel_V4 {
public:
    SeqLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool isDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};

// ─── A single labelled knob ───────────────────────────────────────────────────
class LabelledKnob : public juce::Component {
public:
    juce::Slider    slider;
    juce::Label     label;

    explicit LabelledKnob(const juce::String& name);
    void resized() override;
};

// ─── Step grid display ────────────────────────────────────────────────────────
class StepGrid : public juce::Component {
public:
    void setSteps(const std::vector<StepData>& steps, int playStep);
    void paint(juce::Graphics&) override;

private:
    std::vector<StepData> steps_;
    int playStep_ = -1;
};

// ─── Main editor ─────────────────────────────────────────────────────────────
class MidiSequenceGeneratorAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit MidiSequenceGeneratorAudioProcessorEditor(MidiSequenceGeneratorAudioProcessor&);
    ~MidiSequenceGeneratorAudioProcessorEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    MidiSequenceGeneratorAudioProcessor& proc_;
    SeqLookAndFeel laf_;

    // Logo
    juce::Image logoImage_;

    // Step grid
    StepGrid stepGrid_;

    // Knobs
    LabelledKnob stepsKnob_   {"Steps"};
    LabelledKnob swingKnob_   {"Swing"};
    LabelledKnob densityKnob_ {"Density"};
    LabelledKnob rootKnob_    {"Root"};
    LabelledKnob octavesKnob_ {"Octaves"};
    LabelledKnob seedKnob_    {"Seed"};

    // Scale selector
    juce::Label    scaleLabel_;
    juce::ComboBox scaleBox_;

    // APVTS attachments
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAtt> stepsAtt_, swingAtt_, densityAtt_, rootAtt_, octavesAtt_, seedAtt_;
    std::unique_ptr<ComboAtt>  scaleAtt_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiSequenceGeneratorAudioProcessorEditor)
};
