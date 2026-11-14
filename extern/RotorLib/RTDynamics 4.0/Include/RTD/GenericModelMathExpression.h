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
#include "ParamDb.h"

namespace RTD
{

    
    class GenericModelMathExpression : public GenericModelFunction
    {
    public:
        class Parser;

        GenericModelMathExpression( const std::string& name ,GenericModel* owner );
        virtual ~GenericModelMathExpression();
        virtual void    update( smDouble timestep );
        virtual bool    readParamStr( const std::string& parameterString , std::string& error );
        virtual bool    initialize( const std::string& expression , std::string& error );


    protected:

        Parser*  m_parser;

        static  smDouble clamp( smDouble,smDouble,smDouble);
        static  smDouble linearInterp( smDouble,smDouble,smDouble);
        static  smDouble radToDeg( smDouble );
        static  smDouble degToRad( smDouble );

    };

}
