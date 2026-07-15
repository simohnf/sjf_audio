//
// Created by Simon Fay on 08/07/2026.
//
#pragma once
#include <JuceHeader.h>

namespace sjf::helpers
{
class ParameterFactory : public juce::AudioProcessorParameterGroup
{
private:
    struct ConstructorToken { explicit ConstructorToken (int) {} };

public:
    static std::unique_ptr<ParameterFactory> create (const juce::String& factoryID, const juce::String& factoryName)
    {
        return std::make_unique<ParameterFactory> (ConstructorToken{0}, factoryID, factoryName);
    }

    ParameterFactory (ConstructorToken, const juce::String& factoryID, const juce::String& factoryName)
        : juce::AudioProcessorParameterGroup (factoryID, factoryName, " "),
          baseID (factoryID), baseName (factoryName)
    {}

    //==============================================================================
    juce::AudioParameterFloat* createFloatParameter (const juce::ParameterID& parameterID,
                                                     const juce::String& parameterName,
                                                     juce::NormalisableRange<float> normalisableRange,
                                                     float defaultValue,
                                                     const juce::AudioParameterFloatAttributes& attributes = {})
    {
        auto p = std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (baseID + parameterID.getParamID(), parameterID.getVersionHint()),
            baseName + ": " + parameterName,
            normalisableRange,
            defaultValue,
            attributes
        );

        auto* rawPtr = p.get();
        addChild (std::move (p));
        return rawPtr;
    }

    juce::AudioParameterInt* createIntParameter (const juce::ParameterID& parameterID,
                                                 const juce::String& parameterName,
                                                 int minValue, int maxValue, int defaultValue,
                                                 const juce::AudioParameterIntAttributes& attributes = {})
    {
        auto p = std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID (baseID + parameterID.getParamID(), parameterID.getVersionHint()),
            baseName + ": " + parameterName,
            minValue, maxValue, defaultValue,
            attributes
        );

        auto* rawPtr = p.get();
        addChild (std::move (p));
        return rawPtr;
    }

    juce::AudioParameterBool* createBoolParameter (const juce::ParameterID& parameterID,
                                                   const juce::String& parameterName,
                                                   bool defaultValue,
                                                   const juce::AudioParameterBoolAttributes& attributes = {})
    {
        auto p = std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID (baseID + parameterID.getParamID(), parameterID.getVersionHint()),
            baseName + ": " + parameterName,
            defaultValue,
            attributes
        );

        auto* rawPtr = p.get();
        addChild (std::move (p));
        return rawPtr;
    }

    juce::AudioParameterChoice* createChoiceParameter (const juce::ParameterID& parameterID,
                                                       const juce::String& parameterName,
                                                       const juce::StringArray& choices,
                                                       int defaultChoiceIndex,
                                                       const juce::AudioParameterChoiceAttributes& attributes = {})
    {
        auto p = std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID (baseID + parameterID.getParamID(), parameterID.getVersionHint()),
            baseName + ": " + parameterName,
            choices,
            defaultChoiceIndex,
            attributes
        );

        auto* rawPtr = p.get();
        addChild (std::move (p));
        return rawPtr;
    }

    void addChildFactory (std::unique_ptr<ParameterFactory> child)
    {
        if (child != nullptr)
            addChild (std::move (child));
    }

private:
    const juce::String baseID, baseName;
};

//===========//===========//===========//===========//===========//===========
class AudioParametersBase
{
public:
    virtual ~AudioParametersBase() = default;
    virtual std::unique_ptr<ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) = 0;

    //==============================================================================
    template <typename JuceParamType>
    struct TrackedState
    {
        using ValueType = std::conditional_t<std::is_same_v<JuceParamType, juce::AudioParameterFloat>, float,
                          std::conditional_t<std::is_same_v<JuceParamType, juce::AudioParameterBool>, bool,
                          int>>;


        ValueType currentValue {};

    private:
        JuceParamType* juceParameter = nullptr;

        ValueType startValue {};
        ValueType targetValue {};

    public:
        inline ValueType getParameterValue() const noexcept {
            jassert (juceParameter != nullptr);
            if constexpr (std::is_same_v<JuceParamType, juce::AudioParameterChoice>)
                return juceParameter->getIndex();
            else
                return juceParameter->get();
        }

        inline void reset( ValueType mappedTargetValue ) noexcept {
            startValue = currentValue = targetValue = mappedTargetValue;
        }
    private:
        inline void latchTarget( ValueType mappedTargetValue ) noexcept {
            jassert (juceParameter != nullptr);
            startValue = currentValue;
            targetValue = mappedTargetValue;
        }

        friend class AudioParametersBase;
    };

    using FloatState  = TrackedState<juce::AudioParameterFloat>;
    using IntState    = TrackedState<juce::AudioParameterInt>;
    using BoolState   = TrackedState<juce::AudioParameterBool>;
    using ChoiceState = TrackedState<juce::AudioParameterChoice>;

    using FloatMapping  = std::function<float(float)>;
    using IntMapping    = std::function<int (int)>;
    using ChoiceMapping = IntMapping;
    using BoolMapping   = std::function<bool (bool)>;

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec_) {
        spec = spec_;
        masterRamp.reset (spec.sampleRate, 0.020);
        reset();
    }

    void reset() {
        resetAllStates (floatStates, floatMappings);
        resetAllStates (intStates, intMappings);
        resetAllStates (boolStates, boolMappings);
        resetAllStates (choiceStates, choiceMappings);
        masterRamp.setCurrentAndTargetValue (1.0f);
    }

    void setSmootherLength(const double rampLengthInMS)
    {
        masterRamp.reset(spec.sampleRate, rampLengthInMS * 0.001);
    }

    bool checkForStateChange() {
        if (masterRamp.isSmoothing()) return true;

        const bool parametersHaveChanged = anyStatesDiverged (floatStates, floatMappings) || anyStatesDiverged (intStates, intMappings)
                                        || anyStatesDiverged (boolStates, boolMappings)  || anyStatesDiverged (choiceStates, choiceMappings);

        if (parametersHaveChanged) {
            latchAllStates (floatStates, floatMappings);
            latchAllStates (intStates, intMappings);
            latchAllStates (boolStates, boolMappings);
            latchAllStates (choiceStates, choiceMappings);

            masterRamp.setCurrentAndTargetValue (0.0f);
            masterRamp.setTargetValue (1.0f);
            return true;
        }
        return false;
    }

    inline void tickSmoothers() noexcept {
        if (masterRamp.isSmoothing()) {
            const float alpha = masterRamp.getNextValue();
            for (const auto state : floatStates)
                state->currentValue = state->startValue + alpha * (state->targetValue - state->startValue);
            for (const auto state : intStates)
                state->currentValue = state->targetValue;
            for (const auto state : boolStates)
                state->currentValue = state->targetValue;
            for (const auto state : choiceStates)
                state->currentValue = state->targetValue;

            if (!masterRamp.isSmoothing())
                checkForStateChange();
        }
    }

    bool isSmoothing() const noexcept { return masterRamp.isSmoothing(); }

private:

    template<typename TrackedStateType, typename TrackedStateMappingType>
    bool anyStatesDiverged( const std::vector<TrackedStateType*>& states, const std::vector<TrackedStateMappingType>& mappings ) noexcept
    {
        jassert(states.size() == mappings.size());
        for ( auto i = 0ul; i < states.size(); ++i)
        {
            const auto mappedValue = mappings[i](states[i]->getParameterValue());
            if constexpr (std::is_same_v<TrackedStateType, FloatState>)
            {
                if (!(juce::approximatelyEqual(states[i]->targetValue, mappedValue)))
                    return true;
            }
            else
            {
                if (states[i]->targetValue != mappedValue)
                    return true;
            }
        }
        return false;
    }

    template<typename TrackedStateType, typename TrackedStateMappingType>
    void latchAllStates( const std::vector<TrackedStateType*>& states, const std::vector<TrackedStateMappingType>& mappings ) noexcept
    {
        jassert(states.size() == mappings.size());
        for ( auto i = 0ul; i < states.size(); ++i)
            states[i]->latchTarget(mappings[i](states[i]->getParameterValue()));
    }

    template<typename TrackedStateType, typename TrackedStateMappingType>
    void resetAllStates( const std::vector<TrackedStateType*>& states, const std::vector<TrackedStateMappingType>& mappings ) noexcept
    {
        jassert(states.size() == mappings.size());
        for ( auto i = 0ul; i < states.size(); ++i)
            states[i]->reset(mappings[i](states[i]->getParameterValue()));
    }

    template <typename StateType, typename ParamType, typename MapType>
    void initState (StateType& state, ParamType* param, const MapType& mapping) {
        jassert (param != nullptr);

        state.juceParameter = param;

        state.targetValue = mapping ? mapping (state.getParameterValue()) : state.getParameterValue();
        state.currentValue = state.targetValue;
        state.startValue = state.targetValue;
    }

protected:

    //==============================================================================
    void createTrackedParameter (   ParameterFactory& factory,
                                    FloatState& tracker,
                                    const juce::ParameterID& parameterID,
                                    const juce::String& parameterName,
                                    juce::NormalisableRange<float> normalisableRange,
                                    float defaultValue,
                                    const FloatMapping& mapping = {},
                                    const juce::AudioParameterFloatAttributes& attributes = {})
    {
        auto* param = factory.createFloatParameter (parameterID, parameterName, normalisableRange, defaultValue, attributes);
        initState(tracker, param, mapping);
        floatStates.push_back (&tracker);
        floatMappings.push_back (mapping ? mapping : [](const float x) { return x; });
    }

    void createTrackedParameter (   ParameterFactory& factory,
                                    IntState& tracker,
                                    const juce::ParameterID& parameterID,
                                    const juce::String& parameterName,
                                    int minValue, int maxValue, int defaultValue,
                                    const IntMapping& mapping = {},
                                    const juce::AudioParameterIntAttributes& attributes = {})
    {
        auto* param = factory.createIntParameter (parameterID, parameterName, minValue, maxValue, defaultValue, attributes);
        initState(tracker, param, mapping);

        intStates.push_back (&tracker);
        intMappings.push_back (mapping ?  mapping : [](const int x) { return x; });
    }

    void createTrackedParameter (   ParameterFactory& factory,
                                    BoolState& tracker,
                                    const juce::ParameterID& parameterID,
                                    const juce::String& parameterName,
                                    bool defaultValue,
                                    const BoolMapping& mapping = {},
                                    const juce::AudioParameterBoolAttributes& attributes = {})
    {
        auto* param = factory.createBoolParameter (parameterID, parameterName, defaultValue, attributes);
        initState(tracker, param, mapping);

        boolStates.push_back (&tracker);
        boolMappings.push_back (mapping ? mapping : [](const bool x) { return x; });
    }

    void createTrackedParameter (   ParameterFactory& factory,
                                    ChoiceState& tracker,
                                    const juce::ParameterID& parameterID,
                                    const juce::String& parameterName,
                                    const juce::StringArray& choices,
                                    int defaultItemIndex,
                                    const ChoiceMapping& mapping = {},
                                    const juce::AudioParameterChoiceAttributes& attributes = {})
    {
        auto* param = factory.createChoiceParameter (parameterID, parameterName, choices, defaultItemIndex, attributes);
        initState(tracker, param, mapping);

        choiceStates.push_back (&tracker);
        choiceMappings.push_back (mapping ? mapping : [](const int x) { return x; });
    }

    juce::dsp::ProcessSpec spec{44100, 32, 2};

private:
    std::vector<FloatState*>  floatStates;
    std::vector<IntState*>    intStates;
    std::vector<BoolState*>   boolStates;
    std::vector<ChoiceState*> choiceStates;

    std::vector<FloatMapping> floatMappings;
    std::vector<IntMapping> intMappings;
    std::vector<BoolMapping> boolMappings;
    std::vector<ChoiceMapping> choiceMappings;

    juce::LinearSmoothedValue<float> masterRamp;

};
}
