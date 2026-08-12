/*
  ==============================================================================

    TestComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class TestComponent  : public juce::Component,
                        private juce::MultiTimer
{
public:
    explicit TestComponent (XJadeoControlAudioProcessor& processor);
    ~TestComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum TimerId
    {
        frameTimer = 0,
        midiPollTimer
    };

    void sendPlay();
    void sendPause();
    void sendFrame (int frame);
    void updateFrame (int frame);
    void updateFrameLabel();
    void loadFile();
    void sendLoadFile (const juce::File& file);

    void timerCallback (int timerID) override;

    XJadeoControlAudioProcessor& audioProcessor;

    juce::TextButton playButton      { "Play" };
    juce::TextButton pauseButton     { "Pause" };
    juce::TextButton loadFileButton  { "Load File" };
    juce::Label frameLabel;
    juce::Slider frameSlider;

    juce::OSCSender oscSender;

    juce::File videoFile;
    std::unique_ptr<juce::FileChooser> fileChooser;

    int currentFrame = 0;
    juce::int64 numFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestComponent)
};
