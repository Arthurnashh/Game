// Horizont - clipping and draw encapsulation
// (C) 1997, SUMA
#include "El/elementpch.hpp"
//FLY#include "global.hpp"
//FLY#include "scene.hpp"
//FLY#include "engine.hpp"
//FLY#include "tlVertex.hpp"
//FLY#include "landscape.hpp"
//FLY#include "world.hpp"
#include "cutPlane.hpp"
//FLY#include "polyClip.hpp"
#include <El/Common/perfLog.hpp>

#include "cutClipShape.hpp"

static StaticStorage<char> CharStorageF; // storage for faces
static StaticStorage<char> CharStorageS; // storage for sections

#define VERIFY_TXT_PTR 0

#if VERIFY_TXT_PTR
#include "win.h"
#endif

bool FaceArray::VerifyStructure() const
{
	#if VERIFY_TXT_PTR
	// verify all face textures are valid
	for (Offset f=Begin(); f<End(); Next(f))
	{
		const Poly &face = (*this)[f];
		// force access to texture
		// chech if pointer is valid
		Texture *texture = face.GetTexture();
		if (::IsBadReadPtr(texture,sizeof(Texture)))
		{
			LogF("FaceArray::VerifyStructure: Bad read pointer");
			return false;
		}
		if (::IsBadWritePtr(texture,sizeof(Texture)))
		{
			LogF("FaceArray::VerifyStructure: Bad write pointer");
			return false;
		}
	}
	#endif

	// verify section structure
	Offset lastOffset = Offset(0);

	for (int s=0; s<_sections.Size(); s++)
	{
		const ShapeSection &sec = _sections[s];
		if (sec.beg!=lastOffset)
		{
			ErrF("sec.beg!=lastOffset : %d!=%d",sec.beg,lastOffset);
			return false;
		}
		lastOffset = sec.end;
	}

	return true;
}

void FaceArray::ReserveFaces( int size, bool dynamic )
{
	if( !dynamic )
	{
		GetData().SetStorage(CharStorageF.Init(64*1024));
		_sections.SetStorage(CharStorageS.Init(1024));
	}
	base::Reserve(size);
}

FaceArray::FaceArray( int size, bool dynamic )
{
	if( !dynamic )
	{
		GetData().SetStorage(CharStorageF.Init(64*1024));
		_sections.SetStorage(CharStorageS.Init(1024));
	}
	if( size )
	{
		base::Realloc(size);
	}
}

void FaceArray::Clip
(
	const FaceArray &faces, TLVertexTable &tlMesh,
	const Camera &camera, ClipFlags clipFlags, bool doCull
)
{
	// note: clipFlags should be set by CheckClipping
	// copy all faces and perform per-face clipping
	Clear();
	// transfer section information from faces
	// process section by section
	_sections.Realloc(faces._sections.Size());
	for (int i=0; i<faces._sections.Size(); i++)
	{
		const ShapeSection &srcSec = faces._sections[i];
		ShapeSection &sec = _sections.Append();
		// copy all properites from source section
		sec = srcSec;
		// keep track of changed offsets
		sec.beg = End();
		if( !clipFlags )
		{
			// we guarantee no clipping
			for( Offset si = srcSec.beg,se=srcSec.end; si<se; faces.Next(si) )
			{
				const Poly &sf = faces[si];
				if( doCull && sf.BackfaceCull(tlMesh) ) continue;
				Add(sf);
			}
		}
		else
		{
			for( Offset si = srcSec.beg,se=srcSec.end; si<se; faces.Next(si) )
			{
				const Poly &sf = faces[si];
				if( doCull && sf.BackfaceCull(tlMesh) ) continue;
				Poly df=sf;
				df.Clip(tlMesh,camera,clipFlags);
				if( df.N()<3 ) continue;
				Add(df);
			}
		}
		// keep track of changed offsets
		sec.end = End();
	}
	#if _ENABLE_REPORT
		if (_sections.Size()>0)
		{
			DoAssert( _sections[_sections.Size()-1].end==End());
		}
	#endif
}

Poly *FaceArray::AddNoClip
(
	const Poly &face, TLVertexTable &tlMesh, Scene &scene
)
{
	if( face.BackfaceCull(tlMesh) ) return NULL;
	Offset o = Add(face);
	return &Set(o);
} 

extern bool DisableTextures;

inline void swap( Poly *&a, Poly *&b )
{
	Poly *t=a;a=b;b=t;
}