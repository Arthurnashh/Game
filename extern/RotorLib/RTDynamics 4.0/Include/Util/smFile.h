/*
(c) Copyright 2004, MachSim
All Rights Reserved.

Product:		DinaPhysics

The content of this work is proprietary and confidential.Permission to use,copy,
publish and modify this software and its documentation for any commercial and
non-commercial purpose is not granted without written permission of MachSim
*/
#pragma once

#include <string>

#include <stdio.h>
#include <stdlib.h>

class smFile
{
public:
	smFile( ) {}
	~smFile() {}

	enum OpenPolicy
	{
		OPEN_TO_READ,
		OPEN_TO_WRITE
	};

	enum FileType
	{
		BINARY_FILE,
		ASCII_FILE
	};

	bool open( std::string fileName, OpenPolicy openPolicy = OPEN_TO_WRITE, FileType fileType = ASCII_FILE ) ;

	void writeLine( std::string str ) ;

	/**
	*	Reads the file into a string. Clears the inStr
	*	Max line length is 1024
	*	Doesn't reset file cursor
	*/
	void readAllToString( std::string& inStr ) ;

	bool getStatus()
	{ return mStatus ; }

private:
	FILE*	mFilePtr ;
	bool	mStatus ;
};

inline bool
smFile::open( std::string fileName, OpenPolicy openPolicy, FileType fileType )
{
	mStatus = false ;

	std::string openFlag ;

	switch( openPolicy )
	{
	case OPEN_TO_READ:
		openFlag += "r" ;
		break;
	case OPEN_TO_WRITE:
		openFlag += "w" ;
		break;
	}

	switch( fileType )
	{
	case BINARY_FILE:
		openFlag += "b" ;
		break;
	case ASCII_FILE:
		// Empty
		break;
	}

	mFilePtr = fopen( fileName.c_str(), openFlag.c_str() );

	if ( mFilePtr == NULL )
		mStatus = false ;
	else
		mStatus = true ;

	return mStatus ;
}

inline void
smFile::readAllToString( std::string& inStr )
{
	if ( mStatus == false )
		return ;

	inStr.clear() ;

	char str[1024];
	while( fgets(str, 1024, mFilePtr) != NULL )
	{
		inStr += std::string(str) + "\n" ;
	}
}

inline void
smFile::writeLine(std::string str)
{
	fprintf(mFilePtr,"%s", str.c_str());
}
