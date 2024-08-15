//
//  sjf_polyBLEP_OSC.h
//
//  Created by Simon Fay on 12/07/2024.
//



#ifndef sjf_polyBLEP_OSC_h
#define sjf_polyBLEP_OSC_h
#include "sjf_phasor.h"
#include "sjf_oscTypes.h"
namespace sjf::oscillators
{

/**
 polyBlep
 */
    template < typename Sample >
    class polyBLEP
    {
    public:
        polyBLEP( Sample frequency, Sample sampleRate ) : m_increment( frequency / sampleRate ){}
        ~polyBLEP(){}
        
        /** Set the internal frequency */
        void setFrequency( const Sample f, const Sample sampleRate )
        {
            m_increment = f / sampleRate;
        }
        
        /** Process the value of the band limited step at a particular phase ( 0 --> 1 ) */
        Sample process( Sample phase )
        {
            // start of phase
            if ( phase < m_increment )
            {
                auto relPos = phase / m_increment;
                return (relPos+relPos) -(relPos*relPos) - 1;
            }
            // end of phase
            else if( phase > (1 - m_increment) )
            {
                auto relPos = (phase - 1) / m_increment;
                return (relPos+relPos) + (relPos*relPos) + 1;
            }
            // otherwise just output 0
            return 0;
        }
    private:
        Sample m_increment;
    };

//================//================//================//================//================
//================//================//================//================//================
//================//================//================//================//================
//================//================//================//================//================
    template< typename Sample >
    class bandLimitedOscillator
    {
    public:
        
        bandLimitedOscillator() : m_pb( m_freq, m_SR ) /*, m_phasor( m_freq, m_SR )*/ { }
        ~bandLimitedOscillator(){}
        
        /** Set the oscillator type (when using the process() function */
        void setType( oscType type )
        {
            m_type = type;
        }
        
        /** set the internal frequency */
        void setFrequency( const Sample f, const Sample sampleRate )
        {
//            m_phasor.setFrequency( f, sampleRate );
            m_pb.setFrequency( f, sampleRate );
            m_integrator.setFrequency( f, sampleRate );
        }
        
        /** process the next value of the oscillator */
        Sample process( Sample phase )
        {
            Sample pb, pb1, pb2, sq;
//            auto phase = m_phasor.process();
            switch ( m_type ) {
                case oscType::sin:
                    phase *= 2.0 * M_PI;
                    return std::cos( phase );
                    break;
                case oscType::saw:
                    pb = m_pb.process( phase );
                    phase *= 2;
                    phase -= 1;
                    return phase - pb;
                    break;
                case oscType::square:
                    sq = phase < 0.5 ? 1 : -1;
                    pb1 = m_pb.process( phase );
                    phase += 0.5;
                    phase = phase < 1 ? phase : phase - 1;
                    pb2 = m_pb.process( phase );
                    return sq + pb1 - pb2;
                    break;
                case oscType::triangle:
                    sq = phase < 0.5 ? 1 : -1;
                    pb1 = m_pb.process( phase );
                    phase += 0.5;
                    phase = phase < 1 ? phase : phase - 1;
                    pb2 = m_pb.process( phase );
                    sq += pb1 - pb2;
                    return m_integrator.process( sq );
                    break;
                default:
                    break;
            }
        }
        
        /** process the next value of the sin oscillator */
        Sample processSin( Sample phase )
        {
//            auto p = m_phasor.process();
            phase *= 2.0 * M_PI;
            return std::cos( phase );
        }
        
        /** process the next value of the saw oscillator */
        Sample processSaw( Sample phase)
        {
//            auto p = m_phasor.process();
            auto pb = m_pb.process( phase );
            phase *= 2;
            phase -= 1;
            return phase - pb;
        }
        
        /** process the next value of the square oscillator */
        Sample processSquare( Sample phase )
        {
//            auto p = m_phasor.process();
            auto sq = phase < 0.5 ? 1 : -1;
            auto pb1 = m_pb.process( phase );
            phase += 0.5;
            phase = phase < 1 ? phase : phase - 1;
            auto pb2 = m_pb.process( phase );
            return sq + pb1 - pb2;
        }
        
        /** process the next value of the triangle oscillator */
        Sample processTriangle( Sample phase )
        {
            return m_integrator.process( processSquare( phase ) );
        }
        
    private:
        
        class leakyIntegrator
        {
        public:
            void setFrequency( Sample f, Sample sampleRate )
            {
                m_coef = 2 * M_PI * f / sampleRate;
            }
            
            Sample process( Sample x )
            {
                m_y1 += m_coef*( x - m_y1 );
                return m_y1;
            }
            
        private:
            Sample m_coef, m_y1{0};
        };
        Sample m_SR{44100}, m_freq{100};
        polyBLEP< Sample > m_pb;
//        phasor< Sample > m_phasor;
        leakyIntegrator m_integrator;
        oscType m_type = oscType::saw;
    };

}


#endif /* sjf_polyBLEP_OSC_h */



