#pragma once
#include "PluginProcessor.h"

namespace pix
{
    const juce::Colour bg      { 0xff1b1530 };
    const juce::Colour bgDot   { 0xff241c40 };
    const juce::Colour panel   { 0xff2a2150 };
    const juce::Colour dark    { 0xff120e22 };
    const juce::Colour accent  { 0xffffd23f };
    const juce::Colour accent2 { 0xff4ee0a0 };
    const juce::Colour text    { 0xfff4f4f4 };

    inline juce::Font font (float h)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::bold));
    }
}

class PixelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PixelLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;

    juce::Font getLabelFont (juce::Label&) override        { return pix::font (14.0f); }
    juce::Font getComboBoxFont (juce::ComboBox&) override  { return pix::font (14.0f); }
    juce::Font getPopupMenuFont() override                 { return pix::font (14.0f); }
};

class GnomeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit GnomeAudioProcessorEditor (GnomeAudioProcessor&);
    ~GnomeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider&);
    void setupLabel (juce::Label&, const juce::String&);

    GnomeAudioProcessor& proc;
    PixelLookAndFeel laf;

    juce::ComboBox wave1, wave2;
    juce::Slider vol1, vol2, gain;
    juce::Label vol1Label, vol2Label, gainLabel;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboAtt>  wave1Att, wave2Att;
    std::unique_ptr<SliderAtt> vol1Att, vol2Att, gainAtt;

    juce::Rectangle<int> panelOsc1, panelOsc2, panelOut, gnomeArea;
    int bob = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GnomeAudioProcessorEditor)
};
