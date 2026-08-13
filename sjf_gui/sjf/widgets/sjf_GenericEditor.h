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
	/**
	 * @brief A generic, responsive audio processor editor that automatically generates complex
	 *        hierarchical user interfaces from parameter metadata.
	 *
	 * This editor acts as a high-level container that inspects parameter trees built via
	 * `ParameterFactory` and constructs an auto-layout UI driven by `GroupMetadata`. It wraps the
	 * internally constructed interface in a flexible `juce::Viewport` to handle arbitrary layout
	 * scaling, dynamic collapsible sub-panels, and reorderable sequence views safely.
	 *
	 * It automatically incorporates integrated top-level controls (such as an embedded `PresetPanel`)
	 * and delegates detailed parameter control generation, sub-group layout, and dynamic viewport
	 * sizing to internal view containers (`AutoEditor`).
	 *
	 * @see ParameterFactory::GroupMetadata, juce::AudioProcessorValueTreeState
	 */
	class GenericEditor : public juce::AudioProcessorEditor
	{
		public:

			explicit GenericEditor(juce::AudioProcessorValueTreeState& apvts_, juce::AudioProcessor& processor_, const helpers::ParameterFactory::GroupMetadata& metadata_);

			void resized() override;

			void paint(juce::Graphics& g) override;

		private:
			void initialiseMainEditor(juce::AudioProcessorValueTreeState& apvts,
											const juce::AudioProcessorParameterGroup& parameterGroup,
											const helpers::ParameterFactory::GroupMetadata& metadata);

			std::unique_ptr<Component> mainEditor;
			juce::Viewport viewport;
			sjf::gui::PresetPanel presets;
			juce::Label label;
	};
}


//DUMMY_PLUGIN_SJF_GENERICEDITOR_H
