/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 05/08/2026.
//
#include <JuceHeader.h>
#include "sjf_UnitTester/sjf_GenericTests.h"
#include <sjf/helpers/sjf_Gain.h>
#include <sjf/dsp/sjf_Modulation.h>
#include <sjf/dsp/sjf_ModulatorLFO.h>


namespace sjf::tests::modulation
{
using namespace helpers;
using SFC = processor_sequence::SubFactoryConfig;
using TestGain = Gain<>;
using AttenuatingGain = Gain<-60, 0, -12>;
using LFO = sjf::dsp::modulation::BasicLFO;


// The modulation system requires an APVTS as the central source of truth
struct TestProcessor : juce::AudioProcessor
{
	TestProcessor(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
	: apvts(*this, nullptr, "Params", std::move(layout))
	{}

	const String getName() const override { return "Test"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>&, MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0;}
    bool acceptsMidi() const override { return false;}
    bool producesMidi() const override { return false;}
    AudioProcessorEditor* createEditor() override { return nullptr;}
    bool hasEditor() const override { return false;}
    int getNumPrograms() override { return 0;}
    int getCurrentProgram() override { return 0;}
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return "";}
    void changeProgramName(int, const String&) override {}
    void getStateInformation(MemoryBlock&) override {}
	void setStateInformation(const void*, int) override {}

	juce::AudioProcessorValueTreeState apvts;
};
	
// =============================================================================
// ModulationSystem-specific tests
// =============================================================================
class ModulationSystemTests : public juce::UnitTest
{
public:
    ModulationSystemTests()
        : juce::UnitTest("ModulationSystem Specific", "sjf_audio Unit Tests") {}


	template<typename Sequence, typename... Steps>
	void updateSequence(Sequence& sequence, const size_t inactiveSlot, Steps... steps )
    {
	    std::fill(sequence.begin(), sequence.end(), inactiveSlot);

    	size_t index = 0;
    	((sequence[index++] = static_cast<typename Sequence::value_type>(steps)), ...);
    }

    void runTest() override
    {
    	
        juce::dsp::ProcessSpec spec{};
        spec.maximumBlockSize = 32;
        spec.numChannels = 2;
        spec.sampleRate = 44100.0;
    	

    	testCase("Adding modulation actually changes things", [&](){
    		static constexpr auto numBlocks = 10;
			TestGain gain;
			using Mods = dsp::modulation::ModulationSystem<LFO>;
			// using Connection = Mods::Connection;
			// using Connections = Mods::Connections;
   //  		using Modulator = Mods::Modulator;
   //  		using Modulatable = Mods::Modulatable;

			Mods mods{};


			juce::AudioProcessorValueTreeState::ParameterLayout layout;

			auto modParams = mods.createParameters("Mod", "Mod", SFC{"LFO1", "LFO1"});

			layout.add(std::move(modParams));

			auto processorParams = gain.createParameters("Gain", "Gain");
    		auto processorParams_ = processorParams.get();
			layout.add(std::move(processorParams));

			TestProcessor mainProcessor(layout);

			mods.attachAPVTS(mainProcessor.apvts);
			mods.prepare(spec);
			mods.reset();

			gain.prepare(spec);
			gain.reset();
    		auto bufferSize = static_cast<int>(spec.maximumBlockSize * numBlocks);
			juce::AudioBuffer<float> buffer(2, static_cast<int>(spec.maximumBlockSize));
			juce::AudioBuffer<float> original(2, bufferSize);
			juce::AudioBuffer<float> after1(2, bufferSize);
			juce::AudioBuffer<float> after2(2, bufferSize);
			juce::AudioBuffer<float> after3(2, bufferSize);

    		{
    			auto block = juce::dsp::AudioBlock<float>(buffer);
				block.fill(0.5f);
    		}
    		for (auto i = 0; i < numBlocks; ++i)
    		{
    			for ( auto c = 0; c < 2; ++c)
					original.copyFrom(c, i * static_cast<int>(spec.maximumBlockSize), buffer, c, 0, static_cast<int>(spec.maximumBlockSize));
    		}

    		auto processBlocks = [&](juce::AudioBuffer<float>& outputBuffer){
				const auto block = juce::dsp::AudioBlock<float>(outputBuffer);
				const auto _block = juce::dsp::AudioBlock<float>(buffer);
    			for ( auto i = 0ul; i < numBlocks; ++i )
    			{
    				auto subBlock = block.getSubBlock(spec.maximumBlockSize*i, spec.maximumBlockSize);
    				subBlock.copyFrom(_block);
    				auto context = juce::dsp::ProcessContextReplacing<float>(subBlock);
    				mods.process(context);
    				gain.process(context);
    			}
    		};

			processBlocks(after1);

    		mods.addConnection(mods.modulators.getIndexToId()[0], dynamic_cast<RangedAudioParameter*>(processorParams_->getParameters(true)[0])->paramID, 1.0f, nullptr);

			mods.reset();
    		gain.reset();

    		processBlocks(after2);

    		mods.removeAllConnections(nullptr);


			mods.reset();
    		gain.reset();


    		processBlocks(after3);

    		auto minus80dB = juce::Decibels::decibelsToGain(-80.0f);
			const auto eps = juce::absoluteTolerance(minus80dB);


    		{
    			// get difference of original and modulated
    			auto after2Block = juce::dsp::AudioBlock<float>(after2);
			    auto originalBlock = juce::dsp::AudioBlock<float>(original);
			    after2Block.addProductOf(originalBlock, -1.0f);
    		}

			for (int ch = 0; ch < 2; ++ch)
			{
				for (int s = 0; s < static_cast<int>(numBlocks * spec.maximumBlockSize); ++s)
				{
				 	expect(juce::approximatelyEqual(after1.getSample(ch, s), original.getSample(ch, s), eps),
							"Initial state should pass through audio unchanged");
				 	expect(juce::approximatelyEqual(after3.getSample(ch, s), original.getSample(ch, s), eps),
							"Removing modulators should return to initial state");
				}

				expect(after2.getRMSLevel(ch, 0, bufferSize) > minus80dB, "Adding modulator should change things");
			}
	   });
    }

private:
    template<typename Proc>
    void processOneBlock(Proc& processor, const juce::dsp::ProcessSpec& spec)
    {
        juce::AudioBuffer<float> buffer(static_cast<int>(spec.numChannels),
                                        static_cast<int>(spec.maximumBlockSize));
        fillBufferWithSin(buffer);
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        processor.process(context);
    }

    template<typename Proc>
    float processAndGetRms(Proc& processor, const juce::dsp::ProcessSpec& spec)
    {
        juce::AudioBuffer<float> buffer(static_cast<int>(spec.numChannels),
                                        static_cast<int>(spec.maximumBlockSize));
        fillBufferWithSin(buffer);
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        processor.process(context);
        return buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    }

    template<typename Proc>
    juce::AudioBuffer<float> processAndCapture(Proc& processor, const juce::dsp::ProcessSpec& spec)
    {
        juce::AudioBuffer<float> buffer(static_cast<int>(spec.numChannels),
                                        static_cast<int>(spec.maximumBlockSize));
        fillBufferWithSin(buffer);
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        processor.process(context);
        return buffer;
    }

    float getInputRms()
    {
        juce::AudioBuffer<float> buffer(2, 32);
        fillBufferWithSin(buffer);
        return buffer.getRMSLevel(0, 0, 32);
    }

    static bool buffersEqual(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
            return false;
        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int s = 0; s < a.getNumSamples(); ++s)
                if (!juce::approximatelyEqual(a.getSample(ch, s), b.getSample(ch, s)))
                    return false;
        return true;
    }

    static void fillBufferWithSin(juce::AudioBuffer<float>& buffer)
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();
        const auto chan = buffer.getArrayOfWritePointers()[0];
        for (int i = 0; i < numSamples; i++)
            chan[i] = 0.99f * juce::dsp::FastMathApproximations::sin(
                2.0f * juce::MathConstants<float>::pi * static_cast<float>(i) / static_cast<float>(numSamples));

        for (auto i = 1; i < numChannels; i++)
            buffer.copyFrom(i, 0, buffer, 0, 0, numSamples);
    }
};

static ModulationSystemTests modulationSystemTests;

} // namespace sjf::tests
