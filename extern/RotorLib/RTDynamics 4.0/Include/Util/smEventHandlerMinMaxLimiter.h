/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RotorLib
Subsystem:		RLDynamics

The content of this work is propriatery and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "Physics/smIntegralType.h"

/**
*	smEventHandlerMinMaxLimiter checks the value of the type with its limits and
*	trims the value.
*/
class smEventHandlerMinMaxLimiter : public smIntegralType::EventHandler
{
    public:		//	Methods
	explicit smEventHandlerMinMaxLimiter( smPrinter* pr ) : m_printer( pr ) {} ;


    virtual void	handleEvent ( smEventGeneratorTrait* data, smEventGeneratorTrait::EventType type )
    {
		if ( smIntegralType* obj = dynamic_cast<smIntegralType*>( data ) )
		{
			if ( type == smEventGeneratorTrait::VALUE_CHANGED )
			{
				if ( obj->val() > obj->getMax() )
				{
					m_printer->debug("smIntegralType %s trimmed to its max value=%.2f, ignoring new value=%.2f", obj->getName().c_str(), obj->getMax(), obj->val() );

					obj->setVal( obj->getMax() ) ;
				}
				else if ( obj->val() < obj->getMin() )
				{
					m_printer->debug("smIntegralType %s trimmed to its min value=%.2f, ignoring new value=%.2f", obj->getName().c_str(), obj->getMin(), obj->val() );

					obj->setVal( obj->getMin() ) ;
				}
			}
		}
    } ;

protected:	//	Data
	smPrinter*  m_printer ;
};

