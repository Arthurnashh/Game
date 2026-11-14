// Implementation of main map display
#include "wpch.hpp"
#include "envTracker.hpp"

#include "landscape.hpp"
#include "global.hpp"

SurroundTracker::SurroundTracker()
{
  _lastPos=VZero;
  _lastTime=TIME_MIN;
  _value50m=0;
  _value300m=0;
}

#if _PROFILE
#pragma optimize( "", off )
#endif

/**
@param radius2 max distance^2 for hiding
best parameters describe ideal pattern hiding the obstacle
*/

float SurroundTracker::Hiding( float dist2, float radius2, float objArea, float bestArea, float invBestArea, float bestRadius) const
{
  // hiding - size equal to our size -> 1
  float hides = objArea<=bestArea ? objArea*invBestArea : bestArea/objArea;

  // we are best hidden by objects which are near us
  // for 30m radius (used for 300 m distance) fully near is 15.0
  // for 5m radius (used for 50 m distance) fully near is 2.5
  if (dist2>radius2*Square(0.5f))
  {
    // even if the object is too far and its influence is marginal, we still want to consider it, as there may be multiple such objects
    float relSize = radius2*Square(0.5f)/dist2;
    return hides*relSize*0.2f;
  }
  else
  {
    return hides*0.2f;
  }
}

/*!
\patch 5134 Date 2/26/2007 by Ondra
- Fixed: Improved AI spotting ability in towns.
*/

void SurroundTracker::Update(const Object *who, Vector3Par pos)
{
  PROFILE_SCOPE_EX(surTr,ai);
  _lastPos=pos;
  _lastTime=Glob.time;
  // calculate density surroundings

  int xMin,xMax,zMin,zMax;
  const float radius300 = 30.0f;
  const float radius50 = 5.0f;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,pos,pos,floatMax(radius300,radius50));

  const float radius2For50 = Square(radius50);
  const float radius2For300 = Square(radius300);

  float total50=0;
  float total300=0;
  int x,z;

  // we are best hidden when there are near objects with approximately the same size as we have
  float bestRadius = who->VisibleSize(who->FutureVisualState());
  float bestArea = Square(bestRadius);
  float invBestArea = bestArea>0 ? 1/bestArea : 1e10f;
  for( x=xMin; x<=xMax; x++ ) for( z=zMin; z<=zMax; z++ )
  {
    const ObjectList &list=GLandscape->GetObjects(z,x);
    int n=list.Size();
    for( int i=0; i<n; i++ )
    {
      Object *obj=list[i];
      if (!obj || obj==who ) continue;
      if (obj->GetType()==Network ) continue; // no collisions with roads
      if (obj->GetType()==Temporary ) continue; // no collisions with roads
      if (obj->GetType()==TypeTempVehicle ) continue; // no collisions with roads
#if _ARMA3_TERRAIN_DECALS
      if (obj->GetType()==LandDecal ) continue; // no collisions with land decals
#endif 
      if (!obj->Static()) continue;
      float objRadius=obj->GetRadius();
      if( objRadius<0.5f ) continue;
      float objArea = obj->GetShape()->EstimateArea(obj->FutureVisualState().Scale());
      // check average "radius" based on min-max box (this is a better approximation for long or tall objects)

      float dist2=obj->FutureVisualState().Position().Distance2(pos);

      total50 += Hiding(dist2, radius2For50, objArea, bestArea, invBestArea, bestRadius);
      total300 += Hiding(dist2, radius2For300, objArea, bestArea, invBestArea, bestRadius);
    }
  }

  float inForest = GLandscape->GeographicalDensity(Landscape::FilterGeogrForest(),pos);

  saturateMin(total50,0.90f);
  saturateMin(total300,0.90f);
  
  saturateMax(total50, inForest*0.7f);
  saturateMax(total300, inForest*0.9f);

  _value50m = total50;
  _value300m = total300;

}

#if _PROFILE
#pragma optimize( "", on )
#endif

float SurroundTracker::Track(const Object *who, Vector3Par pos, float dist)
{
  // TODO: consider time
  if( _lastPos.Distance2(pos)>Square(5.0f) )
  {
    Update(who,pos);
  }
  return GetDensity(dist);
}

LightTracker::LightTracker()
{
  _value = 0;
  _lastPos = VZero;
  _lastTime = TIME_MIN;
}

void LightTracker::Update(const Object *who, Vector3Par pos)
{
  PROFILE_SCOPE_EX(litTr,ai);
  _lastPos=pos;
  _lastTime=Glob.time;

  // TODO: lights stored in OffTree for fast querying
  // walk all lights
  float sumLight = 0.0f;
  for (int i=0; i<GScene->NLightsAgg(); i++)
  {
    Light *light = GScene->GetLightAgg(i);
    Vector3 normal = VUp;
    if (light) sumLight += light->Apply(pos,normal).Brightness();
  }
  // empirical constant to make sure the result range is appropriate
  static float lightScale = 8.0f;
  sumLight *= lightScale;
  saturate(sumLight,0,1);
  _value = sumLight;
}

float LightTracker::Track(const Object *who, Vector3Par pos)
{
  // TODO: consider time
  if( _lastPos.Distance2(pos)>Square(5.0f) || Glob.time.Diff(_lastTime)>5.0f)
  {
    Update(who,pos);
  }
  return GetValue();
}
