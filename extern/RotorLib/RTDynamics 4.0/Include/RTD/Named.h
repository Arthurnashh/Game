/*
(c) Copyright 2004, RTDynamics
All Rights Reserved.

Product:		RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include <string>
#include "RTD/Common.h"

namespace RTD
{
	/**
	*	Class to manage a class with a name. Use this class as a trait.
	*/
	class RTDYNAMICS_EXPORT Named
	{
	public:
		explicit Named() ;
		explicit Named( const std::string& name ) ;

		virtual void setName( const std::string& name ) ;

		virtual const std::string& getName() const ;

	private:

		// Keep m_name under private for better prevention against bugs
		std::string m_name ;

	};
}