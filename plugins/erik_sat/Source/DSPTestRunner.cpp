#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include "SaturationDSP.h"

int main()
{
    std::cout << "============================================" << std::endl;
    std::cout << "   RUNNING ERIK_SAT REAL-TIME DSP TESTS     " << std::endl;
    std::cout << "============================================" << std::endl;

    int passedTests = 0;
    int totalTests = 0;

    auto EXPECT_TRUE = [&](bool condition, const std::string& testName) {
        totalTests++;
        if (condition) {
            std::cout << " [PASS] " << testName << std::endl;
            passedTests++;
        } else {
            std::cerr << " [FAIL] " << testName << std::endl;
        }
    };

    // --- TEST 1: Zero-input stability ---
    {
        float zeroTape = ErikDSP::SaturationTransfer::processSample(0.0f, 1.0f, ErikDSP::SaturationType::Tape, 0.0f);
        float zeroTube = ErikDSP::SaturationTransfer::processSample(0.0f, 1.0f, ErikDSP::SaturationType::Tube, 0.0f);
        float zeroHard = ErikDSP::SaturationTransfer::processSample(0.0f, 1.0f, ErikDSP::SaturationType::HardClip, 0.0f);

        EXPECT_TRUE(std::abs(zeroTape) < 1e-6f, "Tape zero-input produces zero output");
        EXPECT_TRUE(std::abs(zeroTube) < 1e-6f, "Tube zero-input produces zero output");
        EXPECT_TRUE(std::abs(zeroHard) < 1e-6f, "HardClip zero-input produces zero output");
    }

    // --- TEST 2: Symmetry vs Asymmetry ---
    {
        float tapePos = ErikDSP::SaturationTransfer::processSample(0.5f, 2.0f, ErikDSP::SaturationType::Tape, 0.0f);
        float tapeNeg = ErikDSP::SaturationTransfer::processSample(-0.5f, 2.0f, ErikDSP::SaturationType::Tape, 0.0f);
        EXPECT_TRUE(std::abs(tapePos + tapeNeg) < 1e-5f, "Tape saturation is odd-symmetric (f(-x) == -f(x))");

        float tubePos = ErikDSP::SaturationTransfer::processSample(0.5f, 2.0f, ErikDSP::SaturationType::Tube, 0.0f);
        float tubeNeg = ErikDSP::SaturationTransfer::processSample(-0.5f, 2.0f, ErikDSP::SaturationType::Tube, 0.0f);
        EXPECT_TRUE(std::abs(tubePos + tubeNeg) > 0.02f, "Tube saturation is asymmetric (even-order harmonics)");
    }

    // --- TEST 3: Extreme input clipping bounds ---
    {
        float extremeOut = ErikDSP::SaturationTransfer::processSample(100.0f, 10.0f, ErikDSP::SaturationType::HardClip, 0.0f);
        EXPECT_TRUE(extremeOut <= 1.0f && extremeOut >= -1.0f, "HardClip strictly confines extreme values to [-1.0, 1.0]");
    }

    // --- TEST 4: Full audio buffer processor with oversampling & variable block sizes ---
    {
        ErikDSP::SaturationProcessor processor;
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = 48000.0;
        spec.maximumBlockSize = 2048;
        spec.numChannels = 2;

        processor.prepare(spec);

        float latency = processor.getLatencyInSamples();
        EXPECT_TRUE(latency > 0.0f, "2x Oversampling correctly reports latency for DAW compensation (latency = " + std::to_string(latency) + " samples)");

        std::vector<int> testBlockSizes = { 32, 64, 128, 256, 512, 1024, 2048 };
        bool allBlocksClean = true;

        for (int blockSize : testBlockSizes)
        {
            juce::AudioBuffer<float> buffer(2, blockSize);

            // Generate 1 kHz sine wave
            for (int ch = 0; ch < 2; ++ch)
            {
                float* channel = buffer.getWritePointer(ch);
                for (int i = 0; i < blockSize; ++i)
                {
                    channel[i] = std::sin(2.0f * 3.14159265f * 1000.0f * (static_cast<float>(i) / 48000.0f)) * 0.7f;
                }
            }

            // Process with Drive = 6 dB (2.0f linear), Tape, 100% Wet
            processor.process(buffer, 2.0f, ErikDSP::SaturationType::Tape, 0.0f, 1.0f, 1.0f);

            // Verify no NaN or Inf
            for (int ch = 0; ch < 2; ++ch)
            {
                const float* channel = buffer.getReadPointer(ch);
                for (int i = 0; i < blockSize; ++i)
                {
                    if (std::isnan(channel[i]) || std::isinf(channel[i]))
                    {
                        allBlocksClean = false;
                        break;
                    }
                }
            }
        }

        EXPECT_TRUE(allBlocksClean, "Real-time audio processing across all buffer sizes (32-2048) contains 0 NaNs and 0 Infs");
    }

    std::cout << "============================================" << std::endl;
    std::cout << " Result: " << passedTests << " / " << totalTests << " tests passed." << std::endl;
    std::cout << "============================================" << std::endl;

    return (passedTests == totalTests) ? 0 : 1;
}
