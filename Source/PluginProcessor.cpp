/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FfmpegVideoInfo.h"
#include "juce_core/juce_core.h"
#include <cmath>
#include <memory>

namespace
{
    const juce::String xjadeoHost = "127.0.0.1";
    constexpr int xjadeoPort = 7890;

    // Used when no video is loaded yet, or its frame rate couldn't be determined.
    constexpr double defaultFrameRate = 25.0;

    const juce::String loadCmd = "/jadeo/load";
    const juce::String seekCmd = "/jadeo/seek";
}

//==============================================================================
XJadeoControlAudioProcessor::XJadeoControlAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    oscSender.connect (xjadeoHost, xjadeoPort);
    startTimer (midiPollTimerId, 20);
}

XJadeoControlAudioProcessor::~XJadeoControlAudioProcessor()
{
    stopTimer (frameTimerId);
    stopTimer (midiPollTimerId);
}

//==============================================================================
const juce::String XJadeoControlAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool XJadeoControlAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool XJadeoControlAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool XJadeoControlAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double XJadeoControlAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int XJadeoControlAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int XJadeoControlAudioProcessor::getCurrentProgram()
{
    return 0;
}

void XJadeoControlAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String XJadeoControlAudioProcessor::getProgramName (int index)
{
    return {};
}

void XJadeoControlAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void XJadeoControlAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void XJadeoControlAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool XJadeoControlAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void XJadeoControlAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (! message.isNoteOn())
            continue;

        int start1, size1, start2, size2;
        midiNoteFifo.prepareToWrite (1, start1, size1, start2, size2);

        if (size1 > 0)
            midiNoteBuffer[(size_t) start1] = message.getNoteNumber();
        else if (size2 > 0)
            midiNoteBuffer[(size_t) start2] = message.getNoteNumber();
        else
            continue; // FIFO full; drop the message.

        midiNoteFifo.finishedWrite (size1 + size2);
    }
}

//==============================================================================
bool XJadeoControlAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* XJadeoControlAudioProcessor::createEditor()
{
    return new XJadeoControlAudioProcessorEditor (*this);
}

//==============================================================================
void XJadeoControlAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    copyXmlToBinary (createStateXml(), destData);
}

void XJadeoControlAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState == nullptr || ! xmlState->hasTagName ("XJadeoControlState"))
        return;

    // Hosts can call setStateInformation() from a background loading thread, but
    // cuePoints and the transport timers are only safe to touch from the message
    // thread (JUCE's Timer start/stop isn't safe cross-thread, and cuePoints is
    // read concurrently by the message-thread UI). Defer applying the state.
    juce::MessageManager::callAsync ([this, xmlState = std::shared_ptr<juce::XmlElement> (xmlState.release())]
    {
        restoreFromXml (*xmlState);
    });
}

juce::XmlElement XJadeoControlAudioProcessor::createStateXml() const
{
    juce::XmlElement xml ("XJadeoControlState");

    xml.setAttribute ("videoFile", videoFile.getFullPathName());
    xml.setAttribute ("isPlaying", isPlaying);

    for (auto frame : cuePoints)
    {
        auto* cueXml = xml.createNewChildElement ("CUEPOINT");
        cueXml->setAttribute ("frame", frame);
    }

    return xml;
}

void XJadeoControlAudioProcessor::restoreFromXml (const juce::XmlElement& xmlState)
{
    const juce::File restoredVideoFile (xmlState.getStringAttribute ("videoFile"));
    const bool restoredIsPlaying = xmlState.getBoolAttribute ("isPlaying");

    cuePoints.clear();

    for (auto* cueXml : xmlState.getChildWithTagNameIterator ("CUEPOINT"))
        cuePoints.add (cueXml->getIntAttribute ("frame"));

    if (restoredVideoFile != juce::File{})
        loadVideoFile (restoredVideoFile);

    if (restoredIsPlaying)
        play();
    else
        pause();

    sendChangeMessage();
}

void XJadeoControlAudioProcessor::addCuePoint (int frame)
{
    cuePoints.add (frame);
    sendChangeMessage();
}

void XJadeoControlAudioProcessor::removeCuePoint (int frame)
{
    cuePoints.removeValue (frame);
    sendChangeMessage();
}

void XJadeoControlAudioProcessor::play()
{
    isPlaying = true;
    startTimer (frameTimerId, (int) std::round (1000.0 / frameRate));
    sendChangeMessage();
}

void XJadeoControlAudioProcessor::pause()
{
    isPlaying = false;
    stopTimer (frameTimerId);
    sendChangeMessage();
}

void XJadeoControlAudioProcessor::seek (int frame)
{
    frame = numFrames > 0 ? juce::jlimit (0, (int) numFrames - 1, frame) : 0;

    currentFrame = frame;
    sendFrame (frame);
}

void XJadeoControlAudioProcessor::sendFrame (int frame)
{
    oscSender.send (seekCmd, frame);
}

void XJadeoControlAudioProcessor::loadVideoFile (const juce::File& file)
{
    videoFile = file;
    oscSender.send (loadCmd, file.getFullPathName());

    const auto info = readFfmpegVideoInfo (file);

    numFrames = info.isValid ? info.frameCount : 0;
    frameRate = (info.isValid && info.frameRate > 0.0) ? info.frameRate : defaultFrameRate;
    currentFrame = 0;

    // Reflect the new rate immediately if a play is already in progress.
    if (isPlaying)
        startTimer (frameTimerId, (int) std::round (1000.0 / frameRate));

    sendChangeMessage();
}

void XJadeoControlAudioProcessor::triggerCueForNote (int midiNote)
{
    const auto row = midiNote - firstCueNoteNumber;

    if (juce::isPositiveAndBelow (row, cuePoints.size()))
        seek (cuePoints[row]);
}

void XJadeoControlAudioProcessor::timerCallback (int timerID)
{
    if (timerID == frameTimerId)
    {
        seek (currentFrame + 1);

        if (numFrames > 0 && currentFrame >= numFrames - 1)
            pause();
    }
    else if (timerID == midiPollTimerId)
    {
        int noteNumber;

        while (popNoteOn (noteNumber))
        {
            if (noteNumber == playNoteNumber)
                play();
            else if (noteNumber == pauseNoteNumber)
                pause();
            else
                triggerCueForNote (noteNumber);
        }
    }
}

bool XJadeoControlAudioProcessor::popNoteOn(int &noteNumberOut) noexcept
{
    int start1, size1, start2, size2;
    midiNoteFifo.prepareToRead(1, start1, size1, start2, size2);

    if (size1 > 0)
        noteNumberOut = midiNoteBuffer[(size_t)start1];
    else if (size2 > 0)
        noteNumberOut = midiNoteBuffer[(size_t)start2];
    else
        return false;

    midiNoteFifo.finishedRead(size1 + size2);
    return true;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new XJadeoControlAudioProcessor();
}
