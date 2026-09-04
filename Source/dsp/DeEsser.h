#pragma once

#include <array>

namespace g3x::deesser::dsp
{
constexpr float minimumDecibels = -120.0f;
float decibelsToGain(float decibels) noexcept;
float gainToDecibels(float gain) noexcept;

enum class DetectorMode { highPass, bandPass };

class DetectorFilter
{
public:
    void prepare(double newSampleRate) noexcept;
    void setModeAndFrequency(DetectorMode newMode, float frequencyHz) noexcept;
    void reset() noexcept;
    float process(float input) noexcept;

private:
    void updateCoefficients() noexcept;
    double sampleRate = 44100.0;
    float frequency = 5500.0f;
    DetectorMode mode = DetectorMode::highPass;
    std::array<float, 3> b { 1.0f, 0.0f, 0.0f };
    std::array<float, 2> a {};
    float z1 = 0.0f;
    float z2 = 0.0f;
};

class EnvelopeFollower
{
public:
    void prepare(double newSampleRate) noexcept;
    void setTimes(float attackMs, float releaseMs) noexcept;
    void reset() noexcept { envelope = 0.0f; }
    float process(float magnitude) noexcept;
    float getCurrentValue() const noexcept { return envelope; }

private:
    void updateCoefficients() noexcept;
    double sampleRate = 44100.0;
    float attack = 0.5f;
    float release = 80.0f;
    float attackCoefficient = 0.0f;
    float releaseCoefficient = 0.0f;
    float envelope = 0.0f;
};

class GainComputer
{
public:
    void setThresholdDb(float value) noexcept { thresholdDb = value; }
    void setRangeDb(float value) noexcept { rangeDb = value; }
    float process(float detectorEnvelope) noexcept;
    static float computeGainDb(float detectorDb, float thresholdDb, float rangeDb,
                               float ratio = 6.0f) noexcept;
    float getReductionDb() const noexcept { return reductionDb; }

private:
    float thresholdDb = -30.0f;
    float rangeDb = 8.0f;
    float reductionDb = 0.0f;
};
}
