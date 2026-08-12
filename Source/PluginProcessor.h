/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

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

    bool consumePlayRequest() noexcept  { return playRequested.exchange (false); }
    bool consumePauseRequest() noexcept { return pauseRequested.exchange (false); }

private:
    //==============================================================================
    std::atomic<bool> playRequested  { false };
    std::atomic<bool> pauseRequested { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XJadeoControlAudioProcessor)
};
