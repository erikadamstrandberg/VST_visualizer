#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SaturationDSP.h"

class ErikSatAudioProcessor : public juce::AudioProcessor
{
public:
    ErikSatAudioProcessor();
    ~ErikSatAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    float getCurrentInputPeak() const noexcept { return currentInputPeak.load(std::memory_order_relaxed); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState apvts;
    ErikDSP::SaturationProcessor saturationProcessor;

    // Smoothed atomic peak for UI ballistics
    std::atomic<float> currentInputPeak { 0.0f };

    // Smoothed values to avoid clicks during rapid automation
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDriveDb;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedOutputDb;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ErikSatAudioProcessor)
};
