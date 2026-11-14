/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once


#pragma warning(disable : 4786)
#include <string>
#include <vector>

// Inclusion of stdio is needed for gcc 2.95.3
#include <stdio.h>

class smDirectory
{
public:
	/**
	*	Returns a vector containing the files in a std::vector.
	*
	*/
	static std::vector<std::string> getFiles( std::string pattern, bool skipDebugDlls )	;
};

/**
*	This class provides utility functionality
*	for dealing with file paths ( can be a file or a directory ).
*/
class smFilePath
{
public:
	/**	Checks if the file name ends with "-d.dll"	*/
	static bool isDebugDLL( std::string filePath ) ;

	static void			removeTrailingSlash	( std::string& filePath ) ;
	static void			removeExtension		( std::string& filePath ) ;
	static void			removeQuotesIfAny	( std::string& filePath );
	static std::string	getExtension		( const std::string& filePath ) ;
	static std::string	getFile				( const std::string& filePath ) ;
	static bool			doesExist			( const std::string& filePath ) ;
	static bool			isRelative			( const std::string& filePath ) ;

	/**	Converts a filename with windows style slashes to unix slashes	*/
	static void			convertToUnixSlashes( std::string& filePath ) ;

	/**
	*	Returns the directory of this executable.
	*
	*	argv0 is argv[0]
	*
	*	If you would like to maintain portability, you shall also provide argv0
	*	to getAppDir. Windows users do not need to provide argv0
	*
	*/
	static std::string	getAppDir			( bool addBackSlash = true, std::string argv0 = "" ) ;
};
