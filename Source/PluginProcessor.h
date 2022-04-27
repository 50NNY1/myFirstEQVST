/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
enum Slope
{
	Slope_12,
	Slope_24,
	Slope_36,
	Slope_48
};
struct EqSettings
{
	float lowCutFreq{ 0 }, highCutFreq{ 0 }, peakFreq{ 0 };
	float peakGain{ 0 }, peakQ{ 1.f };
	Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};
using Filter = juce::dsp::IIR::Filter<float>;
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
enum eqTypes
{
	LowCut,
	Peak,
	HighCut
};

template<typename Oldcoeff, typename Newcoeff>
void updateCoeffs(Oldcoeff& oldcoeff, Newcoeff& newcoeff)
{
	oldcoeff = newcoeff;
}
using Coefficients = Filter::CoefficientsPtr;
Coefficients makePeakFilter(const EqSettings& eqSettings, double samplerate);

inline auto makeLowCutFilter(const EqSettings& eqSettings, double samplerate);
inline auto makeHighCutFilter(const EqSettings& eqSettings, double samplerate);
template<typename EqType, typename CoeffType>
void updateCutFilters(EqType& cutFilter,
					  CoeffType& cutCoeffs,
					  Slope& slope)
{
	cutFilter.template setBypassed<0>(true);
	cutFilter.template setBypassed<1>(true);
	cutFilter.template setBypassed<2>(true);
	cutFilter.template setBypassed<3>(true);
	switch (slope)
	{
	case Slope_48:
		*cutFilter.template get<3>().coefficients = *cutCoeffs[3];
		cutFilter.template setBypassed<3>(false);
	case Slope_36:
		*cutFilter.template get<2>().coefficients = *cutCoeffs[2];
		cutFilter.template setBypassed<2>(false);
	case Slope_24:
		*cutFilter.template get<1>().coefficients = *cutCoeffs[1];
		cutFilter.template setBypassed<1>(false);
	case Slope_12:
		*cutFilter.template get<0>().coefficients = *cutCoeffs[0];
		cutFilter.template setBypassed<0>(false);
	}
}

EqSettings getEqSettings(juce::AudioProcessorValueTreeState& parameters);

//==============================================================================
/**
*/
class MyEQAudioProcessor : public juce::AudioProcessor
{
public:
	//==============================================================================
	MyEQAudioProcessor();
	~MyEQAudioProcessor() override;

	//==============================================================================
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

	//==============================================================================
	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	//==============================================================================
	const juce::String getName() const override;

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	//==============================================================================
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram(int index) override;
	const juce::String getProgramName(int index) override;
	void changeProgramName(int index, const juce::String& newName) override;

	//==============================================================================
	void getStateInformation(juce::MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;
	juce::AudioProcessorValueTreeState::ParameterLayout
		createParameterLayout();
	juce::AudioProcessorValueTreeState parameters{ *this, nullptr,
	"Parameters",createParameterLayout() };

private:
	//==============================================================================
	MonoChain leftChain, rightChain;
	void updatePeakFilter(EqSettings& settings);
	void updateLowCutFilter(EqSettings& settings);
	void updateHighCutFilter(EqSettings& settings);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyEQAudioProcessor)
};
