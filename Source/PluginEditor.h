#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

namespace g3x::deesser
{
class DeEsserLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    DeEsserLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float,
                          juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class DeEsserMeter final : public juce::Component
{
public:
    enum class Kind { detector, reduction, output };
    DeEsserMeter(Kind, juce::String labelText);
    void setValue(float decibels);
    void paint(juce::Graphics&) override;

private:
    Kind kind;
    juce::String label;
    float valueDb = -120.0f;
    float heldDb = -120.0f;
    int holdTicks = 0;
};

class DeEsserAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit DeEsserAudioProcessorEditor(DeEsserAudioProcessor&);
    ~DeEsserAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void configureRotary(juce::Slider&, const juce::String&, double);
    void drawPanel(juce::Graphics&, juce::Rectangle<int>, const juce::String&, bool = false) const;

    DeEsserAudioProcessor& processor;
    DeEsserLookAndFeel lookAndFeel;
    juce::Slider frequencySlider;
    juce::Slider thresholdSlider;
    juce::Slider rangeSlider;
    juce::ComboBox processingBox;
    juce::ComboBox detectorBox;
    juce::ComboBox monitorBox;
    juce::ComboBox presetBox;
    juce::ToggleButton bypassButton { "BYPASS" };
    DeEsserMeter detectorMeter { DeEsserMeter::Kind::detector, "DET" };
    DeEsserMeter reductionMeter { DeEsserMeter::Kind::reduction, "GR" };
    DeEsserMeter outputLeftMeter { DeEsserMeter::Kind::output, "L" };
    DeEsserMeter outputRightMeter { DeEsserMeter::Kind::output, "R" };
    juce::Label activityLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> frequencyAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttachment;
    std::unique_ptr<SliderAttachment> rangeAttachment;
    std::unique_ptr<ComboAttachment> processingAttachment;
    std::unique_ptr<ComboAttachment> detectorAttachment;
    std::unique_ptr<ComboAttachment> monitorAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeEsserAudioProcessorEditor)
};
}
