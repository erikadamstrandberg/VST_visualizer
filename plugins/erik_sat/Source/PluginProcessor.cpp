#include "PluginProcessor.h"
#include "PluginEditor.h"

ErikSatAudioProcessor::ErikSatAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

ErikSatAudioProcessor::~ErikSatAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ErikSatAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Drive: 0.0 dB to 24.0 dB
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "drive", 1 },
        "Drive",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // Saturation Type: Tape, Tube, HardClip
    juce::StringArray types { "Tape", "Tube", "Hard Clip" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "type", 1 },
        "Type",
        types,
        0));

    // Character / Bias: -1.0 to +1.0
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "character", 1 },
        "Character",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
        0.0f));

    // Mix: 0% to 100%
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Output Trim: -24.0 dB to +6.0 dB
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "output", 1 },
        "Output",
        juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    return { params.begin(), params.end() };
}

const juce::String ErikSatAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ErikSatAudioProcessor::acceptsMidi() const
{
    return false;
}

bool ErikSatAudioProcessor::producesMidi() const
{
    return false;
}

bool ErikSatAudioProcessor::isMidiEffect() const
{
    return false;
}

double ErikSatAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ErikSatAudioProcessor::getNumPrograms()
{
    return 1;
}

int ErikSatAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ErikSatAudioProcessor::setCurrentProgram(int)
{
}

const juce::String ErikSatAudioProcessor::getProgramName(int)
{
    return {};
}

void ErikSatAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void ErikSatAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    saturationProcessor.prepare(spec);

    smoothedDriveDb.reset(sampleRate, 0.02);
    smoothedOutputDb.reset(sampleRate, 0.02);
    smoothedMix.reset(sampleRate, 0.02);

    setLatencySamples(juce::roundToInt(saturationProcessor.getLatencyInSamples()));
}

void ErikSatAudioProcessor::releaseResources()
{
    saturationProcessor.reset();
}

bool ErikSatAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainInput = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput.isDisabled() || mainOutput.isDisabled())
        return false;

    // Support mono-mono or stereo-stereo
    if (mainInput != mainOutput)
        return false;

    return mainInput == juce::AudioChannelSet::mono() || mainInput == juce::AudioChannelSet::stereo();
}

void ErikSatAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Clear unused output channels
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    if (numSamples == 0)
        return;

    // Track input peak for real-time visualization ballistics
    float maxPeak = 0.0f;
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
    {
        const float chMag = buffer.getMagnitude(ch, 0, numSamples);
        if (chMag > maxPeak)
            maxPeak = chMag;
    }
    currentInputPeak.store(maxPeak, std::memory_order_relaxed);

    // Fetch parameter targets
    const float targetDriveDb = apvts.getRawParameterValue("drive")->load();
    const int typeIndex = static_cast<int>(apvts.getRawParameterValue("type")->load());
    const float characterBias = apvts.getRawParameterValue("character")->load();
    const float targetMix = apvts.getRawParameterValue("mix")->load() / 100.0f;
    const float targetOutputDb = apvts.getRawParameterValue("output")->load();

    smoothedDriveDb.setTargetValue(targetDriveDb);
    smoothedMix.setTargetValue(targetMix);
    smoothedOutputDb.setTargetValue(targetOutputDb);

    const float currentDriveLinear = juce::Decibels::decibelsToGain(smoothedDriveDb.getNextValue());
    const float currentOutputLinear = juce::Decibels::decibelsToGain(smoothedOutputDb.getNextValue());
    const float currentMix = smoothedMix.getNextValue();

    const auto satType = static_cast<ErikDSP::SaturationType>(juce::jlimit(0, 2, typeIndex));

    // Process through saturation engine
    saturationProcessor.process(buffer, currentDriveLinear, satType, characterBias, currentMix, currentOutputLinear);
}

bool ErikSatAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ErikSatAudioProcessor::createEditor()
{
    return new ErikSatAudioProcessorEditor(*this);
}

void ErikSatAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ErikSatAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

// Plugin creation factory
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ErikSatAudioProcessor();
}
