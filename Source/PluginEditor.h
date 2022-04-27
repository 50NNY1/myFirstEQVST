/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomSlider : juce::Slider
{
	CustomSlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
								  juce::Slider::TextEntryBoxPosition::NoTextBox)
	{
	}
};
//==============================================================================
/**
*/
class MyEQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
	MyEQAudioProcessorEditor(MyEQAudioProcessor&);
	~MyEQAudioProcessorEditor() override;

	//==============================================================================
	void paint(juce::Graphics&) override;
	void resized() override;

private:
	// This reference is provided as a quick way for your editor to
	// access the processor object that created it.
	MyEQAudioProcessor& audioProcessor;
	CustomSlider lowCutFreqSlider,
		highCutFreqSlider,
		peakFreqSlider,
		peakGainSlider,
		peakQSlider,
		lowCutSlopeSlider,
		highCutSlopeSlider;
	using Parameters = juce::AudioProcessorValueTreeState;
	using Attatchment = Parameters::SliderAttachment;

	Attatchment lowCutFreqSliderAttatchment,
		highCutFreqSliderAttatchment,
		peakFreqSliderAttatchment,
		peakGainSliderAttatchment,
		peakQSliderAttatchment,
		lowCutSlopeSliderAttatchment,
		highCutSlopeSliderAttatchment;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyEQAudioProcessorEditor)
};
