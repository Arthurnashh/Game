/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once



#include "Util/smPrinter.h"
#include "Physics/smIntegralType.h"

/**
*   A event handler which automatically prints the value of a
*   smIntegralType when value changes
*/
class smEventHandlerAutoPrint : public smIntegralType::EventHandler
{
    public:		//	Data types

    protected: 	//	Data Types

    public:		//	Methods
    explicit smEventHandlerAutoPrint( smPrinter* pr ) : m_printer( pr ) {} ;

    virtual void	handleEvent ( smEventGeneratorTrait* data, smEventGeneratorTrait::EventType type )
    {
        //fprintf( stderr, "This is handleEvent of smEventHandlerAutoPrint 0x%x\n", this ) ;
        if ( smIntegralType* integral = dynamic_cast<smIntegralType*>( data ) )
        {
            //fprintf( stderr, "For smIntegralType '%s' event type:%d\n", integral->getName().c_str(), type ) ;

            if ( type == smEventGeneratorTrait::VALUE_CHANGED )
            {
                //fprintf( stderr, "For smIntegralType '%s' VALUE_CHANGED\n", integral->getName().c_str() ) ;
                m_printer->debug("smIntegralType %s=%.2f\n", integral->getName().c_str(), integral->val() );
            }
        }
    } ;

    protected:	//	Data
    smPrinter*  m_printer ;
};

