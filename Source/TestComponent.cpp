/*
  ==============================================================================

    TestComponent.cpp

  ==============================================================================
*/

#include "TestComponent.h"

namespace
{
    const juce::String xjadeoHost = "127.0.0.1";
    constexpr int xjadeoPort = 7890;
    constexpr int framesPerSecond = 25;
}

//==============================================================================
TestComponent::TestComponent()
{
    oscSender.connect (xjadeoHost, xjadeoPort);

    playButton.onClick = [this] { sendPlay(); };
    addAndMakeVisible (playButton);

    pauseButton.onClick = [this] { sendPause(); };
    addAndMakeVisible (pauseButton);

    addAndMakeVisible (frameLabel);

    frameInput.setInputRestrictions (0, "0123456789");
    frameInput.setText ("0", juce::dontSendNotification);
    frameInput.onReturnKey = [this] { sendFrame (frameInput.getText().getIntValue()); };
    addAndMakeVisible (frameInput);
}

TestComponent::~TestComponent()
{
    stopTimer();
}

void TestComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void TestComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    auto buttonRow = bounds.removeFromTop (30);
    playButton.setBounds (buttonRow.removeFromLeft (80));
    buttonRow.removeFromLeft (10);
    pauseButton.setBounds (buttonRow.removeFromLeft (80));

    bounds.removeFromTop (10);

    auto frameRow = bounds.removeFromTop (30);
    frameLabel.setBounds (frameRow.removeFromLeft (60));
    frameInput.setBounds (frameRow.removeFromLeft (100));
}

void TestComponent::sendPlay()
{
    startTimerHz (framesPerSecond);
}

void TestComponent::sendPause()
{
    stopTimer();
}

void TestComponent::sendFrame (int frame)
{
    oscSender.send ("/jadeo/seek", frame);
    currentFrame = frame;
}

void TestComponent::timerCallback()
{
    sendFrame (currentFrame + 1);
}
