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
                        private juce::MultiTimer,
                        private juce::TableListBoxModel
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

    enum ColumnId
    {
        midiNoteColumnId = 1,
        frameColumnId,
        seekColumnId,
        removeColumnId
    };

    void sendPlay();
    void sendPause();
    void sendFrame (int frame);
    void updateFrame (int frame);
    void updateFrameLabel();
    void loadFile();
    void sendLoadFile (const juce::File& file);

    void addCuePoint();
    void seekToCue (int row);
    void removeCue (int row);
    void triggerCueForNote (int midiNote);
    int nextAvailableCueNote() const;

    void timerCallback (int timerID) override;

    // juce::TableListBoxModel
    int getNumRows() override;
    void paintRowBackground (juce::Graphics&, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell (juce::Graphics&, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    juce::Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected,
                                               juce::Component* existingComponentToUpdate) override;

    XJadeoControlAudioProcessor& audioProcessor;

    juce::TextButton playButton      { "Play" };
    juce::TextButton pauseButton     { "Pause" };
    juce::TextButton loadFileButton  { "Load File" };
    juce::TextButton addCueButton    { "Add Cue" };
    juce::Label frameLabel;
    juce::Slider frameSlider;
    juce::TableListBox cueTable;

    juce::OSCSender oscSender;

    juce::File videoFile;
    std::unique_ptr<juce::FileChooser> fileChooser;

    int currentFrame = 0;
    juce::int64 numFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestComponent)
};
