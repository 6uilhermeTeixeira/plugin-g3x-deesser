#include "PluginEditor.h"
#include <cmath>

namespace g3x::deesser
{
namespace Palette
{
const auto background = juce::Colour::fromRGB(10, 19, 24);
const auto surface = juce::Colour::fromRGB(18, 32, 39);
const auto surfaceRaised = juce::Colour::fromRGB(25, 43, 51);
const auto edge = juce::Colour::fromRGB(48, 73, 82);
const auto cyan = juce::Colour::fromRGB(48, 205, 190);
const auto mint = juce::Colour::fromRGB(126, 240, 200);
const auto coral = juce::Colour::fromRGB(255, 112, 92);
const auto text = juce::Colour::fromRGB(231, 241, 240);
const auto muted = juce::Colour::fromRGB(132, 157, 162);
}

DeEsserLookAndFeel::DeEsserLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, Palette::text);
    setColour(juce::Slider::textBoxBackgroundColourId, Palette::background);
    setColour(juce::Slider::textBoxOutlineColourId, Palette::edge);
    setColour(juce::ComboBox::backgroundColourId, Palette::background);
    setColour(juce::ComboBox::textColourId, Palette::text);
    setColour(juce::ComboBox::outlineColourId, Palette::edge);
    setColour(juce::ComboBox::arrowColourId, Palette::cyan);
    setColour(juce::PopupMenu::backgroundColourId, Palette::surface);
    setColour(juce::PopupMenu::textColourId, Palette::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Palette::cyan.darker(0.45f));
}

void DeEsserLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float position, float start, float end, juce::Slider& slider)
{
    const auto radius = 0.5f * static_cast<float>(std::min(width, height)) - 8.0f;
    const auto centre = juce::Point<float>(static_cast<float>(x + width / 2),
                                           static_cast<float>(y + height / 2));
    const auto bounds = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
    const auto angle = start + position * (end - start);
    g.setColour(Palette::background);
    g.fillEllipse(bounds);
    g.setColour(Palette::edge);
    g.drawEllipse(bounds, 2.0f);
    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f,
                      start, angle, true);
    g.setColour(Palette::cyan);
    g.strokePath(arc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
    juce::Path pointer;
    pointer.addRoundedRectangle(-2.0f, -radius + 11.0f, 4.0f, radius * 0.42f, 2.0f);
    g.setColour(Palette::mint);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    if (slider.hasKeyboardFocus(false))
    {
        g.setColour(Palette::text);
        g.drawRoundedRectangle(bounds.expanded(4.0f), 8.0f, 1.5f);
    }
}

void DeEsserLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                          bool highlighted, bool)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(button.getToggleState() ? Palette::coral.darker(0.2f) : Palette::background);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(button.getToggleState() ? Palette::coral
                                        : Palette::edge.brighter(highlighted ? 0.25f : 0.0f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.5f);
    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
    if (button.hasKeyboardFocus(false))
    {
        g.setColour(Palette::text);
        g.drawRoundedRectangle(bounds.reduced(3.0f), 4.0f, 1.0f);
    }
}

DeEsserMeter::DeEsserMeter(Kind meterKind, juce::String labelText)
    : kind(meterKind), label(std::move(labelText))
{
    setTitle(label + " meter");
    setDescription("Audio level meter updated from the processing engine");
    setInterceptsMouseClicks(false, false);
}

void DeEsserMeter::setValue(float decibels)
{
    const auto safe = std::isfinite(decibels) ? decibels : -120.0f;
    valueDb = safe;
    if (kind == Kind::reduction ? safe < heldDb : safe > heldDb)
    {
        heldDb = safe;
        holdTicks = 24;
    }
    else if (holdTicks > 0)
        --holdTicks;
    else
        heldDb += (safe - heldDb) * 0.12f;
    repaint();
}

void DeEsserMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    const auto caption = bounds.removeFromBottom(18);
    const auto track = bounds.toFloat().reduced(4.0f, 3.0f);
    g.setColour(Palette::background);
    g.fillRoundedRectangle(track, 4.0f);
    g.setColour(Palette::edge);
    g.drawRoundedRectangle(track, 4.0f, 1.0f);
    const auto normalized = kind == Kind::reduction
        ? juce::jlimit(0.0f, 1.0f, -valueDb / 24.0f)
        : juce::jlimit(0.0f, 1.0f, (valueDb + 60.0f) / 60.0f);
    auto fill = track.reduced(3.0f);
    fill.removeFromTop(fill.getHeight() * (1.0f - normalized));
    g.setGradientFill({ kind == Kind::reduction ? Palette::coral : Palette::mint,
                        fill.getCentreX(), fill.getY(), Palette::cyan.darker(0.5f),
                        fill.getCentreX(), fill.getBottom(), false });
    g.fillRoundedRectangle(fill, 2.0f);
    const auto heldNormalized = kind == Kind::reduction
        ? juce::jlimit(0.0f, 1.0f, -heldDb / 24.0f)
        : juce::jlimit(0.0f, 1.0f, (heldDb + 60.0f) / 60.0f);
    const auto heldY = track.getBottom() - heldNormalized * track.getHeight();
    g.setColour(Palette::text.withAlpha(0.8f));
    g.fillRect(track.getX() + 3.0f, heldY, track.getWidth() - 6.0f, 1.0f);
    g.setColour(Palette::muted);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText(label, caption, juce::Justification::centred);
}

DeEsserAudioProcessorEditor::DeEsserAudioProcessorEditor(DeEsserAudioProcessor& owner)
    : AudioProcessorEditor(owner), processor(owner)
{
    setLookAndFeel(&lookAndFeel);
    setResizable(true, true);
    setResizeLimits(680, 430, 1180, 760);
    setSize(820, 510);

    configureRotary(frequencySlider, "", 5500.0);
    configureRotary(thresholdSlider, " dB", -30.0);
    configureRotary(rangeSlider, " dB", 8.0);
    frequencySlider.setTitle("Frequency");
    frequencySlider.setDescription("Sets detector and split crossover frequency");
    thresholdSlider.setTitle("Threshold");
    thresholdSlider.setDescription("Sets the detector level that begins gain reduction");
    rangeSlider.setTitle("Range");
    rangeSlider.setDescription("Limits maximum attenuation in decibels");

    processingBox.addItemList({ "Split", "Wideband" }, 1);
    detectorBox.addItemList({ "High-pass", "Band-pass" }, 1);
    monitorBox.addItemList({ "Audio", "Sidechain" }, 1);
    processingBox.setTitle("Processing mode");
    processingBox.setDescription("Selects split-band or full-signal gain reduction");
    detectorBox.setTitle("Detector filter mode");
    detectorBox.setDescription("Selects high-pass or focused band-pass detection");
    monitorBox.setTitle("Monitor source");
    monitorBox.setDescription("Selects processed audio or detector sidechain monitoring");
    bypassButton.setTitle("Plugin bypass");
    bypassButton.setDescription("Smoothly bypasses all de-essing and monitoring");

    for (int index = 0; index < processor.getNumPrograms(); ++index)
        presetBox.addItem(processor.getProgramName(index), index + 1);
    presetBox.setSelectedItemIndex(processor.getCurrentProgram(), juce::dontSendNotification);
    presetBox.setTitle("Factory preset");
    presetBox.setDescription("Loads one of the original G3X starting points");
    presetBox.onChange = [this]
    {
        processor.setCurrentProgram(presetBox.getSelectedItemIndex());
    };

    activityLabel.setText("SIBILANCE", juce::dontSendNotification);
    activityLabel.setJustificationType(juce::Justification::centred);
    activityLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    activityLabel.setTitle("Gain reduction activity");

    for (auto* component : std::initializer_list<juce::Component*> {
             &frequencySlider, &thresholdSlider, &rangeSlider, &processingBox, &detectorBox,
             &monitorBox, &presetBox, &bypassButton, &detectorMeter, &reductionMeter,
             &outputLeftMeter, &outputRightMeter, &activityLabel })
        addAndMakeVisible(component);

    auto& state = processor.getParameters();
    frequencyAttachment = std::make_unique<SliderAttachment>(state, "frequencyHz", frequencySlider);
    thresholdAttachment = std::make_unique<SliderAttachment>(state, "thresholdDb", thresholdSlider);
    rangeAttachment = std::make_unique<SliderAttachment>(state, "rangeDb", rangeSlider);
    processingAttachment = std::make_unique<ComboAttachment>(state, "processingMode", processingBox);
    detectorAttachment = std::make_unique<ComboAttachment>(state, "detectorMode", detectorBox);
    monitorAttachment = std::make_unique<ComboAttachment>(state, "monitorMode", monitorBox);
    bypassAttachment = std::make_unique<ButtonAttachment>(state, "bypass", bypassButton);
    startTimerHz(40);
}

DeEsserAudioProcessorEditor::~DeEsserAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void DeEsserAudioProcessorEditor::configureRotary(juce::Slider& slider,
                                                   const juce::String& suffix, double resetValue)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 92, 25);
    slider.setTextValueSuffix(suffix);
    slider.setDoubleClickReturnValue(true, resetValue);
    slider.setScrollWheelEnabled(true);
    slider.setWantsKeyboardFocus(true);
}

void DeEsserAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds,
                                             const juce::String& title, bool highlighted) const
{
    g.setColour(Palette::surface);
    g.fillRoundedRectangle(bounds.toFloat(), 11.0f);
    g.setColour(highlighted ? Palette::cyan.withAlpha(0.75f) : Palette::edge);
    g.drawRoundedRectangle(bounds.toFloat(), 11.0f, highlighted ? 1.5f : 1.0f);
    g.setColour(highlighted ? Palette::mint : Palette::muted);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(title, bounds.removeFromTop(32), juce::Justification::centred);
}

void DeEsserAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.setGradientFill({ Palette::background.brighter(0.07f), 0.0f, 0.0f,
                        Palette::background.darker(0.18f), 0.0f,
                        static_cast<float>(getHeight()), false });
    g.fillAll();
    auto header = getLocalBounds().removeFromTop(76);
    g.setColour(Palette::cyan);
    g.fillRect(header.removeFromBottom(2));
    g.setColour(Palette::text);
    g.setFont(juce::FontOptions(27.0f, juce::Font::bold));
    g.drawText("G3X", header.reduced(24, 8).removeFromLeft(78), juce::Justification::centredLeft);
    g.setColour(Palette::mint);
    g.setFont(juce::FontOptions(18.0f, juce::Font::plain));
    g.drawText("DEESSER", header.reduced(100, 8), juce::Justification::centredLeft);

    auto content = getLocalBounds().withTrimmedTop(90).withTrimmedBottom(62).reduced(18, 0);
    const auto gap = 12;
    const auto leftWidth = content.getWidth() * 28 / 100;
    const auto rightWidth = content.getWidth() * 27 / 100;
    const auto left = content.removeFromLeft(leftWidth);
    content.removeFromLeft(gap);
    const auto right = content.removeFromRight(rightWidth);
    content.removeFromRight(gap);
    drawPanel(g, left, "FOCUS");
    drawPanel(g, content, "CONTROL", true);
    drawPanel(g, right, "RANGE / OUTPUT");

    auto footer = getLocalBounds().removeFromBottom(50).reduced(20, 7);
    g.setColour(Palette::muted);
    g.setFont(juce::FontOptions(9.5f));
    g.drawText("G3X AUDIO  ·  v0.1.0", footer.removeFromLeft(150),
               juce::Justification::centredLeft);
}

void DeEsserAudioProcessorEditor::resized()
{
    auto header = getLocalBounds().removeFromTop(76).reduced(20, 15);
    bypassButton.setBounds(header.removeFromRight(88));
    header.removeFromRight(10);
    presetBox.setBounds(header.removeFromRight(250));

    auto content = getLocalBounds().withTrimmedTop(90).withTrimmedBottom(62).reduced(18, 0);
    const auto gap = 12;
    const auto leftWidth = content.getWidth() * 28 / 100;
    const auto rightWidth = content.getWidth() * 27 / 100;
    auto left = content.removeFromLeft(leftWidth).reduced(13).withTrimmedTop(23);
    content.removeFromLeft(gap);
    auto right = content.removeFromRight(rightWidth).reduced(13).withTrimmedTop(23);
    content.removeFromRight(gap);
    auto centre = content.reduced(13).withTrimmedTop(23);

    processingBox.setBounds(left.removeFromTop(30));
    left.removeFromTop(8);
    detectorBox.setBounds(left.removeFromTop(30));
    left.removeFromTop(6);
    monitorBox.setBounds(left.removeFromBottom(30));
    frequencySlider.setBounds(left.reduced(5, 2));

    auto centreMeters = centre.removeFromRight(82);
    detectorMeter.setBounds(centreMeters.removeFromLeft(38));
    centreMeters.removeFromLeft(6);
    reductionMeter.setBounds(centreMeters.removeFromLeft(38));
    activityLabel.setBounds(centre.removeFromBottom(24));
    thresholdSlider.setBounds(centre.reduced(5, 0));

    auto outputMeters = right.removeFromRight(70);
    outputLeftMeter.setBounds(outputMeters.removeFromLeft(32));
    outputMeters.removeFromLeft(5);
    outputRightMeter.setBounds(outputMeters.removeFromLeft(32));
    rangeSlider.setBounds(right.reduced(3, 6));
}

void DeEsserAudioProcessorEditor::timerCallback()
{
    const auto reduction = processor.getReductionDb();
    detectorMeter.setValue(processor.getDetectorDb());
    reductionMeter.setValue(reduction);
    outputLeftMeter.setValue(dsp::gainToDecibels(processor.getOutputPeak(0)));
    outputRightMeter.setValue(dsp::gainToDecibels(processor.getOutputPeak(1)));
    const auto active = reduction < -0.1f;
    activityLabel.setColour(juce::Label::textColourId, active ? Palette::coral : Palette::muted);
    activityLabel.setText(active ? "SIBILANCE ACTIVE" : "SIBILANCE", juce::dontSendNotification);
}
}
