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
#include <sjf/helpers/sjf_DynamicProcessorSequence.h>

// #include <thread>
// #include <atomic>
// #include <chrono>
// #include <cstdlib>
// #include <new>

static thread_local bool trackAllocations = false;
static thread_local int  allocationCount  = 0;

void* operator new(std::size_t size)
{
    if (trackAllocations)
        ++allocationCount;
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void* operator new[](std::size_t size)
{
    if (trackAllocations)
        ++allocationCount;
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void operator delete(void* p) noexcept   { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept   { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace sjf::tests
{
using namespace helpers;
using SFC = processor_sequence::SubFactoryConfig;
using TestGain = Gain<>;
using AttenuatingGain = Gain<-60, 0, -12>;

struct TanhProcessor
{
    void prepare(const juce::dsp::ProcessSpec&) {}
    void reset() {}

    template <typename ProcessContext>
    void process(const ProcessContext& context) noexcept
    {
        auto& outputBlock = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples = outputBlock.getNumSamples();

        if (ProcessContext::usesSeparateInputAndOutputBlocks())
            outputBlock.copyFrom(context.getInputBlock());

        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* samples = outputBlock.getChannelPointer(ch);
            for (size_t i = 0; i < numSamples; ++i)
                samples[i] = std::tanh(samples[i]);
        }
    }

    std::unique_ptr<ParameterFactory> createParameters(const juce::String& factoryID, const juce::String& factoryName)
    {
        return ParameterFactory::create(factoryID, factoryName);
    }
};

// =============================================================================
// Generic Tests (Steady State + Parameter Ranges) - same pattern as ProcessorSequence
// =============================================================================
static GenericTests<DynamicProcessorSequence<TestGain, TestGain, TestGain>, SFC, SFC, SFC>
    dynamicProcessorSequenceGeneric(
        juce::String("DynamicProcessorSequence"),
        {"G1", "G1"}, {"G2", "G2"}, {"G3", "G3"});

// =============================================================================
// DynamicProcessorSequence-specific tests
// =============================================================================
class DynamicProcessorSequenceTests : public juce::UnitTest
{
public:
    DynamicProcessorSequenceTests()
        : juce::UnitTest("DynamicProcessorSequence Specific", "sjf_audio Unit Tests") {}


	template<typename Sequence, typename... Steps>
	void updateSequence(Sequence& sequence, const size_t inactiveSlot, Steps... steps )
    {
	    std::fill(sequence.begin(), sequence.end(), inactiveSlot);

    	size_t index = 0;
    	((sequence[index++] = static_cast<typename Sequence::value_type>(steps)), ...);
    }

    void runTest() override
    {


        juce::dsp::ProcessSpec spec;
        spec.maximumBlockSize = 32;
        spec.numChannels = 2;
        spec.sampleRate = 44100.0;

        testCase("Initial state - passthrough", [&](){
        	using DPS = DynamicProcessorSequence<AttenuatingGain, AttenuatingGain, AttenuatingGain>;
			DPS dps;
        	[[maybe_unused]] static constexpr auto InactiveSlot = DPS::InactiveSlot;
        	using Sequence = DPS::SequenceOrder;
        	[[maybe_unused]] Sequence seq;

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
            dps.reset();

            juce::AudioBuffer<float> buffer(2, 32);
            juce::AudioBuffer<float> original(2, 32);
            fillBufferWithSin(buffer);
            original.makeCopyOf(buffer);

            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            dps.process(context);

            for (int ch = 0; ch < 2; ++ch)
            {
                for (int s = 0; s < 32; ++s)
                {
                    expect(juce::approximatelyEqual(
                        buffer.getSample(ch, s),
                        original.getSample(ch, s)),
                        "Initial state should pass through audio unchanged");
                }
            }
        });

        testCase("Add single processor - actually processes audio", [&](){
        	using DPS = DynamicProcessorSequence<AttenuatingGain, AttenuatingGain, AttenuatingGain>;
            DPS dps;
        	static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
        	Sequence seq;
        	ValueTree state{"Params"};


            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
            dps.reset();

        	dps.attachToState(state);

            auto beforeBuffer = processAndCapture(dps, spec);

        	updateSequence(seq, InactiveSlot, 0);

        	dps.setSequence(seq);

            auto afterBuffer = processAndCapture(dps, spec);

            expect(!buffersEqual(beforeBuffer, afterBuffer),
                "Output should change after adding an attenuating processor");
        });

        testCase("Add and remove processor - output changes", [&](){
        	using DPS = DynamicProcessorSequence<AttenuatingGain, AttenuatingGain, AttenuatingGain>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;
			ValueTree state{"Params"};

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
            dps.reset();

        	dps.attachToState(state);

            updateSequence(seq, InactiveSlot, 0, 1);
        	dps.setSequence(seq);
            auto rmsWithTwo = processAndGetRms(dps, spec);

            updateSequence(seq, InactiveSlot, 1);
        	dps.setSequence(seq);
            auto rmsWithOne = processAndGetRms(dps, spec);

            expect(rmsWithOne > rmsWithTwo * 1.5f,
                "Removing one of two attenuating processors should increase output level. "
                "Two: " + juce::String(rmsWithTwo) + " One: " + juce::String(rmsWithOne));
        });

        testCase("Remove restores passthrough", [&](){
        	using DPS = DynamicProcessorSequence<AttenuatingGain, AttenuatingGain, AttenuatingGain>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;
			ValueTree state{"Params"};

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
            dps.reset();

        	dps.attachToState(state);

            updateSequence(seq, InactiveSlot, 0);
        	dps.setSequence(seq);
            processOneBlock(dps, spec);
            updateSequence(seq, InactiveSlot);
        	dps.setSequence(seq);

            juce::AudioBuffer<float> buffer(2, 32);
            juce::AudioBuffer<float> original(2, 32);
            fillBufferWithSin(buffer);
            original.makeCopyOf(buffer);

            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            dps.process(context);

            for (int ch = 0; ch < 2; ++ch)
            {
                for (int s = 0; s < 32; ++s)
                {
                    expect(juce::approximatelyEqual(
                        buffer.getSample(ch, s),
                        original.getSample(ch, s)),
                        "After removing all processors, should pass through");
                }
            }
        });

        testCase("moveProcessor changes processing order", [&](){
        	using DPS = DynamicProcessorSequence<AttenuatingGain, TanhProcessor>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;
			ValueTree state{"Params"};

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"T1", "T1"});
            dps.prepare(spec);
            dps.reset();

        	dps.attachToState(state);

        	updateSequence(seq, InactiveSlot, 0, 1);
        	dps.setSequence(seq);

            auto bufferGainFirst = processAndCapture(dps, spec);

            updateSequence(seq, InactiveSlot, 1, 0);
        	dps.setSequence(seq);

            auto bufferTanhFirst = processAndCapture(dps, spec);

            expect(!buffersEqual(bufferGainFirst, bufferTanhFirst),
                "Gain->Tanh should produce different output than Tanh->Gain");
        });

        testCase("swapProcessor replaces active with inactive", [&](){
            using Quiet = Gain<-60, 0, -30>;
        	using DPS = DynamicProcessorSequence<AttenuatingGain, TestGain, Quiet>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;
			ValueTree state{"Params"};

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
            dps.reset();

        	dps.attachToState(state);

            updateSequence(seq, InactiveSlot, 0, 1);
        	dps.setSequence(seq);

            auto beforeBuffer = processAndCapture(dps, spec);

            updateSequence(seq, InactiveSlot, 2, 1);
        	dps.setSequence(seq);

            auto afterBuffer = processAndCapture(dps, spec);

            expect(!buffersEqual(beforeBuffer, afterBuffer),
                "Swapping -12dB for -30dB processor should change output");

            auto rmsAfter = afterBuffer.getRMSLevel(0, 0, 32);
            auto rmsBefore = beforeBuffer.getRMSLevel(0, 0, 32);
            expect(rmsAfter < rmsBefore * 0.5f,
                "Swapping to quieter processor should decrease output. "
                "Before: " + juce::String(rmsBefore) + " After: " + juce::String(rmsAfter));
        });

        testCase("swapProcessor deactivates old and activates new", [&](){
            using DPS = DynamicProcessorSequence<AttenuatingGain, TestGain, AttenuatingGain>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;
			ValueTree state{"Params"};


            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
            dps.reset();

        	dps.attachToState(state);

        	updateSequence(seq, InactiveSlot, 2, 1);
        	dps.setSequence(seq);

            auto rmsWithSwap = processAndGetRms(dps, spec);

        	updateSequence(seq, InactiveSlot, 2, 1, 0);
        	dps.setSequence(seq);

            auto rmsWithOldReAdded = processAndGetRms(dps, spec);

            expect(rmsWithOldReAdded < rmsWithSwap * 0.5f,
                "Old processor (0) should be deactivated after swap and re-addable. "
                "Adding it back should further attenuate. "
                "Before re-add: " + juce::String(rmsWithSwap) +
                " After re-add: " + juce::String(rmsWithOldReAdded));
        });

        testCase("get() returns processors by index", [&](){
            using DPS = DynamicProcessorSequence<TestGain, TestGain, TestGain>;
        	DPS dps;
			[[maybe_unused]] static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			[[maybe_unused]] Sequence seq;
        	ValueTree state{"Params"};

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
        	dps.reset();

        	dps.attachToState(state);

            auto& g0 = dps.get<0>();
            auto& g1 = dps.get<1>();
            auto& g2 = dps.get<2>();

            expect(&g0 != &g1, "get<0>() and get<1>() should return different processors");
            expect(&g1 != &g2, "get<1>() and get<2>() should return different processors");
            expect(&g0 != &g2, "get<0>() and get<2>() should return different processors");
        });
   //
   //      testCase("prepare calls prepare on all processors", [&](){
   //          using DPS = DynamicProcessorSequence<AttenuatingGain, AttenuatingGain, AttenuatingGain>;
   //      	DPS dps;
			// static constexpr auto InactiveSlot = DPS::InactiveSlot;
			// using Sequence = DPS::SequenceOrder;
			// Sequence seq;
			// ValueTree state{"Params"};
   //
   //          auto params = dps.createParameters("Test", "Test",
   //              SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
   //          dps.prepare(spec);
   //      	dps.reset();
   //
   //      	dps.attachToState(state);
   //
   //      	updateSequence(seq, InactiveSlot, 0, 1, 2);
   //
   //          auto rms = processAndGetRms(dps, spec);
   //          auto inputRms = getInputRms();
   //
   //          expect(rms < inputRms * 0.3f,
   //              "All three -12dB processors should attenuate ~36dB total. "
   //              "Input: " + juce::String(inputRms) + " Output: " + juce::String(rms));
   //      });

        testCase("reset is called before first process on added processor", [&](){
            using DPS = DynamicProcessorSequence<AttenuatingGain, AttenuatingGain, AttenuatingGain>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;
			ValueTree state{"Params"};

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
            dps.reset();

        	dps.attachToState(state);

        	updateSequence(seq, InactiveSlot, 0);

            auto rmsFirstAdd = processAndGetRms(dps, spec);

        	updateSequence(seq, InactiveSlot);

            processOneBlock(dps, spec);
            processOneBlock(dps, spec);

        	updateSequence(seq, InactiveSlot, 0);
            auto rmsSecondAdd = processAndGetRms(dps, spec);

            expect(juce::approximatelyEqual(rmsFirstAdd, rmsSecondAdd, juce::absoluteTolerance(0.01f)),
                "Processor should produce same output after re-add (reset called). "
                "First: " + juce::String(rmsFirstAdd) +
                " Second: " + juce::String(rmsSecondAdd));
        });

   //      testCase("setPositionInfo propagates to all processors", [&](){
   //          using DPS = DynamicProcessorSequence<DummyProcessor, DummyProcessor>;
   //      	DPS dps;
			// static constexpr auto InactiveSlot = DPS::InactiveSlot;
			// using Sequence = DPS::SequenceOrder;
			// Sequence seq;
   //
   //          auto params = dps.createParameters("Test", "Test",
   //              SFC{"D1", "D1"}, SFC{"D2", "D2"});
   //          dps.prepare(spec);
   //
   //          juce::AudioPlayHead::PositionInfo posInfo;
   //          posInfo.setBpm(120.0);
   //          dps.setPositionInfo(posInfo);
   //
   //      	updateSequence(seq, InactiveSlot, 0, 1);
   //
   //          processOneBlock(dps, spec);
   //
   //          expect(true, "setPositionInfo should propagate without crashing");
   //      });

        // testCase("getLatencySamples returns sum", [&](){
        //     DynamicProcessorSequence<TestGain, TestGain, TestGain> dps;
        //     dps.prepare(spec);
        //
        //     auto latency = dps.getLatencySamples();
        //     expect(latency == 0, "Gains have no latency, should return 0");
        // });

        testCase("createParameters returns valid factory", [&](){
            DynamicProcessorSequence<TestGain, TestGain, TestGain> dps;
            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});

            expect(params != nullptr, "createParameters should return a valid ParameterFactory");
            expect(params->getParameters(true).size() > 0,
                "ParameterFactory should contain parameters from all processors");
        });

        testCase("Processor IDs match template parameter pack position", [&](){
            using DPS = DynamicProcessorSequence<AttenuatingGain, TestGain, TestGain>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;
        	ValueTree state{"Params"};

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
        	dps.reset();

        	dps.attachToState(state);

        	updateSequence(seq, InactiveSlot, 0);
        	dps.setSequence(seq);

            auto rmsWithId0 = processAndGetRms(dps, spec);

        	updateSequence(seq, InactiveSlot, 1);
        	dps.setSequence(seq);

            auto rmsWithId1 = processAndGetRms(dps, spec);

            auto inputRms = getInputRms();

            expect(rmsWithId0 < inputRms * 0.5f,
                "Processor ID 0 (AttenuatingGain at -12dB) should attenuate. "
                "Input: " + juce::String(inputRms) + " Output: " + juce::String(rmsWithId0));
            expect(juce::approximatelyEqual(rmsWithId1, inputRms, juce::absoluteTolerance(0.01f)),
                "Processor ID 1 (TestGain at 0dB) should pass through. "
                "Input: " + juce::String(inputRms) + " Output: " + juce::String(rmsWithId1));
        });

        testCase("moveProcessor valid range", [&](){
            using DPS = DynamicProcessorSequence<AttenuatingGain, TanhProcessor, AttenuatingGain>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;
        	ValueTree state{"Params"};

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"T1", "T1"}, SFC{"G2", "G2"});
            dps.prepare(spec);
            dps.reset();

        	dps.attachToState(state);

        	updateSequence(seq, InactiveSlot, 0, 1, 2);
        	dps.setSequence(seq);

            const auto bufferOriginal = processAndCapture(dps, spec);

        	updateSequence(seq, InactiveSlot, 0, 2, 1);
        	dps.setSequence(seq);

            const auto bufferMoved = processAndCapture(dps, spec);

            expect(!buffersEqual(bufferOriginal, bufferMoved),
                "Moving tanh to a different position should change output");

        	updateSequence(seq, InactiveSlot, 0, 1, 2);
        	dps.setSequence(seq);

            auto bufferRestored = processAndCapture(dps, spec);

            expect(buffersEqual(bufferOriginal, bufferRestored),
                "Moving tanh back to original position should restore output");
        });

        testCase("Concurrent add/remove and process - no crash", [&](){
            using DPS = DynamicProcessorSequence<AttenuatingGain, AttenuatingGain, AttenuatingGain>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
            dps.reset();

            std::atomic<bool> running{true};
            std::atomic<int> blocksProcessed{0};

            std::thread audioThread([&](){
                while (running.load())
                {
                    juce::AudioBuffer<float> buffer(2, 32);
                    fillBufferWithSin(buffer);
                    juce::dsp::AudioBlock<float> block(buffer);
                    juce::dsp::ProcessContextReplacing<float> context(block);
                    dps.process(context);
                    blocksProcessed.fetch_add(1, std::memory_order_relaxed);
                }
            });

            for (int i = 0; i < 100; ++i)
            {
            	updateSequence(seq, InactiveSlot, 0, 1, 2);
                std::this_thread::sleep_for(std::chrono::microseconds(100));

            	updateSequence(seq, InactiveSlot, 0, 2);
            	updateSequence(seq, InactiveSlot, 1, 2);
            	updateSequence(seq, InactiveSlot, 2, 0);
            	updateSequence(seq, InactiveSlot, 2, 0, 1);
                std::this_thread::sleep_for(std::chrono::microseconds(100));

            	updateSequence(seq, InactiveSlot, 1, 2);
                updateSequence(seq, InactiveSlot, 0);
                updateSequence(seq, InactiveSlot, 0, 1, 2);
                updateSequence(seq, InactiveSlot, 1, 2, 0);

                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            running.store(false);
            audioThread.join();

            expect(blocksProcessed.load() > 0,
                "Audio thread should have processed blocks during concurrent mutations");
        });

        testCase("All processors exist for entire lifetime", [&](){
            using DPS = DynamicProcessorSequence<AttenuatingGain, AttenuatingGain, AttenuatingGain>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);

            auto& g0_before = dps.get<0>();
            auto& g1_before = dps.get<1>();
            auto& g2_before = dps.get<2>();

        	updateSequence(seq, InactiveSlot, 0, 1, 2);
        	updateSequence(seq, InactiveSlot);
        	updateSequence(seq, InactiveSlot, 0, 1, 2);


            auto& g0_after = dps.get<0>();
            auto& g1_after = dps.get<1>();
            auto& g2_after = dps.get<2>();

            expect(&g0_before == &g0_after, "Processor 0 address should not change");
            expect(&g1_before == &g1_after, "Processor 1 address should not change");
            expect(&g2_before == &g2_after, "Processor 2 address should not change");
        });

        testCase("Process with no allocation on audio thread", [&](){
            using DPS = DynamicProcessorSequence<AttenuatingGain, AttenuatingGain, AttenuatingGain>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
            dps.reset();

        	updateSequence(seq, InactiveSlot, 0, 1, 2);


            processOneBlock(dps, spec);

            allocationCount = 0;
            trackAllocations = true;

            for (int i = 0; i < 100; ++i)
                processOneBlock(dps, spec);

            trackAllocations = false;

            expect(allocationCount == 0,
                "process() allocated " + juce::String(allocationCount) +
                " time(s) on the audio thread. Must be 0.");
        });

        testCase("process() does not lock", [&](){
            using DPS = DynamicProcessorSequence<AttenuatingGain, AttenuatingGain, AttenuatingGain>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"G2", "G2"}, SFC{"G3", "G3"});
            dps.prepare(spec);
            dps.reset();

            updateSequence(seq, InactiveSlot, 0);
            processOneBlock(dps, spec);

            std::atomic<bool> done{false};
            std::atomic<int> processCompleted{0};

            std::thread mutationThread([&](){
                while (!done.load(std::memory_order_acquire))
                {
                	updateSequence(seq, InactiveSlot, 0, 1);
                	updateSequence(seq, InactiveSlot, 0, 1, 2);
                	updateSequence(seq, InactiveSlot, 1, 0, 2);
                	updateSequence(seq, InactiveSlot, 2, 0, 1);
                	updateSequence(seq, InactiveSlot, 2, 1, 0);
                	updateSequence(seq, InactiveSlot, 1, 2, 0);
                	updateSequence(seq, InactiveSlot, 1, 0);
                	updateSequence(seq, InactiveSlot, 0);

                }
            });

            auto start = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(500))
            {
                juce::AudioBuffer<float> buffer(2, 32);
                fillBufferWithSin(buffer);
                juce::dsp::AudioBlock<float> block(buffer);
                juce::dsp::ProcessContextReplacing<float> context(block);

                auto blockStart = std::chrono::steady_clock::now();
                dps.process(context);
                auto blockDuration = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - blockStart).count();

                expect(blockDuration < 1000,
                    "process() took " + juce::String(blockDuration) + "us - "
                    "suggests locking. process() must never lock.");

                processCompleted.fetch_add(1, std::memory_order_relaxed);
            }

            done.store(true, std::memory_order_release);
            mutationThread.join();

            const auto totalBlocks = processCompleted.load();
            expect(totalBlocks > 100,
                "Only " + juce::String(totalBlocks) + " blocks processed in 500ms. "
                "Expected thousands - process() is likely being blocked by a lock.");
        });

        testCase("Processor order affects output", [&](){
            using DPS = DynamicProcessorSequence<AttenuatingGain, TanhProcessor>;
        	DPS dps;
			static constexpr auto InactiveSlot = DPS::InactiveSlot;
			using Sequence = DPS::SequenceOrder;
			Sequence seq;
        	ValueTree state{"Params"};

            auto params = dps.createParameters("Test", "Test",
                SFC{"G1", "G1"}, SFC{"T1", "T1"});
            dps.prepare(spec);
            dps.reset();
        	dps.attachToState(state);

        	updateSequence(seq, InactiveSlot, 0, 1);
        	dps.setSequence(seq);

            auto bufferGainThenTanh = processAndCapture(dps, spec);

        	updateSequence(seq, InactiveSlot, 1, 0);
        	dps.setSequence(seq);
        	
            auto bufferTanhThenGain = processAndCapture(dps, spec);

            expect(!buffersEqual(bufferGainThenTanh, bufferTanhThenGain),
                "Gain->Tanh should produce different output than Tanh->Gain "
                "because tanh is nonlinear");
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

static DynamicProcessorSequenceTests dynamicProcessorSequenceSpecificTests;

} // namespace sjf::tests
