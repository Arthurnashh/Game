/*
(c) Copyright 2007, RTDynamics
All Rights Reserved.

Product:        RTDynamics


The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of RTDynamics
*/
#pragma once

#include "RTD/Common.h"

namespace RTD
{
	RTDYNAMICS_EXPORT std::string makeString( const char *format, ... );

	// Replaces the characters in the given string.
	RTDYNAMICS_EXPORT void replaceChars( std::string& str, char toFind, char toPut) ;

	// Prints the strings to stdout
	RTDYNAMICS_EXPORT void printStringList( const std::vector<std::string>& strList ) ;

	// Splits a / or \ separated string into a string vector.
	RTDYNAMICS_EXPORT std::vector<std::string> splitPathString( std::string str );

	// Runs Unit test for splitPathString function.
	RTDYNAMICS_EXPORT void splitPathString_unitTest(); 
}

