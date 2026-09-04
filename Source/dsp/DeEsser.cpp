#include "DeEsser.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace g3x::deesser::dsp
{
float decibelsToGain(float decibels) noexcept
{
    if (!std::isfinite(decibels))
        return 0.0f;
    return decibels <= minimumDecibels ? 0.0f : std::pow(10.0f, decibels / 20.0f);
}

float gainToDecibels(float gain) noexcept
{
    return !std::isfinite(gain) || gain <= 0.0f ? minimumDecibels
                        : std::max(minimumDecibels, 20.0f * std::log10(gain));
}

void DetectorFilter::prepare(double newSampleRate) noexcept
{
    sampleRate = std::max(1.0, newSampleRate);
    reset();
    updateCoefficients();
}

void DetectorFilter::setModeAndFrequency(DetectorMode newMode, float frequencyHz) noexcept
{
    mode = newMode;
    const auto safeFrequency = std::isfinite(frequencyHz) ? frequencyHz : 5500.0f;
    frequency = std::clamp(safeFrequency, 2000.0f,
                           static_cast<float>(sampleRate * 0.45));
    updateCoefficients();
}

void DetectorFilter::reset() noexcept
{
    z1 = 0.0f;
    z2 = 0.0f;
}

void DetectorFilter::updateCoefficients() noexcept
{
    constexpr auto q = 0.70710678f;
    const auto omega = 2.0f * std::numbers::pi_v<float> * frequency
                     / static_cast<float>(sampleRate);
    const auto cosine = std::cos(omega);
    const auto sine = std::sin(omega);
    const auto alpha = sine / (2.0f * q);
    const auto a0 = 1.0f + alpha;

    if (mode == DetectorMode::highPass)
    {
        b = { (1.0f + cosine) * 0.5f / a0, -(1.0f + cosine) / a0,
              (1.0f + cosine) * 0.5f / a0 };
    }
    else
    {
        b = { alpha / a0, 0.0f, -alpha / a0 };
    }
    a = { (-2.0f * cosine) / a0, (1.0f - alpha) / a0 };
}

float DetectorFilter::process(float input) noexcept
{
    if (!std::isfinite(input))
        input = 0.0f;
    const auto output = b[0] * input + z1;
    z1 = b[1] * input - a[0] * output + z2;
    z2 = b[2] * input - a[1] * output;
    return output;
}

void EnvelopeFollower::prepare(double newSampleRate) noexcept
{
    sampleRate = std::max(1.0, newSampleRate);
    reset();
    updateCoefficients();
}

void EnvelopeFollower::setTimes(float attackMs, float releaseMs) noexcept
{
    attack = std::max(0.01f, attackMs);
    release = std::max(0.01f, releaseMs);
    updateCoefficients();
}

void EnvelopeFollower::updateCoefficients() noexcept
{
    const auto coefficient = [this](float milliseconds)
    {
        return std::exp(-1.0f / (0.001f * milliseconds * static_cast<float>(sampleRate)));
    };
    attackCoefficient = coefficient(attack);
    releaseCoefficient = coefficient(release);
}

float EnvelopeFollower::process(float magnitude) noexcept
{
    const auto target = std::isfinite(magnitude) ? std::abs(magnitude) : 0.0f;
    const auto coefficient = target > envelope ? attackCoefficient : releaseCoefficient;
    envelope = target + coefficient * (envelope - target);
    return envelope;
}

float GainComputer::computeGainDb(float detectorDb, float threshold, float maximumReduction,
                                  float ratio) noexcept
{
    if (!std::isfinite(detectorDb) || !std::isfinite(threshold)
        || !std::isfinite(maximumReduction) || !std::isfinite(ratio))
        return 0.0f;
    if (detectorDb <= threshold || ratio <= 1.0f)
        return 0.0f;
    const auto overshoot = detectorDb - threshold;
    const auto reduction = overshoot * (1.0f - 1.0f / ratio);
    return -std::min(std::max(0.0f, maximumReduction), reduction);
}

float GainComputer::process(float detectorEnvelope) noexcept
{
    reductionDb = computeGainDb(gainToDecibels(detectorEnvelope), thresholdDb, rangeDb);
    return decibelsToGain(reductionDb);
}
}
