/*
  ==============================================================================

    MainComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class MainComponent  : public juce::Component,
                        private juce::MultiTimer,
                        private juce::TableListBoxModel,
                        private juce::ChangeListener
{
public:
    explicit MainComponent (XJadeoControlAudioProcessor& processor);
    ~MainComponent() override;

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

    const juce::String notes[12] = {
        "C", 
        "C#",
        "D", 
        "D#", 
        "E", 
        "F", 
        "F#", 
        "G", 
        "G#", 
        "A", 
        "A#", 
        "B"
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

    void timerCallback (int timerID) override;

    // juce::ChangeListener
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    // juce::TableListBoxModel
    int getNumRows() override;
    void paintRowBackground (juce::Graphics&, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell (juce::Graphics&, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    const juce::String midiToNoteName(int noteValue);
    
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
