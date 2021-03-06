/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//here i've used a struct for a style sheet, the colours declared a just temporary place holders.
struct GuiStyleSheet
{
	const juce::Colour l1 = juce::Colours::blue;
	const juce::Colour l2 = juce::Colours::white;
	const juce::Colour p1 = juce::Colours::purple;
	const juce::Colour p2 = juce::Colours::white;
	const juce::Colour h1 = juce::Colours::yellow;
	const juce::Colour h2 = juce::Colours::white;
};

/*here i created a class to replace the standard juce::Slider classes, as I want to customise them
in their construction. we used a class as we need colour1 and colour2 to be private members,
so there can be multiple instances holding different datum. here we also use some simple maths to
create the dial with lines to show where the dial has been turned.*/
class CustomDial : public juce::LookAndFeel_V4
{
public:
	CustomDial(juce::Colour _colour1, juce::Colour _colour2)
	{
		colour1 = _colour1;
		colour2 = _colour2;

	}
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
		g.setColour(colour1);
		g.fillEllipse(dialArea);

		g.setColour(colour2);
		juce::Path dialTick;
		dialTick.addRectangle(0, -radius, 4.0, radius * 0.66);
		g.fillPath(dialTick, juce::AffineTransform::rotation(angle).translated(centreX, centreY));
	}
private:
	juce::Colour colour1;
	juce::Colour colour2;
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

	GuiStyleSheet guiStyleSheet;
	CustomDial lowDials{ guiStyleSheet.l1, guiStyleSheet.l2 };
	CustomDial peakDials{ guiStyleSheet.p1, guiStyleSheet.p2 };
	CustomDial highDials{ guiStyleSheet.h1, guiStyleSheet.h2 };

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
