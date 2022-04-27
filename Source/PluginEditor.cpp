/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MyEQAudioProcessorEditor::MyEQAudioProcessorEditor(MyEQAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p),
	peakFreqSliderAttatchment(audioProcessor.parameters, "Peak Freq", peakFreqSlider),
	peakGainSliderAttatchment(audioProcessor.parameters, "Peak Gain", peakGainSlider),
	peakQSliderAttatchment(audioProcessor.parameters, "Peak Q", peakQSlider),
	lowCutFreqSliderAttatchment(audioProcessor.parameters, "LowCut Freq", lowCutFreqSlider),
	lowCutSlopeSliderAttatchment(audioProcessor.parameters, "LowCut Slope", lowCutSlopeSlider),
	highCutSlopeSliderAttatchment(audioProcessor.parameters, "HighCut Slope", highCutSlopeSlider),
	highCutFreqSliderAttatchment(audioProcessor.parameters, "HighCut Freq", highCutFreqSlider)
{
	// Make sure that before the constructor has finished, you've set the
	// editor's size to whatever you need it to be.
	addAndMakeVisible(peakFreqSlider);
	addAndMakeVisible(peakGainSlider);
	addAndMakeVisible(peakQSlider);
	addAndMakeVisible(lowCutFreqSlider);
	addAndMakeVisible(highCutFreqSlider);
	addAndMakeVisible(lowCutSlopeSlider);
	addAndMakeVisible(highCutSlopeSlider);
	setSize(800, 600);
}

MyEQAudioProcessorEditor::~MyEQAudioProcessorEditor()
{
}

//==============================================================================
void MyEQAudioProcessorEditor::paint(juce::Graphics& g)
{
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

	g.setColour(juce::Colours::white);
	g.setFont(15.0f);
	g.drawFittedText("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void MyEQAudioProcessorEditor::resized()
{
	// This is generally where you'll want to lay out the positions of any
	// subcomponents in your editor..
	auto bounds = getLocalBounds();
	auto spectraArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

	auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
	auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

	lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
	lowCutSlopeSlider.setBounds(lowCutArea);
	highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
	highCutSlopeSlider.setBounds(highCutArea);

	peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
	peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
	peakQSlider.setBounds(bounds);
}
