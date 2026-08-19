/*
  ==============================================================================

    MainComponent.cpp

  ==============================================================================
*/

#include "MainComponent.h"
#include <cmath>

namespace
{
    // UI refresh rate for the slider/frame label. Playback, MIDI handling, and
    // OSC communication all happen in the processor regardless of this rate.
    constexpr int uiRefreshHz = 25;

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
    playButton.onClick = [this] { audioProcessor.play(); };
    addAndMakeVisible (playButton);

    pauseButton.onClick = [this] { audioProcessor.pause(); };
    addAndMakeVisible (pauseButton);

    frameSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    frameSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    frameSlider.setRange (0, 1, 1);
    frameSlider.onValueChange = [this] { audioProcessor.seek ((int) frameSlider.getValue()); };
    addAndMakeVisible (frameSlider);

    updateFrameLabel();
    addAndMakeVisible (frameLabel);

    filenameLabel.setText (audioProcessor.videoFile != juce::File{} ? audioProcessor.videoFile.getFileName() : "-",
                            juce::dontSendNotification);
    addAndMakeVisible (filenameLabel);

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

    frameSlider.setRange (0, (double) juce::jmax ((juce::int64) 0, audioProcessor.numFrames - 1), 1);
    frameSlider.setValue (audioProcessor.currentFrame, juce::dontSendNotification);

    startTimerHz (uiRefreshHz);
}

MainComponent::~MainComponent()
{
    audioProcessor.removeChangeListener (this);
    stopTimer();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10);


    // file row
    auto fileRow = bounds.removeFromTop (30);
    fileRow.removeFromLeft (5);
    loadFileButton.setBounds (fileRow.removeFromLeft (100));
    fileRow.removeFromLeft (5);
    filenameLabel.setBounds(fileRow.removeFromLeft (300));

    bounds.removeFromTop (10);

    // nav row
    auto navRow = bounds.removeFromTop(30);
    playButton.setBounds (navRow.removeFromLeft (80));
    navRow.removeFromLeft (5);
    pauseButton.setBounds (navRow.removeFromLeft (80));
    navRow.removeFromLeft(5);
    frameLabel.setBounds(navRow.removeFromLeft(100));

    // slider row
    auto sliderRow = bounds.removeFromTop(30);
    frameSlider.setBounds (sliderRow.removeFromLeft (300));
    sliderRow.removeFromLeft(5);
    addCueButton.setBounds (sliderRow.removeFromLeft (100));

    bounds.removeFromTop (10);
    cueTable.setBounds (bounds);
}

void MainComponent::updateFrameLabel()
{
    frameLabel.setText (juce::String (audioProcessor.currentFrame) + " / " + juce::String (audioProcessor.numFrames),
                         juce::dontSendNotification);
}

void MainComponent::timerCallback()
{
    frameSlider.setRange (0, (double) juce::jmax ((juce::int64) 0, audioProcessor.numFrames - 1), 1);
    frameSlider.setValue (audioProcessor.currentFrame, juce::dontSendNotification);
    updateFrameLabel();
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster* /*source*/)
{
    cueTable.updateContent();

    filenameLabel.setText (audioProcessor.videoFile != juce::File{} ? audioProcessor.videoFile.getFileName() : "-",
                            juce::dontSendNotification);
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

        audioProcessor.loadVideoFile (file);
    });
}

void MainComponent::addCuePoint()
{
    audioProcessor.addCuePoint (audioProcessor.currentFrame);
}

void MainComponent::seekToCue (int row)
{
    if (! juce::isPositiveAndBelow (row, audioProcessor.cuePoints.size()))
        return;

    audioProcessor.seek (audioProcessor.cuePoints[row]);
}

void MainComponent::removeCue (int row)
{
    if (! juce::isPositiveAndBelow (row, audioProcessor.cuePoints.size()))
        return;

    audioProcessor.removeCuePoint (audioProcessor.cuePoints[row]);
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
        text = midiToNoteName (XJadeoControlAudioProcessor::firstCueNoteNumber + rowNumber);
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
