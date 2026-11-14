// Poseidon - shape functions that require landscape interface
// spliting...
// (C) SUMA, 1998

// Poseidon - shape management
// (C) 1998, SUMA

#include "wpch.hpp"
#include "Shape/shape.hpp"
#include "landscape.hpp"
#include "Shape/plane.hpp"
#include <El/Common/perfProf.hpp>

/*!
\patch_internal 1.17 Date 08/14/2001 by Ondra
- Fixed: selection were kept for shadows and roadsplits,
but they got corrupted. This caused Assert in debug build,
othewise it should be harmless.
*/

/**
Splits this Shape using TerrainGrid (and its subdivision by A/B triangles)
It also cut it along the specified (segment) rectangle.
@param land is used only to get SurfaceY in order to make this shape lying on the surface
@param y value added to all shape points (typically = 0.01, i.e. little above the surface)
@param useOrigY multiplication factor (typically = 1.0)
@param rect clipping rectangle
@param quickAdd do not try to merge vertices, use fast AddVertex (causes duplicate vertices)
@param lod terrain lod detail - log2 subdivision
*/

void Shape::SurfaceSplit
(
  const Landscape *land, const Matrix4 &toWorld, float y, float useOrigY,
  const GridRectangle *rect, bool quickAdd, int lod
)
{
  PROFILE_SCOPE(shSuS);
  
  Assert(lod<=16);
  // we want to avoid division
  // we assume lod range is somewhat limited
  int lodStep = 1<<lod;
  float invLodStep = (1<<(16-lod))/float(1<<16);
  
  float terrainGridReal = land->GetTerrainGrid();
  float terrainGrid = land->GetTerrainGrid()*lodStep;
  float invTerrainGrid = land->GetInvTerrainGrid()*invLodStep;
  // selection and point indices must be ignored: otherwise they would be invalidated
  _sel.Clear();
  _pointToVertex.Clear();
  _vertexToPoint.Clear();
  // start: fit all vertices to landscape
  Matrix4 fromWorld(MInverseGeneral,toWorld);

  FaceArray splitFaces(_face.Size()*3+16,false);

  Vector3 sNo(1,0,0);
  Vector3 uNo(0,0,1);
  Vector3 cNo(+H_SQRT2/2,0,+H_SQRT2/2);

  Poly zTemp1,zTemp2;
  Poly xTemp1,xTemp2;
  Poly splitA,splitB;
  for (int s=0; s<NSections(); s++)
  {
    const ShapeSection &ss = GetSection(s);
    ShapeSection dstSec;
    dstSec = ss;
    dstSec.beg = splitFaces.End();
    for( Offset i=ss.beg; i<ss.end; NextFace(i) )
    {
      Poly source=Face(i);

#if !_RELEASE
      if( source.N()<=0 )
      {
        LogF("Invalid source?");
        continue;
      }
#endif

      // determine range of landscape squares polygon is in
      Vector3 pos(VFastTransform,toWorld,Pos(source.GetVertex(0)));
      float xFMin=pos.X(),xFMax=xFMin;
      float zFMin=pos.Z(),zFMax=zFMin;
      for( int vi=1; vi<source.N(); vi++ )
      {
        Vector3 pos(VFastTransform,toWorld,Pos(source.GetVertex(vi)));
        xFMin=floatMin(xFMin,pos.X());
        xFMax=floatMax(xFMax,pos.X());
        zFMin=floatMin(zFMin,pos.Z());
        zFMax=floatMax(zFMax,pos.Z());
      }

      int xMin=toIntFloor(xFMin*invTerrainGrid);
      int zMin=toIntFloor(zFMin*invTerrainGrid);
      int xMax=toIntFloor(xFMax*invTerrainGrid);
      int zMax=toIntFloor(zFMax*invTerrainGrid);

      Poly *zRest=&source;
      Poly *zSplit=&zTemp1;
      Poly *zFree=&zTemp2;
      if (rect) /*doCut*/
      {
        const float stretch = 0.1f;
        float segXBeg = rect->xBeg*terrainGridReal-stretch;
        float segXEnd = rect->xEnd*terrainGridReal+stretch;
        float segZBeg = rect->zBeg*terrainGridReal-stretch;
        float segZEnd = rect->zEnd*terrainGridReal+stretch;
        if (xFMin >= segXEnd || xFMax <= segXBeg || zFMin >= segZEnd || zFMax <= segZBeg) 
          continue;
        if (xFMin < segXBeg)
        {
          Vector3 ptL=Vector3(segXBeg,0,0); 
          Plane left(sNo,ptL);
          zRest->Split(toWorld,*this,*zSplit,*zFree,left.Normal(),left.D(),quickAdd);
          Swap(zSplit,zRest);
        }
        if (xFMax > segXEnd)
        {
          Vector3 ptL=Vector3(segXEnd,0,0); 
          Plane left(-sNo,ptL);
          zRest->Split(toWorld,*this,*zSplit,*zFree,left.Normal(),left.D(),quickAdd);
          Swap(zSplit,zRest);
        }
        if (zFMin < segZBeg) 
        {
          Vector3 ptT=Vector3(0,0,segZBeg); 
          Plane top(uNo,ptT);
          zRest->Split(toWorld,*this,*zSplit,*zFree,top.Normal(),top.D(),quickAdd);
          Swap(zSplit,zRest);
        }
        if (zFMax > segZEnd) 
        {
          Vector3 ptB=Vector3(0,0,segZEnd); 
          Plane bottom(-uNo,ptB);
          zRest->Split(toWorld,*this,*zSplit,*zFree,bottom.Normal(),bottom.D(),quickAdd);
          Swap(zSplit,zRest);
        }
      }

      //int z=zMax,x=xMax;
      for( int z=zMin; z<=zMax; z++ )
      {
        float zT=z*terrainGrid; //top
        if( z<zMax )
        {
          float zB=(z+1)*terrainGrid; //bottom
          Vector3 ptB=Vector3(0,0,zB);
          Plane bottom(-uNo,ptB);
          // cut a part from zRest and use it for x-splitting
          zRest->Split(toWorld,*this,*zSplit,*zFree,bottom.Normal(),bottom.D(),quickAdd);
          Swap(zFree,zRest);
        }
        else Swap(zRest,zSplit); // use whole zRest
        Poly *xRest=zSplit; // zSplit will be destroyed during x splitting
        Poly *xSplit=&xTemp1;
        Poly *xFree=&xTemp2;
        for( int x=xMin; x<=xMax; x++ )
        {
          // four clipping planes
          float xR=(x+1)*terrainGrid;    //right
          Vector3 ptRT=Vector3(xR,0,zT); //z coordinate is necessary (only) for A/B triangles splitting
          if( x<xMax )
          {
            // left and right side clipping 
            Plane right(-sNo,ptRT);
            xRest->Split(toWorld,*this,*xSplit,*xFree,right.Normal(),right.D(),quickAdd);
            Swap(xFree,xRest);
          }
          else Swap(xSplit,xRest);

          // split 'xSplit' to two part (by A/B triangles)
          Plane cutA(-cNo,ptRT);
          xSplit->Split(toWorld,*this,splitA,splitB,cutA.Normal(),cutA.D(),quickAdd);

          if( splitA.N()>=3 )
          {
            splitFaces.Add(splitA);
          }

          if( splitB.N()>=3 )
          {
            splitFaces.Add(splitB);
          }
        }
      }
    }
    dstSec.end = splitFaces.End();
    if (dstSec.beg<dstSec.end)
    {
      splitFaces._sections.Add(dstSec);
    }
  }

  //change the y coordinate of all vertexes to make the shape lying on the surface
  if (splitFaces.NSections()>0) //otherwise no sections means no output
  { 
    for (int i=0; i<NPos(); i++)
    {
      Vector3 &pos = SetPos(i);
      Vector3 wPos(VFastTransform,toWorld,pos);
      float oy = y + pos.Y()*useOrigY;
      // TODO: we cannot use SurfaceY, we need to use data corresponding to the LOD
      wPos[1] = land->SurfaceYLod(wPos[0],wPos[2],lod)+oy;
      pos.SetFastTransform(fromWorld,wPos);
    }
  }

  _face=splitFaces; // copy to compact array
  _face._data->_faces.GetData().UnlinkStorage(); // unlink from static storage
  // un-mark all splitting hints
  ClipAndAll(~ClipLandMask);
  _data->_clip.Condense();
  Assert(CheckIntegrity());

#if _VBS3_CRATERS_DEFORM_TERRAIN
  // Planes are no longer valid
  _plane.Clear();
#endif
}

