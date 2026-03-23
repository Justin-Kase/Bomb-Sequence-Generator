#include "PluginEditor.h"
#include "PluginProcessor.h"

// ─── Colours ──────────────────────────────────────────────────────────────────
namespace Col {
    const juce::Colour bg       { 0xFF0F0F1A };
    const juce::Colour panel    { 0xFF1A1A2E };
    const juce::Colour accent   { 0xFF4FC3F7 };  // sky blue
    const juce::Colour active   { 0xFF00E676 };  // green – active step
    const juce::Colour inactive { 0xFF263040 };  // dim – inactive step
    const juce::Colour play     { 0xFFFFD740 };  // amber – playhead
    const juce::Colour knobBg   { 0xFF22304A };
    const juce::Colour knobArc  { 0xFF4FC3F7 };
    const juce::Colour text     { 0xFFCFD8DC };
    const juce::Colour textDim  { 0xFF546E7A };
}

// ─── SeqLookAndFeel ──────────────────────────────────────────────────────────
SeqLookAndFeel::SeqLookAndFeel() {
    setColour(juce::Slider::thumbColourId,            Col::accent);
    setColour(juce::Slider::rotarySliderFillColourId,  Col::knobArc);
    setColour(juce::Slider::textBoxTextColourId,       Col::text);
    setColour(juce::Slider::textBoxBackgroundColourId, Col::panel);
    setColour(juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId,               Col::text);
    setColour(juce::ComboBox::backgroundColourId,      Col::panel);
    setColour(juce::ComboBox::textColourId,            Col::text);
    setColour(juce::ComboBox::outlineColourId,         Col::textDim);
    setColour(juce::ComboBox::arrowColourId,           Col::accent);
    setColour(juce::PopupMenu::backgroundColourId,     Col::panel);
    setColour(juce::PopupMenu::textColourId,           Col::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Col::accent.withAlpha(0.3f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

void SeqLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                       int x, int y, int w, int h,
                                       float sliderPos,
                                       float startAngle, float endAngle,
                                       juce::Slider&) {
    const float cx   = x + w * 0.5f;
    const float cy   = y + h * 0.5f;
    const float r    = std::min(w, h) * 0.38f;
    const float thick = r * 0.18f;

    // Background track
    juce::Path bgArc;
    bgArc.addCentredArc(cx, cy, r, r, 0.f, startAngle, endAngle, true);
    g.setColour(Col::knobBg);
    g.strokePath(bgArc, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    const float valueAngle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Path valArc;
    valArc.addCentredArc(cx, cy, r, r, 0.f, startAngle, valueAngle, true);
    g.setColour(Col::knobArc);
    g.strokePath(valArc, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Centre dot
    g.setColour(Col::accent);
    g.fillEllipse(cx - 4.f, cy - 4.f, 8.f, 8.f);

    // Pointer line
    const float px = cx + (r - thick) * std::sin(valueAngle);
    const float py = cy - (r - thick) * std::cos(valueAngle);
    g.setColour(juce::Colours::white);
    g.drawLine(cx, cy, px, py, 2.f);
}

void SeqLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool /*isDown*/,
                                   int bx, int by, int bw, int bh, juce::ComboBox& box) {
    g.setColour(Col::panel);
    g.fillRoundedRectangle(0, 0, (float)w, (float)h, 6.f);
    g.setColour(Col::textDim);
    g.drawRoundedRectangle(0.5f, 0.5f, w - 1.f, h - 1.f, 6.f, 1.f);

    // Arrow
    const float arrowCX = bx + bw * 0.5f;
    const float arrowCY = by + bh * 0.5f;
    juce::Path arrow;
    arrow.addTriangle(arrowCX - 5.f, arrowCY - 2.f,
                      arrowCX + 5.f, arrowCY - 2.f,
                      arrowCX,       arrowCY + 4.f);
    g.setColour(Col::accent);
    g.fillPath(arrow);
}

void SeqLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label) {
    label.setBounds(6, 0, box.getWidth() - 30, box.getHeight());
    label.setFont(juce::Font(13.f));
}

// ─── LabelledKnob ────────────────────────────────────────────────────────────
LabelledKnob::LabelledKnob(const juce::String& name) {
    slider.setSliderStyle(juce::Slider::Rotary);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(11.f, juce::Font::bold));
    addAndMakeVisible(label);
}

void LabelledKnob::resized() {
    auto b = getLocalBounds();
    label .setBounds(b.removeFromBottom(16));
    slider.setBounds(b);
}

// ─── StepGrid ────────────────────────────────────────────────────────────────
void StepGrid::setSteps(const std::vector<StepData>& steps, int playStep) {
    steps_    = steps;
    playStep_ = playStep;
    repaint();
}

void StepGrid::paint(juce::Graphics& g) {
    if (steps_.empty()) {
        g.setColour(Col::inactive);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.f);
        return;
    }

    const int n    = (int)steps_.size();
    const float W  = getWidth();
    const float H  = getHeight();
    const float gap = 3.f;
    const float cellW = (W - gap * (n - 1)) / n;

    for (int i = 0; i < n; ++i) {
        const float x = i * (cellW + gap);
        juce::Rectangle<float> cell(x, 0.f, cellW, H);

        if (steps_[i].active) {
            // velocity-driven brightness
            const float v = steps_[i].velocity;
            g.setColour(Col::active.withMultipliedBrightness(0.6f + v * 0.4f));
        } else {
            g.setColour(Col::inactive);
        }
        g.fillRoundedRectangle(cell, 4.f);

        // Playhead highlight
        if (i == playStep_) {
            g.setColour(Col::play.withAlpha(0.55f));
            g.fillRoundedRectangle(cell, 4.f);
            g.setColour(Col::play);
            g.drawRoundedRectangle(cell.reduced(1.f), 4.f, 1.5f);
        }
    }
}

// ─── Main Editor ─────────────────────────────────────────────────────────────
MidiSequenceGeneratorAudioProcessorEditor::MidiSequenceGeneratorAudioProcessorEditor(
    MidiSequenceGeneratorAudioProcessor& p)
: juce::AudioProcessorEditor(&p), proc_(p)
{
    setLookAndFeel(&laf_);
    setResizable(true, true);
    setSize(580, 310);

    auto& params = proc_.parameters();

    // Wire up knobs
    stepsAtt_   = std::make_unique<SliderAtt>(params, "steps",   stepsKnob_.slider);
    swingAtt_   = std::make_unique<SliderAtt>(params, "swing",   swingKnob_.slider);
    densityAtt_ = std::make_unique<SliderAtt>(params, "density", densityKnob_.slider);
    rootAtt_    = std::make_unique<SliderAtt>(params, "root",    rootKnob_.slider);
    octavesAtt_ = std::make_unique<SliderAtt>(params, "octaves", octavesKnob_.slider);
    seedAtt_    = std::make_unique<SliderAtt>(params, "seed",    seedKnob_.slider);

    addAndMakeVisible(stepsKnob_);
    addAndMakeVisible(swingKnob_);
    addAndMakeVisible(densityKnob_);
    addAndMakeVisible(rootKnob_);
    addAndMakeVisible(octavesKnob_);
    addAndMakeVisible(seedKnob_);

    // Scale combo
    scaleLabel_.setText("Scale", juce::dontSendNotification);
    scaleLabel_.setJustificationType(juce::Justification::centred);
    scaleLabel_.setFont(juce::Font(11.f, juce::Font::bold));
    addAndMakeVisible(scaleLabel_);

    for (int i = 0; i < (int)getAllScales().size(); ++i)
        scaleBox_.addItem(getAllScales()[i].name, i + 1);

    scaleAtt_ = std::make_unique<ComboAtt>(params, "scale", scaleBox_);
    addAndMakeVisible(scaleBox_);

    // Step grid
    addAndMakeVisible(stepGrid_);

    startTimerHz(20);
}

MidiSequenceGeneratorAudioProcessorEditor::~MidiSequenceGeneratorAudioProcessorEditor() {
    setLookAndFeel(nullptr);
    stopTimer();
}

void MidiSequenceGeneratorAudioProcessorEditor::timerCallback() {
    auto pattern = proc_.getStepPattern();
    int  playStep = proc_.getCurrentPlayStep();
    stepGrid_.setSteps(pattern, playStep);
}

void MidiSequenceGeneratorAudioProcessorEditor::paint(juce::Graphics& g) {
    // Background
    g.fillAll(Col::bg);

    // Title
    g.setColour(Col::accent);
    g.setFont(juce::Font(16.f, juce::Font::bold));
    g.drawText("MIDI SEQ GEN", getLocalBounds().removeFromTop(36).withTrimmedLeft(16),
               juce::Justification::centredLeft);

    // Version
    g.setColour(Col::textDim);
    g.setFont(juce::Font(10.f));
    g.drawText("v0.2.0", getLocalBounds().removeFromTop(36),
               juce::Justification::centredRight);
}

void MidiSequenceGeneratorAudioProcessorEditor::resized() {
    auto area = getLocalBounds().reduced(12);

    // Title row
    area.removeFromTop(30);

    // Step grid
    stepGrid_.setBounds(area.removeFromTop(52));
    area.removeFromTop(10);

    // Knob row
    auto knobRow = area.removeFromTop(130);
    const int knobW = knobRow.getWidth() / 7;

    stepsKnob_  .setBounds(knobRow.removeFromLeft(knobW));
    swingKnob_  .setBounds(knobRow.removeFromLeft(knobW));
    densityKnob_.setBounds(knobRow.removeFromLeft(knobW));
    rootKnob_   .setBounds(knobRow.removeFromLeft(knobW));
    octavesKnob_.setBounds(knobRow.removeFromLeft(knobW));
    seedKnob_   .setBounds(knobRow.removeFromLeft(knobW));

    // Scale selector fills remaining width
    auto scaleCol = knobRow;
    scaleLabel_.setBounds(scaleCol.removeFromBottom(16));
    scaleBox_  .setBounds(scaleCol.reduced(4));
}
