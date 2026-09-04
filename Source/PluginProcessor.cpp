#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>

namespace g3x::deesser
{
namespace Ids
{
constexpr auto frequency = "frequencyHz";
constexpr auto threshold = "thresholdDb";
constexpr auto range = "rangeDb";
constexpr auto detector = "detectorMode";
constexpr auto processing = "processingMode";
constexpr auto monitor = "monitorMode";
constexpr auto bypass = "bypass";
}

struct FactoryPreset
{
    const char* name;
    float frequencyHz;
    float thresholdDb;
    float rangeDb;
    int detectorMode;
    int processingMode;
};

constexpr std::array<FactoryPreset, 6> factoryPresets {{
    { "Vocal Female - Gentle", 6500.0f, -28.0f, 5.0f, 0, 0 },
    { "Vocal Female - Focused", 7200.0f, -34.0f, 9.0f, 1, 0 },
    { "Vocal Male - Gentle", 4800.0f, -27.0f, 5.0f, 0, 0 },
    { "Vocal Male - Focused", 5600.0f, -34.0f, 9.0f, 1, 0 },
    { "Speech", 6000.0f, -32.0f, 7.0f, 0, 1 },
    { "Cymbal Tamer", 8500.0f, -24.0f, 6.0f, 0, 0 },
}};

DeEsserAudioProcessor::DeEsserAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "G3XDeEsserState", createParameterLayout())
{
}

void DeEsserAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock),
                     static_cast<juce::uint32>(getTotalNumOutputChannels()) });
    updateEngine();
}

void DeEsserAudioProcessor::releaseResources() { engine.reset(); }

bool DeEsserAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())
        && (output == input || (input == juce::AudioChannelSet::mono()
                               && output == juce::AudioChannelSet::stereo()));
}

void DeEsserAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (getTotalNumInputChannels() == 1 && getTotalNumOutputChannels() == 2)
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
    updateEngine();
    engine.process(buffer);
}

juce::AudioProcessorEditor* DeEsserAudioProcessor::createEditor()
{
    return new DeEsserAudioProcessorEditor(*this);
}

int DeEsserAudioProcessor::getNumPrograms() { return static_cast<int>(factoryPresets.size()); }

int DeEsserAudioProcessor::getCurrentProgram() { return currentProgram.load(); }

const juce::String DeEsserAudioProcessor::getProgramName(int index)
{
    return juce::isPositiveAndBelow(index, getNumPrograms())
        ? factoryPresets[static_cast<std::size_t>(index)].name : juce::String {};
}

void DeEsserAudioProcessor::setCurrentProgram(int index)
{
    if (!juce::isPositiveAndBelow(index, getNumPrograms()))
        return;
    const auto& preset = factoryPresets[static_cast<std::size_t>(index)];
    const auto setParameter = [this](const char* id, float plainValue)
    {
        if (auto* parameter = state.getParameter(id))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
            parameter->endChangeGesture();
        }
    };
    setParameter(Ids::frequency, preset.frequencyHz);
    setParameter(Ids::threshold, preset.thresholdDb);
    setParameter(Ids::range, preset.rangeDb);
    setParameter(Ids::detector, static_cast<float>(preset.detectorMode));
    setParameter(Ids::processing, static_cast<float>(preset.processingMode));
    setParameter(Ids::monitor, 0.0f);
    setParameter(Ids::bypass, 0.0f);
    currentProgram.store(index);
}

void DeEsserAudioProcessor::getStateInformation(juce::MemoryBlock& data)
{
    auto savedState = state.copyState();
    savedState.setProperty("schemaVersion", 1, nullptr);
    savedState.setProperty("currentProgram", currentProgram.load(), nullptr);
    if (const auto xml = savedState.createXml())
        copyXmlToBinary(*xml, data);
}

void DeEsserAudioProcessor::setStateInformation(const void* data, int bytes)
{
    if (const auto xml = getXmlFromBinary(data, bytes); xml != nullptr)
        if (xml->hasTagName(state.state.getType()))
        {
            auto restoredState = juce::ValueTree::fromXml(*xml);
            currentProgram.store(juce::jlimit(0, getNumPrograms() - 1,
                static_cast<int>(restoredState.getProperty("currentProgram", 0))));
            state.replaceState(restoredState);
        }
}

juce::AudioProcessorValueTreeState::ParameterLayout DeEsserAudioProcessor::createParameterLayout()
{
    using Parameter = std::unique_ptr<juce::RangedAudioParameter>;
    std::vector<Parameter> layout;
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Ids::frequency, 1 }, "Frequency",
        juce::NormalisableRange<float> { 2000.0f, 16000.0f, 1.0f, 0.35f }, 5500.0f,
        juce::AudioParameterFloatAttributes {}.withStringFromValueFunction(
            [](float value, int) { return value >= 1000.0f
                ? juce::String(value / 1000.0f, 2) + " kHz"
                : juce::String(juce::roundToInt(value)) + " Hz"; })));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Ids::threshold, 1 }, "Threshold",
        juce::NormalisableRange<float> { -80.0f, 0.0f, 0.1f }, -30.0f,
        juce::AudioParameterFloatAttributes {}.withLabel("dB")));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { Ids::range, 1 }, "Range",
        juce::NormalisableRange<float> { 0.0f, 24.0f, 0.1f }, 8.0f,
        juce::AudioParameterFloatAttributes {}.withLabel("dB")));
    layout.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { Ids::detector, 1 }, "Detector",
        juce::StringArray { "HighPass", "BandPass" }, 0));
    layout.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { Ids::processing, 1 }, "Processing",
        juce::StringArray { "Split", "Wideband" }, 0));
    layout.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { Ids::monitor, 1 }, "Monitor",
        juce::StringArray { "Audio", "Sidechain" }, 0));
    layout.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { Ids::bypass, 1 }, "Bypass", false));
    return { layout.begin(), layout.end() };
}

void DeEsserAudioProcessor::updateEngine() noexcept
{
    EngineParameters parameters;
    parameters.frequencyHz = state.getRawParameterValue(Ids::frequency)->load();
    parameters.thresholdDb = state.getRawParameterValue(Ids::threshold)->load();
    parameters.rangeDb = state.getRawParameterValue(Ids::range)->load();
    parameters.detectorMode = state.getRawParameterValue(Ids::detector)->load() < 0.5f
        ? dsp::DetectorMode::highPass : dsp::DetectorMode::bandPass;
    parameters.splitMode = state.getRawParameterValue(Ids::processing)->load() < 0.5f;
    parameters.monitorSidechain = state.getRawParameterValue(Ids::monitor)->load() >= 0.5f;
    parameters.bypass = state.getRawParameterValue(Ids::bypass)->load() >= 0.5f;
    engine.setParameters(parameters);
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new g3x::deesser::DeEsserAudioProcessor();
}
