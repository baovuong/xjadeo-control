/*
  ==============================================================================

    TestComponent.cpp

  ==============================================================================
*/

#include "TestComponent.h"
#include "FfmpegVideoInfo.h"

namespace
{
    const juce::String xjadeoHost = "127.0.0.1";
    constexpr int xjadeoPort = 7890;
    constexpr int framesPerSecond = 25;

    const juce::String loadCmd = "/jadeo/load";
    const juce::String seekCmd = "/jadeo/seek";
}

//==============================================================================
TestComponent::TestComponent()
{
    oscSender.connect (xjadeoHost, xjadeoPort);

    playButton.onClick = [this] { sendPlay(); };
    addAndMakeVisible (playButton);

    pauseButton.onClick = [this] { sendPause(); };
    addAndMakeVisible (pauseButton);

    frameSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    frameSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    frameSlider.setRange (0, 1, 1);
    frameSlider.onValueChange = [this] { updateFrame ((int) frameSlider.getValue()); };
    addAndMakeVisible (frameSlider);

    updateFrameLabel();
    addAndMakeVisible (frameLabel);

    loadFileButton.onClick = [this] { loadFile(); };
    addAndMakeVisible (loadFileButton);
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
    buttonRow.removeFromLeft (5);
    pauseButton.setBounds (buttonRow.removeFromLeft (80));
    buttonRow.removeFromLeft (5);
    loadFileButton.setBounds (buttonRow.removeFromLeft (100));

    auto sliderRow = bounds.removeFromTop(30);
    frameSlider.setBounds (sliderRow.removeFromLeft (300));
    sliderRow.removeFromLeft(5);
    frameLabel.setBounds(sliderRow.removeFromLeft(100));
}

void TestComponent::sendPlay()
{
    startTimer (1000 / framesPerSecond);
}

void TestComponent::sendPause()
{
    stopTimer();
}

void TestComponent::updateFrame (int frame)
{
    frame = numFrames > 0 ? juce::jlimit (0, (int) numFrames - 1, frame) : 0;

    currentFrame = frame;
    sendFrame (frame);

    frameSlider.setValue (frame, juce::dontSendNotification);
    updateFrameLabel();
}

void TestComponent::updateFrameLabel()
{
    frameLabel.setText (juce::String (currentFrame) + " / " + juce::String (numFrames),
                         juce::dontSendNotification);
}

void TestComponent::sendFrame (int frame)
{
    oscSender.send (seekCmd, frame);
}

void TestComponent::timerCallback()
{
    updateFrame (currentFrame + 1);

    if (numFrames > 0 && currentFrame >= numFrames - 1)
        sendPause();
}

void TestComponent::loadFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Select a video file to load...",
                                                         juce::File(),
                                                         "*.mov;*.mp4;*.avi;*.mkv;*.mpg;*.mpeg;*.webm");

    constexpr auto chooserFlags = juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();

        if (file == juce::File{})
            return;

        videoFile = file;
        sendLoadFile (videoFile);
    });
}

void TestComponent::sendLoadFile (const juce::File& file)
{
    oscSender.send (loadCmd, file.getFullPathName());

    const auto info = readFfmpegVideoInfo (file);

    numFrames = info.isValid ? info.frameCount : 0;
    currentFrame = 0;

    frameSlider.setRange (0, (double) juce::jmax ((juce::int64) 0, numFrames - 1), 1);
    frameSlider.setValue (0, juce::dontSendNotification);
    updateFrameLabel();
}
