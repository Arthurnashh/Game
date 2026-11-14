#ifdef _MSC_VER
#pragma once
#endif

#ifndef _CUTPACTEXT_HPP
#define _CUTPACTEXT_HPP

// basic structure for storing bitmap data
#include <Es/Types/memtype.h>
#include "cutTypes.hpp"
#include "cutHeap.hpp"
#include <El/QStream/QStream.hpp>
#include <El/Color/colors.hpp>

#include <Es/Containers/boolArray.hpp>

// provides standard alocator and deallocator

#if !_RELEASE
	#define DO_FILLS 0 // fill on Alloc and Free
#else
	#define DO_FILLS 0 // fill on Alloc and Free
#endif



class MemoryHeap: public Heap<byte *,int>
{
	#if DO_FILLS
		#define NEW_FILL 0xfffdfefc // it can be decoded as 1555 ARGB
		#define DEL_FILL 0x02010301 // it can be decoded as 1555 ARGB

		public:
		HeapItem *Alloc( int size );
		void Free( HeapItem *pos );
	#endif
};

typedef Heap<byte *,int>::HeapItem MemoryItem;

// manage texture heap



class SystemHeap: public MemoryHeap
{
	private:
	byte *_alocated;
	int _size;

	public:
	SystemHeap();
	void Release();
	~SystemHeap();
	void Init( int size );
	void *Base() const {return Memory();}
	bool Check() const;
};


enum PacFormat
{
	PacP8,PacAI88,PacRGB565,PacARGB1555,PacARGB4444,
	PacARGB8888,
	PacDXT1,PacDXT2,PacDXT3,PacDXT4,PacDXT5,
	PacFormatN
};

class PacPalette;

inline int fgetiw( QIStream &f )
{
	int c=f.get(),r=f.get();
	return (r<<8)|c;
}
inline int fgeti24( QIStream &f )
{
	int c=f.get(),r=f.get();
	c|=r<<8;
	r=f.get();
	c|=r<<16;
	return c;
}
inline int fgetil( QIStream &f )
{
	int c=f.get(),r=f.get();
	c|=r<<8;
	r=f.get();
	c|=r<<16;
	r=f.get();
	c|=r<<24;
	return c;
}

#if _RELEASE
	#define CHECKSUMS 0
#else
	#define CHECKSUMS 0
#endif

class PacLevelMem
{
	public:
	// short for texture dimensions and pitch should be enough
	short _w,_h; // note: 
	short _pitch; // DDraw requires DWORD (or QWORD) alignement

	SizedEnum<PacFormat,char> _sFormat; // source (file) format
	SizedEnum<PacFormat,char> _dFormat; // destination (memory) format

	int _start; // where in the file this mipmap level start

	// note: _memData is often used as temporary pointer
	//void *_memData; // _data for one mipmap levels


	public:
	PacLevelMem();

	PackedColor GetPixel(void *data, float u, float v) const;
	PackedColor GetPixelInt(void *data, int u, int v) const;

	static void DecompressDXT1(void *dst, const void *src, int w, int h);

	protected:

	// paa 16b formats loading
	int LoadPaaBin16( QIStream &in, void *mem, const PacPalette *pal ) const;
	int LoadPaaDXT( QIStream &in, void *mem, const PacPalette *pal ) const;


	// pac formats loading
	int LoadPacARGB1555( QIStream &in, void *mem, const PacPalette *pal ) const;
	int LoadPacRGB565( QIStream &in, void *mem, const PacPalette *pal ) const;
	int LoadPacP8( QIStream &in, void *mem, const PacPalette *pal ) const;
	
	public:
	void Interpolate
	(
		void *data, void *withData,
		const PacLevelMem &with, float factor
	);

	int LoadPac( QIStream &in, void *mem, const PacPalette *pal ) const; // format set by init

	int LoadPaa( QIStream &in, void *mem, const PacPalette *pal ) const; // format set by init

	void SetStart( int offset ){_start=offset;}
	int Init(QIStream &in, PacFormat sFormat);
	void SetDestFormat(PacFormat dFormat, int align);
	//int Skip( QIStream &in );
	void SeekLevel( QIStream &in ) const;
	PacFormat SrcFormat() const {return _sFormat;}
	PacFormat DstFormat() const {return _dFormat;}
	int Pitch() const {return _pitch;}
	int Size() const {return _pitch*_h;}
	bool TooLarge( int max=256 ) const {return _w>max || _h>max;}
	//bool Check( int max ) const;
	//bool Check( const PacPalette &pal ) const;
	// disable copy constructor
	//NoCopy(PacLevelMem)
};

PacFormat PacFormatFromDesc( int desc, bool &alpha );

class MipInfo
{
	public:
	Texture *_texture;
	int _level;

	MipInfo( Texture *texture, int level )
	:_texture(texture),_level(level)
	{}
	MipInfo()
	:_texture(NULL),_level(-1)
	{}

	bool IsOK() const {return _level>=0;}
};

TypeIsBinary(MipInfo);


#define TRANSPARENT1_RGB 0xff00ff
#define TRANSPARENT2_RGB 0x00ffff

class PacPalette
{
	//friend class PacLevel;

	public:
	//MemoryHeap *_heap;
	int _nColors; // palette size
	DWORD *_palette; // 24-bit RGB format
	//Pixel *_palPixel; // pixel-format palette (suitable for copy mode)

	Color _averageColor;
	PackedColor _averageColor32;

	// this information is same for all mipmaps, but is also copied in mipmap header
	int _transparentColor;
	bool _isAlpha,_isTransparent;

	#if CHECKSUMS
		mutable	bool _checksumValid;
		mutable	int _checksum;
	#endif

	public:
	PacPalette();
	~PacPalette();

	int Load( QIStream &in, int *startOffsets, int maxStartOffsets );
	static int Skip( QIStream &in );

	ColorVal AverageColor() const {return _averageColor;}
	PackedColor AverageColor32() const {return _averageColor32;}

	NoCopy(PacPalette)
};

//! Texture source interface

class ITextureSource
{
	public:
	virtual ~ITextureSource() {}

	//! Init data source
	/*! Must be called before any other methods.*/
	virtual bool Init(const char *name, PacLevelMem *mips, int maxMips) = 0;
	//! Get mipmap count.
	virtual int GetMipmapCount() const = 0;
	//! Get source data format.
	virtual PacFormat GetFormat() const = 0;

	//! Check if texture should be alpha blended
	virtual bool IsAlpha() const = 0;
	//! Check if texture should be alpha tested
	virtual bool IsTransparent() const = 0;
	//! Get mipmap data.
	virtual bool GetMipmapData(void *mem, const PacLevelMem &mip, int level) const = 0;	

	//! Get average color (packed format)
	virtual PackedColor GetAverageColor() const = 0;

	//! We know from some reason texture is alpha blended
	virtual void ForceAlpha() = 0;
};

//! all routines required to create texture
class ITextureSourceFactory
{
	public:
	//! quick check if texture source can exist
	virtual bool Check(const char *name) = NULL;
	//! preload any files as necessary so that Create will be quick.
	/*!
	In typical implementation data source asks file server to preload any files as necessary.
	Calling this function before Create is optional.
	*/
	virtual void PreInit(const char *name) = NULL;
	//! create texture source
	virtual ITextureSource *Create(const char *name, PacLevelMem *mips, int maxMips) = NULL;
};

//! create texture source based on identifier (file name)
ITextureSourceFactory *SelectTextureSourceFactory(const char *name);

//bool CheckTextureSource(const char *name);

/*
inline bool PacLevelMem::Check( const PacPalette &pal ) const
{
	return Check(pal._nColors);
}
*/


#endif

