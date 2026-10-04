#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

namespace ErikDSP
{

enum class SaturationType
{
    Tape = 0,
    Tube,
    HardClip
};

/**
 * Real-time saturation math functions.
 * All functions are noexcept and allocation-free.
 */
struct SaturationTransfer
{
    static inline float processSample(float input, float driveLinear, SaturationType type, float characterBias = 0.0f) noexcept
    {
        const float x = input * driveLinear;

        switch (type)
        {
            case SaturationType::Tape:
            {
                // Smooth hyperbolic tangent soft-saturation with mild tape compression
                // Scaled so small signals maintain unit gain
                return std::tanh(x);
            }
            case SaturationType::Tube:
            {
                // Asymmetric curve introducing warm 2nd-order even harmonics
                const float bias = 0.25f + (characterBias * 0.15f);
                const float biasedX = x + bias;
                const float sat = std::tanh(biasedX) - std::tanh(bias);
                return sat * 1.15f;
            }
            case SaturationType::HardClip:
            {
                // Cubic soft-knee transitioning into brickwall clip
                const float threshold = 0.7f;
                if (std::abs(x) < threshold)
                {
                    return x;
                }
                else
                {
                    const float sign = (x > 0.0f) ? 1.0f : -1.0f;
                    const float excess = std::abs(x) - threshold;
                    return sign * (threshold + (1.0f - threshold) * std::tanh(excess / (1.0f - threshold)));
                }
            }
            default:
                return std::tanh(x);
        }
    }
};

/**
 * Saturation processor supporting 2x oversampling to prevent aliasing.
 */
class SaturationProcessor
{
public:
    SaturationProcessor() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        oversampling.reset(new juce::dsp::Oversampling<float>(spec.numChannels, 1, // 2x oversampling (factor 1 = 2^1)
                                                              juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                                                              true, false));
        oversampling->initProcessing(spec.maximumBlockSize);
        oversampling->reset();
    }

    void reset()
    {
        if (oversampling != nullptr)
            oversampling->reset();
    }

    void process(juce::AudioBuffer<float>& buffer,
                 float driveLinear,
                 SaturationType type,
                 float bias,
                 float mixNormalized,
                 float outputLinear) noexcept
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        if (numSamples == 0 || numChannels == 0)
            return;

        // Keep dry buffer for parallel mix
        dryBuffer.setSize(numChannels, numSamples, false, false, true);
        for (int ch = 0; ch < numChannels; ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

        // 2x Oversampling context
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::AudioBlock<float> oversampledBlock = oversampling != nullptr 
            ? oversampling->processSamplesUp(block) 
            : block;

        const size_t osChannels = oversampledBlock.getNumChannels();
        const size_t osSamples = oversampledBlock.getNumSamples();

        for (size_t ch = 0; ch < osChannels; ++ch)
        {
            float* channelData = oversampledBlock.getChannelPointer(ch);
            for (size_t i = 0; i < osSamples; ++i)
            {
                channelData[i] = SaturationTransfer::processSample(channelData[i], driveLinear, type, bias);
            }
        }

        if (oversampling != nullptr)
            oversampling->processSamplesDown(block);

        // Apply Wet/Dry Mix and Output Gain
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* outData = buffer.getWritePointer(ch);
            const float* dryData = dryBuffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                const float wet = outData[i];
                const float dry = dryData[i];
                const float mixed = (dry * (1.0f - mixNormalized)) + (wet * mixNormalized);
                outData[i] = mixed * outputLinear;
            }
        }
    }

    float getLatencyInSamples() const noexcept
    {
        return oversampling != nullptr ? oversampling->getLatencyInSamples() : 0.0f;
    }

private:
    double sampleRate { 44100.0 };
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioBuffer<float> dryBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturationProcessor)
};

} // namespace ErikDSP
