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
BombSeqGeneratorAudioProcessorEditor::BombSeqGeneratorAudioProcessorEditor(
    BombSeqGeneratorAudioProcessor& p)
: juce::AudioProcessorEditor(&p), proc_(p)
{
    setLookAndFeel(&laf_);
    setResizable(true, true);
    setSize(580, 345);

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

    // Export button
    exportBtn_.setColour(juce::TextButton::buttonColourId,   Col::panel);
    exportBtn_.setColour(juce::TextButton::buttonOnColourId,  Col::active.withAlpha(0.3f));
    exportBtn_.setColour(juce::TextButton::textColourOffId,   Col::active);
    exportBtn_.setColour(juce::TextButton::textColourOnId,    Col::active);
    exportBtn_.onClick = [this] { exportMidi(); };
    addAndMakeVisible(exportBtn_);

    exportStatus_.setFont(juce::Font(10.f));
    exportStatus_.setColour(juce::Label::textColourId, Col::textDim);
    exportStatus_.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(exportStatus_);

    // Load logo from binary resources
    int logoDataSize = 0;
    auto logoData = BinaryData::getNamedResource("logo_png", logoDataSize);
    if (logoData != nullptr && logoDataSize > 0)
        logoImage_ = juce::ImageFileFormat::loadFrom(logoData, logoDataSize);

    startTimerHz(20);
}

BombSeqGeneratorAudioProcessorEditor::~BombSeqGeneratorAudioProcessorEditor() {
    setLookAndFeel(nullptr);
    stopTimer();
}

void BombSeqGeneratorAudioProcessorEditor::exportMidi() {
    auto& params   = proc_.parameters();
    const int   steps   = (int)params.getRawParameterValue("steps")->load();
    const float swing   = params.getRawParameterValue("swing")->load();
    const float density = params.getRawParameterValue("density")->load();
    const int   root    = (int)params.getRawParameterValue("root")->load();
    const int   octs    = (int)params.getRawParameterValue("octaves")->load();
    const int   scaleIdx= (int)params.getRawParameterValue("scale")->load();
    const int   seed    = (int)params.getRawParameterValue("seed")->load();

    const auto& scaleInfo = getScale(scaleIdx);

    juce::String defaultName = juce::String("bomb_seq_")
        + juce::String(scaleInfo.name).replace(". ", "_").replace(" ", "_").toLowerCase()
        + "_seed" + juce::String(seed) + ".mid";

    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Export MIDI clip",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
            .getChildFile(defaultName),
        "*.mid");

    fileChooser_->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, steps, swing, density, root, octs, scaleInfo, seed](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result == juce::File{}) return;

            SequenceGenerator gen;
            gen.configure(480, steps, root, scaleInfo.semitones, swing, density, octs, seed);
            auto events = gen.generateBars(4);

            bool ok = MidiExporter::writeMidi(result.getFullPathName().toStdString(), events, 480);
            exportStatus_.setText(ok ? juce::String(juce::CharPointer_UTF8("\xe2\x9c\x93")) + " " + result.getFileName()
                                     : juce::String(juce::CharPointer_UTF8("\xe2\x9c\x97")) + " Export failed",
                                  juce::dontSendNotification);

            juce::Timer::callAfterDelay(4000, [this] {
                exportStatus_.setText("", juce::dontSendNotification);
            });
        });
}

void BombSeqGeneratorAudioProcessorEditor::timerCallback() {
    auto pattern = proc_.getStepPattern();
    int  playStep = proc_.getCurrentPlayStep();
    stepGrid_.setSteps(pattern, playStep);
}

void BombSeqGeneratorAudioProcessorEditor::paint(juce::Graphics& g) {
    // Background
    g.fillAll(Col::bg);

    const int headerH = 36;
    auto headerBounds = getLocalBounds().removeFromTop(headerH);

    // Logo (top-right, matching BombSeq placement)
    if (logoImage_.isValid()) {
        const int logoSize = 26;
        const int logoX    = getWidth() - logoSize - 12;
        const int logoY    = (headerH - logoSize) / 2;

        // Subtle white circle backing
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillEllipse((float)(logoX - 2), (float)(logoY - 2),
                      (float)(logoSize + 4), (float)(logoSize + 4));

        g.drawImage(logoImage_, logoX, logoY, logoSize, logoSize,
                    0, 0, logoImage_.getWidth(), logoImage_.getHeight());
    }

    // Title
    g.setColour(Col::accent);
    g.setFont(juce::Font(16.f, juce::Font::bold));
    g.drawText("MIDI SEQ GEN", headerBounds.withTrimmedLeft(16),
               juce::Justification::centredLeft);

    // Version (just left of the logo)
    g.setColour(Col::textDim);
    g.setFont(juce::Font(10.f));
    auto versionBounds = getLocalBounds().removeFromTop(headerH).withTrimmedRight(logoImage_.isValid() ? 50 : 8);
    g.drawText("v0.2.0", versionBounds, juce::Justification::centredRight);
}

void BombSeqGeneratorAudioProcessorEditor::resized() {
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

    // Export row
    area.removeFromTop(8);
    auto exportRow = area.removeFromTop(30);
    exportBtn_.setBounds(exportRow.removeFromLeft(160));
    exportStatus_.setBounds(exportRow.withTrimmedLeft(8));
}
