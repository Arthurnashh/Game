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
    

    class GenericModelCurveLookUp : public GenericModelFunction
    {
    public:
        GenericModelCurveLookUp( const std::string& name ,GenericModel* owner );
        virtual void    update( smDouble timestep );
        virtual bool    readParamStr( const std::string& parameterString , std::string& error );

        virtual bool    initialize( const std::string& curveName , std::string& error );
    protected:
        smGraph2D*    m_graph;
    };
}