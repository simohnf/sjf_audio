/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 22/07/2026.
//
#include <sjf/processors/sjf_Delay.h>
#include <sjf/processors/Waveshaper/sjf_Waveshaper.h>
#include "sjf_UnitTester/sjf_GenericTests.h"
#include <sjf/processors/Reverbs/sjf_KeithBarrReverb.h>
#include <sjf/processors/Reverbs/sjf_MultitapDiffuser.h>
#include <sjf/processors/Reverbs/sjf_RotateDelayDiffuser.h>
#include <sjf/processors/sjf_Filter_juce.h>
#include <sjf/processors/sjf_Compressor_juce.h>
#include <sjf/processors/sjf_Limiter_juce.h>

namespace
{
	using LFO = sjf::dsp::oscillators::lfo::LFO<sjf::dsp::oscillators::lfo::DefaultWaveformProvider,
												sjf::dsp::oscillators::lfo::lfo_config::TempoSync,
												sjf::dsp::oscillators::lfo::lfo_config::Invert,
												sjf::dsp::oscillators::lfo::lfo_config::PhaseOffset,
												sjf::dsp::oscillators::lfo::lfo_config::Smooth,
												sjf::dsp::oscillators::lfo::lfo_config::Depth
											   >;

	using Saturator = sjf::dsp::waveshaper::WaveshaperTypeProvider  <   sjf::dsp::waveshaper::SoftClip,
																		sjf::dsp::waveshaper::HardClip,
																		sjf::dsp::waveshaper::Overdrive,
																		sjf::dsp::waveshaper::BucketBrigade,
																		sjf::dsp::waveshaper::Tape
																	>;

	using Delay = sjf::dsp::Delay<  LFO,
									Saturator,
									// sjf::dsp::delay_config::Mono,
									sjf::dsp::delay_config::Feedback,
									sjf::dsp::delay_config::Offset,
									sjf::dsp::delay_config::TempoSync,
									sjf::dsp::delay_config::Filter,
									sjf::dsp::delay_config::Detune,
									sjf::dsp::delay_config::PingPong,
									sjf::dsp::delay_config::Link
								>;

}

namespace sjf::tests
{


	static GenericTests<Delay> delayTestMaximal("Maximal Delay");
	static GenericTests<sjf::dsp::Delay<>> delayTestMinimal("Minimal Delay");
	static GenericTests<sjf::dsp::modulation_effects::Chorus> chorus("Chorus");
	static GenericTests<sjf::dsp::modulation_effects::Flanger> flanger("Flanger");

	static GenericTests<dsp::SimpleDelay<0, 100, 0, 50>> delay1("SimpleDelay");

	static GenericTests<sjf::dsp::SVF<>> filter{"SVF Filter"};
	static GenericTests<sjf::dsp::Compressor> compressor{"Compressor"};
	static GenericTests<sjf::dsp::Limiter> limiter{"Limiter"};

	static GenericTests<dsp::waveshaper::FilteredWaveshaper<Saturator>> filteredSaturator("Filtered Saturator");
	static GenericTests<dsp::waveshaper::Waveshaper<Saturator>> saturator("Saturator");

	static GenericTests<dsp::keith_barr::reverb::Tank<>> rev1 ("KeithBarrReverb");
	static GenericTests<dsp::MultiTapDiffuser<>> diff1("MTDiffuser");
	static GenericTests<dsp::RotateDelayDiffuser<>> diff2("RotateDelayDiffuser");




}



//DUMMY_PLUGIN_TESTS_H
