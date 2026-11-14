//-----------------------------------------------------------------------------
// File: enf_supershader.h
//
// Desc: Super shader
//
// Copyright (c) 2000-2006 Black Element Software. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_SUPERSHADER_H
#define ENF_SUPERSHADER_H

#include "enf_config.h"
#include "common/enf_types.h"

namespace enf
{
/** @addtogroup MaterialSystem
@{*/

class VertexShader;
class PixelShader;
//-----------------------------------------------------------------------
/*!
Manages combinations of the shader.
For creation, see enf::MaterialSystem::CreateSuperShader
*/
class PUBLIC_API SuperShader: public RefCountObject
{
protected:

	friend class IMaterialSystem;
	friend struct SuperShaderHashFunc;

	static const uint MAX_OPTIONS = 7;

	uint	Multiply[MAX_OPTIONS - 1];

	uint Params2Index(int p0 = 0, int p1 = 0, int p2 = 0, int p3 = 0, int p4 = 0, int p5 = 0, int p6 = 0) const
	{
		//unused options will be optimized away
		uint index = 0;
		if(p0) index += p0;
		if(p1) index += p1 * Multiply[0];
		if(p2) index += p2 * Multiply[1];
		if(p3) index += p3 * Multiply[2];
		if(p4) index += p4 * Multiply[3];
		if(p5) index += p5 * Multiply[4];
		if(p6) index += p6 * Multiply[5];
		return index;
	}
	
	virtual uint GetHashValue() const=0;
	virtual int Compare(const SuperShader* ss) const=0;

	virtual VertexShader* _GetVShader(uint index) const=0;
	virtual PixelShader* _GetPShader(uint index) const=0;
	virtual ~SuperShader() {};

public:
	ENF_DECLARE_SMALLOBJECT(SuperShader);

	//!Preloads selected combinations of shader
	virtual void Preload(int p0 = 0, int p1 = 0, int p2 = 0, int p3 = 0, int p4 = 0, int p5 = 0, int p6 = 0)=0;
	
	//!Preloads all combinations of shader. Must be done prior to use of the shader
	virtual void PreloadAll()=0;

	//!Returns variant of shader according to options. Use it for vertex shaders only!
	ENF_INLINE VertexShader* GetVShader(int p0 = 0, int p1 = 0, int p2 = 0, int p3 = 0, int p4 = 0, int p5 = 0, int p6 = 0) const
	{
		return _GetVShader(Params2Index(p0, p1, p2, p3, p4, p5, p6));
	}
	//!Returns variant of shader according to options. Use it for pixel shaders only!
	ENF_INLINE PixelShader* GetPShader(int p0 = 0, int p1 = 0, int p2 = 0, int p3 = 0, int p4 = 0, int p5 = 0, int p6 = 0) const
	{
		return _GetPShader(Params2Index(p0, p1, p2, p3, p4, p5, p6));
	}
};
//@}

}
#endif //ENF_SUPERSHADER_H