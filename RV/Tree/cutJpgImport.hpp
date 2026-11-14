#ifdef _MSC_VER
#pragma once
#endif

#ifndef JPG_IMPORT_HPP
#define JPG_IMPORT_HPP

#include "cutPactext.hpp"
#if defined _WIN32 && !defined _XBOX
#include <ijl.h>
#endif

//! Load texture from JPEG file

class TextureSourceJPEG: public ITextureSource
{
	#if defined _WIN32 && !defined _XBOX
		mutable JPEG_CORE_PROPERTIES _prop;
	#endif
	bool _propInit;

	int _mipmaps; //!< source file mipmap count
	PacFormat _format; //!< source file pixel format
	RStringB _name;

	public:
	TextureSourceJPEG();
	~TextureSourceJPEG();

	bool Init(const char *name, PacLevelMem *mips, int maxMips);

	int GetMipmapCount() const;
	PacFormat GetFormat() const;
	bool GetMipmapData(void *mem, const PacLevelMem &mip, int level) const;	

	PackedColor GetAverageColor() const {return PackedBlack;}

	bool IsAlpha() const {return false;}
	bool IsTransparent() const {return false;}
	void ForceAlpha() {}

	bool InitProp();
	void FreeProp();
};

//! all routines required to create JPEG texture
class TextureSourceJPEGFactory: public ITextureSourceFactory
{
	public:
	bool Check(const char *name);
	void PreInit(const char *name);
	ITextureSource *Create(const char *name, PacLevelMem *mips, int maxMips);
};

extern TextureSourceJPEGFactory *GTextureSourceJPEGFactory;

#endif
