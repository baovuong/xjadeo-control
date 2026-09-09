/*
  ==============================================================================

    FooterComponent.cpp
    Created: 2 Sep 2026 1:24:15pm
    Author:  bvuong

  ==============================================================================
*/

#include <JuceHeader.h>
#include "FooterComponent.h"

//==============================================================================
FooterComponent::FooterComponent()
{
    infoLabel.setText (juce::String (ProjectInfo::companyName) + "  |  v" + ProjectInfo::versionString,
                        juce::dontSendNotification);
    infoLabel.setJustificationType (juce::Justification::centred);
    infoLabel.setFont (juce::FontOptions (12.0f));
    infoLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (infoLabel);
}

FooterComponent::~FooterComponent()
{
}

void FooterComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void FooterComponent::resized()
{
    infoLabel.setBounds (getLocalBounds());
}
