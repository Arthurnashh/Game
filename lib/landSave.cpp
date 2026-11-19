// Landscape - generalized landscape drawing
// (C) 1996, SUMA
#include "wpch.hpp"

#include "landscape.hpp"
#include "landImpl.hpp"
#include <El/QStream/qbStream.hpp>
#include <El/FileServer/fileServer.hpp>
#include <El/ParamFile/paramFile.hpp>
#include "paramArchiveExt.hpp"
#include <El/Rectangle/rectangle.hpp>
#include "AI/ai.hpp"
#include "objectClasses.hpp"
#include "Shape/mapTypes.hpp"
#include "Shape/material.hpp"
#include <El/HiResTime/hiResTime.hpp>

#include "timeManager.hpp"
#include "Shape/poly.hpp"
#include "object.hpp"
#include "camera.hpp"
#include "scene.hpp"
#include "global.hpp"
#include "progress.hpp"
#include "objectId.hpp"
#include "roads.hpp"

#include "stringtableExt.hpp"

#include <El/Math/math3dP.hpp>

#include "landFile.hpp"
#include "serializeBinExt.hpp"

#include <time.h>

#include <El/Common/perfLog.hpp>
#include <El/Common/perfProf.hpp>

#include <Es/Strings/bString.hpp>

#if _DEBUG // _PROFILE || _DEBUG
  //#define INTERESTED_IN_ID 15041
#endif

//#define LAND_TEXTURES_MAX 256
#define LAND_TEXTURES_MAX 512

LSError Landscape::LoadData(QIStream &f, ParamEntryPar cls, RString cachePath)
{
  ProgressReset();

  //FreeIds();

  IProgressDisplay *CreateDisplayLoadIsland(RString text, RString date);
  Ref<ProgressHandle> p = ProgressStart(CreateDisplayLoadIsland(LocalizeString(IDS_LOAD_WORLD), RString()));

  int lastRest = f.rest();
  ProgressAdd(lastRest);

  Log("Begin landscape load (%d KB).",MemoryUsed()/1024);

  int i;
  int x, z;

  Init(cls); // clear the landscape
    
  FlushSegCache();
  _fileVersion = 0;
  
  //WorldHeader header;
  int magic;
  f.read((char *)&magic,sizeof(magic));
  
  if( f.fail() )
  {
    ProgressFinish(p);
    return LSUnknownError;
  }
  int version=0;
  if (magic==FILE_MAGIC_8 ) version=8;
  else if (magic==FILE_MAGIC_7 ) version=7;
  else if (magic==FILE_MAGIC_6 ) version=6;
  else if( magic==FILE_MAGIC_5 ) version=5;
  else if( magic==FILE_MAGIC_4 ) version=4;
  else if( magic==FILE_MAGIC_3 ) version=3;
  else if( magic==FILE_MAGIC ) version=2;
  else
  {
    ProgressFinish(p);
    return LSUnknownError;
  }

  f.seekg(0, QIOS::beg);
  WorldHeader5 header;
  if (version <= 4)
  {    
    WorldHeader headerOld;
    f.read((char *)&headerOld,sizeof(headerOld));

    header.xTexRange = headerOld.xRange;
    header.zTexRange = headerOld.zRange;
    header.xTerrainRange = headerOld.xRange;
    header.zTerrainRange = headerOld.zRange;
    header.texGrid = cls >> "LandGrid";
  }
  else  
    f.read((char *)&header,sizeof(header));
  
  Dim(header.xTexRange,header.zTexRange,header.xTerrainRange,header.zTerrainRange,header.texGrid);

  FlushCache();

  for( z=0; z<GetLandRange(); z++ ) for( x=0; x<GetLandRange(); x++ )  
    SetTex(x,z,0);
  for( z=0; z<GetTerrainRange(); z++ ) for( x=0; x<GetTerrainRange(); x++ )
    _terrain->SetData(x,z,0);
  // each change in _data will change this as needed
  _terrain->_maxHeight = 0;

  if (version>=8)
  {
    for( z=0; z<header.zTerrainRange; z++ ) for( x=0; x<header.xTerrainRange; x++ )
    {
      float val;
      f.read((char *)&val,sizeof(val));
      if ((x|z)<GetTerrainRange())
        _terrain->SetData(x,z,val);
    }
  }
  else
  {
    for( z=0; z<header.zTerrainRange; z++ ) for( x=0; x<header.xTerrainRange; x++ )
    {
      short val;
      f.read((char *)&val,sizeof(val));
      float data = val*LANDDATA_SCALE_WRP;
      if ((x|z)<GetTerrainRange())
        _terrain->SetData(x,z,data);
    }
  }
  for( z=0; z<header.zTexRange; z++ ) for( x=0; x<header.xTexRange; x++ )
  {
    short val;
    f.read((char *)&val,sizeof(val));
    if( (x|z)<GetLandRange() )
    {
      if( val<0 || ((version <= 5) && val>LAND_TEXTURES_MAX))
      {
        BString<256> msg;
        sprintf(msg,"Bad texture ID %d on landscape (%d,%d)" ,val, x,z );
        Fail(msg.cstr());
        val=0;
      }
      SetTex(x,z,val);
    }
  }

  ProgressFrame();

  if (version <= 5)
  {
    for( i=0; i<LAND_TEXTURES_MAX; i++ )
    {
      char texName[32];
      f.read(texName,sizeof(texName));
      if( *texName )
      {
        Assert(!strstr(texName,"$$")); // SetTexture(i,"landText\\pi.pac");
        SetTexture(i,texName,0);
      }
      ProgressAdvance(lastRest - f.rest());
      lastRest = f.rest();
      ProgressRefresh();
    }
  }
  else
  {
    unsigned int nTextures;
    f.read(&nTextures,sizeof(nTextures));

    unsigned int bufferSize = 4096;
    char * texName = new char[bufferSize];  

    for(unsigned int i=0; i<nTextures; i++ )
    {
      unsigned int length;
      f.read(&length,sizeof(length));
      if (length == 0)
        continue;

      if (length + 1 > bufferSize)
      {      
        delete [] texName;
        bufferSize = length + 1;
        texName = new char[bufferSize];
      }

      f.read(texName,length);
      texName[length] = '\0';
      int major = -1;
      if (version>=7)
        f.read(&major,sizeof(major));
      if( *texName )
      {
        Assert(!strstr(texName,"$$"));
        SetTexture(i,texName,major);
      }
      ProgressAdvance(lastRest - f.rest());
      lastRest = f.rest();
      ProgressRefresh();
    }
    delete[] texName;
  }

#if 1 // validate texture indexes
  for( z=0; z<GetLandRange(); z++ ) for( x=0; x<GetLandRange(); x++ )  
  {
    int textInd = GetTex(x,z);
    if (textInd >= _terrain->_texture.Size())
    {
      RptF("Bad land texture on %d,%d",x,z);
      SetTex(x,z,0);
    }
  }
#endif
  _terrain->_outsideTextureInfo._mat = LoadOutsideMaterial(cls,_terrain->_texture[_terrain->_tex.Get(0,0)]._mat,_terrain->_terSynth);

  if( f.fail() )
  {
    ProgressFinish(p);
    return LSUnknownError;
  }

  #if _ENABLE_REPORT
  const int maxID=65536;
  bool isID[maxID];
  memset(isID,0,sizeof(isID));
  #endif

  // load all objects
  //bool someForest=false;
  int countObjects=0;
  int maxObjID=-1;

  unsigned int bufferSize = 4096;
  char * objName = new char[bufferSize];
  
  for(;;)
  {
    countObjects++;
    if (version >= 6)
    {
      SingleObject6 so;
      f.read(&so,sizeof(so));
      if (f.fail() || f.eof())
        break;
      if (!so.nameLength)
        break;
#if _ENABLE_REPORT
      saturateMax(maxObjID,so.id);
      if (so.id<maxID && isID[so.id])
        RptF("Conflict id %d",so.id);
      if (so.id<maxID)
        isID[so.id] = true;
#endif
      Assert(so.id>=0);
      if (so.nameLength + 1 > bufferSize)
      {      
        delete [] objName;
        bufferSize = so.nameLength + 1;
        objName = new char[bufferSize];
      }

      f.read(objName,so.nameLength);
      objName[so.nameLength] = '\0';

      RString fullName;
#ifdef _HELISIM_MAPS
      if (!cachePath.IsEmpty())
      {
        QFileTime obj1Time = QFBankQueryFunctions::TimeStamp(cachePath+objName);
        QFileTime obj2Time = QFBankQueryFunctions::TimeStamp(objName);
        if (obj1Time>=obj2Time) 
        {
          fullName = cachePath+objName;
        }
        else
          RptF("using non-binarized object %s",objName);
      }
#endif

      // load binarized object
      Object *obj = ObjectCreate(so.id,objName,ConvertToM(so.matrix),false,fullName);
      if (!obj)
      {
        // otherwise load MLOD model but it'll slow down map binarization
        obj = ObjectCreate(so.id,objName,ConvertToM(so.matrix),false);
        if (obj)
          RptF("unable to use binarized object %s, used non-binarized instead",objName);
        else
          RptF("unable to use neither objects %s (binarized as well as non-binarized)",objName);
      }
      (void)obj;

#ifdef INTERESTED_IN_ID
        if (so.id==INTERESTED_IN_ID)
        {
          Vector3 pos = so.matrix.Position();
          Vector3 oPos = obj->Position();
          LODShape *shape = obj->GetShape();
          Vector3 bcWorld = obj->PositionModelToWorld(-shape->BoundingCenter());
          float sourceSurfY = SurfaceYSource(pos.X(),pos.Z());
          LogF("Id %d: pos %.2f,%.2f,%.2f, surfY %.2f",so.id,pos.X(),pos.Y(),pos.Z(), sourceSurfY);
        }
#endif
    }
    else if (version<3)
    {
      SingleObject so;
      f.read(&so,sizeof(so));
      if (f.fail() || f.eof())
        break;
      if (!*so.name)
        break; // object list terminated
      float x=so.x*GetLandGrid();
      float z=so.z*GetLandGrid();
      float y=SurfaceY(x,z)+so.y;
      Vector3 oPos(x,y,z);
      AddObject(x,y,z,so.heading,so.name);
    }
    else if(version==3)
    {
      SingleObject3 so;
      f.read(&so,sizeof(so));
      if (f.fail() || f.eof())
        break;
      if (!*so.name)
        break; // object list terminated
      int id=NewObjectID();
      ObjectCreate(id,so.name,ConvertToM(so.matrix));
    }
    else
    {
      SingleObject4 so;
      f.read(&so,sizeof(so));
      if (f.fail() || f.eof())
        break;
      if (!*so.name)
        break; // object list terminated
#if _ENABLE_REPORT
        saturateMax(maxObjID,so.id);
        if (so.id<maxID && isID[so.id])
          RptF("Conflict id %d",so.id);
        if (so.id<maxID)
          isID[so.id] = true;
#endif
      Assert(so.id>=0);
      ObjectCreate(so.id,so.name,ConvertToM(so.matrix));
    }
    ProgressAdvance(lastRest - f.rest());
    lastRest = f.rest();
    ProgressRefresh();
  }
  delete [] objName;
  
#if _VBS3_LARGE_OBJECTS_WRP
  AddLargeStaticObjectsToRTree();
#endif  
  
  LogF("Total %d objects, max. id %d",countObjects,maxObjID);
  
  _objectId = _objectIdPrimary = VisitorObjId(maxObjID);

  ProgressFrame();

  Log("Landscape loaded. (%d KB).",MemoryUsed()/1024);

  InitGeography();
  InitMapObjects();
  InitCache(true);
  // calling InitGrassMap makes sure binarize processes the grass 
  BuildClutterMapping();
  InitGrassMap();
  // InitWaterAndGrassMap will not perform InitGrass, as it was already done
  InitWaterAndGrassMap(false);
  InitPrimaryTexture();
  _terrain->InitOutsideTerrain();

  FixDoubleIds();

  ProgressFinish(p);

  return LSOK;
}

#if _VBS3_LARGE_OBJECTS_WRP
void Landscape::AddLargeStaticObjectsToRTree()
{
  //PROFILE_SCOPE_LOG(Landscape_AddLargeStaticObjectsToRTree, "Adding of large static objects to R-tree.")
  for (int z = 0; z < GetLandRange(); ++z)
    for (int x = 0; x < GetLandRange(); ++x)
    {
      const ObjectList &list = _objects(x,z);
      for (int i = 0; i < list.Size(); ++i)
      {
        Object* obj = list[i];
        if (IsObjectLarge(obj))
        {
          float min[2]; float max[2];
          obj->CalcBRect(min[0], min[1], max[0], max[1], false);
          int64 pos = ((int64)(toIntFloor(obj->FutureVisualState().Position().X()*GetInvLandGrid()))<<32) | (int64)toIntFloor(obj->FutureVisualState().Position().Z()*GetInvLandGrid());
          _largeStaticObjects.Insert(min, max, pos);

          LogF("{warning} Object %s is larger than GetLandGrid() (%fm), adding to R-tree for large static objects.", (const char*)obj->GetDebugName(), GetLandGrid());
        }
      }
    }
}
#endif

#if _VBS3_LARGE_OBJECTS && _ENABLE_CHEATS
bool Landscape::AddLargeStaticObjectToRTree(Object *obj)
{
  if (IsObjectLarge(obj))
  {
    float min[2]; float max[2];
    obj->CalcBRect(min[0], min[1], max[0], max[1], false);
    //int64 pos = ((int64)(toIntFloor(obj->FutureVisualState().Position().X()*_invLandGrid))<<32) | (int64)toIntFloor(obj->FutureVisualState().Position().Z()*_invLandGrid);
    int objIx = _largeStaticObjectInfos.Add(LargeObjectInfo(obj));
    _largeStaticObjects.Insert(min, max, objIx);

    LogF("{warning} Object %s is larger than GetLandGrid() (%fm), adding to R-tree for large static objects.", (const char*)obj->GetDebugName(), GetLandGrid());
    return true;
  }
  return false;
}

#endif

void Landscape::FixDoubleIds()
{
  AutoArray< InitPtr<Object> > idCache;
  AutoArray< AutoArray< InitPtr<Object> > > conflicts;

  for (int z=0; z<GetLandRange(); z++) for (int x=0; x<GetLandRange(); x++)
  {
    const ObjectList &list = _objects(x,z);
    for (int i=0; i<list.Size(); i++)
    {
      Object *obj = list[i];
      int id = obj->ID();
      if (id<0)
        continue;
      idCache.Access(id);
      if (idCache[id])
      {
        // conflict detected
        conflicts.Access(id);
        conflicts[id].Add(obj);
      }
      else
        idCache[id] = obj;
    }
  }
  //int lastID = idCache.Size()-1;
  for (int i=0; i<conflicts.Size(); i++)
  {
    if (conflicts[i].Size()==0) continue;
    Object *o1 = idCache[i];
#if _HELISIM_LARGE_OBJECTID
    RptF("Double id %I64d detected (%dx)",o1->ID().Encode(),conflicts[i].Size());
#else
    RptF("Double id %d detected (%dx)",o1->ID().Encode(),conflicts[i].Size());
#endif
    for (int j=0; j<conflicts[i].Size(); j++)
    {
      Object *o2 = conflicts[i][j];
      DoAssert(o2->ID()==o1->ID());
      // swap so that O1 contains valid id
      if (dyn_cast<ForestPlain>(o1))
        Swap(o1,o2);
      // check new ID
      //LogF("Setting new id %d for %d:%s",newID,o2->ID(),o2->GetShape()->Name());
      o2->ChangeVisitorId(NewObjectID());
    }
  }
  //SetLastObjectID(VisitorObjId(newId));
}

int Landscape::LoadData(const char *name, ParamEntryPar cls, RString cachePath)
{
  #if _ENABLE_REPORT
  DWORD start = GlobalTickCount();
  LogF("Landscape::LoadData start");
  Log("Load landscape %s",name);
  #endif
  _name=name; // remember name
  LSError ret=LSUnknownError;
  // FIX - allow worlds in banks
  _stream.close();
  _stream.AutoOpen(name);
  if (_stream.fail())
  {
    // nothing
  }
  else if (LoadOptimized(_stream, cls))
    ret = LSOK;
  else
  {
    _stream.seekg(0,QIOS::beg);
    ret = LoadData(_stream, cls, cachePath);
  }
  if (ret != LSOK)
    ErrorMessage("Cannot load world '%s'",name);
#if _ENABLE_REPORT
  LogF("Landscape::LoadData time %d",GlobalTickCount()-start);
#endif
  return ret;
}

bool Landscape::SaveXYZ(QOFStream &f) const
{
  int x,z;
  for( z=0; z<GetTerrainRange(); z++ ) for( x=0; x<GetTerrainRange(); x++ )
  {
    float height = GetData(x,z);
    char buff[256];
    sprintf(buff,"%g\t%g\t%g\r\n",x*GetTerrainGrid(),z*GetTerrainGrid(),height);
    f.write(buff,strlen(buff));
  }
  return true;
}

bool Landscape::SaveXYZ(const char *name) const
{
  QOFStream out(name);
  if (out.error()!=0) return false;
  if (!SaveXYZ(out)) return false;
  return out.error()==0;
}

void Landscape::SaveData(QOStream &f) const
{
  int i;
  int x,z;

  //f.read((char *)&temp,sizeof(temp));
  WorldHeader5 header;

  header.magic=FILE_MAGIC_8;
  header.xTexRange=GetLandRange();
  header.zTexRange=GetLandRange();
  header.xTerrainRange=GetTerrainRange();
  header.zTerrainRange=GetTerrainRange();
  header.texGrid = GetLandGrid();

  f.write((char *)&header,sizeof(header));

  for( z=0; z<GetTerrainRange(); z++ ) for( x=0; x<GetTerrainRange(); x++ )
  {
    float height = GetData(x,z);
    /*
    const float maxheight = LANDDATA_SCALE_WRP * SHRT_MAX;
    const float minheight = LANDDATA_SCALE_WRP * SHRT_MIN;
    short val; 
    if (height >= maxheight)
      val = SHRT_MAX;
    else
      if (height <= minheight)
        val = SHRT_MIN;
      else
        val=toIntFloor(GetData(x,z)/LANDDATA_SCALE_WRP);

    //short val=HeightToShort(GetData(x,z));
    f.write((char *)&val,sizeof(val));
    */
    f.write((char *)&height,sizeof(height));
  }
  for( z=0; z<GetLandRange(); z++ ) for( x=0; x<GetLandRange(); x++ )
  {
    short val=GetTex(x,z);
    f.write((char *)&val,sizeof(val));
  }

  // save all texture names

  int nTextures = _terrain->_texture.Size();
  f.write(&nTextures, sizeof(nTextures));
  for( i=0; i<nTextures; i++ )
  {    
    if (_terrain->_texture[i]._mat)
    {
      const char *name=_terrain->_texture[i]._mat->GetName();
      int major = 0;
      unsigned int lenght = strlen(name);      
      f.write(&lenght, sizeof(lenght));
      f.write(name, lenght);      
      DoAssert(lenght>0);
      if (lenght>0)
        f.write(&major,sizeof(major));
    }
    else 
    {
      unsigned int lenght = 0;      
      f.write(&lenght, sizeof(lenght));
    }     
  }

  // save all objects
  // check for ID duplicate
#define CHECK_ID 1
#if CHECK_ID
  const int maxID=65536;
  bool isID[maxID];
  memset(isID,0,sizeof(isID));
#endif
  for( z=0; z<GetLandRange(); z++ ) for( x=0; x<GetLandRange(); x++ )
  {
    const ObjectListUsed &list=UseObjects(x,z);
    int i,n=list.Size();
    for( i=0; i<n; i++ )
    {
      Object *o=list[i];
      if( !o )
        continue;
//#if !_VBS2
      if( o->GetType()!=Primary && o->GetType()!=Network 
#if _ARMA3_TERRAIN_DECALS
        && o->GetType()!=LandDecal 
#endif         
        ) continue;
//#endif
      SingleObject6 so;

      // matrix can be skewed deskew it...
      float relHeight;
      Matrix4 deSkewed = o->FutureVisualState().Transform();
      o->DeSkew(this,deSkewed, relHeight);

      so.matrix=ConvertToP(deSkewed); 

      so.id=o->ID();
      const char * name = o->GetShape()->GetName();
      so.nameLength = strlen(name);
#if CHECK_ID
      if (so.id<maxID && isID[so.id])
        RptF("Conflict id %d",so.id);
      if (so.id<maxID)
        isID[so.id] = true;
#endif
      if (so.nameLength > 0)
      {
        f.write((char *)&so,sizeof(so));      
        f.write(name,so.nameLength);
      }
    }
  }
  {
    SingleObject6 so;
    so.nameLength=0;
    f.write((char *)&so,sizeof(so));
  }
}

int Landscape::SaveData(const char *name)
{
  QOFStream f(name);
  SaveData(f);
  f.close();
  if (f.fail())
  {
    ErrorMessage("Cannot save world '%s'",name);
    return -1;
  }
  return 0;
}

#define PROGRESS \
  if (f.IsLoading()) \
  { \
    ProgressAdvance(lastRest - f.GetRest()), lastRest = f.GetRest(); \
    ProgressRefresh(); \
    FreeOnDemandFrame(); \
  }

#include "integrity.hpp"

#include <Es/Algorithms/splineEqD.hpp>

//! bilinear interpolation, yxz, xf and zf = <0,1>

static __forceinline float Bilint(float y00, float y01, float y10, float y11, float xf, float zf)
{
  float y0z = y00*(1-zf) + y01*zf;
  float y1z = y10*(1-zf) + y11*zf;
  return y0z*(1-xf) + y1z*xf;
}

DEF_RSB(house);
DEF_RSB(church);
DEF_RSB(tower);
DEF_RSB(vehicle);
DEF_RSB(road);

template <class Type>
class OpTypeAdd
{
  const Type &_val;
public:
  OpTypeAdd(const Type &val) :_val(val) {}
  void operator() (Type &val) const {val += _val;}
};

/// index of streaming data - objects
class ObjectRectIndex: public RefCount
{
  friend class Landscape;

  FindArray<RStringB> _objectNames;
  QuadTreeExRoot<int> _offset;
  int _endOffset;
  /// when true, offsets are ordered based on QuadTreeExRoot<>::Iterator ordering
  //bool _quadTreeOrder;
  bool IsQuadTreeOrdered() const {return true;}

public:
  ObjectRectIndex(bool order);
  ~ObjectRectIndex();

  void Dim(int x, int z);
  void Clear();
  void ClearOffsets();
  void SetOffset(int x, int z, int offset) {_offset.Set(x, z, offset);}
  void SetEndOffset(int offset) {_endOffset = offset;}
  const RStringB &GetObjectName(int i) {return _objectNames[i];}

  //int GetBegOffset(int x, int z) const {return _offset(x,z);}
  /// get beg and end in one operation (fasten than getting separate values)
  void GetBegEndOffsets(int x, int z, int &beg, int &end) const
  {
    if (IsQuadTreeOrdered())
    {
      if (!_offset.Get2(x, z, beg, end))
        end = _endOffset;
    }
    else
    {
      // this part is for non-retail PC build only
      // compiler will not include it when IsQuadTreeOrdered is true
      beg = _offset(x,z);
      z++;
      if (z<_offset.GetYRange())
        end = _offset(x,z);
      else
      {
        z = 0;
        x++;
        if (x<_offset.GetXRange())
          end = _offset(x,z);
        else
          end = _endOffset;
      }
    }
  }

  bool IsEmpty(int x, int z) const
  {
    int beg, end;
    GetBegEndOffsets(x,z,beg,end);
    return end <= beg;
  }
  //void Reset();
  void TransferNames(SerializeBinStream &f);
  //! check if index is loaded and can be used
  bool IsLoaded() const {return _endOffset >= 0;}
  bool CheckIntegrity() const;
};

ObjectRectIndex::ObjectRectIndex(bool order)
:_offset(1,1,-1)
{
  _endOffset = -1;
#if _ENABLE_REPORT
    if (order!=IsQuadTreeOrdered())
      Fail("Quad tree order does not match");
#endif
  //_quadTreeOrder = order;
}

ObjectRectIndex::~ObjectRectIndex()
{}

void ObjectRectIndex::Dim(int x, int z)
{
  _offset.Dim(x,z);
  _offset.Init(-1);
  //Reset();
  _endOffset = -1;
  _objectNames.Clear();
}

/*
void ObjectRectIndex::Reset()
{
  for (int z=0; z<_offset.GetYRange(); z++)
  {
    _offset.SetLine(z,-1);
  }
}
*/

void ObjectRectIndex::ClearOffsets()
{
  _offset.Init(-1);
  _endOffset = -1;
}

void ObjectRectIndex::Clear()
{
  _offset.Init(-1);
  _endOffset = -1;
  _objectNames.Clear();
}

void ObjectRectIndex::TransferNames(SerializeBinStream &f)
{
  f.TransferBasicArray(_objectNames);
  // check christmas
  time_t t;
  time(&t);
  struct tm *lt = localtime(&t);
  if (lt)
  {
    bool christmas = lt->tm_mon == 11 && (lt->tm_mday == 24 || lt->tm_mday == 25);
    if (christmas)
    {
      for (int j=0; j<_objectNames.Size(); j++)
      {
        if (stricmp(_objectNames[j], "data3d\\str_smrcicicek.p3d") == 0)
          _objectNames[j] = "data3d\\pa_sx.p3d";
      }
    }
  }
}

bool ObjectRectIndex::CheckIntegrity() const
{
  bool monotone = true;
  if (IsQuadTreeOrdered())
  {
    QuadTreeExRoot<int>::Iterator it(_offset);
    int lastOffset = 0;
    while (it)
    {
      int x = it.x;
      int z = it.y;
      int off = _offset(x,z);
      if (off<lastOffset)
      {
        LogF("Offsets not monotone %d,%d",x,z);
        monotone = false;
        goto Break;
      }
      if (off>_endOffset)
      {
        LogF("Offsets too large %d,%d",x,z);
        monotone = false;
        goto Break;
      }
      lastOffset = off;
      ++it;
    }
  }
  else
  {
    int lastOffset = 0;
    for (int x=0; x<_offset.GetXRange(); x++)
    for (int z=0; z<_offset.GetXRange(); z++)
    {
      int off = _offset(x,z);
      if (off<lastOffset)
      {
        LogF("Offsets not monotone %d,%d",x,z);
        monotone = false;
        goto Break;
      }
      lastOffset = off;
    }
    if (lastOffset!=_endOffset)
    {
      Fail("Bad end offset");
      return false;
    }
  }
  Break:
  return monotone;
}

class MapObjectRectIndex: public ObjectRectIndex
{
  public:
  MapObjectRectIndex(bool order)
  :ObjectRectIndex(order)
  {}
  
};

void Landscape::InitObjectIndex()
{
  _objIndex = new ObjectRectIndex(true);
  // make sure object index is valid (empty)
  _objIndex->Dim(GetLandRange(),GetLandRange());
  //_objIndex->Clear();
}

OLink(Object) Landscape::GetObject(VisitorObjId id, int x, int z) const
{
  // first check the given coordinates
  {
    const ObjectListUsed &list = UseObjects(x,z);
    int n = list.Size();
    for (int i=0; i<n; i++)
    {
      Object *obj = list[i];
      if (obj->ID()==id)
        return obj;
    }
  }
  // when not found, try enlarging the search area
  int maxRange = GetLandRange();
  // if zero-limit search failed, try larger area
  int range;
  const int maxGoodRange = 10;
  for (range=1; range<=maxRange; range++)
  {
    if (range==maxGoodRange)
    {
      RptF("Performance warning: Very large search for %d (>%.0f m)", id.Encode(), range*GetLandGrid());
    }
    for (int zz=z-range; zz<=z+range; zz++)
    {
      int xStep = ( zz==z-range || zz==z+range ) ? 1 : range*2;
      for (int xx=x-range; xx<=x+range; xx+=xStep)
      {
        if (!land_InRange(zz,xx)) continue;
        const ObjectListUsed &list = UseObjects(xx,zz);
        int n = list.Size();
        for (int i=0; i<n; i++)
        {
          Object *obj = list[i];
          if (obj->ID()==id)
          {
            if (range>=maxGoodRange)
              RptF("Performance warning: Search for %s was very large (%.0f m)",(const char *)obj->GetDebugName(),range*GetLandGrid());
            Log("Landscape::GetObject(VisId) found %s in %d area",(const char *)obj->GetDebugName(),range);
            return obj;
          }
        }
      }
    }
  }

  if (range>=10)
  {
#if _HELISIM_LARGE_OBJECTID
    RptF
      (
      "Performance warning: Unsucessfull search for %I64d was very large (%.0f m)",
      id.Encode(), range*GetLandGrid()
      );
#else
    RptF("Performance warning: Unsucessfull search for %d was very large (%.0f m)",id.Encode(), range*GetLandGrid());
#endif
  }
  return NULL;
}

OLink(Object) Landscape::GetObject(VisitorObjId id, Vector3Par pos) const
{
  int x,z;
  _terrain->SelectObjectList(x,z,pos.X(),pos.Z());
  return GetObject(id,x,z);
}

ObjectId Landscape::GetObjectId(VisitorObjId id, int x, int z) const
{
  ObjectListUsed used = UseObjects(x,z);
  OLink(Object) obj = GetObject(id,x,z);
  if (!obj)
    return ObjectId();
  return obj->GetObjectId();
}

ObjectId Landscape::GetObjectId(VisitorObjId id, Vector3Par pos) const
{
  int x,z;
  _terrain->SelectObjectList(x,z,pos.X(),pos.Z());
  return GetObjectId(id,x,z);
}

template <class Type>
struct CacheTraits: public FramedMemoryControlledListTraitsRef<Type>
{
  //static void AddRef(Type *item) {item->AddRef();}
  //static void Release(Type *item) {item->Release();}
  //static double GetMemoryUsed(Type *item) {return item->GetMemoryUsed();}
  static float GetPriority() {return 0.1f;} // default priority
  static const char *GetDebugName() {return typeid(Type).name();}
};

template <class Type, class Traits=CacheTraits<Type> >
class CacheContainer: public MemoryFreeOnDemandHelper
{
protected:
  FramedMemoryControlledList<Type,Traits> _items;

public:
  CacheContainer() {RegisterFreeOnDemandMemory(this);}
  ~CacheContainer() {Clear();}
  
  void Add(Type *item) {_items.Add(item);}
  size_t Remove(Type *item) {return _items.Delete(item);}
  void Refresh(Type *item) {_items.Refresh(item);}
  void Clear()
  {
    while (_items.First())
      Remove(_items.First());
  }
  int GetItemCount() const {return _items.ItemCount();}

  //void LimitCount(int maxItems)
  //{
  //  if (maxItems<0) maxItems = 0;
  //  while (_itemCount>maxItems)
  //  {
  //    Assert(_items.Last());
  //    FreeOneItem();
  //  }
  //}

  // { memory free on demand implementation
  size_t FreeOneItem() {return _items.First() ? Remove(_items.First()) : 0;}
  void MemoryControlledFrame() {_items.Frame();}
  size_t MemoryControlledRecent() const {return _items.MemoryControlledRecent();}
  size_t MemoryControlled() const {return _items.MemoryControlled();}
  float Priority() const {return Traits::GetPriority();}
  RString GetDebugName() const {return Traits::GetDebugName();}
    virtual bool CheckIntegrity() const {return true;}
  // }
private:
  //! no copy
  CacheContainer(const CacheContainer &src);
  void operator=(const CacheContainer &src);
};

template <>
struct CacheTraits<ObjectListFull>: public FramedMemoryControlledListTraitsRef<ObjectListFull>
{
  typedef ObjectListFull Type;

  static float GetPriority() {return 0.25f;}
  static const char *GetDebugName() {return "Object lists";}
  static void Release(Type *item)
  {
    if (item->GetUsedCount()==1)
    {
      int x=-1,z=-1;
      int refCount = item->Release(x,z);
      if (refCount<=0)
      {
        DoAssert(x>=0 && z>=0);
#if _ENABLE_REPORT
          if (refCount!=0)
          {
            RptF("RefCount %d for %d,%d",refCount,x,z);
            Fail("refCount expected to be zero");
          }
#endif
        if (item->Size()==0 && x>=0 && z>=0)
        { // if it is cached, it is because there are some static objects in it
          GLandscape->DeleteObjectList(x,z);
        }
      }
      else
      {
        RptF("RefCount %d for %d, %d",refCount,x,z);
        Fail("Release expected to be done");
      }
    }
    else
      Verify(item->Release()>0);
  }
};


//! cache currently unused lists that have a chance to be used soon again
//! actually they are released from the landscape only after released from the cache
class ObjectListCache: public CacheContainer<ObjectListFull>, public RefCount
{
  typedef CacheContainer<ObjectListFull> base;
public:
  ObjectListCache() {}
  ~ObjectListCache() {Clear();}
  bool CheckIntegrity() const;
  void MemoryControlledFrame() {base::MemoryControlledFrame();}
/*	size_t Remove(ObjectListFull *item)
	{
#if _ENABLE_PHYSICS1
		if(item->Size())
		{
			enf::PhysicsWorld* pw = item->Get(0)->GetWorldPhysics();
			if(pw)
			{
				for(int i = 0; i < item->Size(); ++i)
				{
					if(item->Get(i)->GetPhysics())
						pw->AddStaticBody(item->Get(i));
				}
			}
		}
#endif
		Assert(item->CountPrimary()>0);
		return base::Remove(item);
	}
*/
  void Add(ObjectListFull *item)
  {
#if _ENABLE_PHYSICS1
		if(item->Size())
		{
			enf::PhysicsWorld* pw = item->Get(0)->GetWorldPhysics();
			if(pw)
			{
				for(int i = 0; i < item->Size(); ++i)
				{
					if(item->Get(i)->GetPhysics())
						pw->RemoveBody(item->Get(i));
				}
			}
		}
#endif

    Assert(item->CountPrimary()>0);
    base::Add(item);
  }
};


//! find object based on object id, and lock it
/*!
Note: returned object is locked. Normally this function is used only in SoftLink<Object> auto-restore.
It is usually best to use SoftLink<> functionality. If using this function is really necessary,
make sure object is properly unlocked.
*/
Object *Landscape::GetObjectLocked( const ObjectId &id ) const
{
  if (!id.IsObject())
  {
    ErrF("Object id %s is not static",(const char *)id.GetDebugName());
    return NULL;
  }

  int x = id.GetObjX();
  int z = id.GetObjZ();
  if(!land_InRange(x,z))
  {
    RptF("Accessing static object outside landscape (%s in %d,%d)",cc_cast(id.GetDebugName()),GetLandRange(),GetLandRange());
    return NULL;
  }
  const ObjectListUsed &list = UseObjects(x,z);
  int n = list.Size();
  for (int i=0; i<n; i++)
  {
    Object *obj = list[i];
    if (obj->GetObjectId()==id)
    {
      const_cast<Landscape *>(this)->AddRefObjectList(list.GetList());
      return obj;
    }
  }
  // if zero-limit search failed, try larger area
#if _HELISIM_LARGE_OBJECTID
  ErrF("Object id %I64x (%d) not found in slot %d,%d",id.Encode(),id.GetObjId(),x,z);
#else
  ErrF("Object id %x (%d) not found in slot %d,%d",id.Encode(),id.GetObjId(),x,z);
#endif
  Fail("Link cannot be resolved");

  int maxRange = 4;
  saturateMin(maxRange,GetLandRange());

  // if zero-limit search failed, try larger area
  for (int range=1; range<=maxRange; range++)
  for (int zz=z-range; zz<=z+range; zz++)
  {
    int xStep = ( zz==z-range || zz==z+range ) ? 1 : range*2;
    for (int xx=x-range; xx<=x+range; xx+=xStep)
    {
      if (!land_InRange(zz,xx)) continue;
      const ObjectListUsed &list = UseObjects(xx,zz);
      int n = list.Size();
      for (int i=0; i<n; i++)
      {
        Object *obj = list[i];
        if (obj->GetObjectId()==id)
        {
          ErrF("Landscape::GetObject(ObjId) found %s in %d area", (const char *)obj->GetDebugName(),range);
          const_cast<Landscape *>(this)->AddRefObjectList(list.GetList());
          return obj;
        }
      }
    }
  }
  return NULL;
}

OLink(Object) Landscape::GetObject(const ObjectId &id) const
{
  return OLink(Object)(SoftLinkById,id);
}

OLinkL(Object) Landscape::GetObjectNoLock(const ObjectId &id) const
{
  return OLinkL(Object)(SoftLinkById,id);
}

struct ShapeBankName
{
  RStringB _name;
  ShapeParameters _pars;
  
  ShapeBankName(const RStringB &name, ShapeParameters pars)
  :_name(name),_pars(pars)
  {}
  
  static int Compare(const ShapeBankName *n1, const ShapeBankName *n2)
  {
    int diff = strcmp(n1->_name,n2->_name);
    if (diff) return diff;
    return n1->_pars-n2->_pars;
  }
};

TypeIsMovableZeroed(ShapeBankName)

bool Landscape::DoPreloadObjects(FileRequestPriority prior, int x, int z, int beg, int end) const
{
  DoAssert(land_InRange(x,z));
  TimeManagerScope time(TCLoadObjects);
  
  PROFILE_SCOPE(lndPO);

  // open the stream
  QIStream &in = _stream;
  in.seekg(beg,QIOS::beg);
  if (!in.PreRead(prior,beg,end-beg))
    return false;
  // the wrp data are ready - preload necessary shapes
  bool allShapesReady = true;
  SerializeBinStream f(&in);
#ifdef _M_PPC
  // TODOX360: support for big-endian binarized landscape
  f.SetEndianState(EndianLittle);
#endif
  AutoArray<ShapeBankName, MemAllocLocal<ShapeBankName,32> > names;
  while (f.TellG()<end && !f.GetError())
  {
    // load object info
    OIDInt id = f.LoadInt();
    if (id<0)
      break;
    int nameIndex = f.LoadInt();
    const RStringB &name = _objIndex->GetObjectName(nameIndex);
    Matrix4 trans;
    f.Transfer(trans);
    ShapeParameters pars = ShapeShadow;
    if (_fileVersion>=14)
      pars = (ShapeParameters)f.LoadInt();
    if (name.GetLength()==0) // empty object - do not preload, continue
      continue;
    names.Add(ShapeBankName(name,pars));
  }
  RStringB lastName;
  QSort(names,ShapeBankName::Compare);
  for (int i=0; i<names.Size(); i++)
  {
    const RStringB &name = names[i]._name;
    ShapeParameters pars = names[i]._pars;
    if (name==lastName) // happens frequently: objects ordered by name
      continue;
    lastName = name;
    if (VehicleTypes.CheckPlainShapeLoaded(name,pars))
      continue;
    // until the class is ready, we cannot proceed
    // if it does not exist, it is ready
    if (!VehicleTypes.RequestShapeDirect(name))
    {
      allShapesReady = false;
      continue;
    }
    // check object type based on the shape name
    Ref<EntityType> type = VehicleTypes.FindShapeDirect(name,pars);
    if (type)
    {
      if (!type->PreloadShape(prior))
        allShapesReady = false;
    }
    else
    {
      // no type - preload the shape directly
      Ref<LODShape> shape;
      if (!Shapes.Preload(prior,name,pars,shape))
      {
        //LogF("  shape %s - %d not ready",(const char *)name,pars);
        allShapesReady = false;
      }
    }
    
  }
  // if all shapes are ready, load them so that they are ready in the landscape
  // instead of file cache - this will make presence test much faster
  return allShapesReady;
}

Landscape::PreloadResult Landscape::PreloadObjects(FileRequestPriority prior, int x, int z) const
{
  if (!IsStreaming()) return PreloadFast;
  int beg,end;
  _objIndex->GetBegEndOffsets(x,z,beg,end);
  if (beg>=end)
  {
    Assert(beg==end);
    // list is empty - no change required
    return PreloadFast;
  }

  // check if the object list is already loaded
  const ObjectList &list = _objects(x,z);
  if (list.GetList() && list.GetList()->IsUsed())
    return PreloadFast;
  if (GTimeManager.TimeLeft(TCLoadObjects)<=0)
  {
    // if we already spent too much time, do not make any more requests
    //LogF("  no time left - %d,%d",x,z);
    return PreloadRequested;
  }
  bool preloaded = DoPreloadObjects(prior,x,z,beg,end);
  if (preloaded)
  {
    // use objects temporarily to get them into a cache
    ObjectListUsed used = UseObjects(x,z);
    return PreloadSlow;
  }
  //LogF("  preload requested - %d,%d",x,z);
  return PreloadRequested;
}

bool Landscape::ObjectsReady(int x, int z) const
{
  if (!IsStreaming())
    return true;
  const ObjectList &list = _objects(x,z);
  return list.GetList() && list.GetList()->IsUsed();
}

/// index array, clamp out of bounds indices
template <class Type,int maxIndex>
class ArraySaturateIndex
{
  Type _array[maxIndex];
public:
  Type &operator[] (int index)
  {
    saturate(index,0,maxIndex-1);
    return _array[index];
  }
  const Type &operator[] (int index) const
  {
    saturate(index,0,maxIndex-1);
    return _array[index];
  }
};

const int NDistSlots = 20;

/// distance discretization for preloading
static DiscreteSlots<NDistSlots> DistSlots(2,10,NDistSlots);

/// information about a preloaded entity type
struct PreloadTypeInfo
{
  LLink<LODShape> _shape;
  LLink<EntityType> _type;

  PreloadTypeInfo(){}
  PreloadTypeInfo(LODShape *shape, const EntityType *type) :_shape(shape), _type(unconst_cast(type)) {}
  bool operator == (const PreloadTypeInfo &info) const {return _shape == info._shape && _type == info._type;}
};

TypeIsMovableZeroed(PreloadTypeInfo)

/// keep list of objects for preloading ready, so that they do not need to be build in each frame
class PreloadDataStateItem: public RefCount
{
  Vector3 _camPos;
  float _maxDistance;
  /// even when not moving, refresh time from time, this is esp. important when many fields are not loaded yet
  Time _nextUpdate;
  
  void ClearSlots()
  {
    for (int i=0; i<NDistSlots; i++)
      _distSlots[i].Resize(0);
  }

  
public:
  /// separate objects based on a distance from a camera
  ArraySaturateIndex< FindArray<PreloadTypeInfo>,NDistSlots > _distSlots;
  
  PreloadDataStateItem() {Reset();}
  /// make sure all slots are zeroed
  void Reset()
  {
    ClearSlots();
    _camPos = VZero;
    _maxDistance = 0;
    _nextUpdate = Time(0);
  }
  /// perform maintenance after slots are done
  void Close()
  {
    for (int i=0; i<NDistSlots; i++)
      // if the list is reasonable small or it is used, leave it
      _distSlots[i].CompactIfNeeded(2,32);
  }
  /// recreate - authoritative
  void Recreate(Vector3Par camPos, float maxDistance, SectionTimeHandle section, float maxTime, bool alwaysRecreate);
  /// recreate or reuse, as possible
  void RecreateIfNeeded(Vector3Par camPos, float maxDistance, SectionTimeHandle section, float maxTime, bool alwaysRecreate);
};


void PreloadDataStateItem::Recreate(Vector3Par camPos, float maxDistance, SectionTimeHandle section, float maxTime, bool alwaysRecreate)
{
  PROFILE_SCOPE_GRF(preRc,0);
  ClearSlots();
  _camPos = camPos;
  _maxDistance = maxDistance;

  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,_camPos,_camPos,_maxDistance);
  int fieldsToLoad = (xMax-xMin+1)*(zMax-zMin+1);
  for( int z=zMin; z<=zMax; z++ ) for( int x=xMin; x<=xMax; x++ )
  {
    //LogF("  Preload %d,%d",x,z);
    Landscape::PreloadResult result = GLandscape->PreloadObjects(FileRequestPriority(5000),x,z);
    if (result != Landscape::PreloadRequested)
      fieldsToLoad--;
    else
    {
      if (CompareSectionTimeGE(section,maxTime))
        break;
      // preload not ready yet - we cannot use the objects yet
      continue;
    }

    const ObjectListUsed &listUsed=GLandscape->UseObjects(x,z);
    
    // approximate distance only
    // we are preloading back-side objects -
    // TODO: for far grids SlotFromDistance2 is almost constant
    const ObjectListFull *list = listUsed.GetList();
    if (list)
    {
      int index = 0;
      /*
      // TODO: when constant distance, use grouped information, assume constant distance
      if (dist2>Square(_maxDistance)) continue;
      
      for (int g=0; g<list->NGroups(); g++)
      {
        const ObjectListGroup &grp = list->Group(g);
        Object *obj=list->Get(index);
        LODShape *lShape = obj->GetShape();
        // grouped objects are always static and always have shape

        if (!lShape) continue;
        // do not preload ... only add to the corresponding slot
        int slot = DistSlots.SlotFromValue2(dist2);
        _distSlots[slot].AddUnique(PreloadTypeInfo(lShape,obj->GetEntityType()));
        
        index = grp._end;
      }
      */
      
      for (; index<list->Size(); index++)
      {
        Object *obj = list->Get(index);
        LODShape *lShape = obj->GetShape();
        if (!lShape) continue;
        float dist2 = _camPos.Distance2(obj->FutureVisualState().Position());
        if (dist2>Square(_maxDistance)) continue;
        // do not preload ... only add to the corresponding slot
        if (obj->Static())
        {
          int slot = DistSlots.SlotFromValue2(dist2);
          _distSlots[slot].AddUnique(PreloadTypeInfo(lShape,obj->GetEntityType()));
        }
      }
      if (CompareSectionTimeGE(section,maxTime))
        break;
    }
  }
  if (fieldsToLoad<=0)
    _nextUpdate = Glob.time+5;
  else if (alwaysRecreate)
    // make sure we continue building until data are ready, this is esp. important when time is not running
    _nextUpdate = Glob.time;
  else if (fieldsToLoad<10)
    _nextUpdate = Glob.time+0.5f;
  else
    _nextUpdate = Glob.time+0.2f;
  Close();
}

/**
@param alwaysRecreate when true, next time is set so that next iter. build again unless
  the list is built completely.
*/
void PreloadDataStateItem::RecreateIfNeeded(Vector3Par camPos, float maxDistance, SectionTimeHandle section, float maxTime, bool alwaysRecreate)
{
  const float minCamChange = 5;
  const float minDistChange = 5;
  // if distance is getting shorter, do not recreate
  if (camPos.Distance2(_camPos)>Square(minCamChange) || maxDistance>_maxDistance+minDistChange || Glob.time>=_nextUpdate)
    Recreate(camPos,maxDistance,section,maxTime,alwaysRecreate);
}


/// information about a single preload timing out
struct PreloadTimeOutState
{
  /// preload position
  Vector3 pos;
  /// time when it will auto-fail
  Time timeToFail;
  /// one preload is failed, do not continue performing it
  bool failed;
  /// data state
  Ref<PreloadDataStateItem> state;
};

TypeIsMovableZeroed(PreloadTimeOutState)

/// all information persistent for reloading
/** encapsulated to avoid header clutter

There may be several pre-loads being pumped at the same time.
We want only one of them to be the active one, the other should be reported as timed out.
*/

class PreloadDataState: public RefCount
{
  /// default state - used for landscape preloading
  PreloadDataStateItem _defState;
  
  /// list of all preload running
  AutoArray<PreloadTimeOutState> _running;
  
  int FindPreload(Vector3Par vector) const;
  
public:
  void Reset();
  PreloadDataStateItem *GetPreload(Vector3Par pos, float autoTimeOut);
  void PreloadDone(PreloadDataStateItem *state);
};

/** find best and close enough match */
int PreloadDataState::FindPreload(Vector3Par vector) const
{
  // preloads in 2 meters range are still considered to be the same one
  float minDist2 = Square(2);
  int bestI = -1;
  for (int i=0; i<_running.Size(); i++)
  {
    float dist2 = _running[i].pos.Distance2(vector);
    if (minDist2>dist2)
    {
      minDist2 = dist2;
      bestI = i;
    }
  }
  return bestI;
}

void PreloadDataState::PreloadDone(PreloadDataStateItem *state)
{
  if (state==&_defState) return;
  for (int i=0; i<_running.Size(); i++)
  {
    if (_running[i].state==state)
    {
      _running.Delete(i);
      return;
    }
  }
}

void PreloadDataState::Reset()
{
  // forget any preloads - they are no longer valid
  _running.Clear();
  _defState.Reset();
}


PreloadDataStateItem *PreloadDataState::GetPreload(Vector3Par pos, float autoTimeOut)
{
  if (autoTimeOut<FLT_MAX)
  {
    // check if there are any other updates running
    // if they are, timeout them
    int running = FindPreload(pos);
    if (running<0)
    {
      running = _running.Add();
      PreloadTimeOutState &state = _running[running];
      state.pos = pos;
      state.timeToFail = Glob.time + autoTimeOut;
      state.failed = false;
      state.state = new PreloadDataStateItem;
    }
    const PreloadTimeOutState &runningState = _running[running];
    if (runningState.failed || Glob.time>runningState.timeToFail)
    {
      // preload already failed - report it
      // when we reported failure, we can destroy it
      _running.Delete(running);
      return NULL;
    }
    /*
    // all other preloads could fail
    for (int i=0; i<_running.Size(); i++)
    {
      if (i==running) continue;
      PreloadTimeOutState &state = _running[i];
      state.failed = true;
      /// state no longer needed - the preload will never continue
      state.state = NULL;
    }
    */
    return runningState.state;
  }
  else
    return &_defState;
}


/// context passed from preloading the scene to preloading a single grid
struct Landscape::PreloadDataContext: private NoCopy
{
  SectionTimeHandle section;
  float maxTime;
  int countPreload;
  bool done;
  Vector3 camPos;
  /// explicit view frustum direction (VZero if none)
  Vector3 camDir; 
  /// explicit view frustum fov (negative if none)
  float camFov;
  // what to include and what not
  float camFovMaxCos;
  // how much are we scaling distance - used for loading behind the camera
  float distScale,distAdd;
  /// limit preloading distance
  float maxPreloadDist;
  /// camera object is excluded from preloading
  Object *doNotPreload;
  
  bool logging;
  /// how many "not done" requests can be submitted
  int notDoneLeft;
  
  PreloadDataStateItem &_state;
  
  /// distance scale based on object position
  float DistScale2(Vector3Par pos, float dist) const
  {
    if (camFov<0)
      return Square(dist*distScale+distAdd);
    // check if within fov
    // if not, return degraded
    // compensate for fov compared to actual camera fov
    const Camera &cam = *GScene->GetCamera();
    
    // during lod selection camera is used: as cam.InvLeft()*cam.InvTop()/dist2,
    // i.e areaUsed = 1/(cam.Left()*cam.Top()*K*dist2);
    // we want this: areaWanted = 1/(fov*fov*cam.Left()/cam.Top()*dist2);

    // 1/(cam.Left()*cam.Top()*K*dist2) = 1/(fov*fov*cam.Left()/cam.Top()*dist2);
    float k = camFov*camFov*cam.InvTop()*cam.InvTop();
    // check if near (or within fov
    
    float cosAngle = (pos-camPos).CosAngle(camDir);
    if (cosAngle>=camFovMaxCos)
      return Square(dist)*k;
    // outside of the fov - degrade
    else return Square(dist*distScale+distAdd)*k;
  }
  explicit PreloadDataContext(PreloadDataStateItem &state) :_state(state) {}
  ~PreloadDataContext() {}
  
  // helper functor
  class PreloadVehicleData
  {
    PreloadDataContext &_ctx;
    
    public:
    explicit PreloadVehicleData(PreloadDataContext &ctx):_ctx(ctx){}
    
    /// helper for operator ()
    void Preload(Entity *obj) const;
    
    __forceinline bool operator () (Entity *obj) const
    {
      Preload(obj);
      return false;
    }
  };
};

void Landscape::PreloadDataContext::PreloadVehicleData::Preload(Entity *obj) const
{
  if (!obj || obj==_ctx.doNotPreload) return;
  LODShape *lShape = obj->GetShape();
  if (!lShape) return;
  // preload appropriate LOD level
  float dist = _ctx.camPos.Distance(obj->FutureVisualState().Position());
  if (dist>_ctx.maxPreloadDist) return;
  if (_ctx.notDoneLeft<=0) return;
  // we may pretend we are a little bit farther from the camera
  // do not preload ... only add to the corresponding slot
  if (!obj->Static())
  {
    float dist2 = _ctx.DistScale2(obj->FutureVisualState().Position(),dist);
    // object data may be requested during LevelFromDistance2
    int level = GScene->PreloadLevelFromDistance2(lShape,dist2,obj->FutureVisualState().Scale());
    if (level<0)
    {
      if (_ctx.logging)
        LogF("  not done, lod level of %s",cc_cast(lShape->GetName()));
      _ctx.done = false;
      _ctx.notDoneLeft--;
      return;
    }
    if (level==LOD_INVISIBLE)
      return;
    
    ShapeUsed shape=lShape->Level(level);

    AnimationContextStorageRender storage;
    AnimationContext animContext(obj->FutureVisualState(),storage);
    shape->InitAnimationContext(animContext, lShape->GetConvexComponents(level), false);

    // we should make sure correct textures are setup on the object
    obj->Animate(animContext, level, true, obj, dist2);
    // if we requested the object data, we should request textures for it as well
    if (!lShape->PreloadVBuffer(level,dist2))
    {
      _ctx.done = false;
      _ctx.notDoneLeft--;
      if (_ctx.logging)
        LogF("  not done, vbuffer on %s",cc_cast(lShape->GetName()));
    }
    if (!shape->PreloadTextures(dist2, &animContext))
    {
      if (_ctx.logging)
      {
        LogF("  not done, texture on %s",cc_cast(lShape->GetName()));
        // debugging opportunity
        //shape->PreloadTextures(dist2);
      }
      _ctx.done = false;
      _ctx.notDoneLeft--;
    }
    if (!obj->PreloadProxies(level,dist2))
    {
      if (_ctx.logging)
        LogF("  not done, proxy on %s",cc_cast(lShape->GetName()));
      _ctx.done = false;
      _ctx.notDoneLeft--;
    }
    obj->Deanimate(level,true);
    
    if (dist2<Square(Glob.config.GetShadowZ()))
    {
      // if near enough, we may want to preload shadow volume as well
      if (!obj->GetShape()->PreloadShadow(dist2))
      {
        _ctx.done = false;
        _ctx.notDoneLeft--;
      }
    }
  }
}

/**
@param x LandGrid coordinates of given rectangle
@param z LandGrid coordinates of given rectangle
*/
bool Landscape::PreloadData(int x, int z, PreloadDataContext &ctx, bool grassLayer)
{
  Vector3 center = PointOnSurface((x+0.5)*GetLandGrid(),0,(z+0.5)*GetLandGrid());
  float dist = floatMax(ctx.camPos.Distance(center)-GetLandGrid()*0.75f,0);
  if (dist<ctx.maxPreloadDist)
  {
    const float areaOverTex = GetLandGrid()*GetLandGrid();
    AreaOverTex aot;
    aot.Set(0, areaOverTex);
    float dist2 = Square(dist*ctx.distScale+ctx.distAdd);
    int subdivLog = GetSubdivLog();
    int xt = x<<subdivLog, zt = z<<subdivLog;
    int st = 1<<subdivLog;
    float waterMin = floatMin(ClippedDataXZ(xt,zt), ClippedDataXZ(xt+st,zt), ClippedDataXZ(xt,zt+st), ClippedDataXZ(xt+st,zt+st));
    if (waterMin<0)
    {
      if (!PreloadWaterTextures(dist2,areaOverTex))
        ctx.done = false;
    }
    if (!land_InRange(z,x)) return true;
    // water texture may be needed as well
    // preload ground textures very near to the player
    const TexMaterial *mat = ClippedTextureInfo(z,x)._mat;
    if (mat)
    {
      if (!mat->PreloadData(aot, dist2))
      {
        ctx.done = false;
        if (--ctx.notDoneLeft<=0)
          return false;
      }
      if (grassLayer && dist2<Square(Glob.config.roadsZ))
      {
        // we to preload grass layer textures as well for grids where we are rendering it
        // we need a shared noise mask as well, this is loaded in PreloadGroundClutter
        PixelShaderID ps = mat->GetPixelShaderID();
        if (ps>=PSTerrain1 && ps<=PSTerrain15 || ps==PSTerrainX || ps==PSTerrainSNX)
        {
          int nPasses = (mat->_stageCount+1-LandMatStageLayers)/LandMatStagePerLayer;
          if (ps == PSTerrainSNX)
          {
            if (nPasses == LandMatLayerCount)
              nPasses--; //last layer is used for satellite texture
          }
          for (int p=0; p<nPasses; p++)
          {
            // for each primary texture we need to preload its mask
            Texture *tex = GetGrassMask(mat,p);
            if (tex && !tex->PreloadTexture(dist2,areaOverTex,false))
            {
              ctx.done = false;
              if (--ctx.notDoneLeft<=0)
                return false;

            }
          }
        }
      }
    }
  }
  if (!land_InRange(z,x))
    return true;
  //LogF("  Preload %d,%d",x,z);
  PreloadResult result = PreloadObjects(FileRequestPriority(5000),x,z);
  if (result!=PreloadFast)
  {
    ctx.countPreload++;
    if (CompareSectionTimeGE(ctx.section,ctx.maxTime))
    {
      ctx.done = false;
      return false;
    }
  }
  if (result==PreloadRequested)
  {
    ctx.done = false;
    if (--ctx.notDoneLeft<=0)
      return false;
    return true;
  }
  return true;
}

#if _ENABLE_REPORT
static bool EnablePreload = true;
#endif


/**
@param degrade add and scale for distance are derived for this. Typical values:
  0 - no degradation
  0.5 - degradation for behind the camera
  degrade is valid for everything outside of the dir/fov frustum
@param alwaysRecreate force static objects list building if not fully built
@param autoTimeOut if preload it requested on the same location, it is assumed to be one preload
  and it times out after given time. There may be only one autoTimeOut preload active at any time,
  all other are automatically timed out.
@param turn on logging of data which are not loaded yet
@param dir (optional) direction of the camera we are preloading for
@param fov (optional) field of view of the camere we are preloading for

When this is used in conjunction with rendering, dir and fov are not used.
They are used for cutscene camera preloading.
*/
bool Landscape::PreloadData(bool grassLayer, Vector3Par pos, float maxPreloadDist, float maxTime, float degrade,
  Object *doNotPreload, bool alwaysRecreate, float autoTimeOut, bool logging, const Vector3 *dir, float fov)
{
  if (!_listCache)
    return true;
#if _ENABLE_REPORT
  if (!EnablePreload)
    return true;
#endif
  
  PreloadDataStateItem *state = _preloadState->GetPreload(pos,autoTimeOut);
  
  if (!state)
    // timeout - report success
    return true;
  
  PROFILE_SCOPE_GRF_EX(preLd,*,SCOPE_COLOR_YELLOW);
  if (PROFILE_SCOPE_NAME(preLd).IsActive())
  {
    PROFILE_SCOPE_NAME(preLd).AddMoreInfo(Format("~ %.1f,%.1f ms",maxTime*1000,GTimeManager.TimeLeft(TCLoadObjects)*1000));
  }
  
  PreloadDataContext ctx(*state);
  ctx.section = StartSectionTime();
  ctx.countPreload = 0;
  ctx.maxTime = floatMin(GTimeManager.TimeLeft(TCLoadObjects),maxTime);
  ctx.camPos = pos;
  ctx.camDir = dir ? *dir : VZero;
  ctx.camFov = fov;
  if(fov>0)
  {
    const Camera &cam = *GScene->GetCamera();
    //fov*size(Left(),Top())/Left() is tan(alpha) for the corner
    float tanAlpha = fov*sqrt(Square(cam.Left())+Square(cam.Top()))*cam.InvLeft();
    float alpha = atan(tanAlpha);
    ctx.camFovMaxCos = cos(alpha);
  }
  else
    ctx.camFovMaxCos = 2;
  ctx.distScale = 1+degrade*2;
  ctx.distAdd = 20*degrade;
  ctx.doNotPreload = doNotPreload;
  ctx.logging = logging;
  ctx.done = true; // unless something prove us wrong, assume we are done
  // TODO: tune based on empirical values
  ctx.notDoneLeft = 10;
  ctx.maxPreloadDist = maxPreloadDist;

  
  state->RecreateIfNeeded(pos,maxPreloadDist,ctx.section,ctx.maxTime,alwaysRecreate);
  
  //LogF(
  //  "Preloading (%.1f,%.1f,%.1f), time spent %g",
  //  pos.X(),pos.Y(),pos.Z(),_listCache->GetTimeSpent()
  //);
  // if almost no time is left, return immediately
  if (ctx.maxTime<=0.001)
  {
    if (logging)
    {
     LogF(
       "No time left for preloading (%.1f,%.1f,%.1f), max time %g, dist %g",
       pos.X(),pos.Y(),pos.Z(),
       maxTime,maxPreloadDist
     );
    }
    return false;
  }

  // preload layer mask and ground clutter 
  
  if (!PreloadGroundClutter(pos,floatMin(maxPreloadDist,_clutterDist*1.2),logging))
  {
    if (logging)
      LogF("  not done, ground clutter");
    ctx.done = false;
    return false;
  }
  
  // traverse all vehicles and preload them
  {
    PROFILE_SCOPE_GRF(preLV,0);
    PreloadDataContext::PreloadVehicleData preload(ctx);
    GWorld->ForEachVehicle(preload);
    GWorld->ForEachAnimal(preload);
    if (CompareSectionTimeGE(ctx.section,ctx.maxTime))
    {
      if (logging)
        LogF("  not done, time");
      ctx.done = false;
      return false;
    }
  }
  
  // make sure terrain mesh is ready in case the user will start turning around
  {
    PROFILE_SCOPE_EX(preLT,pre);
    // view frustum may contain something outside of the circle
    // but we never need to display more than a sphere describes
    // what is beyond the range is never visible anyway
    GridRectangle aroundRect;
    // for terrain we always need the full visible range
    float maxTerrainDist = Glob.config.horizontZ;
    GetTerrainPreloadGrid(pos,maxTerrainDist,aroundRect);

    int landSegmentSize = GetLandSegmentSize();
    // TODO: consider traversing from the user position
    // sliding offset: helps when overhead dominates
    // TODO: implement some proper incremental solution
    int zRange = aroundRect.Height();
    int zItems = zRange/landSegmentSize;
    if ( zItems!=0 )
    {
      Assert(zItems*landSegmentSize==zRange);
      int zOffset = (FreeOnDemandFrameID()%zItems)*landSegmentSize;
      int traversed = 0;
      for (int zo=aroundRect.zBeg; zo<aroundRect.zEnd; zo+=landSegmentSize)
      {
        int z = (zo-aroundRect.zBeg+zOffset)%zRange+aroundRect.zBeg;
        Assert(z>=aroundRect.zBeg && z<aroundRect.zEnd);
        for (int x=aroundRect.xBeg; x<aroundRect.xEnd; x+=landSegmentSize)
        {
          // outside of the map terrain creation can be slow, but as we are proceeding from camera, we do not care much
          //if (!land_TerrainInRange(x,z)) continue;
          if (!GEngine->IsAbleToDrawCheckOnly()) break;

          GridRectangle sRect(x,z,x+landSegmentSize,z+landSegmentSize);
          Vector3 minMaxEarly[2];
          GetSegmentMinMaxBox(sRect,minMaxEarly);

          // do not preload beyond visible range
          Vector3 bCenter = (minMaxEarly[0]+minMaxEarly[1])*0.5f;
          float bRadius = minMaxEarly[0].Distance(minMaxEarly[1])*0.5f;
          if (pos.Distance2(bCenter)>Square(Glob.config.horizontZ+bRadius))
            continue;

          traversed++;
          PreloadTerrain(x,z,pos);
          if (ProgressIsActive())
          {
            I_AM_ALIVE();
            ProgressRefresh();
          }
          if (CompareSectionTimeGE(ctx.section,ctx.maxTime))
          {
#if 0 // _PROFILE
            DIAG_MESSAGE(
              1000,"Preload terrain %4d,%4d..%4d,%4d (%4d,%4d) %4d",
              aroundRect.xBeg,aroundRect.zBeg,aroundRect.xEnd,aroundRect.zEnd,x,zo,traversed
              );
#endif
            if (logging) LogF("  not done, terrain at %d,%d",x,z);
            ctx.done = false;
            goto Break;
          }
        }
      }

#if 0 // _PROFILE
      DIAG_MESSAGE(
        1000,"Preload terrain %4d,%4d..%4d,%4d %4d",
        aroundRect.xBeg,aroundRect.zBeg,aroundRect.xEnd,aroundRect.zEnd,traversed
        );
#endif
    }
  }
  
  // preload all object grids that may become visible when user is turning around
  // caution: this may be very slow, when many grids are preloaded
  // we should limit this somehow
  // we should also start from the user position
  {
    PROFILE_SCOPE_GRF(preLG,0);
    int xCenter = toIntFloor(pos.X()/GetLandGrid());
    int zCenter = toIntFloor(pos.Z()/GetLandGrid());
    float maxObjDist = floatMin(maxPreloadDist,1200); // above 1200 we spend too much time traversing
    int maxRange = toIntCeil(maxObjDist/GetLandGrid()+1);
    int range = 0;
    if (!PreloadData(xCenter,zCenter,ctx,grassLayer))
    {
      if (logging) LogF("  not done, PreloadData on %d,%d",xCenter,zCenter);
      goto Break;
    }
    DoAssert(maxRange>=0);
    for (range = 1; range<=maxRange; range++)
    {
      if (ProgressIsActive())
      {
        I_AM_ALIVE();
        ProgressRefresh();
      }
      if (CompareSectionTimeGE(ctx.section,ctx.maxTime))
      {
        if (logging) LogF("  no time, range %d of %d",range,maxRange);
        ctx.done = false;
        goto Break;
      }
      for (int xx=-range; xx<range; xx++)
      {
        int x = xCenter+xx, z = zCenter-range;
        if (!PreloadData(x,z,ctx,grassLayer))
        {
          if (logging) LogF("  not done, PreloadData on %d,%d",x,z);
          goto Break;
        }
      }
      for (int xx=range; xx>-range; xx--)
      {
        int x = xCenter+xx, z = zCenter+range;
        if (!PreloadData(x,z,ctx,grassLayer))
        {
          if (logging) LogF("  not done, PreloadData on %d,%d",x,z);
          goto Break;
        }
      }
      for (int zz=-range; zz<range; zz++)
      {
        int z = zCenter+zz, x = xCenter+range;
        if (!PreloadData(x,z,ctx,grassLayer))
        {
          if (logging) LogF("  not done, PreloadData on %d,%d",x,z);
          goto Break;
        }
      }
      for (int zz=range; zz>-range; zz--)
      {
        int z = zCenter+zz, x = xCenter-range;
        if (!PreloadData(x,z,ctx,grassLayer))
        {
          if (logging) LogF("  not done, PreloadData on %d,%d",x,z);
          goto Break;
        }
      }
    }
  }
// ProgressRefresh();

  if (ctx.notDoneLeft>0)
  {
    PROFILE_SCOPE_GRF(preLS,0);
    // traverse through the slots
    for (int s=0; s<NDistSlots; s++)
    {
      float dist2 = DistSlots.MinValue2(s);
      // object data may be requested during LevelFromDistance2
      for (int i=0; i<ctx._state._distSlots[s].Size(); i++)
      {
        if (ProgressIsActive())
        {
          I_AM_ALIVE();
          ProgressRefresh();
        }
        if (CompareSectionTimeGE(ctx.section,ctx.maxTime))
        {
          if (logging)  LogF("  not done, slot %d (dist %.1f)",s,sqrt(DistSlots.MinValue2(s)));
          ctx.done = false;
          goto BreakLSLoop;
        }

        const PreloadTypeInfo &info = ctx._state._distSlots[s][i];
        LODShape *lShape = info._shape;
        if (!lShape)
          continue;
        // during preload assume scale 1.0
        int level = GScene->PreloadLevelFromDistance2(lShape,dist2,1.0f);
        if (level<0)
        {
          if (logging) LogF("  not done, lod level of %s",cc_cast(lShape->GetName()));
          ctx.done = false;
          if (--ctx.notDoneLeft<=0) goto BreakLSLoop;
          continue;
        }
        if (level==LOD_INVISIBLE) continue;
        // if we requested the object data, we should request textures for it as well
        ShapeUsed shape=lShape->Level(level);
        if (!lShape->PreloadVBuffer(level,dist2))
        {
          if (logging) LogF("  not done, vbuffer on %s",cc_cast(lShape->GetName()));
          ctx.done = false;
          if (--ctx.notDoneLeft<=0)
            goto BreakLSLoop;
        }
        if (!shape->PreloadTextures(dist2, NULL))
        {
          if (logging) LogF("  not done, texture on %s",cc_cast(lShape->GetName()));
          // debugging opportunity
          //shape->PreloadTextures(dist2);
          ctx.done = false;
          if (--ctx.notDoneLeft<=0)
            goto BreakLSLoop;
        }
        // note: we assume scale 1
        if (info._type && !info._type->PreloadProxies(level,dist2,1,false))
        {
          if (logging) LogF("  not done, proxy on %s",cc_cast(lShape->GetName()));
          ctx.done = false;
          if (--ctx.notDoneLeft<=0)
            goto BreakLSLoop;
        }
        if (dist2<Square(Glob.config.GetShadowZ()))
        {
          // if near enough, we may want to preload shadow volume as well
          if (!lShape->PreloadShadow(dist2))
          {
            if (logging) LogF("  not done, shadow on %s",cc_cast(lShape->GetName()));
            ctx.done = false;
            if (--ctx.notDoneLeft<=0)
              goto BreakLSLoop;
          }
        }
      }
    }
BreakLSLoop:;
  }
// ProgressRefresh();

  //LogF("Preload %d,%d (%d of %d)",xCenter,zCenter,range-1,maxRange);
  //LogF("  %d,%d..%d,%d",xCenter-(range-1),zCenter-(range-1),xCenter+(range-1),zCenter+(range-1));
  //LogF("  %s",ctx.done ? "Done" : "");
#if  0 //_ENABLE_REPORT
    LogF("Preload full (%.1f,%.1f,%.1f), range %d, max time %g (%g), dist %g, result %d",
      pos.X(),pos.Y(),pos.Z(),maxRange,maxTime,GetSectionTime(ctx.section),maxPreloadDist,ctx.done);
#endif
  if (ctx.done)
    _preloadState->PreloadDone(state);
  return ctx.done;
  Break:
  #if  0 //_ENABLE_REPORT
    if (_listCache->GetTimeSpent()<maxTime)
    {
    }
    LogF("Preload broken (%.1f,%.1f,%.1f), range %d of %d, max time %g (%g), dist %g",
      pos.X(),pos.Y(),pos.Z(),range,maxRange,maxTime,GetSectionTime(ctx.section),maxPreloadDist);
  #endif
  return false;
}

static bool PreloadObjectTextures(Object *obj)
{
  if (obj && obj->GetShape()->NLevels()>0)
  {
    // TODO: make sure sky textures are set before preloading    
    ShapeUsed shape=obj->GetShape()->Level(0);
    return shape->PreloadTextures(0, NULL);
  }
  return true;
}

bool Landscape::PreloadSkyAndClouds() const
{
  bool ret = true;
  for (int i=0; i<NClouds; i++)
  {
    LODShape *shape = _cloudObj[i]->GetShape();
    if (shape && shape->NLevels()>0)
    {
      const ShapeUsed &level0 = shape->Level(0);
      if (!level0->PreloadTextures(Square(2000), NULL))
        ret = false;
    }
  }

  if (!PreloadObjectTextures(_skyObject))
    ret = false;
  if (!PreloadObjectTextures(_horizontObject))
    ret = false;
  if (!PreloadObjectTextures(_sunObject))
    ret = false;
  if (!PreloadObjectTextures(_moonObject))
    ret = false;
  return ret;
}

bool Landscape::CheckPersistence(int x, int z) const
{
  if (!IsStreaming()) return false;
  int beg,end;
  _objIndex->GetBegEndOffsets(x,z,beg,end);
  if (beg>=end)
  {
    Assert(beg==end);
    // list is empty - no change required
    return false;
  }

  // open the stream
  QIStream &in = _stream;
  in.seekg(beg,QIOS::beg);
  // make sure the "header" (information in the WRP file)
  // is loaded before you will try to seek model files etc.
  bool done = false;
  do
  {
    ProgressRefresh();
    FreeOnDemandFrame();
    GFileServerWaitForOneRequestDone(100);
    done = in.PreRead(FileRequestPriority(1),beg,end-beg);
  } while (!done);
  SerializeBinStream f(&in);
#ifdef _M_PPC
  // TODOX360: support for big-endian binarized landscape
  f.SetEndianState(EndianLittle);
#endif
  bool ret = false;
  while (f.TellG()<end && !f.GetError())
  {
    // load object info
    int id = f.LoadInt();
    if (id<0)
      break;
    int nameIndex = f.LoadInt();
    const RStringB &name = _objIndex->GetObjectName(nameIndex);
    Matrix4 trans;
    f.Transfer(trans);
    ShapeParameters pars = ShapeShadow;
    if (_fileVersion>=14)
      pars = f.LoadInt();
    // check object type based on the shape name
    if (VehicleTypes.CheckPlainShapeLoaded(name,pars))
      continue;
    Ref<EntityType> type = VehicleTypes.FindShapeDirect(name,pars);
    if (!type)
      continue;
    if (type->IsPersistenceRequired())
    {
      ret = true;
#if 0 //_ENABLE_REPORT
        LogF("Persistence required by %s (%s)",cc_cast(name),cc_cast(type->GetName()));
#else
      break;
#endif
    }
  }
  return ret;
}

/*!
\return true if object slot useCount was increased
*/
bool Landscape::LoadObjects(int x, int z, bool noWait) const
{
  if (!IsStreaming()) return false;
  int beg,end;
  _objIndex->GetBegEndOffsets(x,z,beg,end);
  if (beg>=end)
  {
    Assert(beg==end);
    // list is empty - no change required
    return false;
  }
  const ObjectList *ol = &_objects(x,z);
  // if list is loaded, it has increased refcount
  if (ol->GetList() && ol->GetList()->IsUsed())
  {
    //Fail("List already loaded");
    Assert(ol->Size()>0);
    // the list should be considered locked
    ol->GetList()->AddRef();
    return true;
  }
  if (noWait && !DoPreloadObjects(FileRequestPriority(1),x,z,beg,end))
    // field is not loaded and we cannot wait - do not lock
    return false;
  PROFILE_SCOPE(lndLO);

  //LogF("  LoadObjects - %d,%d",x,z);
  
  TimeManagerScope time(TCLoadObjects);
  
  // open source stream
  QIStream &in = _stream;
  in.seekg(beg,QIOS::beg);
  SerializeBinStream f(&in);
#ifdef _M_PPC
  // TODOX360: support for big-endian binarized landscape
  f.SetEndianState(EndianLittle);
#endif
  //f.SeekG(beg);
  // make sure list exists and lock it once, to avoid LoadObjects recursion
  // calling function needs to make sure o
  // const_cast<ObjectList &>(ol).CreateList(x,z)->AddRef();
  if (!ol->GetList())
  {
    DoAssert(land_ObjInRange(x,z));
    ObjectList temp(new ObjectListFull(x, z));
    QuadTreeExRoot<ObjectList, QuadTreeExTraitsObjectList> &container =
      const_cast<QuadTreeExRoot<ObjectList, QuadTreeExTraitsObjectList> &>(_objects);
    container.Set(x, z, temp);
    ol = &_objects(x, z);
    if(!ol->GetList())
    {
      RptF("Accessing static object outside landscape (%d,%d out of %d,%d - %d,%d)",x,z,GetLandRange(),GetLandRange(),_objects.GetXRange(),_objects.GetYRange());
      return false;
    }
  }
  ol->GetList()->AddRef();
  bool somethingLoaded = false;
  size_t memoryUsed = 0;  
  while (f.TellG()<end && !f.GetError())
  {
    // load object info
    OIDInt id = f.LoadInt();
    if (id<0)
    {
#if _HELISIM_LARGE_OBJECTID
      ErrF("Terminator (%I64d) reached on (%d,%d)",id,x,z);
#else
      ErrF("Terminator (%d) reached on (%d,%d)",id,x,z);
#endif
      LogF("File pos %d, size %d",f.TellG(),f.GetRest()+f.TellG());
      
      int nameIndex = f.LoadInt();
      (void)nameIndex;
      Matrix4 trans;
      f.Transfer(trans);

#if _ENABLE_REPORT
      const RStringB &name = _objIndex->GetObjectName(nameIndex);
      void PositionToAA11(Vector3Val pos, char *buffer);
      char buf[128];
      PositionToAA11(trans.Position(),buf);
      LogF("Object %s, pos %.2f,%.2f (%s)",(const char *)name,trans.Position().X(),trans.Position().Z(),buf);
#endif
      continue;
      //break;
    }
    int nameIndex = f.LoadInt();
    const RStringB &name = _objIndex->GetObjectName(nameIndex);
    somethingLoaded = true;
    Matrix4 trans;
    f.Transfer(trans);
    ShapeParameters prop = ShapeShadow;
    if (_fileVersion>=14)
      prop = (ShapeParameters)f.LoadInt();
#if _ENABLE_REPORT
      Vector3 pos = trans.Position();
      if (!trans.IsFinite())
      {
#if _HELISIM_LARGE_OBJECTID
        RptF(
          "%I64d:%s - Bad position %g,%g,%g",id,cc_cast(name),
          pos.X(),pos.Y(),pos.Z()
          );
#else
        RptF("%d:%s - Bad position %g,%g,%g",id,cc_cast(name),pos.X(),pos.Y(),pos.Z());
#endif
        if (!_finite(pos.Y()))
        {
          pos[1] = SurfaceY(pos.X(),pos.Z());
          trans.SetPosition(pos);
        }
        if (!trans.IsFinite())
        {
          if (!trans.Direction().IsFinite())
          {
            Vector3Val dir = trans.Direction();
            RptF("   - Bad direction %g,%g,%g",dir.X(),dir.Y(),dir.Z());
          }
          trans.SetOrientation(M3Identity);
        }
        if (!trans.IsFinite())
        {
          RptF("  - cannot recover");
          continue;
        }
      }
      if (pos.Y()>1e4f || pos.Y()<-1e3f)
      {
        float surfY = SurfaceY(pos.X(),pos.Z());
        if (pos.Y()>surfY+1000 || pos.Y()<surfY-1000)
        {
          RptF("%d:%s - Bad position %g,%g,%g",id,cc_cast(name),pos.X(),pos.Y(),pos.Z());
          continue; // ignore the object
        }
      }
#endif
    Object *obj = unconst_cast(this)->ObjectCreate(id,name,prop,trans,x,z,true);

    // we should at the very least take object type into account (EntityAI, Building...)
    // TODO: consider asking object about its controlled memory
    memoryUsed += obj->dyn_sizeof();
    
#if _ENABLE_REPORT
    // check: persistent entity can be loaded only during an initial pass
    if (_persistentLoaded)
    {
      const EntityType *objType = obj->GetEntityType();
      if (objType && objType->IsPersistenceRequired())
        RptF("Error: Binarize missed persistence for '%s'. Check model properties for '%s', simulation '%s'",
          cc_cast(objType->GetName()),cc_cast(name),obj->GetShape() ? cc_cast(obj->GetShape()->GetPropertyClass()) : "");
    }
#endif
    if (!obj->FixedHeight())
    {
      LODShape *shape = obj->GetShape();
      Vector3 pos = obj->FutureVisualState().Position();
      Vector3 bcWorld = shape ? obj->FutureVisualState().PositionModelToWorld(-shape->BoundingCenter()) : pos;
      float surfY = SurfaceY(bcWorld.X(),bcWorld.Z());
      float sourceSurfY = SurfaceYSource(bcWorld.X(),bcWorld.Z());
      Vector3 newPos = obj->FutureVisualState().Position()+Vector3(0,surfY-sourceSurfY,0);
      obj->SetPosition(newPos);
#ifdef INTERESTED_IN_ID
        if (id==INTERESTED_IN_ID)
        {
#if _HELISIM_LARGE_OBJECTID
          LogF("Id %I64d: change %.2f, tGrid %.2f",obj->ID().Encode(),-surfY,GetTerrainGrid());
#else
          LogF("Id %d, pos %.3f,%.3f,%.3f, surfY %.3f, srcSurfY %.3f, bcWorld %.3f,%.3f,%.3f, tGrid %.2f",
            id,newPos[0],newPos[1],newPos[2],surfY,sourceSurfY,bcWorld.X(),bcWorld.Y(),bcWorld.Z(),GetTerrainGrid());
#endif
        }
#endif
    }
  }
  
  ol->GetList()->_memoryUsedByDiscardable = memoryUsed;

  Assert(somethingLoaded);
  ol->GetList()->Compact();
  // we did not calculate any common information while loading, calculate it now
  ol->GetList()->StaticChanged();
  if (ol->CanBeDeleted())
  {
    Fail("Empty object list loaded");
    QuadTreeExRoot<ObjectList, QuadTreeExTraitsObjectList> &container =
      const_cast<QuadTreeExRoot<ObjectList, QuadTreeExTraitsObjectList> &>(_objects);
    container.Set(x, z, ObjectList());
  }
  return true;
}


bool ObjectListCache::CheckIntegrity() const
{
#if _ENABLE_REPORT
    for (ObjectListFull *list = _items.First(); list; list = _items.Next(list))
    {
      // check if there is some primary object
      bool primary = false;
      for (int i=0; i<list->Size(); i++)
      {
        Object *obj = list->Get(i);
        if (obj->GetObjectId().IsObject())
          primary = true;
      }
      if (!primary)
      {
        LogF("No primary object found");
        return false;
      }
      if (list->Size()==0)
      {
        LogF("Empty list found");
        return false;
      }
    }
#endif
  return true;
}

ObjectListCache *CreateListCache(Landscape *land)
{
  return new ObjectListCache;
}

bool Landscape::VerifyStructure() const
{
  //if (_segCache && !_segCache->VerifyStructure()) return false;
  if (_listCache && !_listCache->CheckIntegrity())
    return false;
  return true;
}

void Landscape::AddRefObjectList(ObjectListFull *list)
{
  if (!IsStreaming()) return;
  list->AddRef();
  if (list->IsInList())
    _listCache->Remove(list);
}

void Landscape::ReleaseObjectListBody(int x, int z, ObjectListFull *listFull)
{
  if (!_objIndex->IsEmpty(x,z))
  {
    // discarding some list which contains unloadable objects
    // instead of releasing it, move it into the cache
    Assert(listFull->CountPrimary()>0);
    Assert(!listFull->IsInList());
    // the list must contain some static objects
    DoAssert(listFull->Size()>listFull->GetNonStaticCount());
    _listCache->Add(listFull);
    Verify(listFull->Release()>0);
  }
  else
  {
    const ObjectList &list = _objects(x,z);
    // list may be released and even destroyed now
    // dynamic object will not be deleted during Release
    Verify(listFull->Release()==0);
    if (list.CanBeDeleted())
      DeleteObjectList(x, z);
  }
}

/*!
Note: Normally this function is used only in Object support for SoftLinks.
If using it in any other situation, make sure you are pairing AddRef/Release properly.
*/
void Landscape::AddRefObjectList(int x, int z)
{
  if (!IsStreaming())
    return;
  const ObjectList &list = _objects(x,z);
  // list.AddRef();
  ObjectListFull *listFull = list.GetList();
  if (listFull)
    AddRefObjectList(listFull);
}

void Landscape::AddRefObjects(int x, int z)
{
  if (!IsStreaming())
    return;
  const ObjectListUsed &list = UseObjects(x,z);
  ObjectListFull *listFull = list.GetList();
  if (listFull && listFull->CountPrimary()>0)
    AddRefObjectList(listFull);
}

void Landscape::ReleaseObjects(int x, int z)
{
  if (!IsStreaming())
    return;
  const ObjectList &list = _objects(x,z);
  ObjectListFull *listFull = list.GetList();
  if (listFull && listFull->CountPrimary()>0)
    ReleaseObjectList(x,z,listFull);
}

/*!
Note: Normally this function is used only in Object support for SoftLinks.
If using it in any other situation, make sure you are pairing AddRef/Release properly.
*/
void Landscape::ReleaseObjectList(int x, int z)
{
  if (!IsStreaming()) return;
  // list.Release();
  const ObjectList &list = _objects(x,z);
  ObjectListFull *listFull = list.GetList();
  if (listFull)
  {
    if (listFull->_used==1)
    {
      if (!_objIndex->IsEmpty(x,z))
      {
        // discarding some list which contains unloadable objects
        // instead of releasing it, move it into the cache
        Assert(!listFull->IsInList());
        // the list must contain some static objects
        DoAssert(listFull->Size()>listFull->GetNonStaticCount());
        _listCache->Add(listFull);
        Verify(listFull->Release()>0);
      }
      else
      {
        // list may be released and even destroyed now
        // dynamic object will not be deleted during Release
        Verify(listFull->Release()==0);
        if (list.CanBeDeleted())
          DeleteObjectList(x, z);
      }
    }
    else if (listFull->_used>0)
      listFull->_used--;
  }
}

void Landscape::DeleteObjectList(int x, int z)
{
  _objects.Set(x, z, ObjectList());
}

/*!
\note constructor can be used only from Landscape::UseObjects
*/
ObjectListUsed Landscape::UseObjects(int x, int z, UseObjectsMode noWait) const
{
  {
    const ObjectList &list = _objects(x,z);
    if (noWait==UONoLock)
      // pass land=NULL to indicate disabled list management
      return ObjectListUsed(list,false,NULL,x,z);
    AssertMainThread();
    if (!IsStreaming())
      // no index -> no cache
      return ObjectListUsed(list,false,unconst_cast(this),x,z);
    // if given object is already cached/loaded reuse it
    ObjectListFull *listData = list.GetList();
    if (listData && listData->IsUsed())
      // return corresponding cached entry
      return ObjectListUsed(list,true,unconst_cast(this),x,z);
  }
  // make sure data are loaded
  {
    bool locked = LoadObjects(x,z,noWait!=UOWait);
    const ObjectList &list = _objects(x,z);
    ObjectListUsed ret(list,locked,unconst_cast(this),x,z);
    if (locked)
    {
      Assert(list.GetList());
      Assert(list.GetList()->_used>1);
      if(list.GetList() && list.GetList()->_used>1)
      {      
      Assert(!list.GetList()->IsInList());
      Verify(list.GetList()->Release()>0);
    }
    }
    return ret;
  }
}

#if _VBS3_LARGE_OBJECTS
ObjectListUsedXXL Landscape::UseObjectsXXL(const ObjectListUsed &list, int x, int z) const
{
  return ObjectListUsedXXL(list,x,z);
}
#endif

bool Landscape::MapObjectsEmpty(int x, int z) const
{
  if (_mapObjIndex.IsNull())
    return _mapObjects(x,z).Size()==0;
  return _mapObjIndex->IsEmpty(x,z);
}

bool Landscape::PreloadMapObjects(int x, int z) const
{
  if (!_mapObjIndex || !land_InRange(z,x))
    return true;
  PROFILE_SCOPE(lndPM)
  const MapObjectList &ol = _mapObjects(x,z);
  // if list is non empty, it must be loaded
  if (ol.GetList())
    return true;
  int beg,end;
  _mapObjIndex->GetBegEndOffsets(x,z,beg,end);
  if (beg>=end)
    return true;
  if (GTimeManager.TimeLeft(TCLoadMapObjects)<=0)
    // if we already spent too much time, do not make any more requests
    return false;
  {
    TimeManagerScope time(TCLoadMapObjects);
    // open source stream
    if (!_stream.PreRead(FileRequestPriority(100),beg,end-beg))
      return false;
  }
  // make sure LoadMapObjects is o
  LoadMapObjects(x,z);
  return true;
}

static int GetHalfPlane(float x, float y, float x1, float y1, float x2, float y2)
{
  return sign((x2 - x1) * (y - y1) - (x - x1) * (y2 - y1));
}

const MapObjectList &Landscape::LoadMapObjects(int x, int z) const
{
  if (!_mapObjIndex)
    return _mapObjects(x,z);
  int beg,end;
  _mapObjIndex->GetBegEndOffsets(x,z,beg,end);
  const MapObjectList &ol = _mapObjects(x,z);
  if (beg>=end)
  {
    // list is empty - no change required
    Assert(beg==end);
    return ol;
  }
  // if list is loaded, it has increased refcount
  if (ol.GetList() && ol.GetList()->IsUsed())
  {
    Assert(ol.Size()>0);
    return ol;
  }
  PROFILE_SCOPE(lndLM)

  TimeManagerScope time(TCLoadMapObjects);
  // open source stream
  _stream.seekg(beg,QIOS::beg);
  SerializeBinStream f(&_stream);
#ifdef _M_PPC
  // TODOX360: support for big-endian binarized landscape
  f.SetEndianState(EndianLittle);
#endif
  //f.SeekG(beg);
  MapObjectList list;
  while (f.TellG()<end && !f.GetError())
  {
    // load map object info
    int type = f.LoadInt();
    if (type<0 || type>=NMapTypes)
    {
      RptF("%s: Bad type reached: %d,%d: %d",(const char *)GetName(),x,z,type);
      break;
    }
    MapObject *obj = CreateMapObject((MapType)type);
    obj->SerializeBin(f);
    list.Add(obj);
  }
  list.Compact();
  Assert(list.Size() > 0);
  QuadTreeExRoot<MapObjectList, QuadTreeExTraitsMapObjectList> &container =
    const_cast<QuadTreeExRoot<MapObjectList, QuadTreeExTraitsMapObjectList> &>(_mapObjects);
  container.Set(x, z, list);
  return _mapObjects(x, z);
}

template <>
struct CacheTraits<MapObjectListFull>: public FramedMemoryControlledListTraitsRef<MapObjectListFull>
{
  typedef MapObjectListFull Type;

  static float GetPriority() {return 0.1f;}
  static const char *GetDebugName() {return "Map lists";}
};

//! cache currently unused lists that have a chance to be used soon again
//! actually they are released from the landscape only after released from the cache
class MapObjectListCache: public CacheContainer<MapObjectListFull>, public RefCount
{
public:
  MapObjectListCache() {}
  ~MapObjectListCache() {Clear();}
};

MapObjectListCache *CreateMapListCache(Landscape *land)
{
  return new MapObjectListCache();
}

MapObjectListUsed Landscape::UseMapObjects(int x, int z) const
{
  // if given object is already cached/loaded reuse it
  const MapObjectList &list = _mapObjects(x,z);
  if (_mapObjIndex.IsNull())
    // no index -> no cache
    return MapObjectListUsed(list, false, unconst_cast(this),x,z);
  MapObjectListFull *listData = list.GetList();
  if (listData && listData->IsInList())
  {
    // refresh cache
    _mapListCache->Refresh(listData);
    // return corresponding cached entry
    return MapObjectListUsed(list,true, unconst_cast(this),x,z);
  }
  // make sure data are loaded
  const MapObjectList &listLoaded = LoadMapObjects(x,z);
  listData = listLoaded.GetList();
  if (listData)
  {
    // keep max 1024 entries
    //_mapListCache->LimitCount(MaxMapCacheEntries);
    _mapListCache->Add(listData);
  }
  return MapObjectListUsed(listLoaded,true, unconst_cast(this),x,z);
}

MapObjectListRectUsed Landscape::UseMapObjects(int xBeg, int zBeg, int xEnd, int zEnd) const
{
  MapObjectListRectUsed rect(this,0,0,0,0,false);
  UseMapObjects(rect,xBeg,zBeg,xEnd,zEnd);
  return rect;
}


/// functor for loading map objects
class LoadMapObjectsItem
{
  bool _useCache;
  const Landscape *_land;
  
public:
  LoadMapObjectsItem(bool useCache, const Landscape *land)
  :_useCache(useCache),_land(land)
  {
  }
  void operator()(int x, int z) const
  {
    const MapObjectList &list = _land->_mapObjects(x,z);
    // if given object is already cached/loaded reuse it
    MapObjectListFull *listData = list.GetList();
    // note: when list is released from the cache, it is empty, but still present
    if (listData && listData->Size()>0)
    {
      listData->AddRef();
      if (_useCache && listData->IsInList())
        _land->_mapListCache->Refresh(listData);
      // return corresponding cached entry
      return;
    }
    // make sure data are loaded
    listData = _land->LoadMapObjects(x,z).GetList();
    if (listData)
    {
      listData->AddRef();
      if (_useCache)
      {
        // keep max 1024 entries
        //_mapListCache->LimitCount(MaxMapCacheEntries);
        _land->_mapListCache->Add(listData);
      }
    }
  }
};

void Landscape::ReleaseMapObjects(int x, int z) const
{
  const MapObjectList &list = _mapObjects(x,z);
  MapObjectListFull *listData = list.GetList();
  if (listData)
  {
    //mimic MapObjectList::Release();
    int count = listData->Release();
    if(count == 0)
    {
      DoAssert(listData->Size() == 0);
      unconst_cast(this)->_mapObjects.Set(x,z,MapObjectList());
    }
  }
}

/// functor for unloading map objects
class UnloadMapObjectsItem
{
  const Landscape *_land;
public:
  explicit UnloadMapObjectsItem(const Landscape *land) {_land = land;}
  void operator()(int x, int z) const {_land->ReleaseMapObjects(x,z);}
};

/// functor for preloading map objects
class PreloadMapObjectsItem
{
  const Landscape *_land;
public:
  PreloadMapObjectsItem(const Landscape *land):_land(land) {}
  void operator()(int x, int z) const {_land->PreloadMapObjects(x,z);}
};

template <class Func>
void ForMapRectangle(int xBeg, int zBeg, int xEnd, int zEnd, const Func &f)
{
  if (xBeg<0) xBeg=0; if (xEnd>LandRangeMask) xEnd=LandRangeMask;
  if (zBeg<0) zBeg=0; if (zEnd>LandRangeMask) zEnd=LandRangeMask;
  DetectInRectangle rect(xBeg,zBeg,xEnd,zEnd);
  GLandscape->ForSomeMapObjectSlots(rect,f);
}

/// functor usefull for ForSome iterations
class DetectInRectangleComplement
{
  /// include
  GridRectangle _i;
  /// exclude
  GridRectangle _e;
  
public:
  DetectInRectangleComplement(const GridRectangle &i, const GridRectangle &e)
  :_i(i),_e(e)
  {}
  bool operator()(int xBeg, int zBeg, int xSize, int zSize) const
  {
    GridRectangle r(xBeg,zBeg,xBeg+xSize,zBeg+zSize);
    // R = {xBeg,zBeg,xEnd,zEnd}
    // intersect R with I (S = R&I)
    GridRectangle s = r&_i;
    if (s.IsEmpty())
      return false;
    // check what is the intersection of S with E
    // if it is S, S is fully outside
    // if it is 0, S is fully inside - if S==R, operator may return "All inside"
    GridRectangle se = s&_e;
    //if (se==r)
    if (se==s)
      return false; // no point inside of the complement
    return true;
  }
};

/// rectangle complement implemented using complement predicate
template <class Func>
void ForMapRectangleComplementPred(const GridRectangle &i, GridRectangle &e, const Func &f)
{
  GridRectangle clip(0,0,LandRange,LandRange);
  GridRectangle iClipped = i&clip;
  GridRectangle eClipped = e&clip;
  DetectInRectangleComplement pred(i,e);
  GLandscape->ForSomeMapObjectSlots(pred, f);
}

void Landscape::UseMapObjects(MapObjectListRectUsed &lock, int xBeg, int zBeg, int xEnd, int zEnd) const
{
  // if we would need to flush the whole cache, do cache
  // caching in such situation is counter productive

  saturateMax(xBeg,0);
  saturateMax(zBeg,0);
  saturateMin(xEnd,GetLandRange());
  saturateMin(zEnd,GetLandRange());
  if (_mapObjIndex.IsNull())
  {
    // no index -> no cache
    lock = MapObjectListRectUsed(this,xBeg,zBeg,xEnd,zEnd,false);
    return;
  }
  
#if 1 //_DEBUG
    #if _ENABLE_REPORT
      // for profiling purposes it may be interesting to turn the cache off
      static bool useCache = true;
      // for profiling purposes it may be interesting to force flush (hence loading) each frame
      static bool flushEachFrame = false;
      if (flushEachFrame)
      {
        UnloadMapObjectsItem unload(this);
        ForMapRectangle(lock._rect.xBeg,lock._rect.zBeg,lock._rect.xEnd,lock._rect.zEnd,unload);
        // mark locked area as empty - we unlocked everything
        lock._rect = GridRectangle(0,0,0,0);
      }
    #else
      const bool useCache = true;
    #endif
#else
    // if rectangle is bigger than cache size, do not cache it
    // the only result of such cacheing would be trashing the cache
    // if there is some memory is free, we can cache more
    int maxEntries = _mapListCache->GetItemCount()*3/4;
    // when cache size is not fixed, cache size may be large when we have plenty of memory
    // 3K was a reasonable limit for old Xbox
    if (maxEntries<3*1024) maxEntries = 3*1024;
    int count = (xEnd-xBeg)*(zEnd-zBeg);
    bool useCache = count<maxEntries;
#endif
  LoadMapObjectsItem load(useCache,this);
  UnloadMapObjectsItem unload(this);
  // "Delete" parts of the old lock which are not covered by the newt one
//   ForMapRectangleComplementPred(lock._xBeg,lock._zBeg,lock._xEnd,lock._zEnd,xBeg,zBeg,xEnd,zEnd,unload);
// 
//   // "Load/AddRef" parts of the new lock which are not covered by the old one
//   ForMapRectangleComplementPred(xBeg,zBeg,xEnd,zEnd,lock._xBeg,lock._zBeg,lock._xEnd,lock._zEnd,load);

  GridRectangle rect(xBeg,zBeg,xEnd,zEnd);
  ForMapRectangleComplementPred(lock._rect,rect,unload);

  // "Load/AddRef" parts of the new lock which are not covered by the old one
  ForMapRectangleComplementPred(rect,lock._rect,load);
  lock._rect = rect;
  lock._land = this;
  Assert(lock.CheckIntegrity());
}


bool Landscape::PreloadMapObjects(const MapObjectListRectUsed &lock, int xBeg, int zBeg, int xEnd, int zEnd) const
{
  // we need to preload only items which are not locked by the lock
  GridRectangle preload(xBeg,zBeg,xEnd,zEnd);
  // preload based on preload complement locked predicate
  DetectInRectangleComplement pred(preload,lock._rect);
  PreloadMapObjectsItem preloadItem(this);
  ForSomeMapObjectSlots(pred,preloadItem);
  return true;
}

typedef bool AddFunc(float, float, Object *);

static bool AddAll(float centerX, float centerZ, Object *obj)
{
  if (!obj)
    return false;
  LODShape *shape = obj->GetShape();
  if (!shape)
    return false;
  MapType type = shape->GetMapType();
  if (type != MapBush && type != MapTree && type != MapSmallTree)
    return false;
  return true;
}

static bool AddTL(float centerX, float centerZ, Object *obj)
{
  if (!AddAll(centerX, centerZ, obj))
    return false;
  float x = obj->FutureVisualState().Position().X() - centerX;
  float z = obj->FutureVisualState().Position().Z() - centerZ;
  return z > x;
}

static bool AddTR(float centerX, float centerZ, Object *obj)
{
  if (!AddAll(centerX, centerZ, obj))
    return false;
  float x = obj->FutureVisualState().Position().X() - centerX;
  float z = obj->FutureVisualState().Position().Z() - centerZ;
  return z > -x;
}

static bool AddBL(float centerX, float centerZ, Object *obj)
{
  if (!AddAll(centerX, centerZ, obj))
    return false;
  float x = obj->FutureVisualState().Position().X() - centerX;
  float z = obj->FutureVisualState().Position().Z() - centerZ;
  return z < -x;
}

static bool AddBR(float centerX, float centerZ, Object *obj)
{
  if (!AddAll(centerX, centerZ, obj))
    return false;
  float x = obj->FutureVisualState().Position().X() - centerX;
  float z = obj->FutureVisualState().Position().Z() - centerZ;
  return z < x;
}

int Landscape::CreateArea(int x, int z, const ObjectListUsed &src, MapObjectList &dst,
  CheckMapType &funcCount, CheckMapType &funcAddOutside, MapType areaType, float minObjPerSquare)
{
  const float forestGridMin = 20.0f; // minimal size of forest or rocks

  int fields = toIntFloor(GetLandGrid() / forestGridMin);
  saturateMax(fields, 1);
  float forestGrid = GetLandGrid() / fields;
  float limit = minObjPerSquare / Square(fields);
  float invLimit = 1.0f / limit;

  // 0. Init counters
  Array2D<int> trees;
  trees.Dim(fields + 1, fields + 1);
  for (int row=0; row<=fields; row++)
    for (int col=0; col<=fields; col++)
      trees(row, col) = 0;

  // 1. Count objects
  for (int di=-1; di<=1; di++)
  {
    if (x + di < 0 || x + di >= GetLandRange()) continue;
    for (int dj=-1; dj<=1; dj++)
    {
      if (z + dj < 0 || z + dj >= GetLandRange()) continue;
      const ObjectListUsed &list = UseObjects(x + di, z + dj); 
      for (int o=0; o<list.Size(); o++)
      {
        Object *obj = list[o];
        if (!obj) continue;
        LODShape *shape = obj->GetShape();
        if (!shape) continue;

        MapType type = shape->GetMapType();
        if (funcCount(type))
        {
          Vector3 pos = obj->FutureVisualState().Position();
          int row = toInt(fmod(pos.Z(), GetLandGrid()) / forestGrid) + dj * fields;
          int col = toInt(fmod(pos.X(), GetLandGrid()) / forestGrid) + di * fields;
          if (row >= 0 && row <= fields && col >= 0 && col <= fields)
            trees(row, col)++;
        }
      }
    }
  }

  // creating of map objects
  int areaCount = 0; // Number of areas
  for (int row=0; row<fields; row++)
  {
    for (int col=0; col<fields; col++)
    {
      float tl = trees(row, col);
      float tr = trees(row, col + 1);
      float bl = trees(row + 1, col);
      float br = trees(row + 1, col + 1);
      if (tl <= limit && tr <= limit && bl <= limit && br <= limit) continue;

      MapObjectForest *mapObj = new MapObjectForest(areaType);
      mapObj->_row = row;
      mapObj->_col = col;
      mapObj->_rows = fields;
      mapObj->_cols = fields;
      mapObj->_tl = tl * invLimit;
      mapObj->_tr = tr * invLimit;
      mapObj->_bl = bl * invLimit;
      mapObj->_br = br * invLimit;
      dst.Add(mapObj);
      areaCount++;
    }
  }

  // 3. Add trees and bushes
  for (int i=0; i<src.Size(); i++)
  {
    Object *obj = src[i];
    if (!obj) continue;
    LODShape *shape = obj->GetShape();
    if (!shape) continue; // already added

    MapType type = shape->GetMapType();
    // already added
    if (!funcAddOutside(type)) continue;

    // Check if outside forest
    Vector3 pos = obj->FutureVisualState().Position();

    float xx = (pos.X() - x * GetLandGrid()) / forestGrid;
    float yy = (pos.Z() - z * GetLandGrid()) / forestGrid;

    int col = toIntFloor(xx);
    int row = toIntFloor(yy);

    xx -= col;
    yy -= row;

    if (col < 0 || col >= fields)
    {
      RptF("Tree at %.3f, %.3f out of grid (x-axis) - %.3f rounded to %d", pos.X(), pos.Z(), xx, col);
      continue;
    }
    if (row < 0 || row >= fields)
    {
      RptF("Tree at %.3f, %.3f out of grid (z-axis) - %.3f rounded to %d", pos.X(), pos.Z(), yy, row);
      continue;
    }

    float tl = trees(row, col);
    float tr = trees(row, col + 1);
    float bl = trees(row + 1, col);
    float br = trees(row + 1, col + 1);

    int bits = 0;
    if (tl > limit) bits |= 8;
    if (tr > limit) bits |= 4;
    if (bl > limit) bits |= 2;
    if (br > limit) bits |= 1;

    bool forest = false;
    switch (bits)
    {
    case 0:
      break;
    case 1:
      {
        // br
        float coef1 = (limit - bl) / (br - bl); 
        float coef2 = (limit - tr) / (br - tr);
        forest = GetHalfPlane(xx, yy, coef1, 1, 1, coef2) > 0;
      }
      break;
    case 2:
      {
        // bl
        float coef1 = (limit - tl) / (bl - tl);
        float coef2 = (bl - limit) / (bl - br); 
        forest = GetHalfPlane(xx, yy, 0, coef1, coef2, 1) > 0;
      }
      break;
    case 3:
      {
        // bl, br
        float coef1 = (limit - tl) / (bl - tl);
        float coef2 = (limit - tr) / (br - tr); 
        forest = GetHalfPlane(xx, yy, 0, coef1, 1, coef2) > 0;
      }
      break;
    case 4:
      {
        // tr
        float coef1 = (tr - limit) / (tr - br);
        float coef2 = (limit - tl) / (tr - tl); 
        forest = GetHalfPlane(xx, yy, 1, coef1, coef2, 0) > 0;
      }
      break;
    case 5:
      {
        // tr, br
        float coef1 = (limit - bl) / (br - bl);
        float coef2 = (limit - tl) / (tr - tl); 
        forest = GetHalfPlane(xx, yy, coef1, 1, coef2, 0) > 0;
      }
      break;
    case 6:
      {
        // tr
        float coef1 = (tr - limit) / (tr - br);
        float coef2 = (limit - tl) / (tr - tl); 
        forest = GetHalfPlane(xx, yy, 1, coef1, coef2, 0) > 0;
      }
      {
        // bl
        float coef1 = (limit - tl) / (bl - tl);
        float coef2 = (bl - limit) / (bl - br); 
        forest = forest || GetHalfPlane(xx, yy, 0, coef1, coef2, 1) > 0;
      }
      break;
    case 7:
      {
        // tr, bl, br
        float coef1 = (limit - tl) / (bl - tl);
        float coef2 = (limit - tl) / (tr - tl); 
        forest = GetHalfPlane(xx, yy, 0, coef1, coef2, 0) > 0;
      }
      break;
    case 8:
      {
        // tl
        float coef1 = (tl - limit) / (tl - bl);
        float coef2 = (tl - limit) / (tl - tr); 
        forest = GetHalfPlane(xx, yy, coef1, 0, 0, coef2) > 0;
      }
      break;
    case 9:
      {
        // tl
        float coef1 = (tl - limit) / (tl - bl);
        float coef2 = (tl - limit) / (tl - tr); 
        forest = GetHalfPlane(xx, yy, coef1, 0, 0, coef2) > 0;
      }
      {
        // br
        float coef1 = (limit - bl) / (br - bl); 
        float coef2 = (limit - tr) / (br - tr);
        forest = forest || GetHalfPlane(xx, yy, coef1, 1, 1, coef2) > 0;
      }
      break;
    case 10:
      {
        // tl, bl
        float coef1 = (tl - limit) / (tl - tr);
        float coef2 = (bl - limit) / (bl - br); 
        forest = GetHalfPlane(xx, yy, coef1, 0, coef2, 1) > 0;
      }
      break;
    case 11:
      {
        // tl, bl, br
        float coef1 = (tl - limit) / (tl - tr);
        float coef2 = (limit - tr) / (br - tr);
        forest = GetHalfPlane(xx, yy, coef1, 0, 1, coef2) > 0;
      }
      break;
    case 12:
      {
        // tl, tr
        float coef1 = (tr - limit) / (tr - br);
        float coef2 = (tl - limit) / (tl - bl); 
        forest = GetHalfPlane(xx, yy, 1, coef1, 0, coef2) > 0;
      }
      break;
    case 13:
      {
        // tl, tr, br
        float coef1 = (limit - bl) / (br - bl);
        float coef2 = (tl - limit) / (tl - bl); 
        forest = GetHalfPlane(xx, yy, coef1, 1, 0, coef2) > 0;
      }
      break;
    case 14:
      {
        // tl, tr, bl
        float coef1 = (tr - limit) / (tr - br);
        float coef2 = (bl - limit) / (bl - br); 
        forest = GetHalfPlane(xx, yy, 1, coef1, coef2, 1) > 0;
      }
      break;
    case 15:
      forest = true;
      break;
    }
    if (forest) continue;

    MapObject *mapObj = CreateMapObject(obj);
    if (mapObj) dst.Add(mapObj);
  }
  return areaCount;
}

static bool IsTree(MapType type)
{
  return type == MapTree;
}

static bool IsTreeOrBush(MapType type)
{
  return type == MapBush || type == MapTree || type == MapSmallTree;
}

static bool IsRock(MapType type)
{
  return type == MapRock;
}

// log basic statistics about objects in map
#define LOG_MAP_STATS 0

#if LOG_MAP_STATS
struct MapModelInfo
{
  RString _model;
  MapType _type;
  int _count;
};
TypeIsMovableZeroed(MapModelInfo)

struct MapModelInfos
{
  AutoArray<MapModelInfo> _list;

  void Clear();
  void Add(LODShape *shape);
  void Report(const RString &world);
};

void MapModelInfos::Clear()
{
  _list.Clear();
}

void MapModelInfos::Add(LODShape *shape)
{
  RString name = shape->GetName();
  int n = _list.Size();
  for (int i=0; i<n; i++)
  {
    if (_list[i]._model == name)
    {
      if (shape->GetMapType() != _list[i]._type)
      {
        RptF("Different map types for model: %s", cc_cast(name));
      }
      _list[i]._count++;
      return;
    }
  }
  // not found
  int i = _list.Add();
  _list[i]._model = name;
  _list[i]._type = shape->GetMapType();
  _list[i]._count = 1;
}

void MapModelInfos::Report(const RString &world)
{
  RptF("Summary of map objects for %s", cc_cast(world));

#define MAP_TYPE_NAME(value, name) \
  name,

  static RString typeNames[] =
  {
  MAP_TYPES_ALL(MAP_TYPE_NAME)
  };

  int n = _list.Size();
  for (int i=0; i<n; i++)
  {
    RString type = "none";
    int t = _list[i]._type;
    if (t >= 0 && t < lenof(typeNames)) type = typeNames[t];
    RptF("  %s, type %s, %d x", cc_cast(_list[i]._model), cc_cast(type), _list[i]._count);
  }
}

static MapModelInfos GMapModelInfos;
#endif

void Landscape::InitMapObjects()
{
#if LOG_MAP_STATS
  GMapModelInfos.Clear();
#endif
  int forestCount = 0;
  int rockCount = 0;
  for (int z=0; z<GetLandRange(); z++)
  {
    I_AM_ALIVE();
    if (GProgress) GProgress->Refresh();
    for (int x=0; x<GetLandRange(); x++)
    {
      const ObjectListUsed &src = UseObjects(x, z); 
      MapObjectList dst;

      // Add objects different from trees, bushes and rocks
      for (int i=0; i<src.Size(); i++)
      {
        Object *obj = src[i];
        if (!obj)
          continue;
        LODShape *shape = obj->GetShape();
#if LOG_MAP_STATS
        if (shape) GMapModelInfos.Add(shape);
#endif
        if (shape && _minTreesInForestSquare > 0)
        {
          MapType type = shape->GetMapType();
          // add only outside forest (in the second pass)
          if (type == MapBush || type == MapTree || type == MapSmallTree)
            continue;
        }
        if (shape && _minRocksInRockSquare > 0)
        {
          MapType type = shape->GetMapType();
          // add only outside rocks (in the second pass)
          if (type == MapRock)
            continue;
        }
        MapObject *mapObj = CreateMapObject(obj);
        if (mapObj)
          dst.Add(mapObj);
      }

      // Special manipulation with trees and bushes
      if (_minTreesInForestSquare > 0)
        forestCount += CreateArea(x, z, src, dst, IsTree, IsTreeOrBush, MapForest, _minTreesInForestSquare);
      // Special manipulation with rocks
      if (_minRocksInRockSquare > 0)
        rockCount += CreateArea(x, z, src, dst, IsRock, IsRock, MapRocks, _minRocksInRockSquare);

      dst.Compact();
      _mapObjects.Set(x, z, dst);
    }
  }
#if LOG_MAP_STATS
  GMapModelInfos.Report(GetName());
#endif

//   RptF("Info: Created %d forests and %d rocks", forestCount, rockCount);
//   RptF("Info: GetLandRange() == %d", GetLandRange());
//   RptF("Info: _minTreesInForestSquare == %f", _minTreesInForestSquare);
//   RptF("Info: _minRocksInRockSquare == %f", _minRocksInRockSquare);
}

//#include <El/Debugging/imexhnd.h>

/// adjust file offset
class QuadTreeAdjustOffset
{
protected:
  int _add;
public:
  QuadTreeAdjustOffset(int add) {_add = add;}
  int operator()(const int &value) const
  {
    // do not touch invalid fields
    if (value<0)
      return value;
    return value + _add;
  }
};

void Landscape::FlushRectCaches()
{
  _listCache.Free();
  _listCache = CreateListCache(this);
  _mapListCache.Free();
  _mapListCache = CreateMapListCache(this);
  _preloadState->Reset();
}

void Landscape::FreeRectCaches()
{
  // make sure no virtual targets exist any longer
  for (int i=0; i<_buildings.Size(); i++)
    _buildings.GetInfo(i).ClearVirtualTarget();
  
  _operCache.Free();
  _lockCache.Free();
  _listCache.Free();
  _suppressCache.Free();
  _mapListCache.Free();
  if (_preloadState)
    _preloadState->Reset();
}

void Landscape::CreateRectCaches()
{
  _operCache = CreateOperCache(this);
  _lockCache = CreateLockCache(this);
  _suppressCache = CreateSuppressCache(this);
  _listCache = CreateListCache(this);
  _mapListCache = CreateMapListCache(this);
  _preloadState = new PreloadDataState;
}

// all static vehicles need to be tracked and know all the time
// we need some basic info about them:
struct StaticEntityInfo
{
  RStringB className;
  RStringB shapeName;
  Vector3 pos;
  ObjectId id;
  
  void SerializeBin(SerializeBinStream &f);
};

TypeIsMovableZeroed(StaticEntityInfo)

SerializeBinStream &operator << (SerializeBinStream &f, StaticEntityInfo &info)
{
  info.SerializeBin(f);
  return f;
}

RString LandscapeObjectName(const char *shape);

static RStringB GetEntityClassName(RStringB shapeName)
{
  ParamEntryVal val = Pars>>"CfgVehicles";
  RString className = LandscapeObjectName(shapeName);
  ConstParamEntryPtr entry = val.FindEntry(className);
  if (entry)
    return className;
  return RStringB();
}

void StaticEntityInfo::SerializeBin(SerializeBinStream &f)
{
  f.Transfer(className);
  f.Transfer(shapeName);
  f.Transfer(pos);
#if _HELISIM_LARGE_OBJECTID
  if (f.IsLoading())
  {
    id.Decode32(f.LoadInt());
  }
  else
  {
    f.SaveInt(id.Encode32());
  }
#else
  if (f.IsLoading())
  {
    id.Decode(f.LoadInt());
  }
  else
  {
    f.SaveInt(id.Encode());
  }
#endif
}

#if USE_LANDSCAPE_LOCATIONS

// select all locations
static bool LocationAll(int x, int y, int size)
{
  return true;
}

struct LocationCollectMountainPositions
{
  Ref<LocationType> _type;
  AutoArray<Vector3> _mountains;

  LocationCollectMountainPositions()
  {
    _type = LocationTypes.New("Mount");
  }

  bool operator () (const Ref<LocationsItem> &item)
  {
    for (int i=0; i<item->_locations.Size(); i++)
    {
      Location *location = item->_locations[i];
      if (location->_type == _type)
        _mountains.Add(location->_position);
    }
    return false; // continue with enumeration
  }
};

struct LocationResetState
{
  bool operator () (const Ref<LocationsItem> &item) const
  {
    for (int i=0; i<item->_locations.Size(); i++)
    {
      Location *location = item->_locations[i];
      location->_side = TSideUnknown;
      GameVarSpace *vars = location->GetVars();
      DoAssert(!vars); // static locations should have no variable space
      if (vars) vars->Reset();
    }
    return false; // continue with enumeration
  }
};

#include <Es/Algorithms/qsort.hpp>

static int CmpMountains(const Vector3 *v0, const Vector3 *v1)
{
  return sign(v1->Y() - v0->Y());
}

#endif

/// functor transforming old water representation into a new one
struct TransformOldWater
{
  unsigned short operator()(unsigned short geogr) const
  {
    GeographyInfo o(geogr);
    const static unsigned maxFromOld[4]={0,1,2,3};
    const static unsigned minFromOld[4]={0,0,1,3};
    int index = o.u.minWaterDepth;
    o.u.minWaterDepth = minFromOld[index];
    o.u.maxWaterDepth = maxFromOld[index];
    return o.packed;
  }
};

DEF_RSB(bushsoft);
DEF_RSB(bushhard);
DEF_RSB(treesoft);
DEF_RSB(treehard);

/// used for debugging false persistence

static RString PersintenceReasons(int reasons)
{
  RString ret = RString();
  if (reasons&PersistentSimulation) ret = ret+" simulation";
  if (reasons&PersistentSound) ret = ret +" sound";
  if (reasons&PersistentRoad) ret = ret + " road";
  if (reasons&PersistentForest) ret = ret + " forest";
  if (reasons&PersistentHouse) ret = ret + " house";
  return ret;
}

static int ParseTerrainMaterial(const char *name, int &satX, int &satY, int &unused)
{
  // material names looks like:
  // 0         1
  // 01234567890
  // p_015-020_l00_l02_l03_n_l06_l07.rvmat
  // p_031-005_l00.rvmat
  // p_031-004_n_l02_l03_l07.rvmat
  // p_031-004_l00_n_n.rvmat
  // verify the name looks as expected
  if (name[0]!='p' || name[1]!='_' || name[5]!='-' || name[9]!='_') return -1;
  // parse coordinates
  satX = atoi(name+2);
  satY = atoi(name+6);
  // parse the layer mask
  // all we need is to count how many field are there. Some are used, other are _n, their count should be constant
  const char *layers = name+9;
  // each field is _lXX or _n.
  // we therefore count _ before reaching the end (dot or terminator zero)
  int layerCount = 0;
  unused = 0;
  while (*layers && *layers!='.')
  {
    if (*layers == '_')
      layerCount++;
    if (*layers == 'n')
      unused++;
    layers++;
  }
  Assert(layerCount>0 && layerCount<=LandMatLayerCount);
  return layerCount;
}

void Landscape::SerializeBin(SerializeBinStream &f, const ParamEntry *cls)
{
  //DebugExceptionTrap :: LogFFF("test",true);

  Ref<ProgressHandle> p;
  float lastRest = 0;
  if (f.IsLoading())
  {
    ProgressReset();
    IProgressDisplay *CreateDisplayLoadIsland(RString text, RString date);
    p = ProgressStart(CreateDisplayLoadIsland(LocalizeString(IDS_LOAD_WORLD), RString()));
    lastRest = f.GetRest();
    ProgressAdd(lastRest);
    ProgressRefresh();
    Assert(cls);
    Init(*cls); // clear the landscape
#if PROTECTION_ENABLED && !CHECK_FADE
    CheckFade();
#endif
  }

  Log("Begin landscape load (%d KB).",MemoryUsed()/1024);
#ifdef _MSC_VER
  if (!f.Version('WRPO'))
#else
  if (!f.Version(StrToInt("OPRW")))
#endif
  {
    f.SetError(f.EBadFileType);
    ProgressFinish(p);
    return;
  }
  // preload a substantial part of the landscape file to prevent fragmentation
  //f.PreReadSequential(512*1024);
#if _VBS3_LARGE_OBJECTS_WRP
  const int actVersion = 25;
#else
  const int actVersion = 24;
#endif
  int version = actVersion;
  f.Transfer(version);
  // version 3 - OFP Retail landscape (no streaming, no map)
  // version 5 - OFP XBox landscape beta (streaming, no map)
  // version 6 - landscape (streaming and map)
  // version 7 - landscape, including roads (streaming and map)
  // version 10 - landscape, quad trees 
  // version 11 - landscape, changed geography
  // version 12 - OFP Xbox/FP2 landscape, different grid for textures and terrain
  // version 13 - landscape, subdivision hints included
  // version 14 - landscape, skew object flag added
  // version 15 - landscape, entity list added
  // version 16 - ArmA landscape, roads transform + LODShape added
  // version 17 - major texture pass added
  // version 18 - grass map added, float used as raw data
  // version 19 - water depth geography info change
  // version 20 - grass map contains flat areas around roads
  // version 21 - randomization array removed
  // version 22 - primary texture info added
  // version 23 - LZO compression used for compressed arrays
  // version 24 - extended info for roads (connection types)
#if _VBS3_LARGE_OBJECTS_WRP
  // version 25 - storing of large static objects R-tree in wrp <-- NOTE: technology not used. Implemented without need of WRP changes!
#endif
  if ((version<15 || version>actVersion) && version!=3)
  {
    WarningMessage("Bad version %d in landscape %s",version,(const char *)_name);
    f.SetError(f.EBadVersion);
    ProgressFinish(p);
    return;
  }

  Assert(f.CheckIntegrity());

  if (version >= 23)
    f.UseLZOCompression();

  if (f.IsLoading())
  {
    _fileVersion = version;
    _lzoCompression = f.IsUsingLZOCompression();
    Assert(cls);
    if (version >= 12)
    {
      int lx = f.LoadInt();
      int ly = f.LoadInt();
      int rx = f.LoadInt(); // terrain x,y
      int ry = f.LoadInt();
      float landGrid;
      f << landGrid;
      Dim(lx,ly,rx,ry,landGrid);
    }
    else
    {
      float landGrid = (*cls) >> "LandGrid";    
      if (version>=3)
      {
        int lx = f.LoadInt();
        int ly = f.LoadInt();
        int rx = f.LoadInt(); // terrain x,y
        int ry = f.LoadInt();
        Dim(lx,ly,rx,ry,landGrid);
      }
      else
        Dim(256,256,256,256,landGrid);
    }
    // set default grid size
    // FIX - why this command ???
    // SetLandGrid(50);
    FlushCache();
  }
  else
  {
    if (version>=12)
    {
      int lx = GetLandRange();
      int ly = GetLandRange();
      int rx = GetTerrainRange(); // terrain x,y
      int ry = GetTerrainRange();
      float landGrid = GetLandGrid();
      f.SaveInt(lx);
      f.SaveInt(ly);
      f.SaveInt(rx);
      f.SaveInt(ry);
      f << landGrid;
    }
    else if (version>=3)
    {
      int lx = GetLandRange();
      int ly = GetLandRange();
      int rx = GetLandRange(); // terrain x,y
      int ry = GetLandRange();
      f.SaveInt(lx);
      f.SaveInt(ly);
      f.SaveInt(rx);
      f.SaveInt(ry);
    }
  }
  // transfer all relevant data
  if (version>=10)
  {
    if (f.IsLoading())
    {
      LoadContentBinary(f, _terrain->_geography);
      if (version<19)
        // transform old water information into a new one
        _terrain->_geography.ForEachItem(TransformOldWater());
    }
    else
      SaveContentBinary(f, _terrain->_geography);
  }
  else if (version>=8)
  {
    Fail("No longer supported");
  }
  else
  {
    DoAssert(f.IsLoading());
    // in old versions geography info was 32b
    Array2D<GeographyInfo> geogr32;
    geogr32.Dim(_terrain->_geography.GetXRange(),_terrain->_geography.GetYRange());
    f.TransferBinaryCompressed(geogr32.RawData(),geogr32.RawSize());
    f.ProcessLittleEndianData(geogr32.RawData(),geogr32.RawSize(),sizeof(_terrain->_geography(0,0)));
    for (int z=0; z<geogr32.GetYRange(); z++)
      for (int x=0; x<geogr32.GetXRange(); x++)
      {
        geogr32(x,z).FromFormat3To7();
        _terrain->_geography.Set(x, z, geogr32(x,z).GetPacked());
      }
    // transform old water information into a new one
    _terrain->_geography.ForEachItem(TransformOldWater());
  }
  PROGRESS;
  if (version>=10)
  {
    if (f.IsLoading())
      LoadContentBinary(f, _soundMap);
    else
      SaveContentBinary(f, _soundMap);
  }
  else
  {
    if (f.IsLoading())
    {
      Array2D<byte> soundMap;
      soundMap.Dim(_soundMap.GetXRange(),_soundMap.GetYRange());
      f.TransferBinaryCompressed(soundMap.RawData(),soundMap.RawSize());
      for (int z=0; z<soundMap.GetYRange(); z++)
        for (int x=0; x<soundMap.GetXRange(); x++)
          _soundMap.Set(x, z, soundMap(x,z));
    }
    else
    {
      Array2D<byte> soundMap;
      soundMap.Dim(_soundMap.GetXRange(),_soundMap.GetYRange());
      for (int z=0; z<soundMap.GetYRange(); z++)
        for (int x=0; x<soundMap.GetXRange(); x++)
          soundMap(x, z) = _soundMap(x, z);
      f.TransferBinaryCompressed(soundMap.RawData(),soundMap.RawSize());
    }
  }
  PROGRESS;
#if USE_LANDSCAPE_LOCATIONS
  if (f.IsLoading())
  {
    AutoArray<Vector3> mountains;
    f.TransferBasicArray(mountains);
    // fill locations
    if (mountains.Size() > 0)
    {
      Ref<LocationType> type = LocationTypes.New("Mount");
      for (int i=0; i<mountains.Size(); i++)
      {
        Location *location = new Location(type, mountains[i], 0, 0);
        location->_importance = mountains[i].Y();
        _locations.AddLocation(location);
      }
    }

    RebuildLocationDistances();
  }
  else
  {
    // extract mountains from _locations
    LocationCollectMountainPositions action;
    _locations.ForEachInRegion(LocationAll, action);
    QSort(action._mountains.Data(), action._mountains.Size(), CmpMountains);
    f.TransferBasicArray(action._mountains);
  }
#else
  f.TransferBasicArray(_mountains);
#endif
  PROGRESS;
  // f.TransferBinaryCompressed(_tex.RawData(),_tex.RawSize());
  if (version>=10)
  {
    if (f.IsLoading())
      LoadContentBinary(f, _terrain->_tex);
    else
      SaveContentBinary(f, _terrain->_tex);
  }
  else
  {
    if (f.IsLoading())
    {
      Array2D<short> tex;
      int xRange = _terrain->_tex.GetXRange();
      int yRange = _terrain->_tex.GetYRange();
      tex.Dim(xRange, yRange);
      f.TransferBinaryCompressed(tex.RawData(), tex.RawSize());
      f.ProcessLittleEndianData(tex.RawData(),tex.RawSize(),sizeof(tex(0,0)));
      for (int z=0; z<yRange; z++)
        for (int x=0; x<xRange; x++)
          _terrain->_tex.Set(x, z, tex(x, z));
    }
    else
    {
      Array2D<short> tex;
      int xRange = _terrain->_tex.GetXRange();
      int yRange = _terrain->_tex.GetYRange();
      tex.Dim(xRange, yRange);
      for (int z=0; z<yRange; z++)
        for (int x=0; x<xRange; x++)
          tex(x, z) = _terrain->_tex(x, z);
      f.TransferBinaryCompressed(tex.RawData(), tex.RawSize());
    }
  }
  PROGRESS;
  if (version<21)
  {
    Assert(f.IsLoading()); // older version can only be loaded, not saved
    if (version>=8) 
    {
      Array2D<short> random;
      random.Dim(_terrain->_tex.GetXRange(),_terrain->_tex.GetYRange());
      f.TransferBinaryCompressed(random.RawData(),random.RawSize());
    }
    else
    {
      Assert(f.IsLoading());
      Array2D<RandomInfo> random;
      random.Dim(_terrain->_tex.GetXRange(),_terrain->_tex.GetYRange());
      f.TransferBinaryCompressed(random.RawData(),random.RawSize());
    }
  }
  PROGRESS;
  if (version>=18)
  {
    // grass approximation is saved in the file
    if (f.IsLoading())
    {
      // grass approximation is not created yet
      // we need to create it so that it can be loaded
      _grassApproxOrig.Dim(_terrain->_data.GetXRange(),_terrain->_data.GetYRange());
    }
    f.TransferBinaryCompressed(_grassApproxOrig.RawData(),_grassApproxOrig.RawSize());
    if (f.IsLoading())
    {
      f.ProcessLittleEndianData(_grassApproxOrig.RawData(),_grassApproxOrig.RawSize(),sizeof(_grassApproxOrig(0,0)));
      _grassApprox = _grassApproxOrig;
    }
  }
  PROGRESS;
  if (version >= 22)
  { // from version 22 on, coord->primary texture index mapping is saved in the file
    if (f.IsLoading())
      _primTexIndex.Dim(_terrain->_data.GetXRange(),_terrain->_data.GetYRange());
    f.TransferBinaryCompressed(_primTexIndex.RawData(),_primTexIndex.RawSize());
  }
  else if (f.IsLoading())
    // otherwise generate some default mapping
    InitPrimaryTexture(false);
  PROGRESS;
  if (RawTypeIsFloat)
  {
    TimeScope time("Load Terrain");
    // unswizzle before saving - to make disk version linear
    if (!f.IsLoading())
      _terrain->_data.Unswizzle();
    f.TransferBinaryCompressed(_terrain->_data.RawData(),_terrain->_data.RawSize());
    if (f.IsLoading())
      f.ProcessLittleEndianData(_terrain->_data.RawData(),_terrain->_data.RawSize(),sizeof(_terrain->_data(0,0)));
    // swizzle data if needed (disk version is always linear)
    _terrain->_data.Swizzle();
  }
  else if (f.IsLoading())
  {
    TimeScope time("Load Terrain");
    Array2D<float> terrain;
    terrain.Dim(_terrain->_data.GetXRange(),_terrain->_data.GetYRange());
    f.TransferBinaryCompressed(terrain.RawData(),terrain.RawSize());
    f.ProcessLittleEndianData(terrain.RawData(),terrain.RawSize(),sizeof(terrain(0,0)));
    for (int z=0; z<_terrain->_data.GetXRange(); z++) for (int x=0; x<_terrain->_data.GetXRange(); x++)
    {
      _terrain->_data(x,z) = HeightToRaw(terrain(x,z));
      //        _data.Set(x, z, HeightToRaw(terrain(x, z)));
    }
  }
  else
  {
    // convert and unswizzle data before saving them
    Array2D<float> terrain;
    terrain.Dim(_terrain->_data.GetXRange(),_terrain->_data.GetYRange());
    for (int z=0; z<_terrain->_data.GetXRange(); z++) for (int x=0; x<_terrain->_data.GetXRange(); x++)
      terrain(x,z) = RawToHeight(_terrain->_data(x,z));
    f.TransferBinaryCompressed(terrain.RawData(),terrain.RawSize());
  }
  PROGRESS;
  // transfer texture index
  if (f.IsLoading())
  {
    // Simple CD Key check
    RString GetPublicKey();
    GetPublicKey();

    bool objectIndex = version>=4;
    bool mapObjectIndex = version>=6;
    _objIndex = new ObjectRectIndex(version>=9);
    PROGRESS;
    
    // load material names
    AutoArray<RString> texName;
    AutoArray<char> major;
    
    int n = f.LoadInt();
    
    texName.Resize(n);
    major.Resize(n);
    
    for (int i=0; i<n; i++)
    {
      f.Transfer(texName[i]);
      f.Transfer(major[i]);
      if (version<17)
        major[i] = -1;
    }
    // first find the sat. segment extents
    int rangeSatX=0,rangeSatY=0;
    for (int i=0; i<n; i++)
    {
      // parse material name to determine coordinates + number of layers
      int satX,satY;
      int unused;
      int layers = ParseTerrainMaterial(GetFilenameExt(texName[i]),satX,satY,unused);
      if (layers<0)
        continue;
      if (rangeSatX<satX+1)
        rangeSatX = satX+1;
      if (rangeSatY<satY+1)
        rangeSatY = satY+1;
    }
    // for each sat. segment find the layer count
    Array2D<int> layerCount;
    Array2D<RString> fullName;
    // index of the slot containing the full name
    Array2D<int> fullIndex;
    layerCount.Dim(rangeSatX,rangeSatY);
    fullIndex.Dim(rangeSatX,rangeSatY);
    // mapping from partial to full tex. info
    AutoArray<int> mapTexInfo;
    mapTexInfo.Resize(n);
    for (int y=0; y<rangeSatY; y++) for (int x=0; x<rangeSatX; x++)
    {
      layerCount(x,y) = 0;
      fullIndex(x,y) = -1;
    }
    for (int i=0; i<n; i++)
    {
      int satX,satY;
      int unused;
      int layers = ParseTerrainMaterial(GetFilenameExt(texName[i]),satX,satY,unused);
      if (layers<0) continue;
      if (unused==0)
        fullIndex(satX,satY) = i;
      if (layerCount(satX,satY)>0)
      {
        Assert(layerCount(satX,satY) == layers);
        saturateMax(layerCount(satX,satY), layers);
      }
      else
        layerCount(satX,satY) = layers;
    }
    
    // when max. version is available, expand to it
    for (int i=0; i<n; i++)
    {
      mapTexInfo[i] = i;
#if 1
      // parse material name to determine coordinates + number of layers
      int satX,satY;
      int unused;
      int layers = ParseTerrainMaterial(GetFilenameExt(texName[i]),satX,satY,unused);
      if (layers<0)
        continue;
      if (fullIndex(satX,satY)>=0)
        mapTexInfo[i] = fullIndex(satX,satY);
#endif
    }
    
#if 1
    // place request for pre-loading all materials
    for (int i=0; i<n; i++)
    {
      // assert each max. material is self-referenced
      // this makes sure followind preload loads all data
      Assert(mapTexInfo[mapTexInfo[i]]==mapTexInfo[i]);
      if (i==mapTexInfo[i])
        PreloadTextureInfo(texName[mapTexInfo[i]]);
    }
    // make sure all requests are executed - file server will order them as needed
    GFileServerSubmitRequestsMinimizeSeek();
#endif
    
    _terrain->_texture.Resize(n);
    for (int i=0; i<_terrain->_texture.Size(); i++)
    {
      if (i==mapTexInfo[i])
        // we only set the "texture" (material) when the texture is used
        SetTexture(i,texName[mapTexInfo[i]],major[i]);
      // unused materials are not referenced, therefore we do not care about them
      PROGRESS;
    }

    for (int z=0; z<_terrain->_tex.GetYRange(); z++) for (int x=0; x<_terrain->_tex.GetXRange(); x++)
    {
      int texIndex = _terrain->_tex.Get(x, z);
      if (texIndex<0 || texIndex>=_terrain->_texture.Size())
      {
        RptF("Bad land texture on %d,%d",x,z);
        _terrain->_tex.Set(x,z,0);
      }
      else
        // use only the "real" materials
        _terrain->_tex.Set(x,z,mapTexInfo[texIndex]);
    }
    
    if (cls)
    {
      _terrain->_outsideTextureInfo._mat = LoadOutsideMaterial(*cls,_terrain->_texture[_terrain->_tex.Get(0,0)]._mat,_terrain->_terSynth);
    }
    else
    {
      _terrain->_outsideTextureInfo._mat = NULL;
      _terrain->_terSynth = false;
    }

    PROGRESS;
    _objIndex->TransferNames(f);
    PROGRESS;

    AutoArray<StaticEntityInfo> entityInfo;
    if (_fileVersion>=15)
    {
      f.TransferBasicArray(entityInfo);
      PROGRESS;
    }
    
    //LogF("Start objects %d",f.GetRest());
    // request all object files to be loaded
    if (objectIndex)
    {
      // pre-quadtree streaming formats no longer supported
      
      Array2D<char> persistent;
      persistent.Dim(GetLandRange(),GetLandRange());

      // load offset table
      _objIndex->_offset.Dim(GetLandRange(), GetLandRange());
      LoadContentBinary(f, _objIndex->_offset);
      _objIndex->_endOffset = f.LoadInt();
      ProgressRefresh();

      // load map object index
      if (mapObjectIndex)
      {
        _mapObjIndex = new MapObjectRectIndex(version>=9);
        _mapObjIndex->_offset.Dim(GetLandRange(), GetLandRange());
        LoadContentBinary(f, _mapObjIndex->_offset);
        _mapObjIndex->_endOffset = f.LoadInt();
      }

      f.LoadCompressed(persistent.RawData(),persistent.RawSize());
      ProgressRefresh();

      if (_fileVersion>=13)
      {
        _subdivHintsOffset = f.TellG();
        f.SkipBinaryCompressed(sizeof(SubdivHints)*GetTerrainRange()*GetTerrainRange());
        // TODO: do not store SubdivHints any more
      }
      // load max. id
      _objectIdPrimary = VisitorObjId(f.LoadInt());
      _objectId = _objectIdPrimary;

      // add current stream position to the index
      int roadNetSize = version >= 7 ? f.LoadInt() : 0;
      int objStart = f.TellG() + roadNetSize;

      _objIndex->_offset.ForEachItem(QuadTreeAdjustOffset(objStart));
      _objIndex->_endOffset += objStart;
      DoAssert(_objIndex->CheckIntegrity());

      if (mapObjectIndex)
      {
        int mapObjStart = _objIndex->_endOffset;
        _mapObjIndex->_offset.ForEachItem(QuadTreeAdjustOffset(mapObjStart));
        _mapObjIndex->_endOffset += mapObjStart;
        DoAssert(_mapObjIndex->CheckIntegrity());
      }

      _roadNet.Free();
      _roadNet = new RoadNet;
      FreeOnDemandGarbageCollect(1024*1024,256*1024);
      ProgressRefresh();
      _roadNet->SerializeBin(f,this);
      ProgressRefresh();

#if _HELISIM_AMBIENT_TRAFFIC
      // ambient roads are not serialized, load them anew
      _ambientRoadNetwork.Free();
      _ambientRoadNetwork = new AmbientRoadNetwork();
      _ambientRoadNetwork->Load(Pars >> "CfgWorlds" >> Glob.header.worldname);
#endif

      // use quadtree order
      int maxOffset = _objIndex->_endOffset;
      for (QuadTreeExRoot<int>::Iterator it=_objIndex->_offset; it; ++it)
      {
        int x = it.x, z = it.y;
        int validReasons =
          (
          //PersistentRoad|
          //PersistentHouse|
          PersistentSimulation|PersistentSound
          );
        // check if stored persistence reason is important for us
        int reasons = persistent(x,z);
        if (reasons&validReasons)
        {
          FreeOnDemandGarbageCollect(1024*1024,256*1024);
          // when we report an exact problem, no need to output the whole list
          bool problemIdentified = false;
          // check types first - full slot loading not necessary if no type simulation or sound
          if (CheckPersistence(x,z))
          {
            // load the slot now
            PreloadResult result;
            do
            {
              ProgressRefresh();
              GFileServerUpdate();
              GTimeManager.Frame(0.1);
/*
              LogF("*** PreloadObjects ***");
              DWORD start = GlobalTickCount();
*/
              // we are waiting for the result
              result = PreloadObjects(FileRequestPriority(1),x,z);
/*
              DWORD time = GlobalTickCount() - start;
              if (time >= 200)
              {
                LogF("*** PreloadObjects duration %d ms ***", time);
              }
*/
            } while (result == PreloadRequested);
            ProgressRefresh();
            
            const ObjectListUsed &list = UseObjects(x,z);
            ProgressRefresh();
            // check if some object really requires persistence
            bool persistenceRequired = false;
            for (int i=0; i<list.Size(); i++)
            {
              const Object *obj = list[i];
              const EntityType *type = obj->GetEntityType();
              if (type && type->IsPersistenceRequired())
              {
                persistenceRequired = true;
                break;
              }
              
              // type does not indicate persistence
#if _ENABLE_REPORT
                int persistenceBinarize = obj->BinarizeCheckPersistance();
                if (persistenceBinarize&(PersistentSound|PersistentSimulation))
                {
                  RStringB simClass = obj->GetShape() ? obj->GetShape()->GetPropertyClass() : RStringB();
                  // report the conflict
                  // but type indicates none
                  RptF("Warning: %s: Binarized guessed persistence wrong, class %s",cc_cast(obj->GetDebugName()),cc_cast(simClass));
                  problemIdentified = true;
                }
#endif
            }
            if (persistenceRequired)
            {
              ObjectListFull *listFull = list.GetList();
              if (listFull)
                listFull->AddRef();
            }
#if _ENABLE_REPORT
            else
            {
              if (!problemIdentified)
              {
                RptF("Late no persistence detection, binarize reasons %d -%s",reasons,cc_cast(PersintenceReasons(reasons)));
                // detect which object have different persistence requirements during binarize / in-game
                for (int i=0; i<list.Size(); i++)
                {
                  Object *obj = list[i];
                  RptF("%s, class '%s'",cc_cast(obj->GetDebugName()),obj->GetShape() ? cc_cast(obj->GetShape()->GetPropertyClass()) : "");
                }
              }
            }
#endif
          }
          ProgressRefresh();

          // scan beg/end for the current slot
          int beg,end;
          _objIndex->GetBegEndOffsets(x,z,beg,end);
          int rest = maxOffset - end;
          ProgressAdvance(lastRest - rest);
          lastRest = rest;
          ProgressRefresh();
        }
      }
      // we have finished loading persistent objects
      // if any more persistent object is loaded, it is a bug
      _persistentLoaded = true;

      // if slot is needed, it will be auto-loaded
    }
    else
    {
      // old OFP CWC format
      int maxId = -1;
      for(;;)
      {
        Matrix4 trans;
        int id = f.LoadInt();
        if (id<0)
          break;

        int nameIndex = f.LoadInt();
        const RStringB &name = _objIndex->GetObjectName(nameIndex);
        f.Transfer(trans);
        //Object *obj =
        if (maxId<id) maxId = id;
        ObjectCreate(id,name,trans);
        PROGRESS;
      }
      _objectId = _objectIdPrimary = VisitorObjId(maxId);

      // scan max. id
      FixDoubleIds();
      _objIndex.Free();
      _roadNet = new RoadNet;
      _roadNet->Build(this);

#if _HELISIM_AMBIENT_TRAFFIC
      // ambient roads are not serialized, load them anew
      _ambientRoadNetwork = new AmbientRoadNetwork;
      _ambientRoadNetwork->Load(Pars >> "CfgWorlds" >> Glob.header.worldname);
#endif
    }

    if (_objIndex && !_objIndex->IsLoaded())
      _objIndex.Free();

    if (!mapObjectIndex)
      InitMapObjects();

    // convert entity info to static entity list
    
    _buildings.Realloc(entityInfo.Size());
    _buildings.Resize0();
    _buildingsOffTree.Dim(0,0,GetLandRange() * GetLandGrid() + 2 * GetLandGrid());

    for (int i=0; i<entityInfo.Size(); i++)
    {
      const StaticEntityInfo &info = entityInfo[i];
      Ref<const EntityType> type = VehicleTypes.New(info.className);
      if (!type) continue;
      const EntityAIType *typeAI = dynamic_cast<const EntityAIType*>(type.GetRef());
      if (!typeAI)
      {
        RptF("Non-AI type %s",cc_cast(type->GetName()));
        continue;
      }
      if (!typeAI->IsPersistenceRequired())
        _buildings.Insert(info.id,typeAI,info.pos);
    }
    _buildings.Compact();

    for (int i = 0; i< _buildings.Size(); i++)
    {
      const StaticEntityLink building = _buildings.GetInfo(i);
      AddBuildingOffTree(
        BuildingsInfo(
        building._pos.X()-GetLandGrid(),building._pos.Z()-GetLandGrid(),building._pos.X()+GetLandGrid(),building._pos.Z()+GetLandGrid(), _buildings.Get(i)));
    }

    BuildFeatureList();

    // note: if grass map is not loaded, it will be created empty
    InitWaterAndGrassMap(false);
    // TODO: serialize _outsideData instead of initializing them
    _terrain->InitOutsideTerrain();
    UpdateGrassMap();
    InitCache(true);
    ProgressAdvance(lastRest);
    ProgressRefresh();
    ProgressFinish(p);
  }
  else
  {
    // save texture info
    f.SaveInt(_terrain->_texture.Size());
    for (int i=0; i<_terrain->_texture.Size(); i++)
    {
      const TexMaterial *txt = _terrain->_texture[i]._mat;
      RStringB name = txt ? cc_cast(txt->GetName()) : "";
      char major = 0;
      f.Transfer(name);
      f.Transfer(major);
    }
    // transfer object lists
    // consider two pass - object name list and index into this list
    FindArray<RStringB> objectNames;
    AutoArray<StaticEntityInfo> entityInfo;
    for (int x=0;x<GetLandRange();x++) for (int z=0;z<GetLandRange();z++)
    {
      const ObjectListUsed &list =UseObjects(x,z);
      for (int i=0; i<list.Size(); i++)
      {
        Object *obj = list[i];
        if( obj->GetType()!=Primary && obj->GetType()!=Network 
#if _ARMA3_TERRAIN_DECALS
          && obj->GetType()!=LandDecal 
#endif           
          ) continue;
        RStringB name = obj->GetShape()->GetName();
        objectNames.AddUnique(name);
        // create corresponding entity info
        // check if it is entity or plain object
        // check is done based on config
        RStringB className = GetEntityClassName(name);
        RStringB classProp = obj->GetShape()->GetPropertyClass();
        if (className.GetLength()>0)
        {
          if (classProp.GetLength()<=0)
          {
            RptF("Class %s exists, but there is no property class in %s",cc_cast(className),cc_cast(obj->GetShape()->GetName()));
            continue;
          }
          if (classProp==RSB(bushsoft) || classProp==RSB(bushhard) || classProp==RSB(treesoft) || classProp==RSB(treehard))
            continue;
          StaticEntityInfo &info = entityInfo.Append();
          info.className = className;
          info.shapeName = name;
          info.id = obj->GetObjectId();
          info.pos = obj->FutureVisualState().Position();
        }
        else if (classProp==RSB(house) || classProp==RSB(church) || classProp==RSB(vehicle) || classProp==RSB(tower))
          // no config class was found, but shape is marked as such
          RptF("%s: %s, config class '%s' missing",cc_cast(name),(const char *)classProp,cc_cast(LandscapeObjectName(name)));
      }
    }
    
    f.TransferBasicArray(objectNames);
    f.TransferBasicArray(entityInfo);
    
    QOStrStream tempQM;
    SerializeBinStream fMapTemp(&tempQM);

    QuadTreeExRoot<int> mapOffsets(GetLandRange(), GetLandRange(), -1);
    QuadTreeExRoot<int> offsets(GetLandRange(), GetLandRange(), -1);
    Array2D<char> persistent;
    persistent.Dim(GetLandRange(),GetLandRange());
    _objectIdPrimary = VisitorObjId(0);
    
    int index; 
    // create and save quadtree structure of map objects but do not serialize it.
    SaveMapObjects(NULL,offsets,objectNames,&persistent,&index);
    
    {
      // create map offset table
      QuadTreeExRoot<int>::Iterator it(mapOffsets);
      while (it)
      {
        int x = it.x, z = it.y;
        mapOffsets.Set(x, z, fMapTemp.TellP());
        const MapObjectListUsed &listU=UseMapObjects(x,z);
        const MapObjectListFull *list=listU.GetList();
        // save index slot
        if (list) for (int i=0; i<list->Size(); i++)
        {
          MapObject *obj = list->Get(i);
          obj->SerializeBin(fMapTemp);
        }
        ++it;
      }
    }
    // save offset table
    SaveContentBinary(f, offsets);
    f.SaveInt(index); // save end of list offset

    SaveContentBinary(f, mapOffsets);
    f.SaveInt(fMapTemp.TellP()); // save end of list offset

    // save persistence table
    f.SaveCompressed(persistent.RawData(),persistent.RawSize());

    // create a subdivision hints and store them in a file
    {
      float minY = 0;
      float minSlope = 0.02;

      if (cls)
      {
        ParamEntryVal subdiv = (*cls)>>"Subdivision";
        minSlope = subdiv>>"minSlope";
        minY = subdiv>>"minY";
      }
      Array2D<SubdivHints> subdivHints;
      subdivHints.Dim(GetTerrainRange(),GetTerrainRange());
      memset(subdivHints.RawData(),0,subdivHints.RawSize());
      f.SaveCompressed(subdivHints.RawData(),subdivHints.RawSize());
    }

    f.SaveInt(_objectIdPrimary);

    QOStrStream tempQRoad;
    SerializeBinStream fRoadTemp(&tempQRoad);
    // save roadnet
    _roadNet->SerializeBin(fRoadTemp,this);
    f.SaveInt(fRoadTemp.TellP()); // save roadnet size (so that it can be skipped)
    f.Append(tempQRoad);

#if _HELISIM_AMBIENT_TRAFFIC
    // ambient roads are not serialized
#endif

    // save object and map streams
    SaveMapObjects(&f,offsets,objectNames,&persistent);
    f.Append(tempQM);
    //f.SaveInt(-1); // terminator
  }

#if _VBS3_LARGE_OBJECTS_WRP
  //// large static objects R-tree //////////////
  if (version >= 25)
  {
    if (f.IsLoading())
    {
      _largeStaticObjects.Load(f);
    }
    else
    {
      _largeStaticObjects.Save(f);
      _largeStaticObjects.RemoveAll();
    }
  }
  PROGRESS;
#endif

  Assert(f.CheckIntegrity());
  
  if (f.IsLoading() && !SupportNonstreaming() && _objIndex.IsNull())
  {
    RptF("Nonstreaming landscape file %s",(const char *)_name);
    Fail("Only streaming landscape supported");
  }
}

// save objects to file and create offset table
void Landscape::SaveMapObjects(SerializeBinStream *f, QuadTreeExRoot<int> &offsets, const FindArray<RStringB> &objectNames, Array2D<char> *persistent, int *index)
{
  if (index) (*index) = 0;
  QuadTreeExRoot<int>::Iterator it(offsets);
  while (it)
  {
    int x = it.x;
    int z = it.y;
    if (index) offsets.Set(x, z, *index);
    const ObjectListUsed &list = UseObjects(x,z);
    // save index slot
    for (int i=0; i<list.Size(); i++)
    {
      Object *obj = list[i];
      if( obj->GetType()!=Primary && obj->GetType()!=Network 
#if _ARMA3_TERRAIN_DECALS
        && obj->GetType()!=LandDecal 
#endif 
        ) continue;
      RStringB name = obj->GetShape()->GetName();
      Matrix4 pos = obj->FutureVisualState().Transform();

      float surfY = SurfaceY(pos.Position().X(),pos.Position().Z());
      if (pos.Position().Y()>surfY+500 || pos.Position().Y()<surfY-100)
        RptF("%d:%s - Bad position %g,%g,%g (surface %.1f)",obj->ID().Encode(),cc_cast(name),
        pos.Position().X(),pos.Position().Y(),pos.Position().Z(),surfY);
      // matrix can be skewed de-skew it...
#if INTERESTED_IN_ID
      if (obj->ID()==INTERESTED_IN_ID)
        LogF("Obj %s: pre de-skew pos %g,%g,%g",cc_cast(obj->GetDebugName()),obj->Position()[0],obj->Position()[1],obj->Position()[2]);
#endif
      float relHeight;
      obj->DeSkew(this, pos, relHeight);
#if INTERESTED_IN_ID
      if (obj->ID()==INTERESTED_IN_ID)
        LogF("Obj %s: post de-skew pos %g,%g,%g",cc_cast(obj->GetDebugName()),obj->Position()[0],obj->Position()[1],obj->Position()[2]);
#endif
      Assert(obj->ID()>=0);
      if (f)
      {
        f->SaveInt(obj->ID());
        f->SaveInt(objectNames.Find(name));
        f->Transfer(pos);
        f->SaveInt(obj->GetShape()->Remarks());
      }
      if (index)
        (*index) += sizeof(obj->ID())+sizeof(objectNames.Find(name))+sizeof(pos)+sizeof(obj->GetShape()->Remarks());

      VisitorObjId id = obj->ID();
      if (_objectIdPrimary<id)
        _objectIdPrimary = id;
    }

    // check if there is any reason why slot should be persistent
    if (persistent)
    {
      int persistenceReason = 0;
      for (int i=0; i<list.Size(); i++)
      {
        Object *obj = list[i];
        if (!obj->GetShape()) continue;
        int persistenceReasonObj = obj->BinarizeCheckPersistance();
  #if 0 //_ENABLE_REPORT
        if (persistenceReasonObj)
          LogF("Persistent object %s - %d",(const char *)obj->GetDebugName(),persistenceReasonObj);
  #endif
        persistenceReason |= persistenceReasonObj;
      }

      (*persistent)(x,z) = persistenceReason;
    }
    ++it;
  }
}

void Landscape::SerializeBinFP1(SerializeBinStream &f, const ParamEntry *cls)
{
  Fail("No longer supported");
  //DebugExceptionTrap :: LogFFF("test",true);

  Ref<ProgressHandle> p;
  //float lastRest = 0;
  if (f.IsLoading())
  {
    LogF("Old serialization cann't be used for loading");
    return;
  }

  Log("Begin landscape load (%d KB).",MemoryUsed()/1024);
#ifdef _MSC_VER
  if (!f.Version('WRPO'))
#else
  if (!f.Version(StrToInt("OPRW")))
#endif
  {
    f.SetError(f.EBadFileType);
    ProgressFinish(p);
    return;
  }
  
  int version = 3;
  f.Transfer(version);
  // version 3 - OFP Retail binarized landscape (no streaming, no map)
  // version 5 - OFP XBox binarized landscape beta (streaming, no map)
  // version 6 - OFP XBox binarized landscape (streaming and map)
  // version 7 - OFP XBox binarized landscape, including roads (streaming and map)
  // version 10 - OFP XBox binarized landscape, quad trees 
  // version 11 - OFP XBox binarized landscape, changed geography
  // version 12 - OFP Xbox/FP2 binarized landscape, different grid for textures and terrain
  // version 13 - OFP Xbox/FP2 binarized landscape, subdivision hints included
  // version 14 - OFP Xbox/FP2 binarized landscape, skew object flag added
  
  int lx = GetLandRange();
  int ly = GetLandRange();
  int rx = GetLandRange(); // terrain x,y
  int ry = GetLandRange();
  f.SaveInt(lx);
  f.SaveInt(ly);
  f.SaveInt(rx);
  f.SaveInt(ry);        

  // transfer all relevant data
  
  Array2D<GeographyInfo> geogr32;
  geogr32.Dim(_terrain->_geography.GetXRange(),_terrain->_geography.GetYRange());
  for (int z=0; z<geogr32.GetYRange(); z++)
   for (int x=0; x<geogr32.GetXRange(); x++)
   {
     geogr32(x, z).packed = _terrain->_geography(x, z);     
     geogr32(x, z).FromFormat7To3();
   }

  f.TransferBinaryCompressed(geogr32.RawData(),geogr32.RawSize());
  
  Array2D<byte> soundMap;
  soundMap.Dim(_soundMap.GetXRange(),_soundMap.GetYRange());
  for (int z=0; z<soundMap.GetYRange(); z++)
    for (int x=0; x<soundMap.GetXRange(); x++)
      soundMap(x, z) = _soundMap(x, z);
  f.TransferBinaryCompressed(soundMap.RawData(),soundMap.RawSize());
 
#if USE_LANDSCAPE_LOCATIONS
  {
    // extract mountains from _locations
    LocationCollectMountainPositions action;
    _locations.ForEachInRegion(LocationAll, action);
    QSort(action._mountains.Data(), action._mountains.Size(), CmpMountains);
    f.TransferBasicArray(action._mountains);
  }
#else
  f.TransferBasicArray(_mountains);
#endif

  // f.TransferBinaryCompressed(_tex.RawData(),_tex.RawSize());
 
  Array2D<short> tex;
  int xRange = _terrain->_tex.GetXRange();
  int yRange = _terrain->_tex.GetYRange();
  tex.Dim(xRange, yRange);
  for (int z=0; z<yRange; z++) for (int x=0; x<xRange; x++)
    tex(x, z) = _terrain->_tex(x, z);

  f.TransferBinaryCompressed(tex.RawData(), tex.RawSize());

  Array2D<RandomInfo> random;
  random.Dim(_terrain->_tex.GetXRange(),_terrain->_tex.GetYRange());
  
  for (int z=0; z<_terrain->_data.GetXRange(); z++) for (int x=0; x<_terrain->_data.GetXRange(); x++)
    // no randomization supported any more - we have to save without it
    random(x,z).packed = 0;

  f.TransferBinaryCompressed(random.RawData(),random.RawSize());
  
  Array2D<float> terrain;
  terrain.Dim(_terrain->_data.GetXRange(),_terrain->_data.GetYRange());
  for (int z=0; z<_terrain->_data.GetXRange(); z++) for (int x=0; x<_terrain->_data.GetXRange(); x++)
    terrain(x,z) = RawToHeight(_terrain->_data(x,z));
  f.TransferBinaryCompressed(terrain.RawData(),terrain.RawSize());
  
  // save texture info
  f.SaveInt(_terrain->_texture.Size());
  for (int i=0; i<_terrain->_texture.Size(); i++)
  {
    const TexMaterial *txt = _terrain->_texture[i]._mat;
    RStringB name = txt ? cc_cast(txt->GetName()) : "";
    bool enableUV = true;
    f.Transfer(name);
    f.Transfer(enableUV);
  }
    // transfer object lists
  // consider two pass - object name list and index into this list
  FindArray<RStringB> objectNames;
  for (int x=0;x<GetLandRange();x++) for (int z=0;z<GetLandRange();z++)
  {
    const ObjectListUsed &list = UseObjects(x,z);
    for (int i=0; i<list.Size(); i++)
    {
      Object *obj = list[i];
      if (obj->GetType()!=Primary && obj->GetType()!=Network
#if _ARMA3_TERRAIN_DECALS
        && obj->GetType()!=LandDecal 
#endif         
        )
        continue;
      RStringB name = obj->GetShape()->GetName();
      objectNames.AddUnique(name);
    }
  }
  f.TransferBasicArray(objectNames);

  for (int x=0;x<GetLandRange();x++) for (int z=0;z<GetLandRange();z++)
  {
    const ObjectListUsed &list=UseObjects(x,z);
    for (int i=0; i<list.Size(); i++)
    {
      Object *obj = list[i];
      if (obj->GetType()!=Primary && obj->GetType()!=Network
#if _ARMA3_TERRAIN_DECALS
        && obj->GetType()!=LandDecal 
#endif         
        )
        continue;
      RStringB name = obj->GetShape()->GetName();
      Matrix4 pos = obj->FutureVisualState().Transform();
      f.SaveInt(obj->ID());
      f.SaveInt(objectNames.Find(name));
      f.Transfer(pos);
    }
  }
  f.SaveInt(-1); // terminator
  Assert(f.CheckIntegrity());
}



bool Landscape::LoadOptimized(QIStream &f, ParamEntryPar cls)
{
  SerializeBinStream str(&f);
  SerializeBin(str, &cls);
  switch (str.GetError())
  {
    case SerializeBinStream::EBadFileType:
      return false;
  }
  return true;
}

void Landscape::SaveOptimized(QOStream &f)
{
  SerializeBinStream str(&f);
  SerializeBin(str, NULL);
}

void Landscape::SaveOptimized(const char *name)
{
  // optional - just to make sure all is converted well
  QOFStream f;
  f.open(name);
  SaveOptimized(f);
  f.close();
}

void Landscape::SaveOptimizedFP1(QOStream &f)
{
  SerializeBinStream str(&f);
  SerializeBinFP1(str, NULL);
}

void Landscape::SaveOptimizedFP1(const char *name)
{
  // optional - just to make sure all is converted well
  QOFStream f;
  f.open(name);
  SaveOptimizedFP1(f);
  f.close();
}

// load/save current status (no terrain/object data save here)
LSError Landscape::Serialize(ParamArchive &ar)
{
  CHECK(_objectId.Serialize(ar, "lastObjectID", 1))
  // TODO: check this construction
  if (ar.IsSaving())
  {
    RefArray<Object> objects;
    for (int x=0; x<GetLandRange(); x++)
      for(int z=0; z<GetLandRange(); z++)
      {
        // TODOINDEX: do not release non-default state objects
        const ObjectList &list = _objects(x, z);
        for (int o=0; o<list.Size(); o++)
        {
          Object *obj = list[o];
          // do not save temporaries
          if (obj->GetType() != Primary && obj->GetType() != Network
#if _ARMA3_TERRAIN_DECALS
            && obj->GetType()!=LandDecal 
#endif             
            ) continue;
          Assert(obj->GetShape());
          Assert(obj->GetShape()->Name());
          if (obj->ID() == 0)
          {
#if _HELISIM_LARGE_OBJECTID
            Log("Object id %I64d %s", obj->ID().Encode(), (const char *)obj->GetShape()->Name());
#else
            Log("Object id %d %s", obj->ID().Encode(), (const char *)obj->GetShape()->Name());
#endif
          }
          if (!obj->MustBeSaved())
            continue; // save only non-default states
          objects.Add(obj);
        }
      }
    CHECK(ar.Serialize("Objects", objects, 1))
  }
  else if (ar.GetPass() == ParamArchive::PassSecond)
  {
    ParamArchive arCls;
    if (!ar.OpenSubclass("Objects", arCls)) return LSOK; // no objects saved
    arCls.FirstPass();
    int n;
    CHECK(arCls.Serialize("items", n, 1))
    for (int i=0; i<n; i++)
    {
      char buffer[256];
      sprintf(buffer, "Item%d", i);
      ParamArchive arRef;
      if (!arCls.OpenSubclass(buffer, arRef))
        continue;
      Object *obj = Object::CreateObject(arRef);  // Find object in landscape
      if (!obj)
        continue;
      arRef.SecondPass();
      CHECK(obj->Serialize(arRef))
      arCls.CloseSubclass(arRef);
    }
  }
  else
  {
    RefArray<Object> objects;
    CHECK(ar.Serialize("Objects", objects, 1))
  }
  CHECK(SerializeWeather(ar));
  return LSOK;
}

bool Landscape::CheckObjectStructure() const
{
#if DO_LINK_DIAGS
/*
  int x,z;
  for( x=0; x<GetLandRange(); x++ ) for( z=0; z<GetLandRange(); z++ )
  {
    const ObjectList &list=_objects(x,z);
    for( int o=0; o<list.Size(); o++ )
    {
      Object *obj=list[o];
      bool ok=obj->VerifyStructure();
      Assert( ok );
      if( !ok ) return false;
    }
  }
  Log("Landscape::CheckObjectStructure OK");
*/
#endif
  return true;
}

#if _VBS3_SHAPEFILES
  #include "hla/ShapeLayers.hpp"
#endif

void Landscape::ResetState()
{
#if _VBS3 
  // Restore deformed ground heights. This must be done prior restoring the objects, as their slope must be set according to reseted ground
  RestoreOldHeightLevels();

  //reseting Water Level
  SetSeaLevelOffset(GWorld->GetWaterLevel());
#endif

#if _VBS3_SHAPEFILES
  GShapeLayers.Clear();
#endif

 // we need to force clutter forgetting its state
  FlushGroundClutterCache();
  _preloadState->Reset();
  Assert(VerifyStructure());
  // repair all objects, remove any non-primaries
  for( int x=0; x<GetLandRange(); x++ ) for( int z=0; z<GetLandRange(); z++ )
  {
    const ObjectList &list=_objects(x,z);
    int maxRetry = 10000;
Retry:
    // Copy the list of objects into working array
    RefArray<Object, MemAllocLocal<Ref<Object>,256> > listCopy;
    listCopy.Realloc(list.Size());
    listCopy.Resize(list.Size());
    for (int i = 0; i < list.Size(); i++)
      listCopy[i] = list[i];

    if (--maxRetry<=0)
    {
      Fail("Too much Retry attempts");
    }
    else for( int o=0; o<listCopy.Size(); )
    {
      int oldSize = listCopy.Size();
      Object *obj=listCopy[o];
      if( obj->GetType()!=Primary && obj->GetType()!=Network 
#if _ARMA3_TERRAIN_DECALS
        && obj->GetType()!=LandDecal 
#endif 
        )
      {
        // delete non-primary
        list.GetList()->Delete(o);
        listCopy.Delete(o);
        // the list may be empty now
        if ( oldSize-1 != list.Size() )
        {
#if 1
          Fail("oldSize-1 != list.Size()");
          RptF("Offending nonprimary object %x:%s,%s",
            obj,(const char *)obj->GetDebugName(),obj->GetShape() ? (const char *)obj->GetShape()->Name() : "<null>");
#endif
          goto Retry;
        }
        if (list.Size()==0)
        {
          // all objects were deleted from the list;  if the list contains non-primaries only, it should not be cached
          Assert(!list->IsInList());
        }
        continue;
      }
      // primary object Id and grid location never changes
      obj->ResetStatus(); // full repair
      // note: obj does not move during ResetStatus
      // it always belongs to the same list
      DoAssert(oldSize == list.Size());

      o++;
    }
//    list.Compact();
    if (list.GetList())
    {
      list.GetList()->Compact();
      if (list.CanBeDeleted())
      {
        // list might be cached
        Assert(!list->IsInList());
        _objects.Set(x, z, ObjectList());
      }
    }
  }

#if _VBS3_LARGE_OBJECTS
  // clear the Large Dynamic Objects
  _largeDynamicObjects.RemoveAll();
#endif

#if _ENABLE_REPORT
  for( int x=0; x<GetLandRange(); x++ ) for( int z=0; z<GetLandRange(); z++ )
  {
    const ObjectList &list=_objects(x,z);
    for( int o=0; o<list.Size(); )
    {
      Object *obj=list[o];
      if( obj->GetType()!=Primary && obj->GetType()!=Network 
#if _ARMA3_TERRAIN_DECALS
        && obj->GetType()!=LandDecal 
#endif         
        )
        // delete non-primary
        ErrF("Non-primary object %x:%s,%s present",
          obj,(const char *)obj->GetDebugName(),obj->GetShape() ? (const char *)obj->GetShape()->Name() : "<null>");
      o++;
    }
  }
#endif
  ResetObjectIDs();
  //RebuildIDCache();
  Assert(VerifyStructure());
  // remove any virtual targets to avoid their accumulation
  for (int i=0; i<_buildings.Size(); i++)
    _buildings.GetInfo(i).ClearVirtualTarget();
#if USE_LANDSCAPE_LOCATIONS
  // reset all locations
  _locations.ForEachInRegion(LocationAll, LocationResetState());
#endif
  UpdateGrassMap();

#if _HELISIM_ROTORLIB && !_DUMMY_ROTORLIB
  WorldRTD *rtdWorld = GWorld->GetRTDWorld();
  if (rtdWorld)
  {
    // Set the airport as the local reference for our simulation
    rtdWorld->setLocalCoordinateSystemCenter( 0.0f, 0.0f, 0.0f);
    rtdWorld->SetWindSpeed(Vector3(0,0,0));
    rtdWorld->resetForcegenerators();
  }

#endif
}

void Landscape::OnInitMission() {}

void Landscape::OnTimeSkipped()
{
  // let all objects react to time change
  for( int x=0; x<GetLandRange(); x++ ) for( int z=0; z<GetLandRange(); z++ )
  {
    const ObjectList &list=_objects(x,z);
    for( int o=0; o<list.Size(); o++)
    {
      Object *obj=list[o];
      obj->OnTimeSkipped();
    }
  }
}

void Landscape::OnShadingQualityChanged()
{
  // in reaction to changed shading quality we may want to reset some caches
}

void Landscape::ResetObjectIDs()
{
  Log("ResetObjectIDs");
  // reset id of all vehicles - use with caution
  int x,z;
  for( x=0; x<GetLandRange(); x++ ) for( z=0; z<GetLandRange(); z++ )
  {
    const ObjectList &list=_objects(x,z);
    for( int o=0; o<list.Size(); o++ )
    {
      Object *obj=list[o];
      // never change primary object ID
      if (obj->GetType()!=Primary && obj->GetType()!=Network
#if _ARMA3_TERRAIN_DECALS
        && obj->GetType()!=LandDecal 
#endif 
        )
        obj->SetID(VISITOR_NO_ID);
    }
  }
  SetLastObjectID(_objectIdPrimary);
  // vehicles will have id assigned by world
}
