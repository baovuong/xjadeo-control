/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include <JuceHeader.h>

//==============================================================================
/**
*/
class XJadeoControlAudioProcessor  : public juce::AudioProcessor,
                                      public juce::ChangeBroadcaster,
                                      private juce::MultiTimer
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

    // Cue points start above the play/pause notes (36/37).
    static constexpr int firstCueNoteNumber = 38;

    // Cue point frame numbers, sorted and de-duplicated. A cue's MIDI note is derived
    // from its position in this set rather than stored explicitly (see MainComponent).
    juce::SortedSet<int> cuePoints;

    juce::File videoFile;
    bool isPlaying = false;

    int currentFrame = 0;
    juce::int64 numFrames = 0;

    // Mutators broadcast a change message so any listening UI can refresh,
    // including when cuePoints is replaced wholesale by setStateInformation().
    void addCuePoint (int frame);
    void removeCuePoint (int frame);

    // Transport control. Drives xjadeo over OSC and, for play/pause, its own
    // timer directly from the processor, so playback and MIDI-triggered cues
    // keep working whether or not a GUI editor is currently open.
    void play();
    void pause();
    void seek (int frame);
    void loadVideoFile (const juce::File& file);

private:
    //==============================================================================
    enum TimerId
    {
        frameTimerId = 0,
        midiPollTimerId
    };

    void timerCallback (int timerID) override;
    void sendFrame (int frame);
    void triggerCueForNote (int midiNote);

    // Called from the message thread to drain MIDI note-on numbers received since the last call.
    // Returns false once no more are available. Single-consumer: call from one thread only.
    bool popNoteOn (int& noteNumberOut) noexcept;

    juce::OSCSender oscSender;

    static constexpr int midiNoteBufferSize = 256;
    juce::AbstractFifo midiNoteFifo { midiNoteBufferSize };
    std::array<int, midiNoteBufferSize> midiNoteBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XJadeoControlAudioProcessor)
};
