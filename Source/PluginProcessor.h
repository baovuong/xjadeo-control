/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "juce_audio_processors/juce_audio_processors.h"
#include <JuceHeader.h>

//==============================================================================
/**
*/
class XJadeoControlAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    XJadeoControlAudioProcessor();
    ~XJadeoControlAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // MIDI note numbers (C3 = middle C / note 60 convention) that trigger transport control.
    static constexpr int playNoteNumber  = 36; // C1
    static constexpr int pauseNoteNumber = 37; // C#1

    struct CuePoint
    {
        int midiNote;
        int frame;
    };

    juce::Array<CuePoint> cuePoints;

    // Called from the message thread to drain MIDI note-on numbers received since the last call.
    // Returns false once no more are available. Single-consumer: call from one thread only.
    bool popNoteOn (int& noteNumberOut) noexcept
    {
        int start1, size1, start2, size2;
        midiNoteFifo.prepareToRead (1, start1, size1, start2, size2);

        if (size1 > 0)
            noteNumberOut = midiNoteBuffer[(size_t) start1];
        else if (size2 > 0)
            noteNumberOut = midiNoteBuffer[(size_t) start2];
        else
            return false;

        midiNoteFifo.finishedRead (size1 + size2);
        return true;
    }

private:
    //==============================================================================
    static constexpr int midiNoteBufferSize = 256;
    juce::AbstractFifo midiNoteFifo { midiNoteBufferSize };
    std::array<int, midiNoteBufferSize> midiNoteBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XJadeoControlAudioProcessor)
};
