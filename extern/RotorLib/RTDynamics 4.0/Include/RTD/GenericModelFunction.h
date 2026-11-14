/*
(c) Copyright 2011, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "Common.h"
#include "ParamDb.h"

namespace RTD
{
    class GenericModel;
    class GenericModelFunction  : public SimulationVariableUser
    {
    public:
        struct DataChannel
        {
            DataChannel( const std::string& _alias ): alias(_alias) {}
            std::string alias;
        };


        GenericModelFunction( const std::string& name , GenericModel* owner );
        
        virtual void    update( smDouble timestep ) = 0;
        virtual bool    readParamStr( const std::string& parameterString , std::string& error );

        
    protected:
        struct ChannelDescription
        {
            ChannelDescription():onlyMembersOfVectors(false) {}

            std::vector<RTD::SimulationVariableBase::DataType>  acceptedDataTypes;
            bool                                                onlyMembersOfVectors;
        };

        struct DataChannelInternal
        {
            DataChannelInternal(): variable(NULL),memberIndex(-1){}
            RTD::SimulationVariableBase*    variable;
            int                             memberIndex;        
        };


        virtual bool    initializeInputAndOutputs( std::vector<DataChannel>& inputs , std::vector<DataChannel>& outputs , std::string& error );

        bool            isAccepted( const DataChannelInternal& dci , const std::vector< ChannelDescription >& desc , unsigned  int index , std::string& error  );
        bool            isAcceptedInput( const DataChannelInternal& dci ,unsigned  int index , std::string& error );
        bool            isAcceptedOutput( const DataChannelInternal& dci , unsigned int index  , std::string& error );




        std::vector<DataChannelInternal>   m_inputs;
        std::vector<DataChannelInternal>   m_outputs;



        std::vector< ChannelDescription > m_inputDescriptions;
        std::vector< ChannelDescription > m_outputDescriptions;

        GenericModel*   m_owner ;
        bool            m_correctlyInitialized;
    };
}