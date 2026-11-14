//-----------------------------------------------------------------------------
// File: enf_ddsformat.h
//
// Desc: DDS file format definition
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_DDSFORMAT_H
#define ENF_DDSFORMAT_H

#include "enf_config.h"

namespace enf
{
/** @addtogroup Renderer
@{*/
#define XDDSD_CAPS 0x00000001 
#define XDDSD_HEIGHT 0x00000002 
#define XDDSD_WIDTH 0x00000004 
#define XDDSD_PITCH 0x00000008 
#define XDDSD_PIXELFORMAT 0x00001000 
#define XDDSD_MIPMAPCOUNT 0x00020000 
#define XDDSD_LINEARSIZE 0x00080000 
#define XDDSD_DEPTH 0x00800000 

#define XDDPF_ALPHAPIXELS 0x00000001 
#define XDDPF_FOURCC 0x00000004 
#define XDDPF_RGB 0x00000040 
#define XDDPF_LUMINANCE	0x00020000

#define XDDSCAPS_COMPLEX 0x00000008 
#define XDDSCAPS_TEXTURE 0x00001000 
#define XDDSCAPS_MIPMAP 0x00400000 

#define XDDSCAPS2_CUBEMAP                        0x00000200L
#define XDDSCAPS2_CUBEMAP_POSITIVEX 0x00000400 
#define XDDSCAPS2_CUBEMAP_NEGATIVEX 0x00000800 
#define XDDSCAPS2_CUBEMAP_POSITIVEY 0x00001000 
#define XDDSCAPS2_CUBEMAP_NEGATIVEY 0x00002000 
#define XDDSCAPS2_CUBEMAP_POSITIVEZ 0x00004000 
#define XDDSCAPS2_CUBEMAP_NEGATIVEZ 0x00008000 
#define XDDSCAPS2_VOLUME 0x00200000 

#ifndef DUMMYUNIONNAMEN
#if defined(__cplusplus) || !defined(NONAMELESSUNION)
#define DUMMYUNIONNAMEN(n)
#else
#define DUMMYUNIONNAMEN(n)      u##n
#endif
#endif

struct XSCAPS2
{
    uint       dwCaps;         // capabilities of surface wanted
    uint       dwCaps2;
    uint       dwCaps3;
    union
    {
        uint       dwCaps4;
        uint       dwVolumeDepth;
    } DUMMYUNIONNAMEN(1);
};

struct XCOLORKEY
{
    uint       dwColorSpaceLowValue;   // low boundary of color space that is to
                                        // be treated as Color Key, inclusive
    uint       dwColorSpaceHighValue;  // high boundary of color space that is
                                        // to be treated as Color Key, inclusive
};

struct XPIXELFORMAT
{
	uint    dwSize;
	uint    dwFlags;
	uint    dwFourCC;
	union
	{
		uint    dwRGBBitCount;
		uint    dwAlphaBitDepth;
	};
	uint    dwRBitMask;
	uint    dwGBitMask;
	uint    dwBBitMask;
	uint    dwRGBAlphaBitMask;
};

struct XSURFACEDESC
{
	uint		dwSize;                 // size of the DDSURFACEDESC structure
	uint		dwFlags;                // determines what fields are valid
	uint		dwHeight;               // height of surface to be created
	uint		dwWidth;                // width of input surface
	union
	{
		uint		lPitch;                 // distance to start of next line (return value only)
		uint		dwLinearSize;           // Formless late-allocated optimized surface size
	} DUMMYUNIONNAMEN(1);
	uint		dwDepth;                // the depth if this is a volume texture 
	uint		dwMipMapCount;          // number of mip-map levels requestde
	uint		dwAlphaBitDepth;        // depth of alpha buffer requested
	uint		dwReserved;             // reserved
	void*		lpSurface;              // pointer to the associated surface memory
	union
	{
		XCOLORKEY	ddckCKDestOverlay;      // color key for destination overlay use
		uint			dwEmptyFaceColor;       // Physical color for empty cubemap faces
	} DUMMYUNIONNAMEN(3);
	XCOLORKEY		ddckCKDestBlt;          // color key for destination blt use
	XCOLORKEY		ddckCKSrcOverlay;       // color key for source overlay use
	XCOLORKEY		ddckCKSrcBlt;           // color key for source blt use
	XPIXELFORMAT	ddpfPixelFormat;        // pixel format description of the surface
	XSCAPS2			ddsCaps;                // direct draw surface capabilities
	uint				dwTextureStage;         // stage in multitexture cascade
};

//-----------------------------------------------------------------------------
/*!
return number of needed DXT blocks
\param size		wanted width or height of surface
*/
//-----------------------------------------------------------------------------
ENF_INLINEFUNC uint GetDXTBlocks(uint size)
{
	enf_assert(size);

	uint divBy4 = (size & 3) ? (size & 0xFFFFFFFC) + 4 : size;
	return enf_max(1U, divBy4 / 4);
}

//@}
}
#endif //ENF_DDSFORMAT_H
