//
//  sjf_parameterHandler.h
//  sjf_verb
//
//  Created by Simon Fay on 28/04/2024.
//

#ifndef sjf_parameterHandler_h
#define sjf_parameterHandler_h

#include "sjf_dataStructures.h"
#include <JuceHeader.h>

namespace sjf::parameterHandler {
    class paramHandlerVector
    {
    public:
        paramHandlerVector(){} // default constructor so that you could add only the parameters desired using method below
        
        ~paramHandlerVector( ){}
        
        void addParameter( juce::AudioProcessorValueTreeState& vts, juce::AudioProcessorParameter* parameterPtr, std::function< void(float) > audioThreadCallback )
        {
            auto parameterID = static_cast< juce::AudioProcessorParameterWithID* >(parameterPtr)->getParameterID();
            auto nP = std::make_unique< param > ( parameterPtr, vts.getRawParameterValue( parameterID ), m_parentCallback, audioThreadCallback );
            vts.addParameterListener( parameterID, nP.get() );
            m_params.push_back( std::move( nP )  );
        }
        
        /** Call this on the audio thread to trigger all of the pending parameter updates */
        void triggerCallbacks()
        {
            if( m_list.isBusy.test_and_set() ){ DBG( "LIST WAS BUSY!!!! Parameters will be set next time it is free"); return; } // Don't WAIT, do everything next block
            auto n = m_list.popNode();
            while( n != nullptr )
            {
                n->triggerCallBack();
                n = m_list.popNode();
            }
            m_list.isBusy.clear();
        }
        
        auto begin() const{ return m_params.begin(); }
        auto end() const{ return m_params.end(); }
        
        auto size(){ return m_params.size(); }
        
        auto operator[]( size_t index ){ return &m_params[ index ]; }
        
    private:
        //======================//======================//======================//======================
        //======================//======================//======================//======================
        //======================//======================//======================//======================
        //======================//======================//======================//======================
        class param : public juce::AudioProcessorValueTreeState::Listener
        {
        public:
            param(
                    juce::AudioProcessorParameter* parameterPtr,
                    std::atomic<float>* rawParameterValue,
                    std::function< void(param*) > parentCallback ,
                    std::function< void(float) > audioThreadCallback ) :
                        m_parameterPtr( parameterPtr ),
                        m_val( rawParameterValue ),
                        m_parentCallback( parentCallback ),
                        m_callback( audioThreadCallback )
                { }
            
            void triggerCallBack()
            {
                if( m_callback )
                    m_callback( m_val->load() );
            }
            
        private:
            void parameterChanged (const juce::String& parameterID, float newValue ) override
            {
                if( m_parentCallback )
                    m_parentCallback( this );
            }
            
            const juce::AudioProcessorParameter* m_parameterPtr = nullptr;
            const std::atomic< float >* m_val = nullptr;
            const std::function<void(param*)> m_parentCallback = nullptr;
            const std::function<void(float)> m_callback = nullptr;
        };
        //======================//======================//======================//======================
        //======================//======================//======================//======================
        //======================//======================//======================//======================
        //======================//======================//======================//======================
        
        
        std::vector< std::unique_ptr< param > > m_params;
        sjf::dataStructures::linkedList< param > m_list;
        
        std::function< void(param*) > m_parentCallback = [ this ]
                                                        ( param* paramToAdd )
                                                        {
                                                            while( m_list.isBusy.test_and_set() ){
                                                                DBG("WAITING FOR LOCK TO FREE before adding to node");
                                                            } // WAIT
                                                            m_list.addNode( paramToAdd );
                                                            m_list.isBusy.clear();
                                                        };
    };
}



#endif /* sjf_parameterHandler_h */
