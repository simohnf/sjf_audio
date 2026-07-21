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

namespace sjf::generic_editor
{
     class GenericEditor : public juce::AudioProcessorEditor
     {
     public:
         explicit GenericEditor(AudioProcessor& processor_)
        : AudioProcessorEditor(processor_)
        , mainEditor(processor_)
        {
            addAndMakeVisible(mainEditor);
             addAndMakeVisible(presets);

             setResizable (true, false);
             setSize (600, 600);

        }

         void resized() override
         {
             auto lb = getLocalBounds();
             auto comboBoxBounds = lb.removeFromTop(30);
             presets.setBounds(comboBoxBounds);
             mainEditor.setBounds (lb);
         }

     private:
         void timerCallback()
         {
             auto menu = presets.getRootMenu();
             *menu = sjf::helpers::PresetManager::getPresetPopupMenu(processor.getParameterTree());
         }
         juce::GenericAudioProcessorEditor mainEditor;
         juce::ComboBox presets;
         VBlankAttachment vBlankAttachment{this, [&](){timerCallback();}};
     };
}


//DUMMY_PLUGIN_SJF_GENERICEDITOR_H
