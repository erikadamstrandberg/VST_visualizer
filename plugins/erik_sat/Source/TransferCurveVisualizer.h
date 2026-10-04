#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SaturationDSP.h"

class TransferCurveVisualizer : public juce::Component
{
public:
    TransferCurveVisualizer()
    {
        setOpaque(false);
    }

    void updateParameters(float newDriveLinear, ErikDSP::SaturationType newType, float newBias, float currentInputPeak)
    {
        driveLinear = newDriveLinear;
        saturationType = newType;
        bias = newBias;
        
        // Exponential decay for ballistics on the input peak dot
        inputPeak = std::max(currentInputPeak, inputPeak * 0.88f);

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced(12.0f);
        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        const float midX = bounds.getCentreX();
        const float midY = bounds.getCentreY();

        // Background panel with soft rounded corners and border
        juce::Path roundedCard;
        roundedCard.addRoundedRectangle(bounds.expanded(6.0f), 8.0f);
        g.setColour(juce::Colour(0xff121217));
        g.fillPath(roundedCard);

        g.setColour(juce::Colour(0xff2a2a35));
        g.strokePath(roundedCard, juce::PathStrokeType(1.2f));

        // Grid lines
        g.setColour(juce::Colour(0xff1f212b));
        g.drawLine(bounds.getX(), midY, bounds.getRight(), midY, 1.0f); // X-axis (zero)
        g.drawLine(midX, bounds.getY(), midX, bounds.getBottom(), 1.0f); // Y-axis (zero)

        // Draw linear reference (45-degree diagonal line)
        g.setColour(juce::Colour(0x35ffffff));
        float dashPattern[] = { 4.0f, 4.0f };
        juce::Line<float> linearRef(bounds.getX(), bounds.getBottom(), bounds.getRight(), bounds.getY());
        g.drawDashedLine(linearRef, dashPattern, 2, 1.0f);

        // Compute and draw saturated transfer curve
        juce::Path curvePath;
        const int steps = 180;
        bool firstPoint = true;

        for (int i = 0; i <= steps; ++i)
        {
            const float normalizedX = (static_cast<float>(i) / static_cast<float>(steps)) * 2.0f - 1.0f; // -1 to +1
            const float saturatedY = ErikDSP::SaturationTransfer::processSample(normalizedX, driveLinear, saturationType, bias);
            
            // Map normalized coordinates [-1, 1] to pixel bounds
            const float px = midX + normalizedX * (w * 0.5f);
            const float py = midY - juce::jlimit(-1.2f, 1.2f, saturatedY) * (h * 0.5f);

            if (firstPoint)
            {
                curvePath.startNewSubPath(px, py);
                firstPoint = false;
            }
            else
            {
                curvePath.lineTo(px, py);
            }
        }

        // Draw glowing aura behind curve
        g.setColour(juce::Colour(0x2200f0ff));
        g.strokePath(curvePath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Main neon cyan curve
        juce::ColourGradient grad(juce::Colour(0xff00d4ff), bounds.getX(), bounds.getBottom(),
                                  juce::Colour(0xffff5577), bounds.getRight(), bounds.getY(), false);
        g.setGradientFill(grad);
        g.strokePath(curvePath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Draw current audio level dot on curve if input peak > 0.005
        if (inputPeak > 0.005f)
        {
            const float clampedPeak = std::min(1.0f, inputPeak);
            const float satY = ErikDSP::SaturationTransfer::processSample(clampedPeak, driveLinear, saturationType, bias);

            const float dotX = midX + clampedPeak * (w * 0.5f);
            const float dotY = midY - juce::jlimit(-1.2f, 1.2f, satY) * (h * 0.5f);

            // Symmetrical negative dot
            const float negSatY = ErikDSP::SaturationTransfer::processSample(-clampedPeak, driveLinear, saturationType, bias);
            const float negDotX = midX - clampedPeak * (w * 0.5f);
            const float negDotY = midY - juce::jlimit(-1.2f, 1.2f, negSatY) * (h * 0.5f);

            // Outer glow
            g.setColour(juce::Colour(0x66ff0055));
            g.fillEllipse(dotX - 7.0f, dotY - 7.0f, 14.0f, 14.0f);
            g.fillEllipse(negDotX - 7.0f, negDotY - 7.0f, 14.0f, 14.0f);

            // Inner core
            g.setColour(juce::Colours::white);
            g.fillEllipse(dotX - 3.5f, dotY - 3.5f, 7.0f, 7.0f);
            g.fillEllipse(negDotX - 3.5f, negDotY - 3.5f, 7.0f, 7.0f);
        }

        // Corner labels
        g.setFont(juce::FontOptions(11.0f));
        g.setColour(juce::Colour(0x88ffffff));
        g.drawText("OUTPUT", juce::Rectangle<float>(bounds.getX() + 6.0f, bounds.getY() + 4.0f, 60.0f, 16.0f), juce::Justification::left);
        g.drawText("INPUT", juce::Rectangle<float>(bounds.getRight() - 56.0f, bounds.getBottom() - 18.0f, 50.0f, 16.0f), juce::Justification::right);
    }

private:
    float driveLinear { 1.0f };
    ErikDSP::SaturationType saturationType { ErikDSP::SaturationType::Tape };
    float bias { 0.0f };
    float inputPeak { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransferCurveVisualizer)
};
