/*
  ==============================================================================

    MainComponent.cpp

  ==============================================================================
*/

#include "MainComponent.h"
#include "FfmpegVideoInfo.h"
#include <cmath>

namespace
{
    const juce::String xjadeoHost = "127.0.0.1";
    constexpr int xjadeoPort = 7890;
    constexpr int framesPerSecond = 25;

    const juce::String loadCmd = "/jadeo/load";
    const juce::String seekCmd = "/jadeo/seek";

    // Cue points start above the play/pause notes (36/37).
    constexpr int firstCueNoteNumber = 38;

    class CueActionButton  : public juce::TextButton
    {
    public:
        CueActionButton (const juce::String& buttonText, std::function<void (int)> callbackIn)
            : juce::TextButton (buttonText), callback (std::move (callbackIn))
        {
            onClick = [this] { if (callback) callback (row); };
        }

        void setRow (int newRow) { row = newRow; }

    private:
        std::function<void (int)> callback;
        int row = -1;
    };
}

//==============================================================================
MainComponent::MainComponent (XJadeoControlAudioProcessor& processor)
    : audioProcessor (processor)
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

    addCueButton.onClick = [this] { addCuePoint(); };
    addAndMakeVisible (addCueButton);

    cueTable.setModel (this);
    cueTable.getHeader().addColumn ("MIDI Note", midiNoteColumnId, 80);
    cueTable.getHeader().addColumn ("Frame", frameColumnId, 80);
    cueTable.getHeader().addColumn ("Seek", seekColumnId, 60);
    cueTable.getHeader().addColumn ("Remove", removeColumnId, 70);
    addAndMakeVisible (cueTable);

    audioProcessor.addChangeListener (this);

    startTimer (midiPollTimer, 20);
}

MainComponent::~MainComponent()
{
    audioProcessor.removeChangeListener (this);

    stopTimer (frameTimer);
    stopTimer (midiPollTimer);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    auto buttonRow = bounds.removeFromTop (30);
    playButton.setBounds (buttonRow.removeFromLeft (80));
    buttonRow.removeFromLeft (5);
    pauseButton.setBounds (buttonRow.removeFromLeft (80));
    buttonRow.removeFromLeft (5);
    loadFileButton.setBounds (buttonRow.removeFromLeft (100));
    buttonRow.removeFromLeft (5);
    addCueButton.setBounds (buttonRow.removeFromLeft (100));

    bounds.removeFromTop (10);

    auto sliderRow = bounds.removeFromTop(30);
    frameSlider.setBounds (sliderRow.removeFromLeft (300));
    sliderRow.removeFromLeft(5);
    frameLabel.setBounds(sliderRow.removeFromLeft(100));

    bounds.removeFromTop (10);
    cueTable.setBounds (bounds);
}

void MainComponent::sendPlay()
{
    startTimer (frameTimer, 1000 / framesPerSecond);
}

void MainComponent::sendPause()
{
    stopTimer (frameTimer);
}

void MainComponent::updateFrame (int frame)
{
    frame = numFrames > 0 ? juce::jlimit (0, (int) numFrames - 1, frame) : 0;

    currentFrame = frame;
    sendFrame (frame);

    frameSlider.setValue (frame, juce::dontSendNotification);
    updateFrameLabel();
}

void MainComponent::updateFrameLabel()
{
    frameLabel.setText (juce::String (currentFrame) + " / " + juce::String (numFrames),
                         juce::dontSendNotification);
}

void MainComponent::sendFrame (int frame)
{
    oscSender.send (seekCmd, frame);
}

void MainComponent::timerCallback (int timerID)
{
    if (timerID == frameTimer)
    {
        updateFrame (currentFrame + 1);

        if (numFrames > 0 && currentFrame >= numFrames - 1)
            sendPause();
    }
    else if (timerID == midiPollTimer)
    {
        int noteNumber;

        while (audioProcessor.popNoteOn (noteNumber))
        {
            if (noteNumber == XJadeoControlAudioProcessor::playNoteNumber)
                sendPlay();
            else if (noteNumber == XJadeoControlAudioProcessor::pauseNoteNumber)
                sendPause();
            else
                triggerCueForNote (noteNumber);
        }
    }
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* /*source*/)
{
    cueTable.updateContent();
}

void MainComponent::loadFile()
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

void MainComponent::sendLoadFile (const juce::File& file)
{
    oscSender.send (loadCmd, file.getFullPathName());

    const auto info = readFfmpegVideoInfo (file);

    numFrames = info.isValid ? info.frameCount : 0;
    currentFrame = 0;

    frameSlider.setRange (0, (double) juce::jmax ((juce::int64) 0, numFrames - 1), 1);
    frameSlider.setValue (0, juce::dontSendNotification);
    updateFrameLabel();
}

void MainComponent::addCuePoint()
{
    audioProcessor.addCuePoint (currentFrame);
}

void MainComponent::seekToCue (int row)
{
    if (! juce::isPositiveAndBelow (row, audioProcessor.cuePoints.size()))
        return;

    updateFrame (audioProcessor.cuePoints[row]);
}

void MainComponent::removeCue (int row)
{
    if (! juce::isPositiveAndBelow (row, audioProcessor.cuePoints.size()))
        return;

    audioProcessor.removeCuePoint (audioProcessor.cuePoints[row]);
}

void MainComponent::triggerCueForNote (int midiNote)
{
    const auto row = midiNote - firstCueNoteNumber;

    if (juce::isPositiveAndBelow (row, audioProcessor.cuePoints.size()))
        updateFrame (audioProcessor.cuePoints[row]);
}

//==============================================================================
int MainComponent::getNumRows()
{
    return audioProcessor.cuePoints.size();
}

void MainComponent::paintRowBackground (juce::Graphics& g, int /*rowNumber*/, int /*width*/, int /*height*/,
                                         bool rowIsSelected)
{
    g.fillAll (rowIsSelected ? getLookAndFeel().findColour (juce::TextEditor::highlightColourId)
                              : getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::paintCell (juce::Graphics& g, int rowNumber, int columnId, int width, int height,
                                bool /*rowIsSelected*/)
{
    if (! juce::isPositiveAndBelow (rowNumber, audioProcessor.cuePoints.size()))
        return;

    juce::String text;

    if (columnId == midiNoteColumnId)
        text = midiToNoteName (firstCueNoteNumber + rowNumber);
    else if (columnId == frameColumnId)
        text = juce::String (audioProcessor.cuePoints[rowNumber]);
    else
        return;

    g.setColour (getLookAndFeel().findColour (juce::ListBox::textColourId));
    g.drawText (text, 4, 0, width - 8, height, juce::Justification::centredLeft);
}

juce::Component* MainComponent::refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/,
                                                           juce::Component* existingComponentToUpdate)
{
    if (columnId != seekColumnId && columnId != removeColumnId)
    {
        delete existingComponentToUpdate;
        return nullptr;
    }

    auto* button = dynamic_cast<CueActionButton*> (existingComponentToUpdate);

    if (button == nullptr)
    {
        delete existingComponentToUpdate;

        button = columnId == seekColumnId
                   ? new CueActionButton ("Seek", [this] (int row) { seekToCue (row); })
                   : new CueActionButton ("Remove", [this] (int row) { removeCue (row); });
    }

    button->setRow (rowNumber);
    return button;
}

const juce::String MainComponent::midiToNoteName(int noteValue)
{
    // minimum is 36 = C1
    return notes[noteValue % 12] 
        + juce::String(std::floor(noteValue / 12.0) - 2);
}