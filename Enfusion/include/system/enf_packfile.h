//-----------------------------------------------------------------------------
// File: enf_packfile.cpp
//
// Desc: Virtual filesystem
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_PACKFILE_H
#define ENF_PACKFILE_H

#include "common/enf_inttypes.h"
#include "enf_ifilesystem.h"

namespace enf
{
enum DPackEntryType
{
	PE_DIR,
	PE_FILE
};


//ENF_MAKEFOURCC('P', 'A', 'C', '1');
#pragma pack(push,1)
struct DPackFileHeader100
{
	uint		Version;
	uint		CRC;
	char		Key[16];
	uint		ProductID;

	ushort	Year;
	ubyte		Month;
	ubyte		Day;
	ubyte		Hour;
	ubyte		Minute;
	ubyte		Second;
};

struct DPackEntry
{
	ubyte Type;
	ubyte	NameLength;
	//char name[NameLength]
};

struct DPackFile
{
	uint		Offset;
	uint		CompressedLength;
	uint		UncompressedLength;
	uint		CRC;
	ushort	Flags;
	ubyte		CompressionType;
	ubyte		CompressionLevel;
};

struct DPackDirectory
{
	uint	NumFiles;
	//DPackFile[NumFiles]
};
#pragma pack(pop)

/*
[PAC1]
	PackFileHeader100
[/PAC1]
[DATA]
[/DATA]
[HEAD]
	DPackEntry[NumDirectories+NumFiles]
[/HEAD]
*/

}

namespace pak
{
PUBLIC_API enf::PackFile* Open(enf::VFile* file);

}

#endif //ENF_PACKFILE_H
