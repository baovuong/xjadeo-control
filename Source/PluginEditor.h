/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "TestComponent.h"

//==============================================================================
/**
*/
class XJadeoControlAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    XJadeoControlAudioProcessorEditor (XJadeoControlAudioProcessor&);
    ~XJadeoControlAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    TestComponent testComponent;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    XJadeoControlAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XJadeoControlAudioProcessorEditor)
};
