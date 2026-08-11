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
	class AutoEditor : public juce::Component
	{
		public:
			static constexpr auto ComponentHeight = 30; // not sure about this yet...
			static constexpr auto VerticalSpacing = 5; // not sure about this yet... space above + below
			static constexpr auto HorizontalSpacing = 5;
			static constexpr auto PresetPanelHeight = 30;

			AutoEditor(juce::AudioProcessorValueTreeState& apvts_,
					   const juce::AudioProcessorParameterGroup& group_,
					   const helpers::ParameterFactory::GroupMetadata& metadata_)
			: apvts(apvts_)
			, parameterGroup(group_)
			, metadata(metadata_)
			{

				collapseButton.onClick = [this](){
					setExpanded(!isExpanded());
				};


				buildUIFromGroup();
			}

			void resized() override
			{
				collapseButton.setBounds(HorizontalSpacing, VerticalSpacing, ComponentHeight, ComponentHeight);
				const auto titleX = collapseButton.getRight() + HorizontalSpacing;
				titleLabel.setBounds(titleX, VerticalSpacing, getWidth()-HorizontalSpacing - titleX, ComponentHeight);
				auto y = titleLabel.getBottom() + VerticalSpacing;
				if (presetPanel)
				{
					presetPanel->setBounds(HorizontalSpacing, y, getWidth() - 2*HorizontalSpacing, PresetPanelHeight);
					y = presetPanel->getBottom() + VerticalSpacing;
				}

				auto b = juce::Rectangle<int>(HorizontalSpacing, y, getWidth()-HorizontalSpacing*2, ComponentHeight);
				if (expanded)
				{
					jassert(paramComponents.size() == paramNames.size());
					for (auto i = 0ul; i < paramComponents.size(); ++i)
					{
						auto b_ = b;
						const auto c = paramComponents[i];
						const auto& l = paramNames[i];
						l->setBounds(b_.removeFromLeft(jmax(ComponentHeight*2, getWidth()/4)));
						c->setBounds(b_.withWidth(b_.getWidth() - HorizontalSpacing));
						b = b.withY(b.getY() + ComponentHeight + VerticalSpacing);
					}

					y = b.getY();
					for (auto& c : childEditors)
					{
						const auto narrow = [&](){
							auto vp =dynamic_cast<Viewport*>(getParentComponent());
							vp = vp ? vp : getParentComponent()->getParentComponent() ? dynamic_cast<Viewport*>(getParentComponent()->getParentComponent()) : nullptr;
							return vp != nullptr;
						}();

						const auto childWidth = getWidth() - ((narrow ? 3 : 2 ) * HorizontalSpacing);
						c->setBounds(HorizontalSpacing, y, childWidth, c->getRequiredSize().getHeight());
						y = c->getBottom() + VerticalSpacing;
					}
				}
			}

			// void paint(juce::Graphics& g) override
			// {
			   //   // not sure what we do here yet
			// }

			virtual juce::Rectangle<int> getRequiredSize() const
			{
				// will need additional logic if changing from single column...
				auto heightOfChildren = [&](){
					auto sum = 0;
					for (const auto& c : childEditors)
						sum += c->getRequiredSize().getHeight() + VerticalSpacing;
					return sum;
				};


				auto w = getWidth();
				auto h = (!expanded ? 0 : static_cast<int>(sliders.size() + comboBoxes.size() + buttons.size()) * (ComponentHeight + VerticalSpacing))
									+ VerticalSpacing // extra spacing at bottom
									+ (presetPanel ? PresetPanelHeight + VerticalSpacing : 0)
									+ juce::jmax(titleLabel.getHeight(), collapseButton.getHeight()) + VerticalSpacing
									+ (expanded ? heightOfChildren() : 0);
				return {w, h};
			}


			virtual void onLayoutChanged()
			{
				if ( const auto parent = findParentComponentOfClass<AutoEditor>())
				{
					parent->onLayoutChanged();
				}
				else
				{
					const auto viewport = findParentComponentOfClass<juce::Viewport>();
					auto pos = viewport ? viewport->getViewPosition() : juce::Point<int>(0, 0);
					setBounds(getRequiredSize().withX(0).withY(0));
					if (viewport)
						viewport->setViewPosition(pos); // force viewport not to jump all over the place
				}
			}


			virtual void setExpanded(const bool shouldBeExpanded)
			{
				expanded = shouldBeExpanded;
				for (const auto c : paramComponents)
					c->setVisible(expanded);

				for (const auto& l : paramNames)
					l->setVisible(expanded);

				for (const auto& c : childEditors)
					c->setVisible(expanded);

				onLayoutChanged();
			}

			[[nodiscard]] bool isExpanded() const noexcept { return expanded; }


			void buildChildEditors();
		protected:

			juce::AudioProcessorValueTreeState& apvts;
			const juce::AudioProcessorParameterGroup& parameterGroup;
			const helpers::ParameterFactory::GroupMetadata metadata;

			bool expanded { true };


			juce::TextButton collapseButton { "^" };
			juce::Label titleLabel;


			std::vector<std::unique_ptr<juce::Slider>> sliders;
			std::vector<std::unique_ptr<juce::Button>> buttons;
			std::vector<std::unique_ptr<juce::ComboBox>> comboBoxes;

			using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
			using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
			using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
			std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
			std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
			std::vector<std::unique_ptr<ComboBoxAttachment>> comboBoxAttachments;

			std::vector<std::unique_ptr<AutoEditor>> childEditors;

			std::unique_ptr<sjf::gui::PresetPanel> presetPanel;

			std::vector<Component*> paramComponents;
			std::unordered_map<const juce::AudioProcessorParameter*, Component*> paramMap;
			std::vector<std::unique_ptr<juce::Label>> paramNames;

		private:
			void buildUIFromGroup()
			{
				auto paramName = [&](juce::AudioProcessorParameter* parameter_){
					if (const auto ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter_))
						return helpers::ParameterFactory::getNameWithoutParentPrefix(*ranged, parameterGroup);

					jassertfalse;
					return juce::String{};
				};

				auto paramId = [&](juce::AudioProcessorParameter* parameter_){
					if (const auto ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter_))
						return ranged->getParameterID();

					jassertfalse;
					return juce::String{};
				};

				addAndMakeVisible(collapseButton);

				addAndMakeVisible(titleLabel);
				titleLabel.setText(helpers::ParameterFactory::getNameWithoutParentPrefix(parameterGroup), juce::sendNotification);


				if (metadata.supportsSubPresets)
				{
					presetPanel = std::make_unique<sjf::gui::PresetPanel>(parameterGroup);
					addAndMakeVisible(*presetPanel);
				}

				for (const auto param : parameterGroup.getParameters(false))
				{
					if (auto choice = dynamic_cast<juce::AudioParameterChoice*>(param))
					{
						comboBoxes.push_back(std::make_unique<juce::ComboBox>(paramName(param)));
						comboBoxAttachments.push_back(std::make_unique<ComboBoxAttachment>(apvts, paramId(param), *comboBoxes.back()));
						comboBoxes.back()->addItemList(choice->choices, 1);
						comboBoxes.back()->setText(choice->getCurrentValueAsText());

						paramComponents.push_back(comboBoxes.back().get());
					}
					else if (dynamic_cast<juce::AudioParameterBool*>(param))
					{
						buttons.push_back(std::make_unique<juce::ToggleButton>(paramName(param)));
						buttonAttachments.push_back(std::make_unique<ButtonAttachment>(apvts, paramId(param), *buttons.back()));
						paramComponents.push_back(buttons.back().get());
					}
					else
					{
						jassert(dynamic_cast<juce::AudioParameterInt*>(param) || dynamic_cast<juce::AudioParameterFloat*>(param));
						sliders.push_back(std::make_unique<juce::Slider>(paramName(param)));
						const auto& s = sliders.back();
						sliderAttachments.push_back(std::make_unique<SliderAttachment>(apvts, paramId(param), *s));
						s->setTextValueSuffix(" " + dynamic_cast<juce::RangedAudioParameter*>(param)->getLabel());
						s->setTextBoxStyle(juce::Slider::TextBoxRight, false, s->getTextBoxWidth(), s->getTextBoxHeight());
						paramComponents.push_back(s.get());
					}
					paramNames.push_back(std::make_unique<juce::Label>(paramName(param)+"Label"));
					paramNames.back()->setText(paramName(param), juce::sendNotification);
					paramMap[param] = paramComponents.back();
				}

				for (const auto c : paramComponents)
					addAndMakeVisible(*c);

				for (const auto& l : paramNames)
					addAndMakeVisible(*l);

				for (const auto& c : childEditors)
					addAndMakeVisible(*c);
			}


			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoEditor)
	};

	class GenericEditor : public juce::AudioProcessorEditor
	{
		public:
			const juce::AudioProcessorParameterGroup* getTopLevelParameterGroup(const juce::AudioProcessorParameterGroup& tree, const helpers::ParameterFactory::GroupMetadata& metadata_)
			{
				auto id = metadata_.groupID;
				if (tree.getID() == id)
					return &tree;
				for (const auto child : tree.getSubgroups(false))
				{
					auto t = getTopLevelParameterGroup(*child, metadata_);
					if (t)
						return t;
				}
				jassertfalse;
				return nullptr;
			}

			explicit GenericEditor(juce::AudioProcessorValueTreeState& apvts_, juce::AudioProcessor& processor_, const helpers::ParameterFactory::GroupMetadata& metadata_)
			: AudioProcessorEditor(processor_)
			, presets(processor.getParameterTree())
			{
				addAndMakeVisible(presets);

				initialiseMainEditor(apvts_, *getTopLevelParameterGroup(processor.getParameterTree(), metadata_), metadata_);
				jassert(mainEditor != nullptr);

				// 2. Configure Viewport
				viewport.setViewedComponent (mainEditor.get(), false);
				viewport.setScrollBarsShown (true, false, true, false);
				addAndMakeVisible (viewport);

				setResizable (true, false);
				mainEditor->buildChildEditors();
				setSize (600, 600);

			}

			void resized() override
			{
				auto bounds = getLocalBounds();



				presets.setBounds (bounds.removeFromTop (30));
				const auto pos = viewport.getViewPosition();
				viewport.setBounds (bounds);

				mainEditor->setBounds (0, 0, getWidth(), mainEditor->getRequiredSize().getHeight());

				viewport.setViewPosition(pos);
			}

		private:
			void initialiseMainEditor(juce::AudioProcessorValueTreeState& apvts,
											const juce::AudioProcessorParameterGroup& parameterGroup,
											const helpers::ParameterFactory::GroupMetadata& metadata);

			std::unique_ptr<AutoEditor> mainEditor;
			juce::Viewport viewport;
			sjf::gui::PresetPanel presets;
	};
}


//DUMMY_PLUGIN_SJF_GENERICEDITOR_H
