// Landscape - geography handling, visibility and AI support
// (C) 1996, SUMA
#include "wpch.hpp"
#include "landscape.hpp"
#include "dynSound.hpp"
#include "world.hpp"
#include <El/Common/randomGen.hpp>
#include "roads.hpp"
#include "Shape/mapTypes.hpp"
#include "global.hpp"

#include "AI/ai.hpp"
#include "objectClasses.hpp"

#include <Es/Files/filenames.hpp>
#include <Es/Algorithms/qsort.hpp>

#include "progress.hpp"

#if _DEBUG
  //#define INTERESTED_IN_ID 15041
#endif

static int CalculateGradient( float gradient )
{
  gradient=fabs(gradient);
  if( gradient>=0.95 ) return 7; // non-penetrable even for man
  else if( gradient>=0.60 ) return 6; // man can go here
  else if( gradient>=0.40 ) return 5; // most vehicles can go here, but very slow
  else if( gradient>=0.25 ) return 4; // most vehicles can go here, but slow
  else if( gradient>=0.15 ) return 3; // most vehicles can go here at 0.33 speed
  else if( gradient>=0.10 ) return 2; // most vehicles can go here at 0.5 speed
  else if( gradient>=0.05 ) return 1; // most vehicles can go here at 0.8 speed
  return 0; // leveled surfaces - top speed for all
}

// calculate max. gradient of plane gradX,gradZ
static float MaxGradient( float gradX, float gradZ )
{
  // note: this is not exact, max. gradient may lay in arbitrary direction
  // TODO: calculate real max. gradient
  float gradXZ=((gradX+gradZ)*0.5)*H_SQRT2;
  return floatMax(gradXZ,floatMax(gradX,gradZ));
}

#if USE_LANDSCAPE_LOCATIONS
// select all locations
static bool LocationAll(int x, int y, int size)
{
  return true;
}

struct LocationCollectAllExceptMountains
{
  Ref<LocationType> _type;
  RefArray<Location> _locations;

  LocationCollectAllExceptMountains()
  {
    _type = LocationTypes.New("Mount");
  }

  bool operator () (const Ref<LocationsItem> &item)
  {
    for (int i=0; i<item->_locations.Size(); i++)
    {
      Location *location = item->_locations[i];
      if (location->_type != _type)
        _locations.Add(location);
    }
    return false; // continue with enumeration
  }
};

struct LocationCollectMountains
{
  Ref<LocationType> _type;
  RefArray<Location> _locations;

  LocationCollectMountains()
  {
    _type = LocationTypes.New("Mount");
  }

  bool operator () (const Ref<LocationsItem> &item)
  {
    for (int i=0; i<item->_locations.Size(); i++)
    {
      Location *location = item->_locations[i];
      if (location->_type == _type)
        _locations.Add(location);
    }
    return false; // continue with enumeration
  }
};

struct LocationsListResult
{
  RefArray<Location, MemAllocLocal<Ref<Location>, 200> > _locations;
};

struct LocationsList
{
  // out
  LocationsListResult *_result;

  LocationsList(LocationsListResult *result)
    : _result(result)
  {
  }

  bool operator () (const Ref<LocationsItem> &item) const
  {
    for (int i=0; i<item->_locations.Size(); i++)
    {
      Location *location = item->_locations[i];
      int style = location->_type->_drawStyle;
      _result[style]._locations.Add(location);
    }
    return false;
  }
};

int CmpLocationsByImportance(const Ref<Location> *location1, const Ref<Location> *location2)
{
  Location *loc1 = *location1;
  Location *loc2 = *location2;
  
  float dif = loc2->_importance - loc1->_importance;
  if (dif != 0) return sign(dif);

  dif = GLandscape->GetRandomValueForCoord(toInt(loc2->_position.X()), toInt(loc2->_position.Z())) -
    GLandscape->GetRandomValueForCoord(toInt(loc1->_position.X()), toInt(loc1->_position.Z()));
  return sign(dif);
}

#include "quadedge.hpp"

void Landscape::RebuildLocationDistances()
{
  // collect locations to lists based on their draw style
  LocationsListResult lists[NLocationDrawStyle];
  _locations.ForEachInRegion(LocationAll, LocationsList(lists));

  // update separately for each draw style
  for (int i=0; i<NLocationDrawStyle; i++)
  {
    LocationsListResult &list = lists[i];
    if (list._locations.Size() == 0) continue;

    // sort by importance
    QSort(list._locations.Data(), list._locations.Size(), CmpLocationsByImportance);

    // calculate bounding box and inflate it a little
    float minX = FLT_MAX, maxX = -FLT_MAX, minZ = FLT_MAX, maxZ = -FLT_MAX;
    for (int j=0; j<list._locations.Size(); j++)
    {
      float x = list._locations[j]->_position.X();
      saturateMin(minX, x); 
      saturateMax(maxX, x); 
      float z = list._locations[j]->_position.Z();
      saturateMin(minZ, z);
      saturateMax(maxZ, z);
    }
    float spanX = maxX-minX, spanZ = maxZ-minZ;
    minX -= spanX*0.25; maxX += spanX*0.25; spanX += spanX*0.5;
    minZ -= spanZ*0.25; maxZ += spanZ*0.25; spanZ += spanZ*0.5;
    float maxDist = sqrtf(spanX*spanX + spanZ*spanZ);

    // initialize to a triangle that
    // - contains all data
    // - its vertices and edges are far from all inside points
    Delaunay::Point2d p1(minX-maxDist,minZ), p2(0.5*(maxX+minX),maxZ+maxDist), p3(maxX+maxDist,minZ);
    Delaunay::Subdivision delaunayMesh(p1, p2, p3);
    // calculate distances
    for (int j=0; j<list._locations.Size(); j++)
    {
      if(j%1000 == 0)
        LogF("[m] Distance calcs: %d / %d", j, list._locations.Size());
      Location *tested = list._locations[j];
      tested->_nearestMoreImportant = delaunayMesh.InsertSite(Delaunay::Point2d(tested->_position.X(), tested->_position.Z()));
    }
  }
  LogF("[m] RebuildLocationDistances done.");
}

#endif

void Landscape::ResetGeography()
{
#if USE_LANDSCAPE_LOCATIONS
  // Remove mountains from locations
  // - find all except mountains
  LocationCollectAllExceptMountains action;
  _locations.ForEachInRegion(LocationAll, action);
  // - remove all
  _locations.Clear();
  // - add all non mountains
  for (int i=0; i<action._locations.Size(); i++)
    _locations.AddLocation(action._locations[i]);

  RebuildLocationDistances();
#else
  _mountains.Clear();
#endif
  _roadNet.Free();
#if _HELISIM_AMBIENT_TRAFFIC
  _ambientRoadNetwork.Free();
#endif
}

static inline int DepthToCode(float y)
{
  if (y<-2.5f) return 3;
  else if (y<-1.0f) return 2;
  else if (y<-0.5f) return 1;
  return 0;
}

static const RStringB roadString="road";

#if _VBS3_LARGE_OBJECTS
static bool AddLargeObject(int objIx, void *ctx)
{
  LargeObjectInfo &objInfo = GLandscape->_largeStaticObjectInfos[objIx];
  if (!objInfo.object.IsReady())
  {
    DoAssert(false);
    return true;
  }
  OLinkLPtr(Object) object = objInfo.object.GetLock();
  if (object)
  {
    RefArray<Object> *largeObj = static_cast<RefArray<Object> *>(ctx);
    Object *obj = object.GetPointer();
    if (obj) 
      largeObj->Add(obj);
  }
  return true; //continue adding
}
#endif

class ObjectListXXL
{
  const ObjectList &_list;
  RefArray<Object>  _largeObj;
public:
  ObjectListXXL(const ObjectList &l, Landscape *land, int x, int z) : _list(l)
  {
#if _VBS3_LARGE_OBJECTS
    Landscape::RTreeStaticObject& largeObjects = land->LargeStaticObjects();
    float min[2]; min[0] = x*ObjGrid; min[1] = z*ObjGrid;
    float max[2]; max[0] = (x+1)*ObjGrid; max[1] = (z+1)*ObjGrid;
    largeObjects.Search(min,max,[this,&largeObjects](int largeObjectIndex)->bool {
      LargeObjectInfo &objInfo = GLandscape->_largeStaticObjectInfos[largeObjectIndex];
      if (!objInfo.object.IsReady()) return true;
      OLinkLPtr(Object) object = objInfo.object.GetLock();
      if (object.NotNull())
      {
        Object *obj = object.GetPointer();
        if (obj) 
          _largeObj.Add(obj);
      }
      return true;
    });

#endif
  }
  __forceinline int Size() const { return _list.Size()+_largeObj.Size(); }

  __forceinline Object *operator[](int index) const {return ( index<_list.Size() ) ? (Object *)_list[index] : (Object *)_largeObj[index-_list.Size()]; }
};

#define MAKE_OBJECT_LIST_XXL(LIST,IX,IZ) \
  const ObjectList& LIST##_use = _objects(IX,IZ); \
  const ObjectListXXL& LIST = ObjectListXXL(LIST##_use,this,IX,IZ);

/*!
If geography information is already present, detect it and return as fast as possible.

\patch 5128 Date 2/15/2007 by Ondra
- Fixed: AI cars not willing to drive under power lines.
*/
void Landscape::InitGeography()
{
  // check if randomization etc. is valid
  Assert( _roadNet==NULL );
#if USE_LANDSCAPE_LOCATIONS
  // - find all except mountains
  LocationCollectMountains action;
  _locations.ForEachInRegion(LocationAll, action);
  Assert(action._locations.Size() == 0);
  if (action._locations.Size() > 0)
  {
    // all data loaded from file
    // no additional processing required
    return;
  }
#else
  Assert (_mountains.Size()==0);
  if (_mountains.Size()>0)
  {
    // all data loaded from file
    // no additional processing required
    return;
  }
#endif

  _roadNet=new RoadNet;
  _roadNet->Build(this);

#if _HELISIM_AMBIENT_TRAFFIC
  _ambientRoadNetwork=new AmbientRoadNetwork;
  _ambientRoadNetwork->Load(Pars >> "CfgWorlds" >> Glob.header.worldname);
#endif

  RString name = Glob.header.worldname;
  if (name.GetLength()<=0)
  {
    char shortName[MAX_PATH];
    GetFilename(shortName,_name);
    name = shortName;
  }
  // if we are running Buldozer, world may be not registered
  extern bool LandEditor;
  if (LandEditor)
  {
    if (name[0] && !(Pars>>"CfgWorlds").FindEntry(name))
    {
      // some default initialization is necessary
/*
      for( int z=0; z<GetLandRange(); z++ )
      {
        for( int x=0; x<GetLandRange(); x++ )
        {
          _geography(x,z) = 0;
        }
      }
*/
      _terrain->_geography.Init(0);
    }
  }
  else
  {

    //DoAssert(GetLandRange()==GetTerrainRange());
    int terrainInLand = 1 << (GetTerrainRangeLog() - GetLandRangeLog());

    for( int z=0; z<GetLandRange(); z++ )
    {
      I_AM_ALIVE();
      if (GProgress) GProgress->Refresh();
      for( int x=0; x<GetLandRange(); x++ )
      {
        GeographyInfo geog(0);
        // check if the current square is in water
        //int tIndex=GetTex(x,z);
        //AbstractTexture *texture=_texture[tIndex];
        // TODO: check texture surface properties
        // check (x,z) gradient
        // gradient is in interval 0..+7 (0 == level)
        float yMin = FLT_MAX;
        float yMax = -FLT_MAX;

        for(int tz = terrainInLand * z; tz <= terrainInLand * (z + 1); tz++)          
        for(int tx = terrainInLand * x; tx <= terrainInLand * (x + 1); tx++)
        {
          float y=ClippedDataXZ(tx,tz);
          yMin = floatMin(y, yMin);
          yMax = floatMax(y, yMax);

        };
        // min is worst case for boats, however best case for land vehicles
        // it is therefore not possible to adjust for tides here
        geog.u.minWaterDepth=DepthToCode(yMax);
        geog.u.maxWaterDepth=DepthToCode(yMin);

        // we are not interested about water terrain gradient
        if (geog.u.minWaterDepth==0)
        {
          float gradMax = 0;
          for(int tz = terrainInLand * z; tz < terrainInLand * (z + 1); tz++)          
          for(int tx = terrainInLand * x; tx < terrainInLand * (x + 1); tx++)
          {
            float y=ClippedDataXZ(tx,tz);
            float yNextX=ClippedDataXZ(tx+1,tz);
            float yNextZ=ClippedDataXZ(tx,tz+1);
            float yNextXZ=ClippedDataXZ(tx+1,tz+1);    

            float gradX0=fabs((yNextX-y)*GetInvTerrainGrid());
            float gradZ0=fabs((yNextZ-y)*GetInvTerrainGrid());
            float gradX1=fabs((yNextXZ-yNextZ)*GetInvTerrainGrid());
            float gradZ1=fabs((yNextXZ-yNextX)*GetInvTerrainGrid());
            float maxGrad0=MaxGradient(gradX0,gradZ0);
            float maxGrad1=MaxGradient(gradX1,gradZ1);
            gradMax = floatMax(gradMax,floatMax(maxGrad0,maxGrad1));
          }
          geog.u.gradient = CalculateGradient(gradMax);
        }
        else
        {
          geog.u.gradient= 0;
        }
        // check how many objects are in the square
        // if there is some forest object, mark square as forest
        float totalArea=0; // sum area of all objects in the square
        int totalCount=0;
        float hardArea=0; // sum area of all objects in the square
        int hardCount=0;
        int howMany=0;
        int trees = 0;
        float xMin=x*GetLandGrid(),zMin=z*GetLandGrid();
        float xMax=xMin+GetLandGrid(),zMax=zMin+GetLandGrid();

        for( int xx=x-1; xx<=x+1; xx++ ) for( int zz=z-1; zz<=z+1; zz++ )
        {
          if (!land_InRange(xx,zz)) continue;
          MAKE_OBJECT_LIST_XXL(list,xx,zz)
          bool someRoadway = false;
          for( int oi=0; oi<list.Size(); oi++ )
          {
            Object *obj=list[oi];
            LODShape *shape=obj->GetShape();
            if( !shape ) continue; // ignore empty objects
            if (shape->FindRoadwayLevel()>=0) someRoadway = true;
            const RStringB &className = shape->GetPropertyClass();
            if( xx==x && zz==z )
            {
              if (className==roadString)
              {
                // it is road - use it as road
                geog.u.road=true;
                continue;
              }
              switch (shape->GetMapType())
              {
              case MapForestTriangle:
              case MapForestSquare:
                geog.u.forest = true;
                break;
              case MapForestBorder:
                continue;
              case MapTree:
              case MapSmallTree:
                trees++;
                break;
              }
            }
            if( obj->GetType()!=Primary ) continue; // consider only primary objects
            if( shape->Mass()<10.0 ) continue;
            if( shape->FindGeometryLevel()<0 ) continue;
            if( shape->GetGeomComponents().Size()<=0 ) continue;
            // calculate exact intersection area
            // of bounding rectangle with landscape rectangle
            // TODO: consider rotation
            float objRad=floatMax(shape->Max()[0],shape->Max()[2]);
            const Shape *geom = shape->GeometryLevel();
            // check min in the original coordinate system
            float minY = geom->Min().Y() + shape->BoundingCenter().Y();
            // if obstacle is higher than 4 m, we can safely ignore it
            if (minY>4.0f) continue;
            
            Vector3 objCenter=obj->FutureVisualState().PositionModelToWorld(shape->GeometryCenter());
            
            float xMaxObj=objCenter.X()+objRad,xMinObj=objCenter.X()-objRad;
            float zMaxObj=objCenter.Z()+objRad,zMinObj=objCenter.Z()-objRad;
            if( xMaxObj<=xMin || xMinObj>=xMax ) continue;
            if( zMaxObj<=zMin || zMinObj>=zMax ) continue;
            saturateMin(xMaxObj,xMax),saturateMax(xMinObj,xMin);
            saturateMin(zMaxObj,zMax),saturateMax(zMinObj,zMin);
            float area=(xMaxObj-xMinObj)*(zMaxObj-zMinObj);
            totalArea+=area;
            totalCount++;
            if (dyn_cast<ForestPlain>(obj)==NULL)
            {
              if( obj->GetDestructType()==DestructNo )
              {
                // undestroyable obstacles calculate much more
                hardArea+=area*4;
                hardCount+=2;
              }
              else if
              (
                obj->GetDestructType()!=DestructTree &&
                obj->GetDestructType()!=DestructTent
              )
              {
                // normal objects are hard to maneuver
                hardArea+=area;
                hardCount+=1;
              }
            }
          }
        }
        MAKE_OBJECT_LIST_XXL(list,x,z)
        if (list.Size()>0)
        {
          geog.u.someObjects = true;
          bool someRoadway = false;
          // check roadway
          for (int i=0; i<list.Size(); i++)
          {
            Object *obj = list[i];
            LODShape *shape = obj->GetShape();
            if (shape && shape->FindRoadwayLevel()>=0)
            {
              someRoadway = true;
              break;
            }
          }
          geog.u.someRoadway = someRoadway;
        }
        else
        {
          geog.u.someObjects = false;
          geog.u.someRoadway = false;
        }
        // filled rectangles are assumed forest
        if( totalArea>=(GetLandGrid()*GetLandGrid())*9/10 ) geog.u.full=1;
        else if( totalArea>=(GetLandGrid()*GetLandGrid())/2 || totalCount>=20 ) {if( howMany<3 ) howMany=3;}
        else if( totalArea>=(GetLandGrid()*GetLandGrid())/4 || totalCount>=10 ) {if( howMany<2 ) howMany=2;}
        else if( totalCount>0 ) {if( howMany<1 ) howMany=1;}
        // detect antitank obstacles
        int hard=0;
        if( hardArea>=(GetLandGrid()*GetLandGrid())/3 || hardCount>=8 ) {if( hard<3 ) hard=3;}
        else if( hardArea>=(GetLandGrid()*GetLandGrid())/6  || hardCount>=4 ) {if( hard<2 ) hard=2;}
        else if( hardCount>0 ) {if( hard<1 ) hard=1;}
        

        geog.u.howManyObjects=howMany;
        geog.u.howManyHardObjects=hard;

        if( geog.u.road )
        { // road is always well penetrable
          geog.u.full=false;
          //geog.waterDepth=0;
          if( geog.u.gradient>5 ) geog.u.gradient=5;
          if( geog.u.howManyObjects>2 ) geog.u.howManyObjects=2;
        }

        if (_minTreesInForestSquare > 0 && trees >= _minTreesInForestSquare)
        {
          geog.u.forest = true;
        }

        _terrain->SetGeographyRaw(x,z,geog);
      }
    }

    //RptF("%s: Quad tree memory %d",cc_cast(GetName()),_geography.GetMemoryAllocated());
    
    if (name.GetLength()>0)
    {
      ReplaceObjects(name);
    }
  }
    
  I_AM_ALIVE();
  if (GProgress) GProgress->Refresh();
  InitSoundMap();
  I_AM_ALIVE();
  if (GProgress) GProgress->Refresh();
  InitRandomization();
  I_AM_ALIVE();
  if (GProgress) GProgress->Refresh();
#if _VBS3
  if(!LandEditor)
#endif
  InitMountains();
  LogF("[InitGeography] done");
}

struct ReplaceObjectInfo
{
  Ref<LODShape> replace;
  RefArray<LODShape> with;
  bool center;
};
TypeIsMovableZeroed(ReplaceObjectInfo)

/*!
\patch 1.52 Date 4/22/2002 by Jirka
- Added: Support for multiple types of forests on single island
\patch_internal 1.85 Date 9/17/2002 by Ondra
- Fixed: Bug in binarization caused same object ID was used twice.
\patch 1.85 Date 9/17/2002 by Ondra
- Fixed: Some object ID for static objects on all islands were used twice.
*/

void Landscape::ReplaceObjects(RString name)
{
  ResetObjectIDs();
  // decide which objects to replace
  ParamEntryVal world = Pars >> "CfgWorlds">> name;
  
  ConstParamEntryPtr entry = world.FindEntry("ReplaceObjects");
  if (!entry) return;
  int n = entry->GetEntryCount();
  if (n == 0) return;

  AutoArray<ReplaceObjectInfo> replace;
  replace.Realloc(n);
  replace.Resize(n);
  
  for (int i=0; i<n; i++)
  {
    ParamEntryVal cls = entry->GetEntry(i);
    ReplaceObjectInfo &info = replace[i];

    RString value = cls >> "replace";
    info.replace = Shapes.New(value, false, true);
    int m = (cls >> "with").GetSize();
    info.with.Realloc(m);
    info.with.Resize(m);
    for (int j=0; j<m; j++)
    {
      RString value = (cls >> "with")[j];
      info.with[j] = Shapes.New(value, false, true);
    }
    info.center = cls.FindEntry("center") ? cls >> "center" : false;
  }

  // replace objects
  for (int z=0; z<GetLandRange(); z++)
  {
    I_AM_ALIVE();
    if (GProgress) GProgress->Refresh();
    for (int x=0; x<GetLandRange(); x++)
    {
      const ObjectList &src = _objects(x, z);
      int on = src.Size();
      AUTO_STATIC_ARRAY(Ref<Object>,copy,64);
      //copy.Realloc(on);
      copy.Resize(on);
      for (int oi=0; oi<on; oi++) copy[oi] = src[oi];
      for (int oi=0; oi<on; oi++)
      {
        Object *obj = copy[oi];
        LODShape *shape = obj->GetShape();
        if (!shape) continue; // ignore empty objects

        // find in replace list
        for (int i=0; i<replace.Size(); i++)
        {
          if (replace[i].replace == shape)
          {
            VisitorObjId id = obj->ID();
            bool idUsed = false;

            // delete old object
            for (int j=0; j<src.Size(); j++)
            {
              if (src[j] == obj)
              {
                // src.Delete(j);
                src.GetList()->Delete(j);
                if (src.CanBeDeleted())
                  _objects.Set(x, z, ObjectList());
                break;
              }
            }
            
            // add new objects
            for (int j=0; j<replace[i].with.Size(); j++)
            {
              LODShape *newShape = replace[i].with[j];
              if (newShape)
              {
                Matrix4 transform = obj->FutureVisualState().Transform();
                
                if (idUsed) id = NewObjectID();
                else idUsed = true;
                
                Assert(id>=0);
                
                CreateObjectId oid(id,NewObjectId(id,x,z));

                Object *newObject = NewObject(newShape, oid);

                if (replace[i].center)
                {
                  // keep object centered
                  Vector3 pos;
                  pos.Init();
                  pos[0] = x * GetLandGrid() + 0.5 * GetLandGrid();
                  pos[2] = z * GetLandGrid() + 0.5 * GetLandGrid();
                  pos[1] = SurfaceY(pos[0], pos[2]);
                  pos += transform.Rotate(newObject->GetShape()->BoundingCenter());
                  transform.SetTranslation(pos);
                }
                else
                {
                  // maintain bcenter position
                  // here is the zero for the old object
                  Vector3 oldZero = obj->FutureVisualState().PositionModelToWorld(-obj->GetShape()->BoundingCenter());
                  // here is the zero for the new object
                  Vector3 newZero = transform.FastTransform(-newShape->BoundingCenter());
                  // move the object so that newZero moves to oldZero
                  transform.SetPosition(transform.Position()+oldZero-newZero);
                }

                #ifdef INTERESTED_IN_ID
                  if (newObject->ID()==INTERESTED_IN_ID)
                  {
                    Vector3 pos = transform.Position();
                    LogF(
                      "Id %d: pos %.3f,%.3f,%.3f",
                      newObject->ID(), pos.X(),pos.Y(),pos.Z()
                    );
                  }
                #endif
                
                if (newObject->InitSkewReplace(this,transform))
                  newObject->SetTransformSkewed(transform);
                else
                  newObject->SetTransform(transform);

                newObject->SetType(Primary);
                AddObject(newObject);
              }
            }
            break;
          }
        }
      }
    }
  }
}

#if !USE_LANDSCAPE_LOCATIONS
#include <Es/Algorithms/qsort.hpp>

static int CmpMountains(const Vector3 *v0, const Vector3 *v1)
{
  return sign(v1->Y() - v0->Y());
}
#endif

bool Landscape::CheckMountain(int x, int z, float radius, float minDiff)
{
  const float pi = 3.141592654;
  const int steps = 12;  // number of points around (x,z) to check

  float top = GetData(x, z);
  float xx = x * GetTerrainGrid();
  float zz = z * GetTerrainGrid();

  for (int i = 0; i < steps; i++)
  {
    float angle = 2*pi*i / steps;
    float rx = cos(angle) * radius;
    float rz = sin(angle) * radius;

    if (top - SurfaceY(xx + rx, zz + rz) > minDiff)
      return true;
  }

  return false; // not a significant mountain
}

void Landscape::InitMountains()
{
#if USE_LANDSCAPE_LOCATIONS
  // _locations.Clear();
  Ref<LocationType> type = LocationTypes.New("Mount");

  // found local maxims
  for (int z=1; z<GetTerrainRangeMask(); z++) for (int x=1; x<GetTerrainRangeMask(); x++)
  {
    float h11 = GetData(x, z);
    if (h11 <= 0) continue;
    float h00 = GetData(x - 1, z - 1);
    float h01 = GetData(x, z - 1);
    float h02 = GetData(x + 1, z - 1);
    float h10 = GetData(x - 1, z);
    float h12 = GetData(x + 1, z);
    float h20 = GetData(x - 1, z + 1);
    float h21 = GetData(x, z + 1);
    float h22 = GetData(x + 1, z + 1);

    if
      (
      h11 > h00 && h11 > h01 && h11 > h02 && h11 > h10 &&
      h11 >= h12 && h11 >= h20 && h11 >= h21 && h11 >= h22
      )
    {

#if _HELISIM_MAPS
      //on takistany testing map scaling created stairs on slopes, this created 790000 fake mountains
      if(z<GetTerrainRangeMask()-1 && z >1 && x<GetTerrainRangeMask() -1 && x> 1)
      {
        h12 = (h11 == h12)? GetData(x + 2, z):h12;
        h20 = (h11 == h20)? GetData(x - 2, z + 2):h20;
        h21 = (h11 == h21)? GetData(x, z + 2):h21;
        h22 = (h11 == h22)? GetData(x + 2, z + 2):h22;

        if(h11 < h12 || h11 < h20 || h11 < h21 || h11 < h22) continue;
      }

      if (!CheckMountain(x, z, 80, 10) &&
        !CheckMountain(x, z, 40,  5)) continue;
#endif

      Location *location = new Location(type, Vector3(x * GetTerrainGrid(), h11, z * GetTerrainGrid()), 0, 0);
      location->_importance = h11;
      _locations.AddLocation(location);
    }
  }

  RebuildLocationDistances();
#else
  _mountains.Clear();
  // found local maximas
  for (int z=1; z<GetTerrainRangeMask(); z++) for (int x=1; x<GetTerrainRangeMask(); x++)
  {
    float h11 = GetData(x, z);
    if (h11 <= 0) continue;
    float h00 = GetData(x - 1, z - 1);
    float h01 = GetData(x, z - 1);
    float h02 = GetData(x + 1, z - 1);
    float h10 = GetData(x - 1, z);
    float h12 = GetData(x + 1, z);
    float h20 = GetData(x - 1, z + 1);
    float h21 = GetData(x, z + 1);
    float h22 = GetData(x + 1, z + 1);

    if
    (
      h11 > h00 && h11 > h01 && h11 > h02 && h11 > h10 &&
      h11 >= h12 && h11 >= h20 && h11 >= h21 && h11 >= h22
    )
    {
      _mountains.Add(Vector3(x * GetTerrainGrid(), h11, z * GetTerrainGrid()));
    }
  }

  QSort(_mountains.Data(), _mountains.Size(), CmpMountains);
#endif
}

void Landscape::AddSoundRegion(int x, int z, int size, EnvType type, float value)
{
  for( int zz=-size; zz<=+size; zz++ ) for( int xx=-size; xx<=+size; xx++ )
  {
    if (!land_InRange(x+xx,z+zz)) continue;
    float distance2 = (zz*zz+xx*xx)/Square(size);
    float atten = distance2>0 ? 0.25/distance2 : 1;
    if (atten<1) atten = 1;
    EnvTypeInfo val(_soundMap.Get(x+xx,z+zz));
    float attenValue = atten*value;
    if (val.GetEnvType(type)<attenValue)
    {
      val.SetEnvType(type,attenValue);
      _soundMap.Set(x+xx,z+zz,val.packed);
    }
  }
}

DEF_RSB(forest);
DEF_RSB(treehard);
DEF_RSB(treesoft);
DEF_RSB(bushhard);
DEF_RSB(bushsoft);
DEF_RSB(house);
DEF_RSB(church);

void Landscape::InitSoundMap()
{
  // some basic environmental sounds
  int x,z;
  // set default sound - no environment
  _soundMap.Init(0);
  for( z=0; z<GetLandRange(); z++ ) for( x=0; x<GetLandRange(); x++ )
  {
    // count trees
    float nTrees = 0;
    float nHouses = 0;
    const ObjectList &list=_objects(x,z);
    for( int i=0; i<list.Size(); i++ )
    {
      Object *obj=list[i];
      LODShape *shape=obj->GetShape();
      if( !shape ) continue;
      if (shape->GetPropertyClass()==RSB(treehard)) nTrees += 0.50f;
      if (shape->GetPropertyClass()==RSB(treesoft)) nTrees += 0.33f;
      if (shape->GetPropertyClass()==RSB(bushhard)) nTrees += 0.10f;
      if (shape->GetPropertyClass()==RSB(bushsoft)) nTrees += 0.03f;
      if (shape->GetPropertyClass()==RSB(forest)) nTrees += 1;
      if (shape->GetPropertyClass()==RSB(house)) nHouses += 0.5f;
      if (shape->GetPropertyClass()==RSB(church)) nHouses += 1.0f;
    }
    if( nTrees>0 )
    {
      AddSoundRegion(x,z,1,EnvTrees,nTrees);
    }
    if (nHouses>0)
    {
      AddSoundRegion(x,z,1,EnvHouses,nHouses);
    }
  }
  // spread sea sound around the sea
  for( z=0; z<GetLandRange(); z++ ) for( x=0; x<GetLandRange(); x++ )
  {
    GeographyInfo geogr=_terrain->GetGeographyRaw(x,z);
    if( geogr.u.maxWaterDepth>0 )
    { // sea is very loud and can be heard very far
      // TODO: sea sound is not that loud once we are on open sea
      AddSoundRegion(x,z,2,EnvSea,1);
    }
  }
  // assume meadow is where nothing else is
  for( z=0; z<GetLandRange(); z++ ) for( x=0; x<GetLandRange(); x++ )
  {
    EnvTypeInfo info(_soundMap(x,z));
    float sea = info.GetEnvType(EnvSea);
    float trees = info.GetEnvType(EnvTrees);
    float houses = info.GetEnvType(EnvHouses);
    
    float meadow = (1-sea)*(1-trees)*(1-houses);
    info.SetEnvType(EnvMeadow,meadow);
    _soundMap.Set(x,z,info.packed);
  }
  
}


void Landscape::InitDynSounds( ParamEntryPar entry )
{
  // init dynamic sounds
  // load config
  ParamEntryVal sounds=entry>>"sounds";
  for( int i=0; i<sounds.GetSize(); i++ )
  {
    ParamEntryVal source=entry >> (sounds[i]).operator RString();
    RString sim=source>>"simulation";
    ParamEntryVal pos=source>>"position";
    float posX=pos[0];
    float posZ=pos[1];
    float posY=RoadSurfaceY(posX,posZ);
    Vector3 position(posX,posY,posZ);
    DynSoundSource *vehicle=new DynSoundSource(sim);
    vehicle->SetPosition(position);
    //GLOB_WORLD->AddVehicle(vehicle);
    GLOB_WORLD->AddSlowVehicle(vehicle);
  }
}

inline int AvoidSteepChange( int val, int x, bool &someChange )
{
  // never increase abs of val
  if( abs(x)>abs(val) ) return val;
  const int maxDif=2;
  int dif=val-x;
  if( dif>maxDif ) dif=maxDif;
  else if( dif<-maxDif ) dif=-maxDif;
  if( dif ) someChange=true;
  return x+dif;
}

void Landscape::InitRandomization()
{
}

static inline int RoundToSegment(int x, int landSegmentSize, int landSegmentAlign)
{
  //landSegmentAlign = 0;
  int xMod = (x-landSegmentAlign)%landSegmentSize;
  // operator % is not handling negative numbers as modulo
  if (xMod<0) xMod += landSegmentSize;
  return x-xMod;
}


void Landscape::InitWaterInSeg(bool empty)
{
  const int subdivisionLevel = GetTerrainRangeLog()-GetLandRangeLog();
  int subdivision = 1<<subdivisionLevel;
  
  float landSize = GetLandRange()*GetLandGrid();

  DWORD start = GlobalTickCount();
  // initialize water map (for water segments)
  int waterSegSize = GetWaterSegmentSize();
  int range = toIntCeil(landSize/(WaterGrid*waterSegSize));
  _waterInSeg.Dim(range,range);
  if (empty)
  {
    // sometimes we are not performing any real init, only reset
    for (int zr=0; zr<range; zr++)
    for (int xr=0; xr<range; xr++)
    {
      _waterInSeg(xr,zr) = 0;
    }
    _terrain->_maxHeight = 0;
    ClearGrassMap();
  }
    else
    {

  // perform terrain to segment mapping
  // walk through the whole terrain
  
  for (int zr=0; zr<range; zr++)
  for (int xr=0; xr<range; xr++)
  {
    // water grid rectangle coordinates
    float xBeg = xr*WaterGrid*waterSegSize;
    float zBeg = zr*WaterGrid*waterSegSize;
    float xEnd = xBeg+WaterGrid*waterSegSize;
    float zEnd = zBeg+WaterGrid*waterSegSize;
    // corresponding terrain grid rectangle
    GridRectangle rectSD;
    rectSD.xBeg = toIntFloor(xBeg*GetInvLandGrid()*subdivision);
    rectSD.xEnd = toIntCeil (xEnd*GetInvLandGrid()*subdivision);
    rectSD.zBeg = toIntFloor(zBeg*GetInvLandGrid()*subdivision);
    rectSD.zEnd = toIntCeil (zEnd*GetInvLandGrid()*subdivision);

    float minHeight = FLT_MAX;
    for(int z=rectSD.zBeg; z<=rectSD.zEnd; z++)
    for(int x=rectSD.xBeg; x<=rectSD.xEnd; x++)
    {
      float l=ClippedDataXZ(x,z);
      if (minHeight>l) minHeight = l;
    }

    float waterEnc = minHeight*WaterMapScale;
    saturate(waterEnc,-127,+127);
    _waterInSeg(xr,zr) = toInt(waterEnc);
  }
    }
  
  if (!empty)
  {
    LogF("Init water (%s): %d ms",cc_cast(_name),GlobalTickCount()-start);
  }
}

void Landscape::InitWaterAndGrassMap(bool empty)
{
  InitWaterInSeg(empty);

  if (empty)
  {
    _terrain->_maxHeight = 0;
    ClearGrassMap();
    return;
  }
  
  // calculate max. height at the same time as calculating water
  RawType maxRaw = 0;
  for (int z=0; z<GetTerrainRange(); z++) for (int x=0; x<GetTerrainRange(); x++)
  {
    RawType raw = _terrain->_data(x,z);
    if (raw>maxRaw) maxRaw = raw;
  }
  
  _terrain->_maxHeight = RawToHeight(maxRaw);
  
  // before calling InitGrassMap we need to make sure clutter mapping is ready
  BuildClutterMapping();

  if (_grassApprox.GetXRange()==0)
  {
    // if needed, init the grass map
    InitGrassMapEmpty();
  }
}

