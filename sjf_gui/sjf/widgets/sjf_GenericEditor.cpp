//
// Created by Simon Fay on 06/08/2026.
//

#include <JuceHeader.h>
#include "sjf_GenericEditor.h"

namespace sjf::generic_editor
{
	namespace
	{
		class DeviceSelectorEditor : public AutoEditor
		{
		public:
			DeviceSelectorEditor(juce::AudioProcessorValueTreeState& apvts_,
					   const juce::AudioProcessorParameterGroup& group_,
					   const helpers::ParameterFactory::GroupMetadata& metadata_)
			: AutoEditor(apvts_, group_, metadata_)
			{

			}

			void resized() override
			{
				AutoEditor::resized();
				for (auto i = 0ul; i < childEditors.size(); ++i)
					childEditors[i]->setBounds(childEditors[i]->getBounds().withY(childEditors[0]->getY()));


				const auto selectorComboBox = dynamic_cast<juce::ComboBox*>(paramMap[dynamic_cast<const juce::AudioProcessorParameter*>(metadata.selectorParameter)]);
				jassert(selectorComboBox);
				if (!selectorComboBox->onChange)
				{
					selectorComboBox->onChange = [&](){
						auto selected = juce::jmin(static_cast<size_t>(metadata.selectorParameter->getIndex()), childEditors.size() - 1);
						for (auto i = 0ul; i < childEditors.size(); i++)
						{
							childEditors[i]->setVisible(i == selected && expanded);
							onLayoutChanged();
						}
						childEditors[selected]->setExpanded(true);
					};
					selectorComboBox->onChange();
				}
			}

			juce::Rectangle<int> getRequiredSize() const override
			{
				// will need additional logic if changing from single column...
				auto heightOfChildren = [&](){
					const auto selected = juce::jmin(static_cast<size_t>(metadata.selectorParameter->getIndex()), childEditors.size() - 1);
					return childEditors[selected]->getRequiredSize().getHeight();
				};


				auto w = getWidth();
				auto h = (!expanded ? 0 : static_cast<int>(sliders.size() + comboBoxes.size() + buttons.size()) * (ComponentHeight + VerticalSpacing))
									+ VerticalSpacing // extra spacing at bottom
									+ (presetPanel ? PresetPanelHeight + VerticalSpacing : 0)
									+ juce::jmax(titleLabel.getHeight(), collapseButton.getHeight()) + VerticalSpacing
									+ (expanded ? heightOfChildren() : 0);
				return {w, h};
			}

			void setExpanded(const bool shouldBeExpanded) override
			{
				expanded = shouldBeExpanded;
				for (const auto c : paramComponents)
					c->setVisible(expanded);

				for (const auto& l : paramNames)
					l->setVisible(expanded);

				jassert(childEditors.size() == static_cast<size_t>(metadata.selectorParameter->choices.size()));
				auto selected = static_cast<size_t>(metadata.selectorParameter->getIndex());
				for (auto i = 0ul; i < childEditors.size(); i++)
				{
					childEditors[i]->setVisible(i == selected && expanded);
				}

				onLayoutChanged();
			}

		};
	}

	void AutoEditor::buildChildEditors()
	{
		for (auto& child : parameterGroup.getSubgroups(false))
		{
			jassert(child);
			auto childMetaData = [&](){
				for (auto& c : metadata.children)
					if (c.groupID == child->getID()) return c;

				jassertfalse;
				return helpers::ParameterFactory::GroupMetadata{};
			}();
			if (childMetaData.isSelectorGroup())
				childEditors.push_back(std::make_unique<DeviceSelectorEditor>(apvts, *child, childMetaData));
			else
				childEditors.push_back(std::make_unique<AutoEditor>(apvts, *child, childMetaData));

			addAndMakeVisible(childEditors.back().get());
			childEditors.back()->buildChildEditors();
		}
		onLayoutChanged();
	}

	void GenericEditor::initialiseMainEditor(juce::AudioProcessorValueTreeState& apvts,
											const juce::AudioProcessorParameterGroup& parameterGroup,
											const helpers::ParameterFactory::GroupMetadata& metadata)
	{
		if (metadata.isSelectorGroup())
		{
			mainEditor = std::make_unique<DeviceSelectorEditor>(apvts, parameterGroup, metadata);
		}
		else if (metadata.isDynamicProcessorSequenceGroup())
		{
			mainEditor = std::make_unique<DynamicProcessorSequenceEditor>(apvts, parameterGroup, metadata);
		}
		else
		{
			mainEditor = std::make_unique<AutoEditor>(apvts, parameterGroup, metadata);
		}
	}

}