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
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MyEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MyEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MyEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void MyEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MyEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    auto eqSettings = getEqSettings(parameters);
    auto peakCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 
        eqSettings.peakFreq,
        eqSettings.peakQ,
        juce::Decibels::decibelsToGain(eqSettings.peakGain));

    *leftChain.get<eqTypes::Peak>().coefficients = *peakCoeffs;
    *rightChain.get<eqTypes::Peak>().coefficients = *peakCoeffs;

    auto lowCutCoeffs =
        juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(eqSettings.lowCutFreq,
            sampleRate,
            2* (eqSettings.lowCutSlope + 1));

    auto& leftLowCut = leftChain.get<eqTypes::LowCut>();
    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    switch (eqSettings.lowCutSlope)
    {
    case Slope_48:
        *leftLowCut.get<3>().coefficients = *lowCutCoeffs[3];
        leftLowCut.setBypassed<3>(false);
    case Slope_36:
        *leftLowCut.get<2>().coefficients = *lowCutCoeffs[2];
        leftLowCut.setBypassed<2>(false);
    case Slope_24:
        *leftLowCut.get<1>().coefficients = *lowCutCoeffs[1];
        leftLowCut.setBypassed<1>(false);
    case Slope_12:
        *leftLowCut.get<0>().coefficients = *lowCutCoeffs[0];
        leftLowCut.setBypassed<0>(false);
    }

    auto& rightLowCut = rightChain.get<eqTypes::LowCut>();
    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);

    switch (eqSettings.lowCutSlope)
    {
    case Slope_48:
        *rightLowCut.get<3>().coefficients = *lowCutCoeffs[3];
        rightLowCut.setBypassed<3>(false);
    case Slope_36:
        *rightLowCut.get<2>().coefficients = *lowCutCoeffs[2];
        rightLowCut.setBypassed<2>(false);
    case Slope_24:
        *rightLowCut.get<1>().coefficients = *lowCutCoeffs[1];
        rightLowCut.setBypassed<1>(false);
    case Slope_12:
        *rightLowCut.get<0>().coefficients = *lowCutCoeffs[0];
        rightLowCut.setBypassed<0>(false);
    }
} 

void MyEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MyEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void MyEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto eqSettings = getEqSettings(parameters);
    
    //process peak filter
    auto peakCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
        eqSettings.peakFreq,
        eqSettings.peakQ,
        juce::Decibels::decibelsToGain(eqSettings.peakGain));
    *leftChain.get<eqTypes::Peak>().coefficients = *peakCoeffs;
    *rightChain.get<eqTypes::Peak>().coefficients = *peakCoeffs;
    
    //process lowcut filter
    auto lowCutCoeffs =
        juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(eqSettings.lowCutFreq,
            getSampleRate(),
            2 * (eqSettings.lowCutSlope + 1));

    auto& leftLowCut = leftChain.get<eqTypes::LowCut>();
    leftLowCut.setBypassed<0>(true);
    leftLowCut.setBypassed<1>(true);
    leftLowCut.setBypassed<2>(true);
    leftLowCut.setBypassed<3>(true);

    switch (eqSettings.lowCutSlope)
    {
    case Slope_48:
        *leftLowCut.get<3>().coefficients = *lowCutCoeffs[3];
        leftLowCut.setBypassed<3>(false);
    case Slope_36:
        *leftLowCut.get<2>().coefficients = *lowCutCoeffs[2];
        leftLowCut.setBypassed<2>(false);
    case Slope_24:
        *leftLowCut.get<1>().coefficients = *lowCutCoeffs[1];
        leftLowCut.setBypassed<1>(false);
    case Slope_12:
        *leftLowCut.get<0>().coefficients = *lowCutCoeffs[0];
        leftLowCut.setBypassed<0>(false);
    }

    auto& rightLowCut = rightChain.get<eqTypes::LowCut>();
    rightLowCut.setBypassed<0>(true);
    rightLowCut.setBypassed<1>(true);
    rightLowCut.setBypassed<2>(true);
    rightLowCut.setBypassed<3>(true);

    switch (eqSettings.lowCutSlope)
    {
    case Slope_48:
        *rightLowCut.get<3>().coefficients = *lowCutCoeffs[3];
        rightLowCut.setBypassed<3>(false);
    case Slope_36:
        *rightLowCut.get<2>().coefficients = *lowCutCoeffs[2];
        rightLowCut.setBypassed<2>(false);
    case Slope_24:
        *rightLowCut.get<1>().coefficients = *lowCutCoeffs[1];
        rightLowCut.setBypassed<1>(false);
    case Slope_12:
        *rightLowCut.get<0>().coefficients = *lowCutCoeffs[0];
        rightLowCut.setBypassed<0>(false);
    }

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
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void MyEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void MyEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout MyEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq", 
        "LowCut Freq", 
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f), 
        20.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
        "HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
        20000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
        "Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
        750.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
        "Peak Gain",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
        0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Q",
        "Peak Q",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        1.f));

    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i) {
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
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyEQAudioProcessor();
}

EqSettings getEqSettings(juce::AudioProcessorValueTreeState& parameters)
{
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
