/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 29/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/processors/Reverbs/sjf_KeithBarrReverb.h>
#include <sjf/processors/Reverbs/sjf_MultitapDiffuser.h>
#include <sjf/processors/sjf_Filter_juce.h>
#include <sjf/helpers/sjf_ProcessorSequence.h>
#include <sjf/helpers/sjf_BypassWrapper.h>

#include <sjf/helpers/sjf_ProcessorSelector.h>
#include <sjf/processors/Reverbs/sjf_RotateDelayDiffuser.h>

#include "sjf/processors/sjf_Delay.h"

namespace sjf::dsp
{
	struct Reverb
	{
	public:

		void prepare(const juce::dsp::ProcessSpec& spec_)
		{
			preDelay.prepare(spec_);
			filter.prepare(spec_);
			inputDiffuser.prepare(spec_);
			tank.prepare(spec_);
			reset();
		}

		void reset()
		{
			preDelay.reset();
			filter.reset();
			inputDiffuser.reset();
			tank.reset();
		}

		template<typename ProcessContext>
		void process(const ProcessContext& context) noexcept
		{
			preDelay.process(context);
			filter.process(context);
			inputDiffuser.process(context);
			tank.process(context);
		}

		std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName)
		{
			auto mainFactory = helpers::ParameterFactory::create (factoryID, factoryName);

			mainFactory->addChildFactory(preDelay.createParameters(factoryID + "PreDelay", factoryName + " Pre Delay"));
			mainFactory->addChildFactory(filter.createParameters(factoryID + "Filter", factoryName + " Filter"));
			mainFactory->addChildFactory(inputDiffuser.createParameters(factoryID + "Diffuser", factoryName + " Diffuser",
																		helpers::processor_sequence::SubFactoryConfig{"MT", "MT"},
																		helpers::processor_sequence::SubFactoryConfig{"RD", "RD"}
																		));
			mainFactory->addChildFactory(tank.createParameters(factoryID + "Tank", factoryName + " Reverb Tank"));



			return mainFactory;
		}


	private:
		/**
		 * @brief Internal DSP processing sequence: Filter >> Input Diffuser >> Reverb Tank.
		 */
		SimpleDelay<0, 100, 0, 50> preDelay;
		SVF<true, true> filter;
		sjf::helpers::ProcessorSelector<MultiTapDiffuser<>, RotateDelayDiffuser<>> inputDiffuser;
		helpers::BypassWrapper<keith_barr::reverb::Tank<>, helpers::bypass_wrapper_config::Mix> tank;
	};
}



//DUMMY_PLUGIN_SJF_REVERBPLAYGROUND_H
