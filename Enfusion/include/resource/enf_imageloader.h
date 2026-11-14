//-----------------------------------------------------------------------------
// File: enf_imageloader.h
//
// Desc: Image loaders
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_IMAGELOADER_H
#define ENF_IMAGELOADER_H

#include "enf_config.h"
#include "common/enf_types.h"
#include "system/enf_memory.h"
#include "Render/enf_renderenums.h"
#include "Render/enf_rendertypes.h"


namespace enf
{
class CInputStream;
enum TextureFormat;
enum TextureUsage;


/*!
enable exporting in platform byte order -> the surfaces have already correct 
format in texture files. Otherwise, the surfaces are always in little endian
and for XBOX360 the order must be swapped during the load
*/
#define EXPORT_IN_PLATFORM_ORDER		


/*!
enable compile with LZX compression - currently, includes and libs are taken from XBOX
source directories, for real use, copy them into EnforceNG/extern 
*/
//#define LZX_COMPRESSION


/** @addtogroup Resources
@{*/


/*!
description for compression
*/
struct CompressedDDSDesc 
{
	enum CompFormat
	{
		CF_NONE,
		CF_ZIP,
		CF_LZO,
		#ifdef LZX_COMPRESSION
			CF_LZX
		#endif
	};

	CompFormat m_Format;				///< format of compression
	uint		  m_iLevel;				///< level, 0 = no compression, 1..0 for zlib, for lzo no role
	float		  m_fRatio;				///< ratio between compressed data and not compressed, if ratio is less, the file is saved
	uint		  m_iMaxMipmap;		///< max available mipmaps
	bool		  m_bToBigEndian;		///< save in big endian format-> XBOX360 (not PS3!)
	
	CompressedDDSDesc () : m_Format(CF_NONE), m_iLevel(0), m_fRatio(2.0f), m_iMaxMipmap(0xFFFFFFFF), m_bToBigEndian (false)
	{}

	/*!
	user compression
	*/	
	CompressedDDSDesc (CompFormat format, float ratio = 2.0f, uint level = 6, uint maxmip = 0xFFFFFFFF, bool bigendian = false) : m_Format(format), m_iLevel(level), m_fRatio(ratio), m_iMaxMipmap(maxmip), m_bToBigEndian(bigendian)
	{}
};


//!Information about source data, for ImageFormatLoader
struct ImageInfo
{
	uint					Width;
	uint					Height;
	uint					Depth;
	uint					MipLevels;
	TextureFormat		Format;
	bool					Compressed;
	bool					Cube;
	ImageInfo(): Width(0), Height(0), Depth(0), MipLevels(0), Format((TextureFormat)0), Compressed(false), Cube(false) {}
};

/*!
Target container for ImageFormatLoader
and usualy source for a Texture
*/
class PUBLIC_API ImageData
{
public:
	ENF_DECLARE_SMALLOBJECT(ImageData);

	ImageData() {}
	virtual ~ImageData() {};
	virtual void Free()=0;
	virtual size_t GetDataSize() const=0;
	virtual uint GetNumLevels() const=0;
	virtual ERESULT Create(uint width, uint height, uint levels, TextureFormat format, uint flags, TextureUsage usage)=0;
	virtual bool Lock(uint face, uint level, LockedImageRect& info)=0;
	virtual void Unlock(uint face, uint level)=0;
	virtual void Finnish()=0;
	virtual TextureFormat GetFormat() const=0;
	virtual uint GetWidth() const=0;
	virtual uint GetHeight() const=0;
	virtual uint GetNumFaces() const=0;
};

/*!
Implementation of ImageData,
data are kept in memory buffers
*/
class PUBLIC_API MemoryImageData: public ImageData
{
protected:
	TextureFormat	m_Format;
	ushort			m_iWidth, m_iHeight;
	ushort			m_iLevels;
	ushort			m_iNumFaces;
	void*				m_pData[6];
	uint				m_iUsedMipmaps[6];
	uint				m_iMipOffsets[MAX_TEXTURE_MIPMAPS - 1];
	size_t			m_iDataSize;

public:
	ENF_DECLARE_SMALLOBJECT(MemoryImageData);

	ENF_INLINE TextureFormat GetFormat() const
	{
		return m_Format;
	}
	ENF_INLINE uint GetWidth() const
	{
		return m_iWidth;
	}
	ENF_INLINE uint GetHeight() const
	{
		return m_iHeight;
	}
	ENF_INLINE uint GetNumFaces() const
	{
		return m_iNumFaces;
	}

	MemoryImageData();
	virtual ~MemoryImageData();
	virtual void Free();

	uint GetNumLevels() const
	{
		return m_iLevels;
	}
	size_t GetDataSize() const;
	virtual ERESULT Create(uint width, uint height, uint levels, TextureFormat format, uint flags, TextureUsage usage);
	bool Lock(uint face, uint level, LockedImageRect& info);
	void Unlock(uint face, uint level);
	void Finnish();
};



class MemoryBuffer;

/*!
memory image data, used when creating from memory buffer
*/
class PUBLIC_API MemoryBufferImageData : public MemoryImageData
{
protected:
	ERESULT Create(uint width, uint height, uint levels, TextureFormat format, uint flags, TextureUsage usage)
	{
		return EE_UNSUPPORTED;
	}

public:
	MemoryBufferImageData(): MemoryImageData()
	{}

	~MemoryBufferImageData()
	{
		Free();
	}

	void Free();
	ERESULT Create(uint width, uint height, uint levels, TextureFormat format, uint flags, TextureUsage usage, const MemoryBuffer& data, uint offset);
};


/*!
Image loader. Source is CInputStream,
target is ImageData
*/
class ImageFormatLoader
{
protected:

public:
	ENF_DECLARE_SMALLOBJECT(ImageFormatLoader);

	virtual ~ImageFormatLoader() {}
	ImageFormatLoader() {}
	virtual ERESULT Load(CInputStream& instream, const ImageInfo& imginfo, uint miplevels, uint flags, uint maxsize, ImageData& textureData)=0;
};

/*!
Handler of image format. Identifies format of source
data and creates a ImageFormatLoader for it.
*/
class ImageLoaderHandler
{
public:
	ENF_DECLARE_SMALLOBJECT(ImageLoaderHandler);

	virtual ~ImageLoaderHandler() {}
	ImageLoaderHandler() {}
	virtual ERESULT GetLoader(CInputStream& instream, ImageFormatLoader** loader, ImageInfo& info)=0;

	virtual const char* GetExtension() const
	{
		return NULL;
	}
};

/*!
Manager of image loaders
*/
class PUBLIC_API ImageLoader
{
protected:
	const char** m_sExtString;
	virtual void UpdateExtensionsString() = 0;

public:
	ENF_DECLARE_SMALLOBJECT(ImageLoader);

	ImageLoader(): m_sExtString(NULL)
	{}

	virtual ~ImageLoader() 
	{
		SAFE_DELETE_ARRAY(m_sExtString);
	}

	//!Registers handler of loader
	virtual void RegisterHandler(ImageLoaderHandler* handler)=0;
	//!Unregisters handler of loader
	virtual void UnregisterHandler(ImageLoaderHandler* handler)=0;
	/*!
	Loads image
	\param instream Source data
	\param imginfo Information about data
	\param minMipLevels Minimum mipmap (the biggest one)
	\param maxMiplevels Either 0 for default mip-chain, or exact number of mipmaps
	\param flags Texture flags See TextureFlags
	\param data Target data container
	*/
	virtual ERESULT Load(CInputStream& instream, ImageInfo& imginfo, uint minMipLevels, uint maxMiplevels, uint flags, ImageData& data)=0;

	/*!
	array of loader extensions, ended with NULL
	*/
	const char** GetExtensionsStringArray() const
	{
		return m_sExtString;
	}
};

/*!
Creates default implementation of manager,
and registers built-in DDS&TGA loaders
*/
PUBLIC_API ImageLoader* CreateImageLoader();

struct XSURFACEDESC;
ERESULT PUBLIC_API GetDDSImageInfo(const XSURFACEDESC &desc, ImageInfo& info);

//@}
}

#endif //ENF_IMAGELOADER_H