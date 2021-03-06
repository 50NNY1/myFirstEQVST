/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

ResponseCurveDraw::ResponseCurveDraw(MyEQAudioProcessor& p) : audioProcessor(p)
{
	//range based for loop to add listeners to all dials
	const auto& params = audioProcessor.getParameters();
	for (auto param : params)
		param->addListener(this);
	startTimerHz(60);
}

ResponseCurveDraw::~ResponseCurveDraw()
{
	//range based for loop to destruct listeners
	const auto& params = audioProcessor.getParameters();
	for (auto param : params)
		param->removeListener(this);
}
void ResponseCurveDraw::parameterValueChanged(int parameterIndex, float newValue)
{
	//listener!
	paramsChanged.set(true);
}

void ResponseCurveDraw::timerCallback()
{
	//test switch for true and then set to false
	if (paramsChanged.compareAndSetBool(false, true))
	{
		auto eqSettings = getEqSettings(audioProcessor.parameters);
		//graphic representation for peak
		auto peakCoeffs = makePeakFilter(eqSettings, audioProcessor.getSampleRate());
		updateCoeffs(monoChain.get<eqTypes::Peak>().coefficients, peakCoeffs);
		//graphic representation for lowcut
		auto lowCutCoeffs = makeLowCutFilter(eqSettings, audioProcessor.getSampleRate());
		auto highCutCoeffs = makeHighCutFilter(eqSettings, audioProcessor.getSampleRate());
		updateCutFilters(monoChain.get<eqTypes::LowCut>(), lowCutCoeffs, eqSettings.lowCutSlope);
		updateCutFilters(monoChain.get<eqTypes::HighCut>(), highCutCoeffs, eqSettings.highCutSlope);
		repaint();
	}
}
void ResponseCurveDraw::paint(juce::Graphics& g)
{
	//bring everything necessary into scope or into variables into scope
	auto responseArea = getLocalBounds();
	auto responseWidth = responseArea.getWidth();
	auto& lowCut = monoChain.get < eqTypes::LowCut>();
	auto& peak = monoChain.get < eqTypes::Peak>();
	auto& highCut = monoChain.get < eqTypes::HighCut>();
	auto sampleRate = audioProcessor.getSampleRate();

	std::vector<double> mags;
	mags.resize(responseWidth);
	/*get magnitudes of our filter coefficients, we use the maptolog10 so the response
	curve is drawn proportional to how frequency is percieved by us.*/
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

	/*here we use the juce::Path class to draw our response curve, this class allows us to plot
	points for to draw a line, here we use all the individual magnitudes and draw a line for the
	response curve to be represented by*/
	juce::Path responseCurve;
	const double outMin = responseArea.getBottom();
	const double outMax = responseArea.getY();
	auto map = [outMin, outMax](double input) {return juce::jmap(input, -24.0, 24.0,
																 outMin, outMax); };
	responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

	for (size_t i = 1; i < mags.size(); ++i)
		responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));


	g.setColour(juce::Colours::orange);
	g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
	g.setColour(juce::Colours::white);
	g.strokePath(responseCurve, juce::PathStrokeType(2.f));
}
//==============================================================================
MyEQAudioProcessorEditor::MyEQAudioProcessorEditor(MyEQAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p),
	responseCurve(audioProcessor),
	peakFreqSliderAttatchment(audioProcessor.parameters, "Peak Freq", peakFreqSlider),
	peakGainSliderAttatchment(audioProcessor.parameters, "Peak Gain", peakGainSlider),
	peakQSliderAttatchment(audioProcessor.parameters, "Peak Q", peakQSlider),
	lowCutFreqSliderAttatchment(audioProcessor.parameters, "LowCut Freq", lowCutFreqSlider),
	lowCutSlopeSliderAttatchment(audioProcessor.parameters, "LowCut Slope", lowCutSlopeSlider),
	highCutSlopeSliderAttatchment(audioProcessor.parameters, "HighCut Slope", highCutSlopeSlider),
	highCutFreqSliderAttatchment(audioProcessor.parameters, "HighCut Freq", highCutFreqSlider)
{
	//push gui members to graphics rendering thread
	addAndMakeVisible(responseCurve);
	addAndMakeVisible(peakFreqSlider);
	peakFreqSlider.setLookAndFeel(&peakDials);
	addAndMakeVisible(peakGainSlider);
	peakGainSlider.setLookAndFeel(&peakDials);
	addAndMakeVisible(peakQSlider);
	peakQSlider.setLookAndFeel(&peakDials);
	addAndMakeVisible(lowCutFreqSlider);
	lowCutFreqSlider.setLookAndFeel(&lowDials);
	addAndMakeVisible(highCutFreqSlider);
	highCutFreqSlider.setLookAndFeel(&highDials);
	addAndMakeVisible(lowCutSlopeSlider);
	lowCutSlopeSlider.setLookAndFeel(&lowDials);
	addAndMakeVisible(highCutSlopeSlider);
	highCutSlopeSlider.setLookAndFeel(&highDials);


	setSize(800, 600);
}

MyEQAudioProcessorEditor::~MyEQAudioProcessorEditor()
{
}

//==============================================================================
void MyEQAudioProcessorEditor::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);
}

void MyEQAudioProcessorEditor::resized()
{

	//get bounds to divide into subsections for each gui component
	auto bounds = getLocalBounds();
	auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
	responseCurve.setBounds(responseArea);

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

