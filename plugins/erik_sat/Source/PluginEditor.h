#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "TransferCurveVisualizer.h"

// Modern sleek look and feel for rotary knobs
class SaturationLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SaturationLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff00d4ff));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00d4ff));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff232530));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override
    {
        const float radius = juce::jmin(width / 2.0f, height / 2.0f) - 6.0f;
        const float centreX = x + width * 0.5f;
        const float centreY = y + height * 0.5f;
        const float currentAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Background track arc
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff232530));
        g.strokePath(backgroundArc, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active value arc
        if (sliderPosProportional > 0.001f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, currentAngle, true);
            
            juce::ColourGradient grad(juce::Colour(0xff00d4ff), centreX - radius, centreY,
                                      juce::Colour(0xffff5577), centreX + radius, centreY, false);
            g.setGradientFill(grad);
            g.strokePath(valueArc, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Inner knob face
        const float innerRadius = radius - 8.0f;
        g.setColour(juce::Colour(0xff181921));
        g.fillEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);
        g.setColour(juce::Colour(0xff323545));
        g.drawEllipse(centreX - innerRadius, centreY - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 1.2f);

        // Pointer notch
        juce::Path pointer;
        const float pointerLength = innerRadius * 0.65f;
        pointer.startNewSubPath(centreX, centreY - innerRadius + 2.0f);
        pointer.lineTo(centreX, centreY - innerRadius + pointerLength);
        pointer.applyTransform(juce::AffineTransform::rotation(currentAngle, centreX, centreY));
        g.setColour(juce::Colours::white);
        g.strokePath(pointer, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
};

class ErikSatAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ErikSatAudioProcessorEditor(ErikSatAudioProcessor&);
    ~ErikSatAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ErikSatAudioProcessor& audioProcessor;

    SaturationLookAndFeel customLookAndFeel;

    // Visualizer
    TransferCurveVisualizer curveVisualizer;

    // UI Sliders & Controls
    juce::Slider driveSlider;
    juce::Label driveLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;

    juce::ComboBox typeBox;
    juce::Label typeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;

    juce::Slider characterSlider;
    juce::Label characterLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> characterAttachment;

    juce::Slider mixSlider;
    juce::Label mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    juce::Slider outputSlider;
    juce::Label outputLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ErikSatAudioProcessorEditor)
};
