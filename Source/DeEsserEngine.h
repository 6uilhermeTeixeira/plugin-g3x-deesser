#pragma once

#include "dsp/DeEsser.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

namespace g3x::deesser
{
struct EngineParameters
{
    float frequencyHz = 5500.0f;
    float thresholdDb = -30.0f;
    float rangeDb = 8.0f;
    dsp::DetectorMode detectorMode = dsp::DetectorMode::highPass;
    bool splitMode = true;
    bool monitorSidechain = false;
    bool bypass = false;
};

class DeEsserEngine
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset() noexcept;
    void setParameters(const EngineParameters& values) noexcept;
    void process(juce::AudioBuffer<float>& buffer) noexcept;
    float getReductionDb() const noexcept { return reductionDb.load(std::memory_order_relaxed); }
    float getDetectorDb() const noexcept { return detectorDb.load(std::memory_order_relaxed); }
    float getOutputPeak(int channel) const noexcept;

private:
    static constexpr std::size_t maximumChannels = 2;
    std::array<dsp::DetectorFilter, maximumChannels> highPassFilters;
    std::array<dsp::DetectorFilter, maximumChannels> bandPassFilters;
    std::array<float, maximumChannels> lowPassState {};
    dsp::EnvelopeFollower envelope;
    dsp::GainComputer gainComputer;
    EngineParameters parameters;
    double sampleRate = 44100.0;
    float crossoverCoefficient = 0.0f;
    float lastFrequency = 5500.0f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> frequency;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> threshold;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> range;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> splitMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> monitorMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> activeMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> detectorMix;
    std::atomic<float> reductionDb { 0.0f };
    std::atomic<float> detectorDb { dsp::minimumDecibels };
    std::array<std::atomic<float>, maximumChannels> outputPeak {};
};
}
