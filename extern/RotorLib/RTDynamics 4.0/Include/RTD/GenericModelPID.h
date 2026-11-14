/*
(c) Copyright 2011, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/

#pragma once

#include "GenericModelFunction.h"
#include "Physics/smGraph2D.h"

namespace RTD
{
    class PIDController;

    class GenericModelPID : public GenericModelFunction
    {
    public:
        GenericModelPID( const std::string& name ,GenericModel* owner );
        virtual ~GenericModelPID();

        virtual void    update( smDouble timestep );
        virtual bool    readParamStr( const std::string& parameterString , std::string& error );

        virtual bool    initialize( smReal P = 0.0f , smReal I = 0.0f , smReal D = 0.0f ,
                                    smReal offset = 0.0f ,
                                    smReal integralMin = -FLT_MAX,
                                    smReal integralMax = FLT_MAX,
                                    smReal proportionalOutputMin = -FLT_MAX,
                                    smReal proportionalOutputMax = FLT_MAX,
                                    smReal derivativeOutputMin = -FLT_MAX,
                                    smReal derivativeOutputMax = FLT_MAX );

        virtual void    reset();
    protected:
        PIDController*    m_pid;
    };
}