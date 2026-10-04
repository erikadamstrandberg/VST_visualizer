#include "PluginProcessor.h"
#include "PluginEditor.h"

ErikSatAudioProcessorEditor::ErikSatAudioProcessorEditor(ErikSatAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Apply custom look and feel
    setLookAndFeel(&customLookAndFeel);

    // Add Curve Visualizer
    addAndMakeVisible(curveVisualizer);

    // Setup Drive Slider
    driveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 18);
    driveSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(driveSlider);

    driveLabel.setText("DRIVE", juce::dontSendNotification);
    driveLabel.setJustificationType(juce::Justification::centred);
    driveLabel.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    driveLabel.setColour(juce::Label::textColourId, juce::Colour(0xff99a0b8));
    addAndMakeVisible(driveLabel);

    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "drive", driveSlider);

    // Setup Saturation Type ComboBox
    typeBox.addItem("Tape", 1);
    typeBox.addItem("Tube", 2);
    typeBox.addItem("Hard Clip", 3);
    typeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1f212b));
    typeBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff2d3142));
    typeBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    addAndMakeVisible(typeBox);

    typeLabel.setText("TYPE", juce::dontSendNotification);
    typeLabel.setJustificationType(juce::Justification::centred);
    typeLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    typeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff99a0b8));
    addAndMakeVisible(typeLabel);

    typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "type", typeBox);

    // Setup Character Slider
    characterSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    characterSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 18);
    addAndMakeVisible(characterSlider);

    characterLabel.setText("CHARACTER", juce::dontSendNotification);
    characterLabel.setJustificationType(juce::Justification::centred);
    characterLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    characterLabel.setColour(juce::Label::textColourId, juce::Colour(0xff99a0b8));
    addAndMakeVisible(characterLabel);

    characterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "character", characterSlider);

    // Setup Mix Slider
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 18);
    mixSlider.setTextValueSuffix(" %");
    addAndMakeVisible(mixSlider);

    mixLabel.setText("MIX", juce::dontSendNotification);
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    mixLabel.setColour(juce::Label::textColourId, juce::Colour(0xff99a0b8));
    addAndMakeVisible(mixLabel);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "mix", mixSlider);

    // Setup Output Slider
    outputSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    outputSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(outputSlider);

    outputLabel.setText("OUTPUT", juce::dontSendNotification);
    outputLabel.setJustificationType(juce::Justification::centred);
    outputLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    outputLabel.setColour(juce::Label::textColourId, juce::Colour(0xff99a0b8));
    addAndMakeVisible(outputLabel);

    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "output", outputSlider);

    // Window dimensions
    setSize(520, 440);

    // 60 FPS update timer for visualizer curve & peak ballistics
    startTimerHz(60);
}

ErikSatAudioProcessorEditor::~ErikSatAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ErikSatAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background gradient
    juce::ColourGradient bgGradient(juce::Colour(0xff15161c), 0, 0,
                                    juce::Colour(0xff0d0e12), 0, static_cast<float>(getHeight()), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Top Title Bar
    g.setColour(juce::Colour(0xff1b1d25));
    g.fillRect(0, 0, getWidth(), 46);

    // Accent line below title bar
    juce::ColourGradient titleLine(juce::Colour(0xff00d4ff), 0, 46,
                                  juce::Colour(0xffff5577), static_cast<float>(getWidth()), 46, false);
    g.setGradientFill(titleLine);
    g.fillRect(0, 45, getWidth(), 2);

    // Title text
    g.setFont(juce::FontOptions(18.0f).withStyle("Bold"));
    g.setColour(juce::Colours::white);
    g.drawText("ERIK // SAT", 20, 12, 200, 24, juce::Justification::left);

    g.setFont(juce::FontOptions(11.0f));
    g.setColour(juce::Colour(0xff6a7185));
    g.drawText("ANALOG SATURATION & OVERSAMPLING", getWidth() - 240, 15, 220, 20, juce::Justification::right);
}

void ErikSatAudioProcessorEditor::resized()
{
    const int pad = 16;
    const int contentY = 56;
    
    // Transfer curve visualizer taking top half
    curveVisualizer.setBounds(pad, contentY, getWidth() - (pad * 2), 175);

    // Lower control area
    const int controlsY = 245;
    const int knobWidth = 90;
    const int knobHeight = 100;
    const int labelHeight = 18;

    // Left: Drive Knob (Prominent)
    const int driveX = 35;
    driveSlider.setBounds(driveX, controlsY + 16, 105, 115);
    driveLabel.setBounds(driveX, controlsY - 4, 105, labelHeight);

    // Center Left: Type selector & Character
    const int typeX = 165;
    typeLabel.setBounds(typeX, controlsY, 85, labelHeight);
    typeBox.setBounds(typeX, controlsY + 22, 85, 26);

    characterSlider.setBounds(typeX, controlsY + 62, 85, 80);
    characterLabel.setBounds(typeX, controlsY + 142, 85, labelHeight);

    // Center Right: Mix
    const int mixX = 275;
    mixSlider.setBounds(mixX, controlsY + 20, knobWidth, knobHeight);
    mixLabel.setBounds(mixX, controlsY, knobWidth, labelHeight);

    // Right: Output Trim
    const int outX = 385;
    outputSlider.setBounds(outX, controlsY + 20, knobWidth, knobHeight);
    outputLabel.setBounds(outX, controlsY, knobWidth, labelHeight);
}

void ErikSatAudioProcessorEditor::timerCallback()
{
    const float driveDb = audioProcessor.getAPVTS().getRawParameterValue("drive")->load();
    const float driveLinear = juce::Decibels::decibelsToGain(driveDb);
    const int typeIndex = static_cast<int>(audioProcessor.getAPVTS().getRawParameterValue("type")->load());
    const float bias = audioProcessor.getAPVTS().getRawParameterValue("character")->load();
    const float inputPeak = audioProcessor.getCurrentInputPeak();

    const auto satType = static_cast<ErikDSP::SaturationType>(juce::jlimit(0, 2, typeIndex));

    curveVisualizer.updateParameters(driveLinear, satType, bias, inputPeak);
}
