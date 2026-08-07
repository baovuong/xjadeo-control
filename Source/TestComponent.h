/*
  ==============================================================================

    TestComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class TestComponent  : public juce::Component,
                        private juce::Timer
{
public:
    TestComponent();
    ~TestComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void sendPlay();
    void sendPause();
    void sendFrame (int frame);
    void updateFrame(int frame);
    void loadFile();
    void sendLoadFile (const juce::File& file);

    void timerCallback() override;

    juce::TextButton playButton      { "Play" };
    juce::TextButton pauseButton     { "Pause" };
    juce::TextButton loadFileButton  { "Load File" };
    juce::Label frameLabel       { {}, "Frame:" };
    juce::TextEditor frameInput;
    juce::Label numFramesLabel;

    juce::OSCSender oscSender;

    juce::File videoFile;
    std::unique_ptr<juce::FileChooser> fileChooser;

    int currentFrame = 0;
    juce::int64 numFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestComponent)
};
