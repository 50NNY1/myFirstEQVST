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

	const auto& params = audioProcessor.getParameters();
	for (auto param : params)
		param->addListener(this);

	startTimerHz(60);

	setSize(800, 600);
}

MyEQAudioProcessorEditor::~MyEQAudioProcessorEditor()
{
	const auto& params = audioProcessor.getParameters();
	for (auto param : params)
		param->removeListener(this);
}

//==============================================================================
void MyEQAudioProcessorEditor::paint(juce::Graphics& g)
{
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(juce::Colours::black);

	auto bounds = getLocalBounds();
	auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
	auto responseWidth = responseArea.getWidth();
	auto& lowCut = monoChain.get < eqTypes::LowCut>();
	auto& peak = monoChain.get < eqTypes::Peak>();
	auto& highCut = monoChain.get < eqTypes::HighCut>();
	auto sampleRate = audioProcessor.getSampleRate();

	std::vector<double> mags;
	mags.resize(responseWidth);

	for (int i = 0; i < mags.size(); ++i)
	{
		double mag = 1.f;
		auto freq = juce::mapToLog10(double(i) / double(responseWidth), 20.0, 20000.0);
		if (!monoChain.isBypassed<eqTypes::Peak>())
			mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!lowCut.isBypassed<0>())
			mag *= lowCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!lowCut.isBypassed<1>())
			mag *= lowCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!lowCut.isBypassed<2>())
			mag *= lowCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!lowCut.isBypassed<3>())
			mag *= lowCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!highCut.isBypassed<0>())
			mag *= highCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!highCut.isBypassed<1>())
			mag *= highCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!highCut.isBypassed<2>())
			mag *= highCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!highCut.isBypassed<3>())
			mag *= highCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		mags[i] = juce::Decibels::gainToDecibels(mag);
	}

	juce::Path responseCurve;
	const double outMin = responseArea.getBottom();
	const double outMax = responseArea.getY();
	auto map = [outMin, outMax](double input) {return juce::jmap(input, -24.0, 24.0, outMin, outMax); };
	responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

	for (size_t i = 1; i < mags.size(); ++i)
		responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));

	g.setColour(juce::Colours::orange);
	g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
	g.setColour(juce::Colours::white);
	g.strokePath(responseCurve, juce::PathStrokeType(2.f));
}

void MyEQAudioProcessorEditor::resized()
{
	// This is generally where you'll want to lay out the positions of any
	// subcomponents in your editor..
	auto bounds = getLocalBounds();
	auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

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

void MyEQAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{
	paramsChanged.set(true);
}

void MyEQAudioProcessorEditor::timerCallback()
{
	if (paramsChanged.compareAndSetBool(false, true))
	{
		auto eqSettings = getEqSettings(audioProcessor.parameters);
		auto peakCoeffs = makePeakFilter(eqSettings, audioProcessor.getSampleRate());
		updateCoeffs(monoChain.get<eqTypes::Peak>().coefficients, peakCoeffs);
		repaint();
	}
}
