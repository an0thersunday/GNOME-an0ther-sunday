#include "PluginEditor.h"

//==============================================================================
// Sprite pixel-art del gnomo (16 columnas x 19 filas)
namespace
{
    constexpr int spriteW = 16, spriteH = 19, spriteScale = 7;

    const char* gnomeSprite[spriteH] =
    {
        ".......RR.......",
        "......RRRR......",
        "......RRRR......",
        ".....RRRRRR.....",
        ".....RRRRRR.....",
        "....RRRRRRRR....",
        "...RRRRRRRRRR...",
        "..DDDDDDDDDDDD..",
        "...SSSSSSSSSS...",
        "...SSKSSSSKSS...",
        "...SSSSNNSSSS...",
        "..WWWWWWWWWWWW..",
        "..WWWWWWWWWWWW..",
        "...WWWWWWWWWW...",
        "....WWWWWWWW....",
        ".....WWWWWW.....",
        "....BBBBBBBB....",
        "...BBBBBBBBBB...",
        "...GG......GG..."
    };

    juce::Colour spriteColour (char c)
    {
        switch (c)
        {
            case 'R': return juce::Colour (0xffd9373d);
            case 'D': return juce::Colour (0xff8b1e2d);
            case 'S': return juce::Colour (0xfff2c18d);
            case 'K': return juce::Colour (0xff1a1a1a);
            case 'N': return juce::Colour (0xffd98a6a);
            case 'W': return juce::Colour (0xfff4f4f4);
            case 'B': return juce::Colour (0xff3a6bd8);
            case 'G': return juce::Colour (0xff6b3f1d);
            default:  return juce::Colours::transparentBlack;
        }
    }
}

//==============================================================================
PixelLookAndFeel::PixelLookAndFeel()
{
    setColour (juce::ComboBox::textColourId, pix::text);
    setColour (juce::ComboBox::backgroundColourId, pix::dark);
    setColour (juce::ComboBox::outlineColourId, pix::accent);
    setColour (juce::PopupMenu::backgroundColourId, pix::dark);
    setColour (juce::PopupMenu::textColourId, pix::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, pix::accent);
    setColour (juce::PopupMenu::highlightedTextColourId, pix::dark);
    setColour (juce::Label::textColourId, pix::text);
    setColour (juce::Slider::textBoxTextColourId, pix::accent2);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void PixelLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).reduced (4);
    const int size = (juce::jmin (bounds.getWidth(), bounds.getHeight()) / 8) * 8;
    auto r = juce::Rectangle<int> (size, size).withCentre (bounds.getCentre());

    g.setColour (pix::dark);
    g.fillRect (r);
    g.setColour (pix::accent);
    g.drawRect (r, 4);

    const float angle = startAngle + pos * (endAngle - startAngle);
    const float cx = (float) r.getCentreX(), cy = (float) r.getCentreY();
    const float len = (float) size * 0.38f;

    g.setColour (pix::accent2);
    for (int i = 0; i <= 8; ++i)
    {
        const float t = (float) i / 8.0f;
        const int px = ((int) (cx + std::sin (angle) * len * t) / 4) * 4;
        const int py = ((int) (cy - std::cos (angle) * len * t) / 4) * 4;
        g.fillRect (px - 2, py - 2, 4, 4);
    }
}

void PixelLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    g.setColour (pix::dark);
    g.fillRect (0, 0, w, h);
    g.setColour (pix::accent);
    g.drawRect (0, 0, w, h, 3);

    g.setColour (pix::accent2);
    g.fillRect (w - 26, h / 2 - 6, 12, 4);
    g.fillRect (w - 24, h / 2 - 2, 8, 4);
    g.fillRect (w - 22, h / 2 + 2, 4, 4);
}

//==============================================================================
GnomeAudioProcessorEditor::GnomeAudioProcessorEditor (GnomeAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (680, 380);

    for (auto* c : std::initializer_list<juce::Component*> { &wave1, &wave2, &vol1, &vol2, &gain,
                                                             &vol1Label, &vol2Label, &gainLabel })
    {
        c->setLookAndFeel (&laf);
        addAndMakeVisible (c);
    }

    wave1.addItemList ({ "SINE", "SQUARE", "SAW" }, 1);
    wave2.addItemList ({ "SINE", "SQUARE", "SAW" }, 1);

    setupKnob (vol1);
    setupKnob (vol2);
    setupKnob (gain);
    setupLabel (vol1Label, "VOL");
    setupLabel (vol2Label, "VOL");
    setupLabel (gainLabel, "OUTPUT GAIN");

    wave1Att = std::make_unique<ComboAtt>  (proc.apvts, "osc1_wave", wave1);
    wave2Att = std::make_unique<ComboAtt>  (proc.apvts, "osc2_wave", wave2);
    vol1Att  = std::make_unique<SliderAtt> (proc.apvts, "osc1_vol",  vol1);
    vol2Att  = std::make_unique<SliderAtt> (proc.apvts, "osc2_vol",  vol2);
    gainAtt  = std::make_unique<SliderAtt> (proc.apvts, "out_gain",  gain);

    startTimer (450);
}

GnomeAudioProcessorEditor::~GnomeAudioProcessorEditor()
{
    for (auto* c : std::initializer_list<juce::Component*> { &wave1, &wave2, &vol1, &vol2, &gain,
                                                             &vol1Label, &vol2Label, &gainLabel })
        c->setLookAndFeel (nullptr);
}

void GnomeAudioProcessorEditor::setupKnob (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 22);
}

void GnomeAudioProcessorEditor::setupLabel (juce::Label& l, const juce::String& t)
{
    l.setText (t, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
}

void GnomeAudioProcessorEditor::timerCallback()
{
    bob = 1 - bob;
    repaint (gnomeArea);
}

void GnomeAudioProcessorEditor::resized()
{
    auto content = getLocalBounds().withTrimmedTop (90).reduced (16);
    gnomeArea = content.removeFromLeft (140);
    panelOsc1 = content.removeFromLeft (180); content.removeFromLeft (12);
    panelOsc2 = content.removeFromLeft (180); content.removeFromLeft (12);
    panelOut  = content;

    auto layoutOsc = [] (juce::Rectangle<int> p, juce::ComboBox& wave, juce::Label& lbl, juce::Slider& vol)
    {
        auto in = p.reduced (12);
        in.removeFromTop (28);
        wave.setBounds (in.removeFromTop (32));
        in.removeFromTop (10);
        lbl.setBounds (in.removeFromTop (18));
        vol.setBounds (in);
    };

    layoutOsc (panelOsc1, wave1, vol1Label, vol1);
    layoutOsc (panelOsc2, wave2, vol2Label, vol2);

    auto out = panelOut.reduced (8);
    out.removeFromTop (28);
    gainLabel.setBounds (out.removeFromTop (18));
    gain.setBounds (out);
}

void GnomeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (pix::bg);

    // Fondo de puntos pixelados
    g.setColour (pix::bgDot);
    for (int yy = 0; yy < getHeight(); yy += 16)
        for (int xx = ((yy / 16) % 2) * 8; xx < getWidth(); xx += 16)
            g.fillRect (xx, yy, 4, 4);

    // Título
    g.setFont (pix::font (44.0f));
    g.setColour (pix::dark);
    g.drawText ("GNOME", 24 + 4, 12 + 4, 300, 50, juce::Justification::left);
    g.setColour (pix::accent);
    g.drawText ("GNOME", 24, 12, 300, 50, juce::Justification::left);

    g.setFont (pix::font (14.0f));
    g.setColour (pix::accent2);
    g.drawText ("2-OSC CHIPTUNE SYNTH", 28, 58, 300, 20, juce::Justification::left);

    // Paneles
    auto drawPanel = [&g] (juce::Rectangle<int> r, const juce::String& title)
    {
        g.setColour (pix::panel);
        g.fillRect (r);
        g.setColour (pix::accent);
        g.drawRect (r, 4);
        g.setFont (pix::font (16.0f));
        g.setColour (pix::text);
        g.drawText (title, r.getX() + 12, r.getY() + 8, r.getWidth() - 24, 20, juce::Justification::left);
    };
    drawPanel (panelOsc1, "OSC 1");
    drawPanel (panelOsc2, "OSC 2");
    drawPanel (panelOut,  "OUT");

    // Gnomo
    const int sx = gnomeArea.getX() + (gnomeArea.getWidth() - spriteW * spriteScale) / 2;
    const int sy = gnomeArea.getY() + 20 + bob * spriteScale;

    for (int row = 0; row < spriteH; ++row)
        for (int col = 0; col < spriteW; ++col)
        {
            const char c = gnomeSprite[row][col];
            if (c == '.') continue;
            g.setColour (spriteColour (c));
            g.fillRect (sx + col * spriteScale, sy + row * spriteScale, spriteScale, spriteScale);
        }
}
