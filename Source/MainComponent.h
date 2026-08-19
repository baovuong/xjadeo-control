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
                        private juce::Timer,
                        private juce::TableListBoxModel,
                        private juce::ChangeListener
{
public:
    explicit MainComponent (XJadeoControlAudioProcessor& processor);
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
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

    void updateFrameLabel();
    void loadFile();

    void addCuePoint();
    void seekToCue (int row);
    void removeCue (int row);

    // juce::Timer — periodically refreshes the slider/labels from processor state.
    // Transport and MIDI handling themselves live in the processor so they keep
    // running even while this component (the GUI) doesn't exist.
    void timerCallback() override;

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
    juce::Label filenameLabel;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
