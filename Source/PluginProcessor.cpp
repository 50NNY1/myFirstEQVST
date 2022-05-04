/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
//==============================================================================
MyEQAudioProcessor::MyEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
					 .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
					 .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
	)
#endif
{
}

MyEQAudioProcessor::~MyEQAudioProcessor()
{
}

//==============================================================================
const juce::String MyEQAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool MyEQAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool MyEQAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool MyEQAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double MyEQAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int MyEQAudioProcessor::getNumPrograms()
{
	return 1;

}

int MyEQAudioProcessor::getCurrentProgram()
{
	return 0;
}

void MyEQAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MyEQAudioProcessor::getProgramName(int index)
{
	return {};
}

void MyEQAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void MyEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	//prepare audio, standard juce stuff.
	juce::dsp::ProcessSpec spec;
	spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = 1;
	spec.sampleRate = sampleRate;
	leftChain.prepare(spec);
	rightChain.prepare(spec);

	//bring eq settings into scope
	auto eqSettings = getEqSettings(parameters);

	//prepare peak filter
	updatePeakFilter(eqSettings);
	updateLowCutFilter(eqSettings);
	updateHighCutFilter(eqSettings);
}

void MyEQAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MyEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused(layouts);
	return true;
#else
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}
#endif

void MyEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	//bring audio stream into scope
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	//clear buffer at beginning of processing the next upcoming block
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	//bring users eq settings into scope
	auto eqSettings = getEqSettings(parameters);

	//process filters
	updatePeakFilter(eqSettings);
	updateLowCutFilter(eqSettings);
	updateHighCutFilter(eqSettings);

	//output now produced audio block
	juce::dsp::AudioBlock<float> block(buffer);
	auto leftBlock = block.getSingleChannelBlock(0);
	auto rightBlock = block.getSingleChannelBlock(1);
	juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
	juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
	leftChain.process(leftContext);
	rightChain.process(rightContext);
}

//==============================================================================
bool MyEQAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MyEQAudioProcessor::createEditor()
{
	return new MyEQAudioProcessorEditor(*this);
}

//==============================================================================
void MyEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	/*this code allows us to recall the users data on closeand open, this code
	saves it */
	juce::MemoryOutputStream mos(destData, true);
	parameters.state.writeToStream(mos);

}

void MyEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	//this code recalls the data saved above ^^^
	auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
	if (tree.isValid())
	{
		parameters.replaceState(tree);
	}
}

juce::AudioProcessorValueTreeState::ParameterLayout MyEQAudioProcessor::createParameterLayout()
{
	/*to create an 'object' of sorts to store our parameters here we use juce's
	audio processor value tree state class, here we declare the parameters, their names
	ranges etc...*/
	juce::AudioProcessorValueTreeState::ParameterLayout layout;
	layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
														   "LowCut Freq",
														   juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
														   20.f));
	layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
														   "HighCut Freq",
														   juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
														   20000.f));
	layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
														   "Peak Freq",
														   juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
														   750.f));
	layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
														   "Peak Gain",
														   juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
														   0.0f));
	layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Q",
														   "Peak Q",
														   juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
														   1.f));
	/*here we are still just adding more parameters for the user, juce through a for loop.
	here we are creating the cutoff slope parameters*/
	juce::StringArray stringArray;
	for (int i = 0; i < 4; ++i)
	{
		juce::String str;
		str << (12 + i * 12);
		str << "db/Oct";
		stringArray.add(str);
	}
	layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope",
															"LowCut Slope",
															stringArray,
															0));
	layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope",
															"HighCut Slope",
															stringArray,
															0));

	return layout;
}
void MyEQAudioProcessor::updatePeakFilter(EqSettings& settings)
{
	/*for the next 3 functions i have written, when in preparetoplay() and processblock()
	and the update filter functions are called, we call another function, declared in our header
	file, coefficients for the IIR filters are generated, and then updated per the user's settings */
	auto peakCoeffs = makePeakFilter(settings, getSampleRate());
	updateCoeffs(*leftChain.get<eqTypes::Peak>().coefficients, *peakCoeffs);
	updateCoeffs(*rightChain.get<eqTypes::Peak>().coefficients, *peakCoeffs);
}
void MyEQAudioProcessor::updateLowCutFilter(EqSettings& settings)
{
	auto lowCutCoeffs = makeLowCutFilter(settings, getSampleRate());
	auto& leftLowCut = leftChain.get<eqTypes::LowCut>();
	auto& rightLowCut = rightChain.get<eqTypes::LowCut>();
	updateCutFilters(leftLowCut, lowCutCoeffs, settings.lowCutSlope);
	updateCutFilters(rightLowCut, lowCutCoeffs, settings.lowCutSlope);
}
void MyEQAudioProcessor::updateHighCutFilter(EqSettings& settings)
{
	auto highCutCoeffs = makeHighCutFilter(settings, getSampleRate());
	auto& leftHighCut = leftChain.get<eqTypes::HighCut>();
	auto& rightHighCut = rightChain.get<eqTypes::HighCut>();
	updateCutFilters(leftHighCut, highCutCoeffs, settings.highCutSlope);
	updateCutFilters(rightHighCut, highCutCoeffs, settings.highCutSlope);
}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new MyEQAudioProcessor();
}

Coefficients makePeakFilter(const EqSettings& eqSettings, double sampleRate)
{
	/*here we generate the coefficients for the peak filter*/
	return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
		sampleRate,
		eqSettings.peakFreq,
		eqSettings.peakQ,
		juce::Decibels::decibelsToGain(eqSettings.peakGain));;
}

EqSettings getEqSettings(juce::AudioProcessorValueTreeState& parameters)
{
	/*allows us to bring eq settings into scope elsewhere in the code*/
	EqSettings eqSettings;
	eqSettings.lowCutFreq = parameters.getRawParameterValue("LowCut Freq")->load();
	eqSettings.highCutFreq = parameters.getRawParameterValue("HighCut Freq")->load();
	eqSettings.peakFreq = parameters.getRawParameterValue("Peak Freq")->load();
	eqSettings.peakGain = parameters.getRawParameterValue("Peak Gain")->load();
	eqSettings.peakQ = parameters.getRawParameterValue("Peak Q")->load();
	eqSettings.lowCutSlope = static_cast<Slope>(parameters.getRawParameterValue("LowCut Slope")->load());
	eqSettings.highCutSlope = static_cast<Slope>(parameters.getRawParameterValue("HighCut Slope")->load());
	return eqSettings;
}
