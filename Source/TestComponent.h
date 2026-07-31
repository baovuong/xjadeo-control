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

    void timerCallback() override;

    juce::TextButton playButton  { "Play" };
    juce::TextButton pauseButton { "Pause" };
    juce::Label frameLabel       { {}, "Frame:" };
    juce::TextEditor frameInput;

    juce::OSCSender oscSender;

    int currentFrame = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestComponent)
};
