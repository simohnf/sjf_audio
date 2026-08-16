/*
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
 */

//
// Created by Simon Fay on 08/07/2026.
//
#pragma once
#include <JuceHeader.h>

#include "sjf_HelperFunctions.h"
#if __has_include("VersionHints.h")
	#include "VersionHints.h"
#else
namespace sjf::version_hints
{
	template <typename T>
	constexpr int getVersionHint(T&&) { return 0; }
}
#endif

namespace sjf::helpers
{

/**
 * @brief A factory wrapper around juce::AudioProcessorParameterGroup that automatically enforces
 *        hierarchical parameter naming and component ID separation.
 *
 * This class captures a base string identifier (`baseID`) and a human-readable prefix (`baseName`)
 * upon creation. Every parameter instantiated through this factory automatically appends its
 * local ID and local display name to these roots, producing grouped parameters with standardized
 * prefixes (e.g., ID: "baseID.paramID", Name: "baseName: paramName").
 */
class ParameterFactory : public juce::AudioProcessorParameterGroup
{
private:
    struct ConstructorToken { explicit ConstructorToken (int) {} };

public:
	struct GroupMetadata
	{
		const juce::String groupID;
		const bool supportsSubPresets = false;
		const bool supportsChildSubPresets = false;
		const size_t numProcessorsInDynamicSequence{0};
		const juce::AudioParameterChoice* selectorParameter = nullptr;
		std::vector<GroupMetadata> children;


		[[nodiscard]] bool isSelectorGroup() const noexcept
		{
			return selectorParameter != nullptr;
		}

		[[nodiscard]] bool isDynamicProcessorSequenceGroup() const noexcept
		{
			return numProcessorsInDynamicSequence > 0;
		}

		[[nodiscard]] const GroupMetadata* findChild (const juce::String& targetID) const noexcept
		{
			for (const auto& child : children)
			{
				if (child.groupID == targetID)
					return &child;
			}
			return nullptr;
		}

	};

	[[nodiscard]] static GroupMetadata createMetadataTree (const ParameterFactory& rootFactory)
	{
		GroupMetadata node{
			.groupID = rootFactory.getID(),
			.supportsSubPresets = rootFactory.supportsSubPresets(),
			.supportsChildSubPresets = rootFactory.supportsChildSubPresets(),
			.numProcessorsInDynamicSequence = rootFactory.getNumProcessorsInDynamicSequence(),
			.children = {},
		};

		if (!rootFactory.childFactories.empty())
		{
			for (const auto rangedParam : rootFactory.getParameters(false))
			{
				if (const auto* choiceParam = dynamic_cast<const juce::AudioParameterChoice*> (rangedParam))
				{
					const auto& choices = choiceParam->choices;

					if (static_cast<size_t>(choices.size()) == rootFactory.childFactories.size())
					{
						bool matchesAll = true;
						for (auto i = 0; i < choices.size(); ++i)
						{
							const auto relativeName = getNameWithoutParentPrefix (*rootFactory.childFactories[static_cast<size_t>(i)]);
							if (choices[i] != rootFactory.childFactories[static_cast<size_t>(i)]->getName() && choices[i] != relativeName)
							{
								matchesAll = false;
								break;
							}
						}

						if (matchesAll)
						{
							node.selectorParameter = choiceParam;
							break;
						}
					}
				}
			}
		}

		for (const auto* childFactory : rootFactory.childFactories)
		{
			if (childFactory != nullptr)
				node.children.push_back (createMetadataTree (*childFactory));
		}
		return node;
	}

    static std::unique_ptr<ParameterFactory> create (const juce::String& factoryID, const juce::String& factoryName, const bool supportsSubPresets = true, const bool supportsChildSubPresets = true)
    {
        return std::make_unique<ParameterFactory> (ConstructorToken{0}, factoryID, factoryName, 0, supportsSubPresets, supportsChildSubPresets);
    }

    static std::unique_ptr<ParameterFactory> createDynamicProcessorSequence (const juce::String& factoryID, const juce::String& factoryName, const size_t numProcessors, const bool supportsSubPresets = true, const bool supportsChildSubPresets = true)
    {
        return std::make_unique<ParameterFactory> (ConstructorToken{0}, factoryID, factoryName, numProcessors, supportsSubPresets, supportsChildSubPresets);
    }

    ParameterFactory (ConstructorToken, const juce::String& factoryID, const juce::String& factoryName, const size_t numProcessorsForDynamicSequence = 0, const bool supportsSubPresets = true, const bool supportsChildSubPresets = true)
        : juce::AudioProcessorParameterGroup (factoryID, factoryName, " "),
          baseID (factoryID), baseName (factoryName), dynamicProcessorSequence(numProcessorsForDynamicSequence), supportsPresets(supportsSubPresets), supportsChildPresets(supportsChildSubPresets)
    {}

    //==============================================================================
    juce::AudioParameterFloat* createFloatParameter (const juce::ParameterID& parameterID,
                                                     const juce::String& parameterName,
                                                     juce::NormalisableRange<float> normalisableRange,
                                                     float defaultValue,
                                                     const juce::AudioParameterFloatAttributes& attributes = {})
    {
        const auto id =  baseID + parameterID.getParamID();
        const auto versionHint = parameterID.getVersionHint() != 0 ? parameterID.getVersionHint() : sjf::version_hints::getVersionHint(id);
        auto p = std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id, versionHint),
            baseName + " " + parameterName,
            normalisableRange,
            defaultValue,
            attributes
        );

        auto* rawPtr = p.get();
        AudioProcessorParameterGroup::addChild (std::move (p));
        return rawPtr;
    }

    juce::AudioParameterInt* createIntParameter (const juce::ParameterID& parameterID,
                                                 const juce::String& parameterName,
                                                 int minValue, int maxValue, int defaultValue,
                                                 const juce::AudioParameterIntAttributes& attributes = {})
    {
        const auto id =  baseID + parameterID.getParamID();
        const auto versionHint = parameterID.getVersionHint() != 0 ? parameterID.getVersionHint() : sjf::version_hints::getVersionHint(id);
        auto p = std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID (id, versionHint),
            baseName + " " + parameterName,
            minValue, maxValue, defaultValue,
            attributes
        );

        auto* rawPtr = p.get();
        AudioProcessorParameterGroup::addChild (std::move (p));
        return rawPtr;
    }

    juce::AudioParameterBool* createBoolParameter (const juce::ParameterID& parameterID,
                                                   const juce::String& parameterName,
                                                   bool defaultValue,
                                                   const juce::AudioParameterBoolAttributes& attributes = {})
    {
        const auto id =  baseID + parameterID.getParamID();
        const auto versionHint = parameterID.getVersionHint() != 0 ? parameterID.getVersionHint() : sjf::version_hints::getVersionHint(id);
        auto p = std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID (id, versionHint),
            baseName + " " + parameterName,
            defaultValue,
            attributes
        );

        auto* rawPtr = p.get();
        AudioProcessorParameterGroup::addChild (std::move (p));
        return rawPtr;
    }

    juce::AudioParameterChoice* createChoiceParameter (const juce::ParameterID& parameterID,
                                                       const juce::String& parameterName,
                                                       const juce::StringArray& choices,
                                                       int defaultChoiceIndex,
                                                       const juce::AudioParameterChoiceAttributes& attributes = {})
    {
        const auto id =  baseID + parameterID.getParamID();
        const auto versionHint = parameterID.getVersionHint() != 0 ? parameterID.getVersionHint() : sjf::version_hints::getVersionHint(id);
        auto p = std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID (id, versionHint),
            baseName + " " + parameterName,
            choices,
            defaultChoiceIndex,
            attributes
        );

        auto* rawPtr = p.get();
        AudioProcessorParameterGroup::addChild (std::move (p));
        return rawPtr;
    }

	template <typename ParameterOrGroup>
	void addChild (std::unique_ptr<ParameterOrGroup>)
	{
		static_assert(false, "You should not call this method, instead use createTrackedParameter or use addChildFactory to add a subgroup");
	}

	template <typename ParameterOrGroup, typename... Args>
	void addChild (std::unique_ptr<ParameterOrGroup>, Args&&...)
	{
		static_assert(false, "You should not call this method, instead use createTrackedParameter or use addChildFactory to add a subgroup");
	}

    void addChildFactory (std::unique_ptr<ParameterFactory> child)
    {
        jassert(child->getID().startsWith(baseID));
        jassert(child->getName().startsWith(baseName));

        if (child != nullptr)
        {
        	childFactories.push_back(child.get());
	        AudioProcessorParameterGroup::addChild (std::move (child));
        }
    }

    [[nodiscard]] static juce::String getIDWithoutParentPrefix(const juce::AudioProcessorParameterGroup& group)
    {
        const auto groupID = group.getID();
        if (const auto parent = group.getParent())
            return groupID.substring(parent->getID().length()).trim();
        else
            return groupID;
    }

    [[nodiscard]] static juce::String getIDWithoutParentPrefix(const ParameterFactory& group)
    {
        return getIDWithoutParentPrefix(*dynamic_cast<const AudioProcessorParameterGroup*>(&group));
    }

    [[nodiscard]] static juce::String getNameWithoutParentPrefix(const juce::AudioProcessorParameterGroup& group)
    {
        const auto groupName = group.getName();
        if (const auto parent = group.getParent())
            return groupName.substring(parent->getName().length()).trim();
        else
            return groupName;
    }

    [[nodiscard]] static juce::String getNameWithoutParentPrefix(const ParameterFactory& group)
    {
        return getNameWithoutParentPrefix(*dynamic_cast<const AudioProcessorParameterGroup*>(&group));
    }

    [[nodiscard]] static juce::String getIDWithoutParentPrefix(const juce::RangedAudioParameter& param, const juce::AudioProcessorParameterGroup& group)
    {
        jassert(param.paramID.startsWith(group.getID()));
        return param.paramID.substring(group.getID().length());
    }

    [[nodiscard]] static juce::String getIDWithoutParentPrefix(const juce::RangedAudioParameter& param, const ParameterFactory& group)
    {
        return getIDWithoutParentPrefix(param, *dynamic_cast<const AudioProcessorParameterGroup*>(&group));
    }

    [[nodiscard]] static juce::String getNameWithoutParentPrefix(const juce::RangedAudioParameter& param, const juce::AudioProcessorParameterGroup& group)
    {
        jassert(param.name.startsWith(group.getName()));
        return param.name.substring(group.getName().length()).trim();
    }

    [[nodiscard]] static juce::String getNameWithoutParentPrefix(const juce::RangedAudioParameter& param, const ParameterFactory& group)
    {
        return getNameWithoutParentPrefix(param, *dynamic_cast<const AudioProcessorParameterGroup*>(&group));
    }

	void setAllToDefault(const bool recursive = true)
    {
	    for (auto p : getParameters(recursive))
	    	p->setValue(p->getDefaultValue());
    }

	bool supportsSubPresets() const
    {
	    return supportsPresets;
    }

	bool supportsChildSubPresets() const
	{
		return supportsChildPresets;
	}

	[[nodiscard]] size_t getNumProcessorsInDynamicSequence() const
	{
		return dynamicProcessorSequence;
	}
private:
	std::vector<const ParameterFactory*> childFactories;
    const juce::String baseID, baseName;
	const size_t dynamicProcessorSequence{0};
	const bool supportsPresets{false};
	const bool supportsChildPresets{true};
};

//===========//===========//===========//===========//===========//===========
/**
 * @brief State management engine that tracks host parameters and coordinates
 *        sample-accurate linear parameter smoothing.
 *
 * This abstract base class decouples audio-rate threads from standard JUCE parameters
 * to facilitate optimal fast-path DSP processing. It provides the following key workflows:
 *
 * 1. **State Isolation:** Translates raw host values into clean thread-safe targets,
 *    applying inline functional value transforms (e.g., mapping decibels to linear gain values).
 * 2. **Divergence Detection:** Automatically flags whether any tracked parameter has
 *    changed relative to its previous target inside `checkForStateChange()`.
 * 3. **Execution Routing:** Drives the high-level branch topology within processors.
 *    If parameters change, it activates a central `masterRamp` to smoothly interpolate values;
 *    if parameters remain static, it skips interpolator overhead to maximize compiler vectorization.
 *
 *    see sjf::helpers::DummyProcessor for basic usage
 */
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

    	bool isSmoothing()
        {
        	if constexpr (std::is_same_v<JuceParamType, juce::AudioParameterFloat>)
				return !juce::approximatelyEqual(currentValue, targetValue);
        	else
				return currentValue != targetValue;

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
    	for ( auto pp : preprableParameters)
    		pp.get().prepare(spec);
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


    /**
     * @brief Evaluates host automation activity to determine the optimal processing topology
     *        for the current audio block.
     *
     * This method must be called exactly once at the beginning of each processing block
     * (e.g., inside `DummyProcessor::process`). It compares current host parameter values
     * against previously captured thread-safe targets.
     *
     * If a divergence is detected, it latches the new targets and triggers an internal
     * master ramp to begin smoothing.
     *
     * @return true  If any parameter is actively interpolating or has just changed,
     *               signaling that the sample-by-sample smoothed processing path must be used.
     * @return false If all parameters are completely stationary, allowing the processor to
     *               switch safely to an optimized, highly vectorizable static processing path.
     */
    bool checkForStateChange()
    {
        #if JUCE_DEBUG
        checkForStateChangeWasCalled = true;
        #endif

        if (masterRamp.isSmoothing()) return true;

        const bool parametersHaveChanged = anyStatesDiverged (floatStates, floatMappings) || anyStatesDiverged (intStates, intMappings)
                                        || anyStatesDiverged (boolStates, boolMappings)  || anyStatesDiverged (choiceStates, choiceMappings);

        if (parametersHaveChanged) {
            latchAllStates (floatStates, floatMappings);
            resetAllStates (intStates, intMappings);
            resetAllStates (boolStates, boolMappings);
            resetAllStates (choiceStates, choiceMappings);

            masterRamp.setCurrentAndTargetValue (0.0f);
            masterRamp.setTargetValue (1.0f);
            return true;
        }
        return false;
    }

    /**
     * @brief Advances internal linear interpolation ramps by exactly one sample step.
     *
     * This method **must be called exactly once per sample** inside your audio-rate loop
     * when executing the smoothed processing path (i.e., when `checkForStateChange()` returns true).
     *
     * It increments the shared `masterRamp` and updates the active `currentValue` fields
     * for all tracked float parameters. Non-smoothed types (int, bool, choice) are snapped
     * immediately to their target states upon the first tick.
     *
     * @note This method runs inside the critical real-time rendering path and executes
     *       with constant $O(N)$ time complexity relative to the number of tracked states.
     */
    inline void tickSmoothers() noexcept {
        #if JUCE_DEBUG
        /*
         * If you hit this assertion it means you have never called checkForStateChange
         * only FloatStates are updated via tickSmoothers, all other states are snapped to the target value
         * as soon as a change is registered
         *
         * You should call parameters.checkForStateChange() at the beginning of each process block
         */
        jassert(checkForStateChangeWasCalled);
        #endif

        if (masterRamp.isSmoothing()) {
            const float alpha = masterRamp.getNextValue();
            for (const auto state : floatStates)
                state.get().currentValue = state.get().startValue + alpha * (state.get().targetValue - state.get().startValue);

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

	template<typename TrackedStateType, typename Mapping>
	void updateMapping(TrackedStateType& trackedState, Mapping mapping) noexcept
    {
    	static constexpr bool isFloatParam	= std::is_same_v<std::decay_t<TrackedStateType>, FloatState>;
    	static constexpr bool isIntParam	= std::is_same_v<std::decay_t<TrackedStateType>, IntState>;
    	static constexpr bool isChoiceParam = std::is_same_v<std::decay_t<TrackedStateType>, ChoiceState>;
    	static constexpr bool isBoolParam	= std::is_same_v<std::decay_t<TrackedStateType>, BoolState>;

	    if constexpr (isFloatParam)
	    {
		    static_assert(std::is_constructible_v<FloatMapping, std::decay_t<Mapping>>, "FloatParameters must use FloatMappings");
	    	for ( auto i = 0ul; i < floatStates.size(); ++i)
	    	{
	    		if (&trackedState == &floatStates[i].get())
	    		{
	    			floatMappings[i] = mapping;
	    			trackedState.latchTarget(mapping(trackedState.getParameterValue()));
	    			return;
	    		}
	    	}
	    }
    	if constexpr (isIntParam)
    	{
    		static_assert(std::is_constructible_v<IntMapping, std::decay_t<Mapping>>, "IntParameters must use IntMappings");
    		for ( auto i = 0ul; i < intStates.size(); ++i)
    		{
    			if (&trackedState == &intStates[i].get())
    			{
    				intMappings[i] = mapping;
    				trackedState.latchTarget(mapping(trackedState.getParameterValue()));
    				return;
    			}
    		}
    	}
    	if constexpr (isChoiceParam)
    	{
    		static_assert(std::is_constructible_v<ChoiceMapping, std::decay_t<Mapping>>, "ChoiceParameters must use ChoiceMappings");
    		for ( auto i = 0ul; i < choiceStates.size(); ++i)
    		{
    			if (&trackedState == &choiceStates[i].get())
    			{
    				choiceMappings[i] = mapping;
    				trackedState.latchTarget(mapping(trackedState.getParameterValue()));

    				return;
    			}
    		}
    	}
    	if constexpr (isBoolParam)
    	{
    		static_assert(std::is_constructible_v<BoolMapping, std::decay_t<Mapping>>, "BoolParameters must use BoolMappings");
    		for ( auto i = 0ul; i < boolStates.size(); ++i)
    		{
    			if (&trackedState == &boolStates[i].get())
    			{
    				boolMappings[i] = mapping;
    				trackedState.latchTarget(mapping(trackedState.getParameterValue()));

    				return;
    			}
    		}
    	}
    	jassertfalse; // you've tried to change a mapping for a parameter that's not tracked by this obect
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

    	preprableParameters.push_back(childParameters_);
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
	std::vector<std::reference_wrapper<AudioParametersBase>> preprableParameters;

    juce::LinearSmoothedValue<float> masterRamp;

    #if JUCE_DEBUG
    bool checkForStateChangeWasCalled = false;
    #endif
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

        const static juce::StringArray& getStrings()
        {
            const static juce::StringArray strings =[]()
            {
                auto arr = juce::StringArray();
                for (auto& d : getValues())
                    arr.add(juce::String(d));
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

        const static juce::StringArray& getStrings()
        {
            const static juce::StringArray strings =[]()
            {
                auto arr = juce::StringArray();
                for (const auto i : getValues())
                    arr.add(juce::String(i));
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

/**
 * @brief Parameter group container managing a parameter that outputs durations in samples,
 *        dynamically switching between millisecond durations or tempo-derived calculations.
 *
 * This class operates as a lightweight structural layout layer. Instead of managing its
 * own data memory allocation, it safely holds modern lvalue references to raw configuration
 * tracking states (`FloatState`, `BoolState`, `ChoiceState`) that reside natively within
 * the parent processor's parameters layout.
 *
 * @note Because this component stores internal lvalue references (`&`), its lifecycle is
 *       strictly bound to the parent class. The tracking states passed into its constructor
 *       must be declared **above** this class instance in the parent's layout to guarantee
 *       valid instantiation order.
 *
 * ### Example Component Setup:
 * @code
 * struct MyParameters : public sjf::helpers::AudioParametersBase
 * {
 *     // 1. Backing states declared first
 *     FloatState  myTime;
 *     BoolState   mySync;
 *     ChoiceState myNumerator, myDenominator;
 *
 *     // 2. Reference-bound wrapper declared next
 *     sjf::helpers::SyncedDurationParameter<> delayTime;
 *
 *     MyParameters()
 *     : delayTime (myTime, mySync, myNumerator, myDenominator,
 *                  SyncedDurationParameter<>::makeTimeRange(1.0f, 2000.0f, 500.0f), 500.0f)
 *     {}
 * };
 * @endcode
 *
 * @tparam SyncRatesProvider Timing definition layout providing arrays of musical division
 *                           numerators, denominators, and text representations. Defaults
 *                           to `DefaultSyncRatesProvider`.
 */
template<typename SyncRatesProvider = DefaultSyncRatesProvider>
struct SyncedDurationParameter : sjf::helpers::AudioParametersBase
{
    SyncedDurationParameter(FloatState& time_, BoolState& sync_, ChoiceState& syncedNumerator_, ChoiceState& syncedDenominator_, const juce::NormalisableRange<float>& timeRange_, const float defaultTimeMS_)
    : time(time_), sync(sync_), syncedNumerator(syncedNumerator_), syncedDenominator(syncedDenominator_)
    , timeRange(timeRange_)
    , defaultTimeMS((defaultTimeMS_ > timeRange.start && defaultTimeMS_ < timeRange.end ? defaultTimeMS_ :  timeRange.convertFrom0to1(0.5f)))
    {
        positionInfo.setBpm(120);
    }

    FloatState  &time;
    BoolState   &sync;
    ChoiceState &syncedNumerator, &syncedDenominator;
    const juce::NormalisableRange<float> timeRange;
    const float defaultTimeMS;

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
                    const auto beat = 60000.0f/static_cast<float>(*positionInfo.getBpm());
                    const auto bar = 4.0f * beat;
                    const auto numerator = static_cast<float>(SyncRatesProvider::Numerator::getValues()[static_cast<size_t>(syncedNumerator.getParameterValue())]);
                    const auto denominator = static_cast<float>(SyncRatesProvider::Denominator::getValues()[static_cast<size_t>(syncedDenominator.getParameterValue())]);
                    const auto div = numerator / denominator;
                    return spec.sampleRate * juce::jlimit(timeRange.start, timeRange.end, bar * div) * 0.001f;
                }
            };
            createTrackedParameter  (*factory, time, "Time",  "Time  (ms)",  timeRange, defaultTimeMS, mapping, getDurationAttributes());
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

    void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
    {
        positionInfo = positionInfo_;
        if (!positionInfo.getBpm().hasValue())
            positionInfo.setBpm(120);
    }

    static juce::NormalisableRange<float> makeTimeRange(const float minMS, const float maxMS, const float skewForCentre_)
    {
        const auto min = juce::jmin(minMS, maxMS);
        const auto max = juce::jmax(minMS, maxMS);
        const auto skew = ((skewForCentre_ > min && skewForCentre_ < max ? skewForCentre_ : juce::jmap(0.5f, min, max)));
        auto range = juce::NormalisableRange<float>(minMS, maxMS, 0.001f);
        range.setSkewForCentre(skew);
        return range;
    }


    static juce::AudioParameterFloatAttributes getDurationAttributes()
    {
        return juce::AudioParameterFloatAttributes{}   .withLabel("ms");
    }

    /** @brief Helper factory to batch-construct an array of wrappers from separate configuration state arrays. */
    template <size_t N>
    static std::array<SyncedDurationParameter, N> createArray(
        std::array<FloatState, N>& times, std::array<BoolState, N>& syncs, std::array<ChoiceState, N>& numerators, std::array<ChoiceState, N>& denominators,
        const float minMS, const float maxMS, float defaultMS, const float skew)
    {
        const auto range = makeTimeRange(minMS, maxMS, skew);

        auto zip = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array<SyncedDurationParameter, N>{
                SyncedDurationParameter{ times[Is], syncs[Is], numerators[Is], denominators[Is], range, defaultMS }...
            };
        };

        return zip(std::make_index_sequence<N>{});
    }

    juce::AudioPlayHead::PositionInfo positionInfo;
};



/**
 * @brief Parameter group container managing a parameter that outputs frequency in Hz,
 *        dynamically switching between absolute values or tempo-derived calculations.
 *
 * This class operates as a lightweight structural layout layer. Instead of managing its
 * own data memory allocation, it safely holds modern lvalue references to raw configuration
 * tracking states (`FloatState`, `BoolState`, `ChoiceState`) that reside natively within
 * the parent processor's parameters layout.
 *
 * @note Because this component stores internal lvalue references (`&`), its lifecycle is
 *       strictly bound to the parent class. The tracking states passed into its constructor
 *       must be declared **above** this class instance in the parent's layout to guarantee
 *       valid instantiation order.
 *
 * ### Example Component Setup:
 * @code
 * struct MyParameters : public sjf::helpers::AudioParametersBase
 * {
 *     // 1. Backing states declared first
 *     FloatState  myFreq;
 *     BoolState   mySync;
 *     ChoiceState myNumerator, myDenominator;
 *
 *     // 2. Reference-bound wrapper declared next
 *     sjf::helpers::SyncedFrequencyParameter<> frequency;
 *
 *     MyParameters()
 *     : frequency (myFreq, mySync, myNumerator, myDenominator,
 *                  SyncedFrequencyParameter<>::makeFrequencyRange(0.1f, 20.0f), 1.0f)
 *     {}
 * };
 * @endcode
 *
 * @tparam SyncRatesProvider Timing definition layout providing arrays of musical division
 *                           numerators, denominators, and text representations. Defaults
 *                           to `DefaultSyncRatesProvider`.
 */
template<typename SyncRatesProvider = DefaultSyncRatesProvider>
struct SyncedFrequencyParameter : sjf::helpers::AudioParametersBase
{
    SyncedFrequencyParameter(FloatState& frequency_, BoolState& sync_, ChoiceState& syncedNumerator_, ChoiceState& syncedDenominator_, const juce::NormalisableRange<float>& frequencyRange_, const float defaultFrequency_)
    : frequency(frequency_), sync(sync_), syncedNumerator(syncedNumerator_), syncedDenominator(syncedDenominator_)
    , frequencyRange(frequencyRange_)
    , defaultFrequency((defaultFrequency_ > frequencyRange.start && defaultFrequency_ < frequencyRange.end ? defaultFrequency_ : frequencyRange.convertFrom0to1(0.5f)))
    {
        positionInfo.setBpm(120);
    }

    FloatState  &frequency;
    BoolState   &sync;
    ChoiceState &syncedNumerator, &syncedDenominator;
    const juce::NormalisableRange<float> frequencyRange;
    const float defaultFrequency;

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
                    return juce::jlimit(frequencyRange.start, frequencyRange.end, 1.0f / (bar * div));
                }
            };

            createTrackedParameter (*factory, frequency, "Freq", "Frequency  (Hz)", frequencyRange, defaultFrequency, mapping, getFrequencyAttributes());
        }

        // SyncedNumerator
        {
            auto mapping = [&](const int indx){ return SyncRatesProvider::Numerator::getValues()[static_cast<size_t>(indx)];};
            createTrackedParameter (*factory, syncedNumerator, "NumDivs", "NumDivisions", SyncRatesProvider::Numerator::getStrings(), SyncRatesProvider::Numerator::getDefault(), mapping);
        }

        // SyncedDenominator
        {
            auto mapping = [&](const int indx){ return SyncRatesProvider::Denominator::getValues()[static_cast<size_t>(indx)];};
            createTrackedParameter (*factory, syncedDenominator, "Div", "Division", SyncRatesProvider::Denominator::getStrings(), SyncRatesProvider::Denominator::getDefault(), mapping);
        }

        return factory;
    }

    void setPositionInfo(const juce::AudioPlayHead::PositionInfo& positionInfo_)
    {
    	positionInfo = positionInfo_;
        if (!positionInfo_.getBpm().hasValue())
            positionInfo.setBpm(120);
    }

    static juce::NormalisableRange<float> makeFrequencyRange(const float minHz, const float maxHz)
    {
        const auto min = juce::jmin(minHz, maxHz) > 0 ? juce::jmin(minHz, maxHz) : 0.001f;
        const auto max = juce::jmax(minHz, maxHz) > min && juce::jmax(minHz, maxHz) <= 20000.0f ? juce::jmax(minHz, maxHz) : 20000.0f;
        const auto skew = std::sqrt(min * max);

        auto range = juce::NormalisableRange<float>(min, max, 0.001f);
        range.setSkewForCentre(skew);
        return range;
    }

    static juce::AudioParameterFloatAttributes getFrequencyAttributes()
    {
        return juce::AudioParameterFloatAttributes{}.withLabel("Hz");
    }

    /** @brief Helper factory to batch-construct an array of wrappers from separate configuration state arrays. */
    template <size_t N>
    static std::array<SyncedFrequencyParameter, N> createArray(
        std::array<FloatState, N>& frequencies, std::array<BoolState, N>& syncs, std::array<ChoiceState, N>& numerators, std::array<ChoiceState, N>& denominators,
        const float minHz, const float maxHz, const float defaultHz)
    {
        const auto range = makeFrequencyRange(minHz, maxHz);

        auto zip = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array<SyncedFrequencyParameter, N>{
                SyncedFrequencyParameter{ frequencies[Is], syncs[Is], numerators[Is], denominators[Is], range, defaultHz }...
            };
        };

        return zip(std::make_index_sequence<N>{});
    }

    juce::AudioPlayHead::PositionInfo positionInfo;
};
}
