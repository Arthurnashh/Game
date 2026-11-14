#include "wpch.hpp"

#if _ENABLE_RIGID_BODIES


#include "lights.hpp"
#include "camera.hpp"
#include "tlVertex.hpp"
#include "world.hpp"
#include "engine.hpp"
#include "landscape.hpp"
#include "fileLocator.hpp"
#include "global.hpp"
#include <El/Common/randomGen.hpp>
#include <El/Common/perfLog.hpp>
#include <Es/Files/filenames.hpp>
#include "AI/operMap.hpp"
#include "Shape/specLods.hpp"
#include <El/Common/perfProf.hpp>
#include "diagModes.hpp"
#include "paramFileExt.hpp"
#include "Shape/material.hpp"
#include "roads.hpp"
#include "El/FreeOnDemand/memFreeReq.hpp"
#include "AI/ai.hpp"

#ifndef max
#undef min
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#include <PhysicalLibs/RigidBody/cd/IntersectLandConvex.hpp>
#include <PhysicalLibs/RigidBody/cd/ContactLandConvex.hpp>
#include "landscapeRB.hpp"

#define __DEBUG_LANDCONTACT 0

///////////////////////////////////////////////////////////////////////////

void LandscapeRB::PrepareDataCD(const AABox& realRect)
{
  GridRectangle rectSD;
  rectSD.xBeg = realRect.Min()[0] * _invTerrainGrid;
  rectSD.zBeg = realRect.Min()[2] * _invTerrainGrid;
  rectSD.xEnd = realRect.Max()[0] * _invTerrainGrid + 1;
  rectSD.zEnd = realRect.Max()[2] * _invTerrainGrid + 1;

  const int xCountSD=rectSD.xEnd-rectSD.xBeg+1;
  const int zCountSD=rectSD.zEnd-rectSD.zBeg+1;
  const int nVertices=zCountSD*xCountSD;

  _vertex.Resize(nVertices);

  for(int xx =  rectSD.xBeg; xx <= rectSD.xEnd; xx++)
    for(int zz =  rectSD.zBeg; zz <= rectSD.zEnd; zz++)
    {
      float l=ClippedData(zz,xx);
      _vertex[zCountSD * (xx - rectSD.xBeg) + zz - rectSD.zBeg] = Vector3(_terrainGrid * xx, l, _terrainGrid * zz);
    };

  _face.Resize(0);
  _face.Reserve((zCountSD-1) * (xCountSD-1) * 2,(zCountSD-1) * (xCountSD-1) * 2);
  for(int xx = 0; xx < xCountSD - 1; xx++)
    for(int zz = 0; zz < zCountSD - 1; zz++)
    {
      MFace face1;
      face1.CreationStart();
      face1.AddVertex(zCountSD * xx + zz);
      face1.AddVertex(zCountSD * xx + (zz + 1));
      face1.AddVertex(zCountSD * (xx + 1) + zz);

      MFace face2;      
      face2.CreationStart();
      face2.AddVertex(zCountSD * xx + (zz + 1));
      face2.AddVertex(zCountSD * (xx + 1) + (zz + 1));
      face2.AddVertex(zCountSD * (xx + 1) + zz);

      _face.Add(face1);
      _face.Add(face2);
    }

  FindEdges();
  CalcDirs();
}

bool LandscapeRB::IsTriviallyOver(const AABox& realRect)
{
  PROFILE_SCOPE(odeITO);
  GridRectangle rectSD;
  rectSD.xBeg = realRect.Min()[0] * _invTerrainGrid;
  rectSD.zBeg = realRect.Min()[2] * _invTerrainGrid;
  rectSD.xEnd = realRect.Max()[0] * _invTerrainGrid + 1;
  rectSD.zEnd = realRect.Max()[2] * _invTerrainGrid + 1;
  
  float minHeight = realRect.Min()[1] - _tolerance;

  for(int xx =  rectSD.xBeg; xx <= rectSD.xEnd; xx++)
    for(int zz =  rectSD.zBeg; zz <= rectSD.zEnd; zz++)
    {
      float l=ClippedData(zz,xx);
      if (minHeight < l)
        return false;
    };

  return true;
}

void LandscapeRB::FindEdges()
{
  _edge.Resize(0);
  for(int i = 0; i < _face.Size(); i++)
  {
    MFace & face = _face[i];
    for(int j = 0; j < face.N(); j++)
    {
      int vert1 = min(face[(j == 0) ? face.N()-1: j-1], face[j]);
      int vert2 = max(face[(j == 0) ? face.N()-1: j-1], face[j]);

      // maybe this edge exist?
      int l = 0;
      for(; l < _edge.Size(); l++)
      {
        if (_edge[l][0] == vert1 && _edge[l][1] == vert2)
          break;
      }

      if ( l == _edge.Size())
      {
        MEdge edge(vert1, vert2);
        edge.SetFace(0) = i;
        _edge.Add(edge);
      }
      else
        _edge[l].SetFace(1) = i;
    }
  }
}

void LandscapeRB::CalcDirs()
{
  {  
    //PROFILE_SCOPE(face); 
    for(int i = 0; i < _face.Size(); i++)
    {
      _face[i].CalcPlane(_vertex);
    }
  }

  {
    //PROFILE_SCOPE(edge); 
    for(int i = 0; i < _edge.Size(); i++)
    {
      _edge[i].CalcDir(_vertex);
    }
  }
}

void LandscapeRB::AdjustScaleOnDist(float &dist, float& scale, const Plane& sepPlane, MeshGeometry& geom2)
{
  float distWanted = _tolerance * 0.75;
  float centerDist = sepPlane.Distance(geom2.GeomCenter()) - dist;
  Assert(centerDist > 0);

  float rescale = 1 +  (dist - distWanted) / centerDist;
  scale *= rescale;

  if (scale > 1)
  {
    rescale /= scale; 
    scale = -1;
  }

  //float sepDDiff = (1 - rescale) * SepPlane.Distance(GeomCenter());
  //SepPlane.SetD(SepPlane.D() - sepDDiff);

  dist -= (rescale - 1) * centerDist;
}

bool LandscapeRB::IsIntersection( IRBGeometry& geom2, int simCycle)
{

  if (IsTriviallyOver(geom2.GetEnclosingRectangle()))
    return false;

  switch (((Geometry&)geom2).GetType())
  {
  case EGT_COMPOSE:
    return geom2.IsIntersection(*this, simCycle);  
  case EGT_LANDSCAPE:
    Assert(FALSE);
    return FALSE;
  case EGT_CONVEX:
    {
      PROFILE_SCOPE(odeLIs);
      Ref<CDCacheItemData> data;
      float scale = -1;

      ((MeshGeometry&) geom2).PrepareDataCD();

      if (cdCache.Get(GetID(), geom2.GetID(), data))
      {    
        if (data->_scale > 0)
        {
          if (data->_scale <= IGNORANCE_SCALE)
          {
            if (data->_lastUpdate + 5 <= simCycle)
            {
              // try it maybe there is allready space
              scale = IGNORANCE_SCALE + 0.01;
            }
            else
              return false;
          }
          else
            scale = data->_scale;

                
          scale = data->_scale;
          geom2.Scale(data->_scale);
          geom2.CalculateEnclosingRectangle();
        }
      }

      PrepareDataCD(geom2.GetEnclosingRectangle());

      Plane sepPlane;     
      float dist;
      IsIntersectionLandConvex<LandscapeRB, MeshGeometry, Plane, MFace, MEdge> intersect;
      bool inter = intersect(*this, (const MeshGeometry&) geom2, sepPlane, dist);

      if (scale > 0)
      {
        if (!inter && dist > _tolerance)
          AdjustScaleOnDist(dist, scale, sepPlane, (MeshGeometry&) geom2);

        geom2.NormScale();
        geom2.CalculateEnclosingRectangle();
      } 

      if (!inter && data.NotNull())
      {
        data->_scale = scale;
        data->_lastUpdate = simCycle;
        data->_dist = dist;
      }
      return inter;
    }
  default:
    Assert(FALSE);
  }
  return FALSE; 
}

void LandscapeRB::FilterLandPairs(RBContactArray& finalContacts, CDContactArray& foundPairs, float scale, MeshGeometry& geom)
{
  if (foundPairs.Size() == 0)
    return;

  if (scale > 0)
  {  
    for( int i = 0; i < foundPairs.Size(); i++)
    {
      CDContactPair& contactPair = foundPairs[i];
      Vector3 diff = contactPair._pos + contactPair._norm * contactPair._dist - geom.GeomCenter();      
      contactPair._dist -= (1 - scale) / scale * (-contactPair._norm * diff) + _tolerance;
      contactPair._dist *= -1;
    }
  }
  else
  {
    for( int i = 0; i < foundPairs.Size(); i++)
    {
      CDContactPair& contactPair = foundPairs[i];      
      contactPair._dist = _tolerance - contactPair._dist;      
    }
  }

  for(int t = 0; t < NFaces(); t++)
  {
    // Find closest par.
    float maxUnder = -1;
    Vector3 maxUnderNorm;
 
    for( int i = 0; i < foundPairs.Size(); i++)
    {
      if ((foundPairs[i]._triangle[0] == t || foundPairs[i]._triangle[1] == t) && maxUnder < foundPairs[i]._dist)
      {      
        maxUnder = foundPairs[i]._dist;
        maxUnderNorm = foundPairs[i]._norm;
      }
    }

    if (maxUnder <= 0)
      continue;

    for( int i = 0; i < foundPairs.Size(); i++)
    {
      if ((foundPairs[i]._triangle[0] == t || foundPairs[i]._triangle[1] == t) && foundPairs[i]._norm * maxUnderNorm > 0.98 && foundPairs[i]._dist > 0)
      {
        // Add to contacts;
        RBContactPoint tContactPoint;

        tContactPoint.pos = foundPairs[i]._pos;
        tContactPoint.under = foundPairs[i]._dist;

        /*if (foundPairs[i]._dist > 0.10f)
        {
          LogF("Landscape big dist %f", foundPairs[i]._dist);
        } */ 

        tContactPoint.dir[0] = -maxUnderNorm;

        // Find pedestal axes
        tContactPoint.dir[1] = tContactPoint.dir[0].CrossProduct( Vector3(1,0,0));
        if (tContactPoint.dir[1].Size() < 0.1f) // is new vector correct
          tContactPoint.dir[1] = tContactPoint.dir[0].CrossProduct( Vector3(0,1,0));

        tContactPoint.dir[2] = tContactPoint.dir[0].CrossProduct(tContactPoint.dir[1]);

        tContactPoint.dir[1].Normalize();
        tContactPoint.dir[2].Normalize();

        finalContacts.Add(tContactPoint);

        foundPairs.Delete(i); //allready used
        i--;
      }
    }
  }
}



int LandscapeRB::FindContacts( RBContactArray& cContactPoints, IRBGeometry& geom2, int simCycle)
{
  RigidBodyObject * pcBody2 = /*geom2.GetBody()*/ NULL;
  if (pcBody2 && !pcBody2->_collideWithLandscape)
    return 0;

  if (IsTriviallyOver(geom2.GetEnclosingRectangle()))
    return 0;

  switch (((Geometry&)geom2).GetType())
  {
  case EGT_COMPOSE:    
    return geom2.FindContacts(cContactPoints, *this, simCycle);  
  case EGT_LANDSCAPE:
    Assert(FALSE);
    return 0;
  case EGT_CONVEX:
    {
      if (IsIntersection( geom2, simCycle - 1))
        ForceInterFree( geom2, simCycle - 1);

      PROFILE_SCOPE(odeLFC);
      Ref<CDCacheItemData> data;
      float scale = -1;
      float maxSize = 0;

      ((MeshGeometry&) geom2).PrepareDataCD();

      if (cdCache.Get(GetID(), geom2.GetID(), data))
      {    
        if (data->_scale > 0 && data->_scale <= IGNORANCE_SCALE)
          return 0; // ignore collision        

        if (data->_scale > 0)
        {              
          scale = data->_scale;
          geom2.Scale(data->_scale);

          // find maximal potential size...          
          maxSize = ((const MeshGeometry&) geom2).MaxScaleSize() * (1 - scale);
          //LogF("scale %lf maxSize %lf", scale, maxSize);

          geom2.CalculateEnclosingRectangle();          
        }        
      }
      
      PrepareDataCD(geom2.GetEnclosingRectangle());

      _tolerance += maxSize;
      CDContactArray res;
      ContactLandConvex<LandscapeRB, MeshGeometry, Plane, MFace, MEdge> findContact;
      findContact(res,*this, (const MeshGeometry&) geom2, _tolerance);

      if (scale > 0)
      {
        geom2.NormScale();
        geom2.CalculateEnclosingRectangle();
      }

      if (res.Size() == 0)
      {
        ADD_COUNTER(meLFCn,1);
      }
      else
      {
        ADD_COUNTER(meLFCy,1);
      }

      _tolerance -= maxSize;
      int nContacts = cContactPoints.Size();
      FilterLandPairs(cContactPoints, res, scale,(MeshGeometry&) geom2);
      

      // under like it will be in normal size     

      RigidBodyObject * pcBody1 =  NULL;//_ptBody;
      //RigidBodyObject * pcBody2 = geom2.GetBody();

      float fRestitutionCoef = 0;
      float fFrictionCoef = 0.8f;

      for(;nContacts < cContactPoints.Size(); nContacts++)
      {
        cContactPoints[nContacts].pcBody1 = pcBody1;
        cContactPoints[nContacts].pcBody2 = pcBody2;
        cContactPoints[nContacts].geomID1 = GetID();
        cContactPoints[nContacts].geomID2 = geom2.GetID();
        cContactPoints[nContacts].fRestitutionCoef = fRestitutionCoef;
        cContactPoints[nContacts].fSlidingFrictionCoef = fFrictionCoef;
#if __DEBUG_LANDCONTACT
        AddForce(cContactPoints[nContacts].pos,-cContactPoints[nContacts].dir[0] * cContactPoints[nContacts].under * 100, Color(255,0,0));
#endif
      }

      return nContacts;
    }
  default:
    Assert(FALSE);
    return 0;
  }
}



bool LandscapeRB::ForceInterFree( IRBGeometry& geom2, int simCycle) 
{
  switch (((Geometry&)geom2).GetType())
  {
  case EGT_COMPOSE:
    return geom2.ForceInterFree(*this, simCycle); 
  case EGT_LANDSCAPE:
    Assert(FALSE);
    return FALSE;
  case EGT_CONVEX:
    {
      PROFILE_SCOPE(odeLFIF);
      IsIntersectionLandConvex<LandscapeRB, MeshGeometry, Plane, MFace, MEdge> intersect;     
      
      float scale = 1;
      ((MeshGeometry&) geom2).PrepareDataCD();

      Ref<CDCacheItemData> data;
      if (cdCache.Get(GetID(), geom2.GetID(), data))
      {
        
        if (data->_scale > 0)
        {
          if (data->_scale <= IGNORANCE_SCALE)
            return TRUE; 
          scale = data->_scale;          
          geom2.Scale(data->_scale);
        }
      }
      else
      {        
        //not found
        data = new CDCacheItemData;
        cdCache.Add(GetID(), geom2.GetID(), data);
      }

      PrepareDataCD(geom2.GetEnclosingRectangle());

      Plane sepPlane;
      float dist;

      bool inter = intersect(*this,(const MeshGeometry&) geom2, sepPlane, dist);      
      if (!inter)  
      {
        if(data->_scale > 0)
        {
          if (dist > _tolerance)
            AdjustScaleOnDist(dist, scale, sepPlane, (MeshGeometry&) geom2);
          
          geom2.NormScale();
        }

        data->_lastUpdate = simCycle;         
        data->_geom1ID = GetID();       
        return TRUE;
      }

      // later effective size can be bounding sphere radius etc...      
      while(inter && scale > IGNORANCE_SCALE)
      {
        scale *= 0.90;
        
        geom2.Scale(scale);
        inter = intersect(*this,(const MeshGeometry&) geom2, sepPlane, dist);
      }

      if (scale <= IGNORANCE_SCALE)
      {
        //ignore collisions
        data->_scale = scale;
        data->_lastUpdate = simCycle;        
        geom2.NormScale();
        return FALSE;
      }
      else
      {
        if (dist > _tolerance)
          AdjustScaleOnDist(dist, scale, sepPlane, (MeshGeometry&) geom2);
        data->_lastUpdate = simCycle; 
        data->_geom1ID = GetID();
        data->_scale = scale;
        data->_dist = dist;
        geom2.NormScale();
        return TRUE;
      }
    }
  default:
    Assert(FALSE);
  }
  return FALSE;  
}
#if __DEBUG_LANDCONTACT
void LandscapeRB::AddForce
(
 Vector3Par pos, Vector3Par force, Color color
 )
{
  LODShapeWithShadow *forceArrow=GScene->ForceArrow();
  float size=force.Size() * 0.1;
  if( size>1 ) size=1;

  Ref<Object> arrow=new ObjectColored(forceArrow,VISITOR_NO_ID);
  Vector3 aside=force.CrossProduct(VAside);
  Vector3 zAside=force.CrossProduct(VForward);
  if( zAside.SquareSize()>aside.SquareSize() ) aside=zAside;
  arrow->SetPosition(pos);
  arrow->SetOrient(force,aside);
  arrow->SetPosition
    (
    arrow->PositionModelToWorld(forceArrow->BoundingCenter()*size)
    );
  arrow->SetScale(size);
  arrow->SetConstantColor(PackedColor(Color(color)));
  //GScene->ObjectForDrawing(arrow);

  ShowObject(arrow);
}
#endif
#endif
