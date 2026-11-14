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


    class GenericModelTableLookUp : public GenericModelFunction
    {
    public:
        GenericModelTableLookUp( const std::string& name ,GenericModel* owner );
        virtual void    update( smDouble timestep);
        virtual bool    readParamStr( const std::string& parameterString , std::string& error );

        virtual bool    initialize( const std::string& tableName , std::string& error );
    protected:
        LookUpTable*    m_table;
    };

}