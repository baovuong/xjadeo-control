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
    constexpr int fileWatchIntervalMs = 200;

    enum TimerIds
    {
        playbackTimerId = 0,
        fileWatchTimerId = 1
    };
}

//==============================================================================
TestComponent::TestComponent()
{
    oscSender.connect (xjadeoHost, xjadeoPort);

    xjadeoOutputFile = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                            .getSiblingFile ("xjadeo_output.log");

    startTimer (fileWatchTimerId, fileWatchIntervalMs);

    playButton.onClick = [this] { sendPlay(); };
    addAndMakeVisible (playButton);

    pauseButton.onClick = [this] { sendPause(); };
    addAndMakeVisible (pauseButton);

    addAndMakeVisible (frameLabel);

    frameInput.setInputRestrictions (0, "0123456789");
    frameInput.setText ("0", juce::dontSendNotification);
    frameInput.onReturnKey = [this] { updateFrame (frameInput.getText().getIntValue()); };
    addAndMakeVisible (frameInput);

    getFramesButton.onClick = [this] { getFrames(); };
    addAndMakeVisible (getFramesButton);

    numFramesLabel.setText ("Total Frames: " + juce::String (numFrames), juce::dontSendNotification);
    addAndMakeVisible (numFramesLabel);

    loadFileButton.onClick = [this] { loadFile(); };
    addAndMakeVisible (loadFileButton);
}

TestComponent::~TestComponent()
{
    stopTimer (playbackTimerId);
    stopTimer (fileWatchTimerId);
}

void TestComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void TestComponent::resized()
{
    juce::FlexBox flexBox;
    flexBox.flexDirection = juce::FlexBox::Direction::column;
    flexBox.flexWrap = juce::FlexBox::Wrap::wrap;
    flexBox.alignItems = juce::FlexBox::AlignItems::center;

    flexBox.items.add (juce::FlexItem (playButton).withWidth (80).withHeight (30).withMargin (5));
    flexBox.items.add (juce::FlexItem (pauseButton).withWidth (80).withHeight (30).withMargin (5));
    flexBox.items.add (juce::FlexItem (frameLabel).withWidth (60).withHeight (30).withMargin (5));
    flexBox.items.add (juce::FlexItem (frameInput).withWidth (100).withHeight (30).withMargin (5));
    flexBox.items.add (juce::FlexItem (getFramesButton).withWidth (100).withHeight (30).withMargin (5));
    flexBox.items.add (juce::FlexItem (numFramesLabel).withWidth (150).withHeight (30).withMargin (5));
    flexBox.items.add (juce::FlexItem (loadFileButton).withWidth (100).withHeight (30).withMargin (5));

    flexBox.performLayout (getLocalBounds().reduced (10));
}

void TestComponent::sendPlay()
{
    startTimer (playbackTimerId, 1000 / framesPerSecond);
}

void TestComponent::sendPause()
{
    stopTimer (playbackTimerId);
}

void TestComponent::updateFrame(int frame) 
{
    currentFrame = frame;
    sendFrame(frame);
}

void TestComponent::sendFrame (int frame)
{
    oscSender.send ("/jadeo/seek", frame);
}

void TestComponent::getFrames()
{
    oscSender.send ("/jadeo/cmd", juce::String ("get frames"));
}

void TestComponent::timerCallback (int timerId)
{
    if (timerId == playbackTimerId)
        updateFrame(currentFrame + 1);
    else if (timerId == fileWatchTimerId)
        checkForFileChanges();
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
    oscSender.send ("/jadeo/load", file.getFullPathName());
    oscSender.send ("/jadeo/cmd", juce::String ("get frames"));
}

void TestComponent::checkForFileChanges()
{
    const auto currentSize = xjadeoOutputFile.getSize();

    if (currentSize < lastReadPosition)
        lastReadPosition = 0;

    if (currentSize == lastReadPosition)
        return;

    juce::FileInputStream stream (xjadeoOutputFile);

    if (! stream.openedOk())
        return;

    stream.setPosition (lastReadPosition);
    const auto newText = stream.readString();
    lastReadPosition = stream.getPosition();

    for (const auto& line : juce::StringArray::fromLines (newText))
    {
        if (line.isEmpty())
            continue;

        DBG ("xjadeo output: " + line);

        if (line.startsWith ("total frames:"))
        {
            DBG ("hell yeah" + line);

            numFrames = line.fromFirstOccurrenceOf (":", false, false).trim().getIntValue();
            numFramesLabel.setText ("Total Frames: " + juce::String (numFrames), juce::dontSendNotification);
        }
    }
}
