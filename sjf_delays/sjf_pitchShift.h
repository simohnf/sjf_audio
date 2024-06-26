//
//  sjf_pitchShift.h
//
//  Created by Simon Fay on 27/03/2024.
//

#ifndef sjf_pitchShift_h
#define sjf_pitchShift_h



#include "sjf_delay.h"
#include "../sjf_oscillators/sjf_phasor.h"
#include "../sjf_table.h"
#include "../sjf_filters.h"

namespace sjf::delayLine
{
    /**
     basic circular buffer based pitchShift line
     */
    template < typename Sample, interpolation::interpolatorTypes interpType /*, typename INTERPOLATION_FUNCTOR = interpolation::fourPointInterpolatePD< Sample >*/ >
    class pitchShift
    {
    public:
        pitchShift() { }
        ~pitchShift(){ }
        
        /** This should be called prior to usage to initialise important values */
        void initialise( const Sample sampleRate )
        {
            m_SR = sampleRate > 0 ? sampleRate : m_SR;
            setWindowSize( m_windowSecs );
            m_lpfInCoef = calculateLPFCoefficient<Sample>( LPFINCUTOFF, m_SR );
            m_lpfOutCoef = calculateLPFCoefficient<Sample>( LPFOUTCUTOFF, m_SR );
            m_delay.initialise( m_SR );
        }
        
        /** Set the playback scaling factor (e.g. 2 would be an octave above, 0.5 an octave below */
        void setPitchScaling( Sample scaleFactor )
        {
            m_scaleFactor = scaleFactor > 0 ? scaleFactor : m_scaleFactor;
            scaleFactor -= 1;
            scaleFactor *= -m_invWindowSecs;
            m_phasor.setFrequency( scaleFactor, m_SR );
        }
        
        /** Set the size of the overlapping window --> larger windows allow greater pitch shifts, but can introduce noticeable delay */
        void setWindowSize( const Sample windowSizeSecs )
        {
            
            m_windowSecs = windowSizeSecs > 0 ? windowSizeSecs : m_windowSecs;
            m_invWindowSecs = 1.0/m_windowSecs;
            m_windowSizeSamps = m_windowSecs * m_SR;
            setPitchScaling( m_scaleFactor );
        }
        
        /** Pitch shift the input signal. */
        Sample process( const Sample x )
        {
            m_delay.setSample( m_lpfIn.process( x, m_lpfInCoef ) );
            Sample outSamp{0}, delayed{0}, phase{m_phasor.process()};
            
            for ( auto i = 0; i < NVOICES; i++ )
            {
                phase += m_voiceOffset;
                phase = phase >= 1.0 ? phase-1.0 : phase;
                delayed = m_delay.getSample( phase * m_windowSizeSamps + m_dtSamps );
                outSamp += delayed + delayed * m_window.getVal( phase+0.5 );
            }
            return m_lpfOut.process(outSamp, m_lpfOutCoef);
        }
        
        /** Set the additional delay time */
        void setDelayTime( const Sample delayInSamps ) { m_dtSamps = delayInSamps > 0 ? delayInSamps : 0; }
        
        void setSetLPFCutOffs( const Sample inCutoff, const Sample outCutoff )
        {
            m_lpfInCoef = calculateLPFCoefficient<Sample>( inCutoff, m_SR);
            m_lpfOutCoef = calculateLPFCoefficient<Sample>( outCutoff, m_SR);
        }
        
    private:
        static constexpr Sample NVOICES{3}, LPFINCUTOFF{2000}, LPFOUTCUTOFF{5000};
        Sample m_SR{44100}, m_windowSecs{0.2}, m_windowSizeSamps{m_SR*m_windowSecs}, m_invWindowSecs{1/m_windowSecs}, m_scaleFactor{1}, m_voiceOffset{1.0/NVOICES}, m_lpfInCoef{ calculateLPFCoefficient<Sample>( LPFINCUTOFF, m_SR) }, m_lpfOutCoef{ calculateLPFCoefficient<Sample>( LPFOUTCUTOFF, m_SR) }, m_dtSamps{4410};
        
        delay< Sample, interpType > m_delay;
        oscillators::phasor< float > m_phasor{ 0, 44100 };
        filters::onepole< Sample > m_lpfIn, m_lpfOut;
        
        struct cosFunc{ const Sample operator()( Sample findex ) const { return gcem::cos<Sample>(findex*2.0*M_PI); } };
        wavetable::tab< Sample, 1024, cosFunc, interpolation::interpolatorTypes::linear > m_window;
    };
}

#endif /* sjf_pitchShift_h */
