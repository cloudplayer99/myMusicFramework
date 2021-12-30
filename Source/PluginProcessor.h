/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/*********************** my code here ************************************/

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct ChainSettings
{
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };

    // int lowCutSlope{ 0 }, highCutSlope{ 0 };
    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;

using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

enum ChainPositions
{
    LowCut,
    Peak,
    HighCut
};

using Coefficients = Filter::CoefficientsPtr; /** CoefficientsPtr: A typedef for a ref-counted pointer to the coefficients object */
void updateCoefficients(Coefficients& old, const Coefficients& replacements);

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);


// template function update
template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain, const CoefficientType& coefficients)
{
    //*leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
    //leftLowCut.template setBypassed<0>(false);
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}

// template function updateCutFilter
template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType& chain,
    const CoefficientType& cutCoefficients,
    //const ChainSettings& chainSettings)
    const Slope& lowCutSlope)
{
    //auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
    //                                                                                               getSampleRate(),
    //                                                                                               2 * (chainSettings.lowCutSlope + 1));
    //                                                                                               // chainSettings.lowCutSlope: 0 1 2 3
    //                                                                                               // 0: 12 db/oct
    //                                                                                               // 1: 24 db/oct
    //                                                                                               // 2: 36 db/oct
    //                                                                                               // 3: 48 db/oct
    //                                                                                               // order: 2 4 6 8

    //auto& leftLowCut = leftChain.get<ChainPositions::LowCut>(); // leftLowCut : chain

    // bypass all of links in the chain
    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    chain.template setBypassed<3>(true);

    //switch (chainSettings.lowCutSlope)
    switch (lowCutSlope)
    {
    case Slope_48:
        update<3>(chain, cutCoefficients);
    case Slope_36:
        update<2>(chain, cutCoefficients);
    case Slope_24:
        update<1>(chain, cutCoefficients);
    case Slope_12:
        update<0>(chain, cutCoefficients);
    default:
        break;
    }
}

inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                       sampleRate,
                                                                                       2 * (chainSettings.lowCutSlope + 1));
}

inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
                                                                                      sampleRate,
                                                                                      2 * (chainSettings.highCutSlope + 1));
}

/*************************************************************************/

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // my code here
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters", 
        createParameterLayout()};

private:

    // my code here
    // let's create some type aliases to eliminate a lot of those namespace and template definitions
    // using Filter = juce::dsp::IIR::Filter<float>;
    // using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    // using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    // enum ChainPositions
    // we need to give the editor its own instance of the mono chain
    // to do that, we need to make all of the stuff that makes the mono chain public

    MonoChain leftChain, rightChain;

    // update the coefficients of Peak Filter
    void updatePeakFilter(const ChainSettings& chainSettings);
    // refactored Peak coefficient generation
    // move to the top
    //using Coefficients = Filter::CoefficientsPtr; /** CoefficientsPtr: A typedef for a ref-counted pointer to the coefficients object */
    //static void updateCoefficients(Coefficients& old, const Coefficients& replacements);

    

    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);

    void updateFilters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
