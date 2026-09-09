/*
  ==============================================================================

    FooterComponent.h
    Created: 2 Sep 2026 1:24:15pm
    Author:  bvuong

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class FooterComponent  : public juce::Component
{
public:
    FooterComponent();
    ~FooterComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Label infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FooterComponent)
};
