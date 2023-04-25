/*
 ==============================================================================
 
 TwoValueSliderAttachment.h
 Created: 11 Dec 2017 7:35:47pm
 Author:  Nikolay Tsenkov
 
 ==============================================================================
 */


#pragma once


#include "JuceHeader.h"


struct MultiValueAttachedControlBase :    public juce::AudioProcessorValueTreeState::Listener,
public juce::AsyncUpdater
{
    MultiValueAttachedControlBase (juce::AudioProcessorValueTreeState& s, juce::Array<juce::String> p)
    :
    state (s),
    paramIDs (p)
    {
        for (int i = 0; i < paramIDs.size (); ++i)
        {
            state.addParameterListener (paramIDs [i], this);
            lastValues.set (paramIDs [i], 0);
        }
    }
    
    void removeListener ()
    {
        for (int i = 0; i < paramIDs.size (); ++i)
        {
            state.removeParameterListener (paramIDs [i], this);
        }
    }
    
    void setNewUnnormalisedValue (const juce::String& paramID, float newUnnormalisedValue)
    {
        if (juce::AudioProcessorParameter* p = state.getParameter (paramID))
        {
            const float newValue = state
            .getParameterRange (paramID)
            .convertTo0to1 (newUnnormalisedValue);
            
            if (p->getValue() != newValue)
            {
                p->setValueNotifyingHost (newValue);
            }
        }
    }
    
    void sendInitialUpdate ()
    {
        for (int i = 0; i < paramIDs.size (); ++i)
        {
            if (std::atomic<float>* v = state.getRawParameterValue (paramIDs [i]))
            {
                parameterChanged (paramIDs [i], *v);
            }
        }
    }
    
    void parameterChanged (const juce::String& paramID, float newValue) override
    {
        lastValues.set (paramID, newValue);
        
        if (juce::MessageManager::getInstance ()->isThisTheMessageThread())
        {
            cancelPendingUpdate ();
            setValue (paramID, newValue);
        }
        else
        {
            triggerAsyncUpdate ();
        }
    }
    
    void beginParameterChange (const juce::String& paramID)
    {
        if (juce::AudioProcessorParameter* p = state.getParameter (paramID))
        {
            p->beginChangeGesture();
        }
    }
    
    void endParameterChange (const juce::String& paramID)
    {
        if (juce::AudioProcessorParameter* p = state.getParameter (paramID))
        {
            p->endChangeGesture();
        }
    }
    
    void handleAsyncUpdate () override
    {
        for (int i = 0; i < paramIDs.size (); ++i)
        {
            const juce::String paramID = paramIDs [i];
            setValue (paramID, lastValues [paramID]);
        }
    }
    
    virtual void setValue (const juce::String&, float) = 0;
    
    juce::AudioProcessorValueTreeState& state;
    juce::Array<juce::String> paramIDs;
    juce::NamedValueSet lastValues;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiValueAttachedControlBase)
};


//==============================================================================


class TwoValueSliderAttachment
{
public:
    TwoValueSliderAttachment (
                              juce::AudioProcessorValueTreeState& stateToControl,
                              const juce::String& parameterID1,
                              const juce::String& parameterID2,
                              juce::Slider& sliderToControl
                              )
    :
    pimpl (new Pimpl (
    stateToControl,
                      parameterID1,
                      parameterID2,
                      sliderToControl
                      ))
    {
    }
    
    ~TwoValueSliderAttachment ()
    {
    }
    
private:
    struct Pimpl :    private MultiValueAttachedControlBase,
    private juce::Slider::Listener
    {
        Pimpl (
               juce::AudioProcessorValueTreeState& s,
               const juce::String& p1,
               const juce::String& p2,
               juce::Slider& sl
               )
        :
        MultiValueAttachedControlBase (s, {p1, p2}),
        minParamID (p1),
        maxParamID (p2),
        slider (sl),
        ignoreCallbacks (false)
        {
            jassert (
                     sl.getSliderStyle () == juce::Slider::TwoValueHorizontal ||
                     sl.getSliderStyle () == juce::Slider::TwoValueVertical
                     );
            
            juce::NormalisableRange<float> range (
                                                  s.getParameterRange (minParamID).start,
                                                  
                                                  s.getParameterRange (maxParamID).end,
                                                  s.getParameterRange (maxParamID).interval,
                                                  s.getParameterRange (maxParamID).skew
                                                  );
            
            slider.setRange (range.start, range.end, range.interval);
            slider.setSkewFactor (range.skew, range.symmetricSkew);
            
            sendInitialUpdate();
            slider.addListener (this);
        }
        
        ~Pimpl()
        {
            slider.removeListener (this);
            removeListener();
        }
        
        void setValue (const juce::String& paramID, float newValue) override
        {
            const juce::ScopedLock selfCallbackLock (selfCallbackMutex);
            
            {
                juce::ScopedValueSetter<bool> svs (ignoreCallbacks, true);
                
                bool allowNudgingOfOtherValues = true;
                if (paramID == minParamID)
                {
                    slider.setMinValue (
                                        newValue,
                                        juce::sendNotificationSync,
                                        allowNudgingOfOtherValues
                                        );
                }
                else if (paramID == maxParamID)
                {
                    slider.setMaxValue (
                                        newValue,
                                        juce::sendNotificationSync,
                                        allowNudgingOfOtherValues
                                        );
                }
                else
                {
                    jassertfalse;
                }
            }
        }
        
        void sliderValueChanged (juce::Slider* s) override
        {
            const juce::ScopedLock selfCallbackLock (selfCallbackMutex);
            
            if (
                (! ignoreCallbacks) &&
                (! juce::ModifierKeys::getCurrentModifiers ().isRightButtonDown ())
                )
            {
                setNewUnnormalisedValue (
                                         minParamID,
                                         (float) s->getMinValue()
                                         );
                
                setNewUnnormalisedValue (
                                         maxParamID,
                                         (float) s->getMaxValue()
                                         );
            }
        }
        
        void sliderDragStarted (juce::Slider*) override
        {
            beginParameterChange (minParamID);
            beginParameterChange (maxParamID);
        }
        
        void sliderDragEnded   (juce::Slider*) override
        {
            endParameterChange (minParamID);
            endParameterChange (maxParamID);
        }
        
        
        juce::String minParamID,
        maxParamID;
        juce::Slider& slider;
        bool ignoreCallbacks;
        juce::CriticalSection selfCallbackMutex;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pimpl)
    };
    friend struct juce::ContainerDeletePolicy<Pimpl>;
    juce::ScopedPointer<Pimpl> pimpl;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TwoValueSliderAttachment)
};




