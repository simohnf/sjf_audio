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
                state.get().currentValue = state.get().startValue + alpha * (state.get().targetValue - state.get().startValue);
            for (const auto state : intStates)
                state.get().currentValue = state.get().targetValue;
            for (const auto state : boolStates)
                state.get().currentValue = state.get().targetValue;
            for (const auto state : choiceStates)
                state.get().currentValue = state.get().targetValue;

            if (!masterRamp.isSmoothing())
                checkForStateChange();
        }
    }

    bool isSmoothing() const noexcept { return masterRamp.isSmoothing(); }


    void addChildParameters (AudioParametersBase* child)
    {
        childParameters.push_back(child);
    }

    void setMasterAudioParameters(AudioParametersBase* masterParameters)
    {

        auto setMaster = [](auto& stateVector, auto& mappingVector, const auto& masterStateVector, const auto& masterMappingVector)
        {
            jassert(stateVector.size() == masterStateVector.size());
            jassert(mappingVector.size() == masterMappingVector.size());
            jassert(stateVector.size() == mappingVector.size());
            for (auto i = 0ul; i < stateVector.size(); ++i)
            {
                const auto& fs = stateVector[i];
                const auto& mfs = masterStateVector[i];
                auto& fm = mappingVector[i];
                const auto& mfm = masterMappingVector[i];
                fs.get().juceParameter = mfs.get().juceParameter;
                fs.get().currentValue = mfs.get().currentValue;
                fs.get().startValue = mfs.get().startValue;
                fs.get().targetValue = mfs.get().targetValue;

                fm = mfm;
            }
        };

        setMaster(floatStates, floatMappings, masterParameters->floatStates, masterParameters->floatMappings);
        setMaster(intStates, intMappings, masterParameters->intStates, masterParameters->intMappings);
        setMaster(boolStates, boolMappings, masterParameters->boolStates, masterParameters->boolMappings);
        setMaster(choiceStates, choiceMappings, masterParameters->choiceStates, masterParameters->choiceMappings);

        jassert(childParameters.size() == masterParameters->childParameters.size());
        for (auto i = 0ul; i < childParameters.size(); ++i)
        {
            childParameters[i]->setMasterAudioParameters(masterParameters->childParameters[i]);
        }
    }

private:

    template<typename TrackedStateType, typename TrackedStateMappingType>
    bool anyStatesDiverged( const std::vector<std::reference_wrapper<TrackedStateType>>& states, const std::vector<TrackedStateMappingType>& mappings ) noexcept
    {
        jassert(states.size() == mappings.size());
        for ( auto i = 0ul; i < states.size(); ++i)
        {
            const auto mappedValue = mappings[i](states[i].get().getParameterValue());
            if constexpr (std::is_same_v<TrackedStateType, FloatState>)
            {
                if (!(juce::approximatelyEqual(states[i].get().targetValue, mappedValue)))
                    return true;
            }
            else
            {
                if (states[i].get().targetValue != mappedValue)
                    return true;
            }
        }
        return false;
    }

    template<typename TrackedStateType, typename TrackedStateMappingType>
    void latchAllStates( const std::vector<std::reference_wrapper<TrackedStateType>>& states, const std::vector<TrackedStateMappingType>& mappings ) noexcept
    {
        jassert(states.size() == mappings.size());
        for ( auto i = 0ul; i < states.size(); ++i)
            states[i].get().latchTarget(mappings[i](states[i].get().getParameterValue()));
    }

    template<typename TrackedStateType, typename TrackedStateMappingType>
    void resetAllStates( const std::vector<std::reference_wrapper<TrackedStateType>>& states, const std::vector<TrackedStateMappingType>& mappings ) noexcept
    {
        jassert(states.size() == mappings.size());
        for ( auto i = 0ul; i < states.size(); ++i)
            states[i].get().reset(mappings[i](states[i].get().getParameterValue()));
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
        floatStates.push_back (tracker);
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

        intStates.push_back (tracker);
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

        boolStates.push_back (tracker);
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

        choiceStates.push_back (tracker);
        choiceMappings.push_back (mapping ? mapping : [](const int x) { return x; });
    }

    void addTrackedChildParameters(AudioParametersBase& childParameters_)
    {
        auto addTrackedChild = [](auto& stateVector, auto& mappingVector, auto& childStateVector, auto& childMappingVector)
        {
            jassert(childStateVector.size() == childMappingVector.size());
            jassert(stateVector.size() == mappingVector.size());
            for (auto i = 0ul; i < childStateVector.size(); ++i)
            {
                stateVector.push_back (childStateVector[i]);
                mappingVector.push_back (childMappingVector[i]);
            }
            childStateVector.clear();
            childMappingVector.clear();
        };

        addTrackedChild(floatStates, floatMappings, childParameters_.floatStates, childParameters_.floatMappings);
        addTrackedChild(intStates, intMappings, childParameters_.intStates, childParameters_.intMappings);
        addTrackedChild(boolStates, boolMappings, childParameters_.boolStates, childParameters_.boolMappings);
        addTrackedChild(choiceStates, choiceMappings, childParameters_.choiceStates, childParameters_.choiceMappings);
    }

    juce::dsp::ProcessSpec spec{44100, 32, 2};

private:
    std::vector<std::reference_wrapper<FloatState>>  floatStates;
    std::vector<std::reference_wrapper<IntState>>    intStates;
    std::vector<std::reference_wrapper<BoolState>>   boolStates;
    std::vector<std::reference_wrapper<ChoiceState>> choiceStates;

    std::vector<FloatMapping> floatMappings;
    std::vector<IntMapping> intMappings;
    std::vector<BoolMapping> boolMappings;
    std::vector<ChoiceMapping> choiceMappings;

    std::vector<AudioParametersBase*> childParameters;

    juce::LinearSmoothedValue<float> masterRamp;

};


/*
 **********************************************************************
 **********************************************************************
 ***********************************************************************
 **********************************************************************
                BPM Synchronised Parameters
 **********************************************************************
 **********************************************************************
 ***********************************************************************
 **********************************************************************
 */

struct DefaultSyncRatesProvider
{
    struct Numerator
    {
        const static std::vector<int>& getValues()
        {
            static const auto values = []()
            {
                const std::vector<int> vals {1, 2, 3, 4, 5, 6, 7};
                std::vector<int> ret{};
                for (auto i = 0; i < 5; ++i)
                {
                    const auto p = static_cast<int>(pow(2, i));
                    for (const auto d : vals)
                    {
                        const auto next = p*d;
                        if (std::find(ret.begin(), ret.end(), next) == ret.end())
                            ret.push_back(next);
                    }
                }
                return ret;
            }();

            return values;
        }

        const static StringArray& getStrings()
        {
            const static StringArray strings =[]()
            {
                auto arr = StringArray();
                for (auto& d : getValues())
                    arr.add(String(d));
                return arr;
            }();
            return strings;
        }

        static int getDefault()
        {
            static const auto defaultValue = getStrings().indexOf("1");
            return defaultValue;
        }
    };

    struct Denominator
    {
        const static std::vector<int>& getValues()
        {
            const static std::vector<int> values {1, 2, 3, 4, 5, 6, 7, 8};
            return values;
        }

        const static StringArray& getStrings()
        {
            const static StringArray strings =[]()
            {
                auto arr = StringArray();
                for (const auto i : getValues())
                    arr.add(String(i));
                return arr;
            }();

            return strings;
        }

        static int getDefault()
        {
            static const auto defaultValue = getStrings().indexOf("8");
            return defaultValue;
        }
    };
};

template<typename SyncRatesProvider = DefaultSyncRatesProvider>
struct SyncedDurationParameter : sjf::helpers::AudioParametersBase
{
    SyncedDurationParameter(const float minTimeMS_, const float maxTimeMS_, const float defaultTimeMS_, const float skewForCentre_)
    : minTimeMS(jmin(minTimeMS_, maxTimeMS_))
    , maxTimeMS(jmax(minTimeMS_, maxTimeMS_))
    , defaultTimeMS((defaultTimeMS_ > minTimeMS && defaultTimeMS_ < maxTimeMS ? defaultTimeMS_ : jmap(0.5f, minTimeMS, maxTimeMS)))
    , skewForCentre(((skewForCentre_ > minTimeMS && skewForCentre_ < maxTimeMS ? skewForCentre_ : jmap(0.5f, minTimeMS, maxTimeMS))))
    {
        positionInfo.setBpm(120);
    }

    FloatState  time;
    BoolState   sync;
    ChoiceState syncedNumerator, syncedDenominator;

    const float minTimeMS, maxTimeMS, defaultTimeMS, skewForCentre;

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
    {
        auto factory = helpers::ParameterFactory::create (factoryID, factoryName);

        // Sync
        createTrackedParameter(*factory, sync, "Sync", "Sync", false);

        // Delay Time
        {
            const auto mapping = [&](const float x)
            {
                if (!sync.currentValue)
                {
                    return x * spec.sampleRate * 0.001f;
                }
                else
                {
                    const auto beat = 60.0f/static_cast<float>(*positionInfo.getBpm());
                    const auto bar = 4.0f * beat;
                    const auto numerator = static_cast<float>(SyncRatesProvider::Numerator::getValues()[static_cast<size_t>(syncedNumerator.getParameterValue())]);
                    const auto denominator = static_cast<float>(SyncRatesProvider::Denominator::getValues()[static_cast<size_t>(syncedDenominator.getParameterValue())]);
                    const auto div = numerator / denominator;
                    return spec.sampleRate * bar * div;
                }
            };
            createTrackedParameter  (*factory, time, "Time",  "Time  (ms)",  getDurationRange(), defaultTimeMS, mapping, getDurationAttributes());
        }

        // SyncedNumerator
        {
            auto mapping = [&](const int indx){ return SyncRatesProvider::Numerator::getValues()[static_cast<size_t>(indx)];};
            createTrackedParameter  (*factory, syncedNumerator, "NumDivisions", "NumDivisions", SyncRatesProvider::Numerator::getStrings(), SyncRatesProvider::Numerator::getDefault(), mapping);
        }
        //syncedDenominator
        {
            auto mapping = [&](const int indx){ return SyncRatesProvider::Denominator::getValues()[static_cast<size_t>(indx)];};
            createTrackedParameter (*factory, syncedDenominator, "Division", "Division", SyncRatesProvider::Denominator::getStrings(), SyncRatesProvider::Denominator::getDefault(), mapping);
        }


        return factory;
    }

    void setPositionInfo(const Optional<juce::AudioPlayHead::PositionInfo>& positionInfo_)
    {
        if (positionInfo_.hasValue() && positionInfo_->getBpm().hasValue())
            positionInfo.setBpm(positionInfo_->getBpm());
        else
            positionInfo.setBpm(120);
    }

    juce::NormalisableRange<float> getDurationRange() const
    {
        auto range = juce::NormalisableRange<float>{ minTimeMS, maxTimeMS, 0.001f };
        range.setSkewForCentre(skewForCentre);
        return range;
    }

    static juce::AudioParameterFloatAttributes getDurationAttributes()
    {
        return juce::AudioParameterFloatAttributes{}   .withLabel("ms");
    }

    AudioPlayHead::PositionInfo positionInfo;

};



template<typename SyncRatesProvider = DefaultSyncRatesProvider>
struct SyncedFrequencyParameter : sjf::helpers::AudioParametersBase
{
    SyncedFrequencyParameter(const float minFrequency_, const float maxFrequency_, const float defaultFrequency_)
    : minFrequency(jmin(minFrequency_, maxFrequency_) > 0 ? jmin(minFrequency_, maxFrequency_) : 0.001f)
    , maxFrequency(jmax(minFrequency_, maxFrequency_) > minFrequency && jmax(minFrequency_, maxFrequency_) <= 20000.0f ? jmax(minFrequency_, maxFrequency_) : 20000.0f)
    , defaultFrequency((defaultFrequency_ > minFrequency_ && defaultFrequency_ < maxFrequency_ ? defaultFrequency_ : jmap(0.5f, minFrequency, maxFrequency)))
    , skewForCentre(sqrt(minFrequency * maxFrequency))
    {
        positionInfo.setBpm(120);
    }

    FloatState  frequency;
    BoolState   sync;
    ChoiceState syncedNumerator, syncedDenominator;

    const float minFrequency, maxFrequency, defaultFrequency, skewForCentre;

    std::unique_ptr<helpers::ParameterFactory> createParameters (const juce::String& factoryID, const juce::String& factoryName) override
    {
        auto factory = helpers::ParameterFactory::create (factoryID, factoryName);

        // Sync
        createTrackedParameter(*factory, sync, "Sync", "Sync", false);

        // Frequency
        {

            const auto mapping = [&](const float x)
            {
                if (!sync.currentValue)
                {
                    return x;
                }
                else
                {
                    const auto beat = 60.0f/static_cast<float>(*positionInfo.getBpm());
                    const auto bar = 4.0f * beat;
                    const auto numerator = static_cast<float>(SyncRatesProvider::Numerator::getValues()[static_cast<size_t>(syncedNumerator.getParameterValue())]);
                    const auto denominator = static_cast<float>(SyncRatesProvider::Denominator::getValues()[static_cast<size_t>(syncedDenominator.getParameterValue())]);
                    const auto div = numerator / denominator;
                    return 1.0f/ (bar * div);
                }
            };

            createTrackedParameter  (*factory, frequency, "Frequency",  "Frequency  (Hz)",  getFrequencyRange(), defaultFrequency, mapping, getFrequencyAttributes());
        }

        // SyncedNumerator
        {
            auto mapping = [&](const int indx){ return SyncRatesProvider::Numerator::getValues()[static_cast<size_t>(indx)];};
            createTrackedParameter  (*factory, syncedNumerator, "NumDivisions", "NumDivisions", SyncRatesProvider::Numerator::getStrings(), SyncRatesProvider::Numerator::getDefault(), mapping);
        }
        //syncedDenominator
        {
            auto mapping = [&](const int indx){ return SyncRatesProvider::Denominator::getValues()[static_cast<size_t>(indx)];};
            createTrackedParameter (*factory, syncedDenominator, "Division", "Division", SyncRatesProvider::Denominator::getStrings(), SyncRatesProvider::Denominator::getDefault(), mapping);
        }


        return factory;
    }

    void setPositionInfo(const Optional<juce::AudioPlayHead::PositionInfo>& positionInfo_)
    {
        if (positionInfo_.hasValue() && positionInfo_->getBpm().hasValue())
            positionInfo.setBpm(positionInfo_->getBpm());
        else
            positionInfo.setBpm(120);
    }

    juce::NormalisableRange<float> getFrequencyRange() const
    {
        auto range = juce::NormalisableRange<float>{ minFrequency, maxFrequency, 0.001f };
        range.setSkewForCentre(skewForCentre);
        return range;
    }

    static juce::AudioParameterFloatAttributes getFrequencyAttributes()
    {
        return juce::AudioParameterFloatAttributes{}   .withLabel("Hz");
    }

    AudioPlayHead::PositionInfo positionInfo;
};


}
