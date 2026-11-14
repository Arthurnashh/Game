// Horizont - clipping and draw encapsulation
// (C) 1997, SUMA
#include "wpch.hpp"
#include "global.hpp"
#include "scene.hpp"
#include "engine.hpp"
#include "tlVertex.hpp"
#include "landscape.hpp"
#include "world.hpp"
#include <El/Common/perfLog.hpp>

static bool BackfaceCull(const Poly &poly, const TLVertexTable &mesh)
{
	if (poly.N()<3) return false; // never discard lines
	Vector3Val p0 = mesh.TransPosA(poly.GetVertex(0));
	Vector3Val p1 = mesh.TransPosA(poly.GetVertex(1));
	Vector3Val p2 = mesh.TransPosA(poly.GetVertex(2));

	Vector3Val p20 = p2-p0; 
	Vector3Val p10 = p1-p0; 
	// manually inlined cross product
	float x = p20.Y()*p10.Z()-p20.Z()*p10.Y();
	float y = p20.Z()*p10.X()-p20.X()*p10.Z();
	float z = p20.X()*p10.Y()-p20.Y()*p10.X();

	//Vector3 normal=(p2-p0).CrossProduct(p1-p0);

	float np=p0.X()*x+p0.Y()*y+p0.Z()*z;
	return np<0; // discard this one
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
	ReserveFaces(faces.Size(),true);
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
				if( doCull && BackfaceCull(sf,tlMesh) ) continue;
				Add(sf);
			}
		}
		else
		{
			for( Offset si = srcSec.beg,se=srcSec.end; si<se; faces.Next(si) )
			{
				const Poly &sf = faces[si];
				if( doCull && BackfaceCull(sf,tlMesh) ) continue;
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

Poly *FaceArray::AddClipped
(
	const Poly &face, TLVertexTable &tlMesh, Scene &scene, ClipFlags clipFlags
)
{
	if( BackfaceCull(face,tlMesh) ) return NULL;
	Poly df=face;
	df.Clip(tlMesh,*scene.GetCamera(),clipFlags);
	if( df.N()<3 ) return NULL;
	Offset o = Add(df);
	return &_data->_faces.Set(o);
} 

Poly *FaceArray::AddNoClip
(
	const Poly &face, TLVertexTable &tlMesh, Scene &scene
)
{
	if( BackfaceCull(face,tlMesh) ) return NULL;
	Offset o = Add(face);
	return &_data->_faces.Set(o);
} 



/*!
\patch_internal 1.50 Date 4/7/2002 by Ondra
- Optimized: SW T&L renderstate setting is now done per-section.
*/

void FaceArray::Draw(
	ColorVal animColor,
	TLVertexTable &tlTable, const LightList &lights,
  AnimationContext *animContext,
	const Shape &mesh, ClipFlags clipFlags, int spec,
	const Matrix4 &invTransform
) const
{
	DoAssert( _sections.Size()==0 || _sections[_sections.Size()-1].end==End());
	Engine *engine=GEngine;
	if( (spec&OnSurface)==0 )
	{
		ClipFlags andClip=clipFlags;
		clipFlags&=tlTable.CheckClipping(*GScene->GetCamera(),clipFlags,andClip);
		if( andClip ) return;
	}
  const int cb = -1;
	engine->PrepareMesh(cb, spec);
	if( spec&(OnSurface|IsOnSurface) )
	{
		engine->SetBias(cb, 0x10);
	}
	else
	{
		int bias = (spec&ZBiasMask)/ZBiasStep;
		// max. bias value is 3
		engine->SetBias(cb, bias*5);
	}

	tlTable.DoLighting(animColor,invTransform,lights,mesh,spec);

	// optimize for common case
	// no clipping - we can draw directly
	const FaceArray *drawFaces=this;
	FaceArray clippedFaces;
	if( (clipFlags&ClipAll) || (spec&OnSurface) )
	{
		clippedFaces.ReserveFaces(0,false);
		if( spec&OnSurface )
		{
			float y=( (spec&IsShadow) ? engine->ZShadowEpsilon() : engine->ZRoadEpsilon() );
			clippedFaces.SurfaceSplit(*this,tlTable,*GScene,clipFlags,y);
		}
		else
		{
			clippedFaces.Clip(*this,tlTable,*GScene->GetCamera(),clipFlags);
		}
		drawFaces=&clippedFaces;
	}

	if( drawFaces->Begin()<drawFaces->End() )
	{
	  // if there are too many vertices, do not attempt to draw
		tlTable.DoPerspective(*GScene->GetCamera(),clipFlags);
		if (engine->BeginMesh(cb, tlTable,spec))
		{
		  int nSections = drawFaces->_sections.Size();
		  // verify there are some sections
		  DoAssert(nSections!=0);
		  // verify all source shape sections are still there
		  DoAssert(mesh.NSections()==nSections);
		  // verify whole FaceArray is covered by sections
		  DoAssert(nSections!=0 && drawFaces->_sections[nSections-1].end==drawFaces->End());
		  for (int s=0; s<nSections; s++)
		  {
        const ShapeSection &ss = drawFaces->_sections[s];
        const PolyProperties &pp = animContext ? animContext->GetSection(&mesh, s) : ss;
			  // TODO: optimize
			  Texture *texture = pp.GetTexture();
			  int spec = pp.Special();
			  if (spec & (IsHidden | IsHiddenProxy) ) continue;
			  pp.Prepare(cb,texture, pp.GetMaterialExt(), spec);
			  //LogF("Tex %s",(const char *)texture->Name());

			  engine->DrawSection(*drawFaces, ss.beg, ss.end);
		  }
		  engine->EndMesh(cb, tlTable);
		  engine->SetBias(cb, 0);
		}
		else
		{
		  return;
		  // some problem in the model
		}
	}
}

inline void Swap( Poly *&a, Poly *&b )
{
	Poly *t=a;a=b;b=t;
}

void FaceArray::SurfaceSplit
(
	const FaceArray &faces, TLVertexTable &tlMesh,
	Scene &scene,
	ClipFlags clipFlags, float y
)
{
	// note: it may be clipped by any plane
	clipFlags=ClipAll;
	// clear the object data
	Matrix3Val nTrans=scene.CamNormalTrans();
	Matrix4Val pTrans=scene.ScaledInvTransform();

	Clear();

	// fill with surface split
	//const Landscape *land=scene.GetLandscape();
	//const Landscape *land=GLOB_LAND;
	const Camera &camera=*scene.GetCamera();
	_data = new FaceData;
	_data->_faces.Realloc(faces.Size()*2+8);

	// determine range of landscape squares polygon is in
	Vector3Val sNo=nTrans.DirectionAside();
	Vector3Val uNo=nTrans.Direction();
	Vector3 cNo(VMultiply,nTrans,Vector3(+H_SQRT2/2,0,+H_SQRT2/2));
	
	Poly zTemp1,zTemp2;
	Poly xTemp1,xTemp2;
	Poly splitA,splitB;
	for (int s=0; s<faces._sections.Size(); s++)
	{
		const ShapeSection &sec = faces._sections[s];
		ShapeSection &dstSec = _sections.Append();
		dstSec = sec;
		dstSec.beg = End();
		for( Offset i=sec.beg,e=sec.end; i<e; faces.Next(i) )
		{
			Poly source;
			source=faces[i];

			if( !(sec.Special()&OnSurface) )
			{
				source.CheckClip(tlMesh,camera,clipFlags);
				if( source.N()>=3 ) Add(source);
				continue;
			}

			Vector3 pos(VFastTransform,scene.CamInvTrans(),tlMesh.TransPosA(source.GetVertex(0)));
			float xFMin=pos.X(),xFMax=xFMin;
			float yFMin=pos.Y(),yFMax=yFMin;
			float zFMin=pos.Z(),zFMax=zFMin;
			for( int ii=1; ii<source.N(); ii++ )
			{
				Vector3 pos(VFastTransform,scene.CamInvTrans(),tlMesh.TransPosA(source.GetVertex(ii)));
				xFMin=floatMin(xFMin,pos.X());
				xFMax=floatMax(xFMax,pos.X());
				yFMin=floatMin(yFMin,pos.Y());
				yFMax=floatMax(yFMax,pos.Y());
				zFMin=floatMin(zFMin,pos.Z());
				zFMax=floatMax(zFMax,pos.Z());
			}

			int xMin=toIntFloor(xFMin*InvTerrainGrid);
			int zMin=toIntFloor(zFMin*InvTerrainGrid);
			int xMax=toIntFloor(xFMax*InvTerrainGrid);
			int zMax=toIntFloor(zFMax*InvTerrainGrid);

			// optimization impossible due to triangulation
			//if( xMin==xMax && zMin==zMax ) continue; // no split

			Poly *zRest=&source;
			Poly *zSplit=&zTemp1;
			Poly *zFree=&zTemp2;
			for( int z=zMin; z<=zMax; z++ )
			{
				float zT=z*TerrainGrid;
				if( z<zMax )
				{
					float zB=(z+1)*TerrainGrid;
					Vector3 ptB(VFastTransform,pTrans,Vector3(0,0,zB));
					Plane bottom(-uNo,ptB);
					// cut a part from zRest and use it for x-splitting
					zRest->Split(tlMesh,*zSplit,*zFree,bottom.Normal(),bottom.D());
					//zSplit->CopyProperties(*zRest);
					//zFree->CopyProperties(*zRest);
					Swap(zFree,zRest);
				}
				else Swap(zRest,zSplit); // use whole zRest
				Poly *xRest=zSplit; // zSplit will be destroyed during x splitting
				Poly *xSplit=&xTemp1;
				Poly *xFree=&xTemp2;
				for( int x=xMin; x<=xMax; x++ )
				{
					// four clipping planes
					float xR=(x+1)*TerrainGrid;
					Vector3 ptRT(VFastTransform,pTrans,Vector3(xR,0,zT));
					if( x<xMax )
					{
						// left and right side clipping 
						Plane right(-sNo,ptRT);
						xRest->Split(tlMesh,*xSplit,*xFree,right.Normal(),right.D());
						//xSplit->CopyProperties(*xRest);
						//xFree->CopyProperties(*xRest);
						Swap(xFree,xRest);
					}
					else Swap(xSplit,xRest);

					// split 'xSplit' to two part (by A/B triangles)
					Plane cutA(-cNo,ptRT);
					xSplit->Split(tlMesh,splitA,splitB,cutA.Normal(),cutA.D());
					
					if( splitA.N()>=3 )
					{
						//splitA.CopyProperties(*xSplit);
						splitA.FitToLandscape(tlMesh,scene,y);
						splitA.CheckClip(tlMesh,camera,clipFlags);
						if( splitA.N()>=3 ) Add(splitA);
					}
					
					if( splitB.N()>=3 )
					{
						//splitB.CopyProperties(*xSplit);
						splitB.FitToLandscape(tlMesh,scene,y);
						splitB.CheckClip(tlMesh,camera,clipFlags);
						if( splitB.N()>=3 ) Add(splitB);
					}
				}
			}
		}
		dstSec.end = End();
	}
	DoAssert
	(
		_sections.Size()==0 || _sections[_sections.Size()-1].end==End()
	);
}

void FaceArray::Draw(
	ColorVal animColor,
	const LightList &lights,
  AnimationContext *animContext,
	const Shape &mesh, ClipFlags clip, int spec,
	const Matrix4 &transform, const Matrix4 &invTransform
) const
{
	//Matrix4Val pointView=GScene->ScaledInvTransform()*transform;
	Matrix4 pointView = GScene->ScaledInvTransform()*transform;

  // select vertex source based on Animation Context state
  const VertexTableAnimationContext *src = animContext ? animContext->GetVertexTableContext() : NULL;
  TLVertexTable tlTable(src, mesh, pointView);
	Draw(animColor,tlTable,lights,animContext,mesh,clip,spec,invTransform);
}
