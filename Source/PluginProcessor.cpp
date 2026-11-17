#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    struct GnomeSound : public juce::SynthesiserSound
    {
        bool appliesToNote (int) override    { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    // 0 = seno, 1 = cuadrada, 2 = diente de sierra
    inline float oscSample (float phase, int type)
    {
        switch (type)
        {
            case 0:  return std::sin (juce::MathConstants<float>::twoPi * phase);
            case 1:  return phase < 0.5f ? 1.0f : -1.0f;
            default: return 2.0f * phase - 1.0f;
        }
    }

    struct GnomeVoice : public juce::SynthesiserVoice
    {
        explicit GnomeVoice (juce::AudioProcessorValueTreeState& s)
        {
            wave1 = s.getRawParameterValue ("osc1_wave");
            wave2 = s.getRawParameterValue ("osc2_wave");
            vol1  = s.getRawParameterValue ("osc1_vol");
            vol2  = s.getRawParameterValue ("osc2_vol");
        }

        bool canPlaySound (juce::SynthesiserSound* s) override
        {
            return dynamic_cast<GnomeSound*> (s) != nullptr;
        }

        void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
        {
            freq  = (float) juce::MidiMessage::getMidiNoteInHertz (midiNote);
            level = velocity;
            phase1 = phase2 = 0.0f;
            adsr.setSampleRate (getSampleRate());
            adsr.setParameters ({ 0.005f, 0.05f, 0.85f, 0.08f });
            adsr.noteOn();
        }

        void stopNote (float, bool allowTailOff) override
        {
            if (allowTailOff)
                adsr.noteOff();
            else
            {
                adsr.reset();
                clearCurrentNote();
            }
        }

        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}

        void renderNextBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override
        {
            if (! adsr.isActive())
                return;

            const int   w1 = (int) wave1->load();
            const int   w2 = (int) wave2->load();
            const float v1 = vol1->load();
            const float v2 = vol2->load();
            const float inc = freq / (float) getSampleRate();

            for (int i = 0; i < numSamples; ++i)
            {
                const float env = adsr.getNextSample();
                const float out = (oscSample (phase1, w1) * v1 + oscSample (phase2, w2) * v2)
                                  * 0.5f * level * env;

                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    buffer.addSample (ch, startSample + i, out);

                phase1 += inc; if (phase1 >= 1.0f) phase1 -= 1.0f;
                phase2 += inc; if (phase2 >= 1.0f) phase2 -= 1.0f;

                if (! adsr.isActive())
                {
                    clearCurrentNote();
                    break;
                }
            }
        }

        std::atomic<float> *wave1 {}, *wave2 {}, *vol1 {}, *vol2 {};
        juce::ADSR adsr;
        float freq = 440.0f, level = 1.0f, phase1 = 0.0f, phase2 = 0.0f;
    };
}

juce::AudioProcessorValueTreeState::ParameterLayout GnomeAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    const StringArray waves { "SINE", "SQUARE", "SAW" };

    auto percent = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; });

    auto decibels = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (v, 1) + " dB"; });

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "osc1_wave", 1 }, "Osc 1 Wave", waves, 0));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "osc1_vol", 1 },  "Osc 1 Volume",
                                                        NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f, percent));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "osc2_wave", 1 }, "Osc 2 Wave", waves, 2));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "osc2_vol", 1 },  "Osc 2 Volume",
                                                        NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f, percent));
    layout.add (std::make_unique<AudioParameterFloat>  (ParameterID { "out_gain", 1 },  "Output Gain",
                                                        NormalisableRange<float> (-48.0f, 6.0f, 0.1f), -6.0f, decibels));
    return layout;
}

GnomeAudioProcessor::GnomeAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    outGain = apvts.getRawParameterValue ("out_gain");

    synth.addSound (new GnomeSound());
    for (int i = 0; i < 8; ++i)
        synth.addVoice (new GnomeVoice (apvts));
}

void GnomeAudioProcessor::prepareToPlay (double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
}

bool GnomeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void GnomeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());

    const float gain = juce::Decibels::decibelsToGain (outGain->load());
    buffer.applyGain (gain);

    // Limitador de seguridad
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            d[i] = juce::jlimit (-1.0f, 1.0f, d[i]);
    }
}

juce::AudioProcessorEditor* GnomeAudioProcessor::createEditor()
{
    return new GnomeAudioProcessorEditor (*this);
}

void GnomeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void GnomeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GnomeAudioProcessor();
}
