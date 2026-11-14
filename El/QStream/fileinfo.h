#ifndef _FILEINFO_H
#define _FILEINFO_H

#define NAME_LEN 128-20

#ifdef __GNUC__

#define EncrMagic    StrToInt("rcnE")
#define CompMagic    StrToInt("srpC")
#define VersionMagic StrToInt("sreV")

#else

#define EncrMagic 'Encr'
#define CompMagic 'Cprs'
#define VersionMagic 'Vers'

#endif

struct FileInfo
{
	char name[NAME_LEN];
	int compressedMagic;
	int uncompressedSize;
	long startOffset;
	long time;
	long length;

	const char *GetKey() const {return name;}
};

#endif
