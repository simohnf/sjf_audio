/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 30/07/2026.
//
#include "sjf_UnitTester/sjf_GenericTests.h"
#include <sjf/helpers/sjf_Gain.h>

#include <sjf/helpers/sjf_BypassWrapper.h>
#include <sjf/helpers/sjf_ChunkedWrapper.h>
#include <sjf/helpers/sjf_OversamplingWrapper.h>

#include <sjf/helpers/sjf_DCBlock.h>
#include <sjf/helpers/sjf_ProcessDuplicator.h>
#include <sjf/helpers/sjf_ProcessorSelector.h>
namespace sjf::tests
{
	using namespace helpers;
	static GenericTests<Gain<>> gain("Gain");
	using TestGain = Gain<-60, 0>;
	static GenericTests<ProcessorDuplicator<TestGain>> processorDuplicator("ProcessorDuplicator");

	static GenericTests<ProcessorDuplicator<DCBlocker<>>> dcBlock("DCBlock");



	static GenericTests< BypassWrapper<TestGain, bypass_wrapper_config::Mix, bypass_wrapper_config::Bypass, bypass_wrapper_config::Mute>> bypassWrapper("BypassWrapper");
	static GenericTests< ChunkedWrapper<TestGain>> chunkedWrapper("ChunkedWrapper");
	static GenericTests< OversamplingWrapper<TestGain>> oversamplingWrapper("OverSamplingWrapper");



	/// Helper for templating multiprocessor wrappers for tests
	///
	using SFC = processor_sequence::SubFactoryConfig; // cut down on some typing
	template<template<typename, typename, typename> typename Processor>
	struct GenericMultiProcessorWrapperTests :  GenericTests<Processor <TestGain, TestGain, TestGain>, SFC, SFC, SFC>
	{
		using GenericTests<Processor <TestGain, TestGain, TestGain>, SFC, SFC, SFC>::GenericTests;
	};

	static GenericMultiProcessorWrapperTests<ProcessorSelector> processorSelector(juce::String("ProcessorSelector"), {"G1", "G1"},{"G2", "G2"}, {"G3", "G3"});
	static GenericMultiProcessorWrapperTests<ProcessorSequence> processorSequence(juce::String("ProcessorSequence"), {"G1", "G1"},{"G2", "G2"}, {"G3", "G3"});
} // namespace sjf::tests
