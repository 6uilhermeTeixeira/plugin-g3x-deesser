#include "DeEsserEngine.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace g3x::deesser
{
void DeEsserEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = std::max(1.0, spec.sampleRate);
    for (auto& filter : highPassFilters)
    {
        filter.prepare(sampleRate);
        filter.setModeAndFrequency(dsp::DetectorMode::highPass, parameters.frequencyHz);
    }
    for (auto& filter : bandPassFilters)
    {
        filter.prepare(sampleRate);
        filter.setModeAndFrequency(dsp::DetectorMode::bandPass, parameters.frequencyHz);
    }
    envelope.prepare(sampleRate);
    envelope.setTimes(0.5f, 80.0f);
    constexpr auto parameterRampSeconds = 0.02;
    constexpr auto switchRampSeconds = 0.01;
    frequency.reset(sampleRate, parameterRampSeconds);
    threshold.reset(sampleRate, parameterRampSeconds);
    range.reset(sampleRate, parameterRampSeconds);
    splitMix.reset(sampleRate, switchRampSeconds);
    monitorMix.reset(sampleRate, switchRampSeconds);
    activeMix.reset(sampleRate, switchRampSeconds);
    detectorMix.reset(sampleRate, switchRampSeconds);
    frequency.setCurrentAndTargetValue(parameters.frequencyHz);
    threshold.setCurrentAndTargetValue(parameters.thresholdDb);
    range.setCurrentAndTargetValue(parameters.rangeDb);
    splitMix.setCurrentAndTargetValue(parameters.splitMode ? 1.0f : 0.0f);
    monitorMix.setCurrentAndTargetValue(parameters.monitorSidechain ? 1.0f : 0.0f);
    activeMix.setCurrentAndTargetValue(parameters.bypass ? 0.0f : 1.0f);
    detectorMix.setCurrentAndTargetValue(
        parameters.detectorMode == dsp::DetectorMode::bandPass ? 1.0f : 0.0f);
    lastFrequency = -1.0f;
    setParameters(parameters);
    reset();
}

void DeEsserEngine::reset() noexcept
{
    for (auto& filter : highPassFilters)
        filter.reset();
    for (auto& filter : bandPassFilters)
        filter.reset();
    lowPassState.fill(0.0f);
    envelope.reset();
    reductionDb.store(0.0f, std::memory_order_relaxed);
    detectorDb.store(dsp::minimumDecibels, std::memory_order_relaxed);
    for (auto& peak : outputPeak)
        peak.store(0.0f, std::memory_order_relaxed);
}

void DeEsserEngine::setParameters(const EngineParameters& values) noexcept
{
    parameters = values;
    const auto finiteOr = [](float value, float fallback)
    {
        return std::isfinite(value) ? value : fallback;
    };
    frequency.setTargetValue(std::clamp(finiteOr(parameters.frequencyHz, 5500.0f),
                                        2000.0f, 16000.0f));
    threshold.setTargetValue(std::clamp(finiteOr(parameters.thresholdDb, -30.0f),
                                        -80.0f, 0.0f));
    range.setTargetValue(std::clamp(finiteOr(parameters.rangeDb, 8.0f), 0.0f, 24.0f));
    splitMix.setTargetValue(parameters.splitMode ? 1.0f : 0.0f);
    monitorMix.setTargetValue(parameters.monitorSidechain ? 1.0f : 0.0f);
    activeMix.setTargetValue(parameters.bypass ? 0.0f : 1.0f);
    detectorMix.setTargetValue(
        parameters.detectorMode == dsp::DetectorMode::bandPass ? 1.0f : 0.0f);
}

void DeEsserEngine::process(juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = std::min(buffer.getNumChannels(), static_cast<int>(maximumChannels));
    std::array<float, maximumChannels> blockPeak {};
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto smoothedFrequency = std::clamp(frequency.getNextValue(), 2000.0f,
                                                  static_cast<float>(sampleRate * 0.45));
        if (std::abs(smoothedFrequency - lastFrequency) > 0.001f)
        {
            for (auto& filter : highPassFilters)
                filter.setModeAndFrequency(dsp::DetectorMode::highPass, smoothedFrequency);
            for (auto& filter : bandPassFilters)
                filter.setModeAndFrequency(dsp::DetectorMode::bandPass, smoothedFrequency);
            crossoverCoefficient = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float>
                * smoothedFrequency / static_cast<float>(sampleRate));
            lastFrequency = smoothedFrequency;
        }
        gainComputer.setThresholdDb(threshold.getNextValue());
        gainComputer.setRangeDb(range.getNextValue());
        const auto currentSplitMix = splitMix.getNextValue();
        const auto currentMonitorMix = monitorMix.getNextValue();
        const auto currentActiveMix = activeMix.getNextValue();
        const auto currentDetectorMix = detectorMix.getNextValue();
        std::array<float, maximumChannels> sidechain {};
        std::array<float, maximumChannels> inputSamples {};
        auto linkedMagnitude = 0.0f;
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto rawInput = buffer.getSample(channel, sample);
            inputSamples[static_cast<std::size_t>(channel)] =
                std::isfinite(rawInput) ? rawInput : 0.0f;
            const auto index = static_cast<std::size_t>(channel);
            const auto highPass = highPassFilters[index].process(inputSamples[index]);
            const auto bandPass = bandPassFilters[index].process(inputSamples[index]);
            sidechain[index] = highPass + currentDetectorMix * (bandPass - highPass);
            linkedMagnitude = std::max(linkedMagnitude,
                std::abs(sidechain[static_cast<std::size_t>(channel)]));
        }
        const auto detectorEnvelope = envelope.process(linkedMagnitude);
        const auto gain = gainComputer.process(detectorEnvelope);

        for (int channel = 0; channel < channels; ++channel)
        {
            const auto index = static_cast<std::size_t>(channel);
            const auto input = inputSamples[index];
            lowPassState[index] += crossoverCoefficient * (input - lowPassState[index]);
            const auto splitOutput = lowPassState[index] + (input - lowPassState[index]) * gain;
            const auto wideOutput = input * gain;
            const auto processed = wideOutput + currentSplitMix * (splitOutput - wideOutput);
            const auto monitored = processed + currentMonitorMix * (sidechain[index] - processed);
            const auto output = input + currentActiveMix * (monitored - input);
            buffer.setSample(channel, sample, std::isfinite(output) ? output : 0.0f);
            blockPeak[index] = std::max(blockPeak[index], std::abs(output));
        }
    }
    reductionDb.store(gainComputer.getReductionDb(), std::memory_order_relaxed);
    detectorDb.store(dsp::gainToDecibels(envelope.getCurrentValue()), std::memory_order_relaxed);
    for (int channel = 0; channel < channels; ++channel)
        outputPeak[static_cast<std::size_t>(channel)].store(
            blockPeak[static_cast<std::size_t>(channel)], std::memory_order_relaxed);
}

float DeEsserEngine::getOutputPeak(int channel) const noexcept
{
    if (channel < 0 || channel >= static_cast<int>(maximumChannels))
        return 0.0f;
    return outputPeak[static_cast<std::size_t>(channel)].load(std::memory_order_relaxed);
}
}
