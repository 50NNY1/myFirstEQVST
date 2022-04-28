/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class GuiStyleSheet : public juce::LookAndFeel_V4
{
public:
	void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
						  float sliderPosProportional, float rotaryStartAngle,
						  float rotaryEndAngle, juce::Slider& slider) override
	{
		float diameter = juce::jmin(width, height);
		float radius = diameter / 2;
		float centreX = x + width / 2;
		float centreY = y + height / 2;
		float rx = centreX - radius;
		float ry = centreY - radius;
		float angle = rotaryStartAngle +
			(sliderPosProportional *
			 (rotaryEndAngle - rotaryStartAngle));

		juce::Rectangle<float> dialArea(rx, ry, diameter, diameter);
		g.setColour(juce::Colours::white);
		g.fillEllipse(dialArea);

		g.setColour(juce::Colours::black);
		juce::Path dialTick;
		dialTick.addRectangle(0, -radius, 4.0, radius * 0.66);
		g.fillPath(dialTick, juce::AffineTransform::rotation(angle).translated(centreX, centreY));
	}
};
//==============================================================================
struct CustomSlider : juce::Slider
{
	CustomSlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
								  juce::Slider::TextEntryBoxPosition::NoTextBox)
	{
	}
};
//==============================================================================
struct ResponseCurveDraw : juce::Component,
	juce::AudioProcessorParameter::Listener,
	juce::Timer
{
	ResponseCurveDraw(MyEQAudioProcessor&);
	~ResponseCurveDraw();
	//==============================================================================
	void paint(juce::Graphics& g);
	//==============================================================================
	void timerCallback() override;
	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {};
private:
	MyEQAudioProcessor& audioProcessor;
	juce::Atomic<bool> paramsChanged{ false };
	MonoChain monoChain;
};
//==============================================================================
class MyEQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
	MyEQAudioProcessorEditor(MyEQAudioProcessor&);
	~MyEQAudioProcessorEditor() override;
	//==============================================================================
	void paint(juce::Graphics&) override;
	void resized() override;
	//==============================================================================
private:
	GuiStyleSheet guiStyleSheet;
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

	ResponseCurveDraw responseCurve;
	MonoChain monoChain;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyEQAudioProcessorEditor)
};
