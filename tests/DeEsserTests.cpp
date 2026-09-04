#include "dsp/DeEsser.h"
#include "DeEsserEngine.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{
int failures = 0;
void expect(bool condition, std::string_view message)
{
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
void near(float actual, float expected, float tolerance, std::string_view message)
{
    expect(std::abs(actual - expected) <= tolerance, message);
}
}

int main()
{
    using namespace g3x::deesser::dsp;
    near(gainToDecibels(decibelsToGain(-18.0f)), -18.0f, 0.001f, "dB conversion");
    near(gainToDecibels(std::numeric_limits<float>::quiet_NaN()), minimumDecibels,
         0.001f, "non-finite gain is sanitised");
    near(decibelsToGain(std::numeric_limits<float>::infinity()), 0.0f,
         0.001f, "non-finite decibels are sanitised");
    near(GainComputer::computeGainDb(-40.0f, -30.0f, 8.0f), 0.0f, 0.001f,
         "below threshold remains unity");
    expect(GainComputer::computeGainDb(-20.0f, -30.0f, 24.0f) < -8.0f,
           "signal above threshold is reduced");
    near(GainComputer::computeGainDb(0.0f, -60.0f, 8.0f), -8.0f, 0.001f,
         "range caps reduction");

    EnvelopeFollower follower;
    follower.prepare(48000.0);
    follower.setTimes(0.5f, 80.0f);
    float envelope = 0.0f;
    for (int i = 0; i < 480; ++i) envelope = follower.process(1.0f);
    expect(envelope > 0.99f && std::isfinite(envelope), "envelope attack converges");
    const auto attacked = envelope;
    for (int i = 0; i < 480; ++i) envelope = follower.process(0.0f);
    expect(envelope < attacked && envelope > 0.0f, "envelope release is gradual");

    DetectorFilter highPass;
    highPass.prepare(48000.0);
    highPass.setModeAndFrequency(DetectorMode::highPass, 5500.0f);
    float lowEnergy = 0.0f;
    float highEnergy = 0.0f;
    for (int i = 0; i < 4800; ++i)
    {
        const auto time = static_cast<float>(i) / 48000.0f;
        const auto low = highPass.process(std::sin(2.0f * 3.14159265f * 500.0f * time));
        if (i > 100) lowEnergy += low * low;
    }
    highPass.reset();
    for (int i = 0; i < 4800; ++i)
    {
        const auto time = static_cast<float>(i) / 48000.0f;
        const auto high = highPass.process(std::sin(2.0f * 3.14159265f * 8000.0f * time));
        if (i > 100) highEnergy += high * high;
    }
    expect(highEnergy > lowEnergy * 20.0f, "high-pass detector rejects low frequencies");

    DetectorFilter bandPass;
    bandPass.prepare(48000.0);
    bandPass.setModeAndFrequency(DetectorMode::bandPass, 5500.0f);
    float output = 0.0f;
    for (int i = 0; i < 10000; ++i) output = bandPass.process(i == 0 ? 1.0f : 0.0f);
    expect(std::isfinite(output), "band-pass remains stable");

    auto measureBandPassEnergy = [](float toneFrequency)
    {
        DetectorFilter filter;
        filter.prepare(48000.0);
        filter.setModeAndFrequency(DetectorMode::bandPass, 5500.0f);
        float energy = 0.0f;
        for (int i = 0; i < 4800; ++i)
        {
            const auto time = static_cast<float>(i) / 48000.0f;
            const auto sample = filter.process(
                std::sin(2.0f * 3.14159265f * toneFrequency * time));
            if (i > 100) energy += sample * sample;
        }
        return energy;
    };
    const auto centreEnergy = measureBandPassEnergy(5500.0f);
    expect(centreEnergy > measureBandPassEnergy(500.0f) * 8.0f,
           "band-pass detector rejects frequencies below its centre");
    expect(centreEnergy > measureBandPassEnergy(16000.0f) * 2.0f,
           "band-pass detector rejects frequencies above its centre");

    for (const auto sampleRate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        DetectorFilter stabilityFilter;
        stabilityFilter.prepare(sampleRate);
        stabilityFilter.setModeAndFrequency(DetectorMode::highPass, 16000.0f);
        float value = 1.0f;
        for (int i = 0; i < 4096; ++i)
            value = stabilityFilter.process(i == 0 ? 1.0f : 0.0f);
        expect(std::isfinite(value), "detector remains stable at supported sample rates");
    }

    {
        using namespace g3x::deesser;
        DeEsserEngine engine;
        engine.prepare({ 48000.0, 512, 2 });
        EngineParameters parameters;
        parameters.thresholdDb = 0.0f;
        parameters.splitMode = true;
        engine.setParameters(parameters);
        juce::AudioBuffer<float> audio(2, 4096);
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                audio.setSample(channel, sample, 0.4f * std::sin(
                    2.0f * 3.14159265f * 997.0f * static_cast<float>(sample) / 48000.0f));
        juce::AudioBuffer<float> reference;
        reference.makeCopyOf(audio);
        engine.process(audio);
        float maximumError = 0.0f;
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                maximumError = std::max(maximumError, std::abs(
                    audio.getSample(channel, sample) - reference.getSample(channel, sample)));
        expect(maximumError < 1.0e-6f, "split mode reconstructs unity signal exactly");

        parameters.thresholdDb = -60.0f;
        parameters.rangeDb = 8.0f;
        parameters.splitMode = false;
        engine.setParameters(parameters);
        audio.setSize(2, 8192, false, false, true);
        for (int sample = 0; sample < audio.getNumSamples(); ++sample)
        {
            audio.setSample(0, sample, 0.9f * std::sin(
                2.0f * 3.14159265f * 8000.0f * static_cast<float>(sample) / 48000.0f));
            audio.setSample(1, sample, 0.5f);
        }
        engine.process(audio);
        expect(audio.getSample(1, audio.getNumSamples() - 1) < 0.25f,
               "linked detector applies gain reduction to both channels");
        expect(engine.getReductionDb() >= -8.001f && engine.getReductionDb() < -7.9f,
               "engine reduction respects Range");

        audio.clear();
        audio.setSample(0, 0, std::numeric_limits<float>::quiet_NaN());
        engine.process(audio);
        bool allFinite = true;
        for (int channel = 0; channel < audio.getNumChannels(); ++channel)
            for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                allFinite = allFinite && std::isfinite(audio.getSample(channel, sample));
        expect(allFinite, "engine sanitises non-finite input without poisoning state");

        parameters.bypass = true;
        parameters.frequencyHz = std::numeric_limits<float>::quiet_NaN();
        parameters.thresholdDb = std::numeric_limits<float>::infinity();
        engine.setParameters(parameters);
        audio.setSize(2, 2048, false, false, true);
        audio.clear();
        for (int sample = 0; sample < audio.getNumSamples(); ++sample)
            audio.setSample(0, sample, 0.25f);
        engine.process(audio);
        near(audio.getSample(0, audio.getNumSamples() - 1), 0.25f, 1.0e-5f,
             "bypass ramp reaches transparent output");
    }

    if (failures == 0) std::cout << "All G3X DeEsser DSP tests passed\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
