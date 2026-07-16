//
//  sjf_phasor.h
//
//  Created by Simon Fay on 12/05/2024.
//



#pragma once
#include <JuceHeader.h>


namespace sjf::dsp::oscillators
{
/**
 Simple phasor class for use with modulators and other oscillators
 */

struct Phasor
{
    float m_increment;
    float m_phase = 0.0f;

    Phasor(){}

    void prepare(const  juce::dsp::ProcessSpec& spec_)
    {
        spec = spec_;
        reset();
    }

    void reset()
    {
        m_phase = 0.0f;
    }

    void setFrequency( const float f )
    {
        setFrequency(f, static_cast<float>(spec.sampleRate));
    }

    /**
     sets the internal frequency the phasor runs at
     */
    void setFrequency( const float f, const float sampleRate )
    {
        setIncrement( f / sampleRate);
        spec.sampleRate = sampleRate;
    }

    /**
     sets the increment per sample
     */
    void setIncrement( const float inc )
    {
        jassert(inc < 1.0f && inc > -1.0f);
        m_increment = inc >= 0.0f ? inc : 1.0f - inc;
    }

    /**
     Output one sample from the phasor
     */
    float process()
    {
        float p = m_phase;
        m_phase += m_increment;
        m_phase = m_phase - static_cast<float>(static_cast<int>(m_phase));
        return p;
    }

    void skip(size_t numSamplesToSkip)
    {
        m_phase += m_increment*static_cast<float>(numSamplesToSkip);
        m_phase = m_phase - static_cast<float>(static_cast<int>(m_phase));
    }
private:
    juce::dsp::ProcessSpec spec{};
};


//=============//=============//=============//=============//=============//=============
//=============//=============//=============//=============//=============//=============
//=============//=============//=============//=============//=============//=============
//=============//=============//=============//=============//=============//=============
struct DefaultSyncRatesProvider
{
    const static std::vector<int>& getDenominators()
    {
        static const auto denominators = []()
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

        return denominators;
    }

    const static StringArray& getDenominatorStrings()
    {
        const static StringArray denominatorStrings =[]()
        {
           auto arr = StringArray();
            for (auto& d : getDenominators())
                arr.add(String(d));
            return arr;
        }();
        return denominatorStrings;
    }

    const static std::vector<int>& getNumerators()
    {
        const static std::vector<int> numerators {1, 2, 3, 4, 5, 6, 7, 8};
        return numerators;
    }

    const static StringArray& getNumeratorStrings()
    {
        const static StringArray numeratorStrings =[]()
        {
            auto arr = StringArray();
            for (const auto i : getNumerators())
                arr.add(String(i));
            return arr;
        }();

        return numeratorStrings;
    }
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
            auto range = juce::NormalisableRange<float>{ minTimeMS, maxTimeMS, 0.01f };
            range.setSkewForCentre(skewForCentre);
            const auto attributes = juce::AudioParameterFloatAttributes{}   .withLabel("ms");
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
                    const auto numerator = static_cast<float>(SyncRatesProvider::getNumerators()[static_cast<size_t>(syncedNumerator.getParameterValue())]);
                    const auto denominator = static_cast<float>(SyncRatesProvider::getDenominators()[static_cast<size_t>(syncedDenominator.getParameterValue())]);
                    const auto div = numerator / denominator;
                    return spec.sampleRate * bar * div;
                }
            };
            createTrackedParameter  (*factory, time, "Time",  "Time  (ms)",  range, defaultTimeMS, mapping, attributes);
        }

        // SyncedNumerator
        {
            auto mapping = [&](const int indx){ return SyncRatesProvider::getNumerators()[static_cast<size_t>(indx)];};
            createTrackedParameter  (*factory, syncedNumerator, "NumDivisions", "NumDivisions", SyncRatesProvider::getNumeratorStrings(), 0, mapping);
        }
        //syncedDenominator
        {
            auto mapping = [&](const int indx){ return SyncRatesProvider::getDenominators()[static_cast<size_t>(indx)];};
            createTrackedParameter (*factory, syncedDenominator, "Division", "Division", SyncRatesProvider::getDenominatorStrings(), 0, mapping);
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
            auto range = juce::NormalisableRange<float>{ minFrequency, maxFrequency, 0.01f };
            range.setSkewForCentre(skewForCentre);
            const auto attributes = juce::AudioParameterFloatAttributes{}   .withLabel("ms");
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
                    const auto numerator = static_cast<float>(SyncRatesProvider::getNumerators()[static_cast<size_t>(syncedNumerator.getParameterValue())]);
                    const auto denominator = static_cast<float>(SyncRatesProvider::getDenominators()[static_cast<size_t>(syncedDenominator.getParameterValue())]);
                    const auto div = numerator / denominator;
                    return 1.0f/ (bar * div);
                }
            };
            createTrackedParameter  (*factory, frequency, "Frequency",  "Frequency  (Hz)",  range, defaultFrequency, mapping, attributes);
        }

        // SyncedNumerator
        {
            auto mapping = [&](const int indx){ return SyncRatesProvider::getNumerators()[static_cast<size_t>(indx)];};
            createTrackedParameter  (*factory, syncedNumerator, "NumDivisions", "NumDivisions", SyncRatesProvider::getNumeratorStrings(), 0, mapping);
        }
        //syncedDenominator
        {
            auto mapping = [&](const int indx){ return SyncRatesProvider::getDenominators()[static_cast<size_t>(indx)];};
            createTrackedParameter (*factory, syncedDenominator, "Division", "Division", SyncRatesProvider::getDenominatorStrings(), 0, mapping);
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

    AudioPlayHead::PositionInfo positionInfo;
};


}





