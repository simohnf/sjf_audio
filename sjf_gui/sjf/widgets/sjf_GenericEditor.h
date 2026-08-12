/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */
//
// Created by Simon Fay on 21/07/2026.
//

#pragma once
#include <JuceHeader.h>
#include <sjf/widgets/sjf_PresetPanel.h>

namespace sjf::generic_editor
{
	namespace colours
	{
		static const juce::Colour borderColour = juce::Colour::greyLevel(0.5f);
		static const juce::Colour insertionIndexColour = juce::Colours::cyan.withAlpha(0.5f);
	}



	class GenericEditor : public juce::AudioProcessorEditor
	{
		public:

			explicit GenericEditor(juce::AudioProcessorValueTreeState& apvts_, juce::AudioProcessor& processor_, const helpers::ParameterFactory::GroupMetadata& metadata_);

			void resized() override;

		private:
			void initialiseMainEditor(juce::AudioProcessorValueTreeState& apvts,
											const juce::AudioProcessorParameterGroup& parameterGroup,
											const helpers::ParameterFactory::GroupMetadata& metadata);

			std::unique_ptr<Component> mainEditor;
			juce::Viewport viewport;
			sjf::gui::PresetPanel presets;
	};
}


//DUMMY_PLUGIN_SJF_GENERICEDITOR_H
