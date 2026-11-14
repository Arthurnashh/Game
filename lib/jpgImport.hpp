#ifdef _MSC_VER
#pragma once
#endif

#ifndef JPG_IMPORT_HPP
#define JPG_IMPORT_HPP

#include "pactext.hpp"
#if defined(_WIN32) 
#include <ijl.h>
#endif

//! Load texture from JPEG file

class TextureSourceJPEG: public ITextureSource
{
	#if defined(_WIN32) 
		mutable JPEG_CORE_PROPERTIES _prop;
	#endif
	bool _propInit;
  AutoArray<unsigned char> _data;

	PacFormat _format; //!< source file pixel format
	RStringB _name;
	QFileTime _timestamp;

	public:
	TextureSourceJPEG();
	~TextureSourceJPEG();

  //! Virtual method
  virtual TextureType GetTextureType() const {return TT_Diffuse;};
  virtual TexFilter GetMaxFilter() const {return TFAnizotropic16;};
  virtual float GetMaxTexDetail(float maxTexDetail) const {return maxTexDetail;};

  void SetName(const char *name);
	bool Init(PacLevelMem *mips, int maxMips);

	int GetMipmapCount() const;
	PacFormat GetFormat() const;
	bool GetMipmapData(void *mem, const PacLevelMem &mip, int level, int texSize) const;	
  bool RequestMipmapData(
    const PacLevelMem *mips, int levelMin, int levelMax, FileRequestPriority prior
  ) const;  
  bool IsLittleEndian() const
  {
    // we assume JPG decompression decompresses into native endian format
    return IsPlatformLittleEndian();
  }

	PackedColor GetAverageColor() const {return PackedBlack;}
	PackedColor GetMaxColor() const {return PackedWhite;}

	bool IsAlpha() const {return false;}
	bool IsAlphaNonOpaque() const {return false;}
	bool IsTransparent() const {return false;}
	void ForceAlpha() {}
  int GetClamp() const {return 0;}
  void SetClamp(int clampFlags) {}
  QFileTime GetTimestamp() const {return _timestamp;}
  virtual void FlushHandles() {}

	bool InitProp();
	void FreeProp();
};

//! all routines required to create JPEG texture
class TextureSourceJPEGFactory: public ITextureSourceFactory
{
	public:
	bool Check(const char *name);
	bool PreInit(
    const char *name,
    RequestableObject *obj=NULL, RequestableObject::RequestContext *ctx=NULL
  );
	ITextureSource *Create(const char *name, PacLevelMem *mips, int maxMips);
  TextureType GetTextureType(const char *name) {return TT_Diffuse;}
  TexFilter GetMaxFilter(const char *name) {return TFAnizotropic16;}
};

extern TextureSourceJPEGFactory *GTextureSourceJPEGFactory;

#endif
