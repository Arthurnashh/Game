#include "objshape.h"
#include "wpch.hpp"
#include "ObjLOD.hpp"
#include <fstream>
using namespace std;

// we assume new never returns NULL

void LODObject::DoConstruct()
  {
  _obj[0]=new ObjectData();
  _resolutions[0]=0;
  _n=1;
  _active=0;
  _dirty=false;
  }

//--------------------------------------------------

void LODObject::DoConstructEmpty()
  {
  _n=0;
  _dirty=false;
  _active=0;
  }

//--------------------------------------------------

void LODObject::DoConstruct( const LODObject &src )
  {
  for( int i=0; i<src._n; i++ )
    {
    _obj[i]=new ObjectData(*src._obj[i]);
    _resolutions[i]=src._resolutions[i];
    }
  _active=src._active;
  _n=src._n;
  _dirty=false;
  }

//--------------------------------------------------

void LODObject::DoDestruct()
  {
  for( int i=0; i<_n; i++ ) delete _obj[i],_obj[i]=NULL;
  _n=0;
  }

//--------------------------------------------------

bool LODObject::RemoveLevel( int level )
  {
  if( _n<=1 ) return false;
  if( _obj[level] ) return false;
  for( int i=level+1; i<_n; i++ ) _obj[i-1]=_obj[i],_resolutions[i-1]=_resolutions[i];
  _n--;
  _dirty=true;
  return true;
  }

//--------------------------------------------------

ObjectData *LODObject::DetachLevel(int level) //remove level and returns pointer
  {
  if( _n<=1 ) return NULL;
  ObjectData *ret=_obj[level];
  _obj[level]=NULL;
  if (RemoveLevel(level)==false) return NULL;
  return ret;
  }

//--------------------------------------------------

int LODObject::InsertLevel( ObjectData *obj, float resolution )
  {
  if( _n>=MAX_LOD_LEVELS ) return false;
  int i,j;
  for( i=0; i<_n; i++ ) if( _resolutions[i]>resolution ) break;
  // insert to position i
  for( j=_n; j>i; j-- ) _obj[j]=_obj[j-1],_resolutions[j]=_resolutions[j-1];
  _obj[i]=obj;
  _resolutions[i]=resolution;
  _n++;
  _dirty=true;
  if( _active>i ) _active++;
  return true;
  }

//--------------------------------------------------

bool LODObject::DeleteLevel( int level )
  {
  if( _n<=1 ) return false;
  delete _obj[level];
  _obj[level]=NULL;
  if( _active>=level && _active>0 ) _active--;
  return RemoveLevel(level);
  }

//--------------------------------------------------

int LODObject::AddLevel( const ObjectData &obj, float resolution )
  {
  if( _n>=MAX_LOD_LEVELS ) return false;
  // levels are sorted by increasing resolution
  ObjectData *ins=new ObjectData(obj);
  return InsertLevel(ins,resolution);
  }

//--------------------------------------------------

int LODObject::FindLevel( float resolution ) const
  {
  float minDif=1e20;
  int minI=-1;
  for( int i=0; i<_n; i++ )
    {
    float dif=fabs(resolution-_resolutions[i]);
    if( minDif>dif ) minDif=dif,minI=i;
    }
  return minI;
  }

//--------------------------------------------------

int LODObject::FindLevelExact( float resolution ) const
  {
  float minDif=resolution*0.001;
  int minI=-1;
  for( int i=0; i<_n; i++ )
    {
    float dif=fabs(resolution-_resolutions[i]);
    if( minDif>=dif ) minDif=dif,minI=i;
    }
  return minI;
  }

//--------------------------------------------------

int LODObject::SetResolution( int level, float newRes )
  {
  _dirty=true;
  if( _n>1 )
    {
    ObjectData *levelData=_obj[level];
    // delete on old position
    bool isActive=( level==_active );
    _obj[level]=NULL;
    bool res;
    res=RemoveLevel(level); // always true
    //WVERIFY( res );
    int newLevel=InsertLevel(levelData,newRes); // always true
    //WVERIFY( newLevel>=0 );
    if( isActive ) _active=newLevel;
    return newLevel;
    }
  else
    {
    //WVERIFY( level==0 );
    _resolutions[level]=newRes;
    return level;
    }
  }

//--------------------------------------------------

bool LODObject::ChangeResolution( float oldRes, float newRes )
  {
  int level=FindLevel(oldRes);
  if( level<0 ) return false;
  // change ordering
  return SetResolution(level,newRes)>=0;
  }

//--------------------------------------------------

//#include "release\resource.h"

/*
WString LODObject::LODName( int level ) const
  {
  return LODResolToName(_resolutions[level]);
  }
*/
//--------------------------------------------------

void LODObject::ClearDirty()
  {
  for( int i=0; i<_n; i++ )
    {
    _obj[i]->ClearDirty();
    _obj[i]->ClearDirtySetup();
    }
  _dirty=false;
  }

//--------------------------------------------------

void LODObject::SetDirty()
  {
  _dirty=true;
  }

//--------------------------------------------------

bool LODObject::Dirty()
  {
  for( int i=0; i<_n; i++ )
    {
    if( _obj[i]->Dirty() ) return true;
    if( _obj[i]->DirtySetup() ) return true;
    }
  return _dirty;
  }

//--------------------------------------------------

void LODObject::CleanNormals()
  {
  for( int i=0; i<_n; i++ )
    {
    if( !_obj[i]->_normalsDirty ) continue;
    _obj[i]->CleanNormals();
    }
  }

//--------------------------------------------------

int LODObject::Save( ostream &f, int version, bool final )
  {
  //f.write("NLOD",4);
  f.write("MLOD",4);
  int ver=0x101;
  f.write((char *)&ver,sizeof(ver)); // save format version number
  int n=NLevels();
  f.write((char *)&n,sizeof(n));
  for( int i=0; i<NLevels(); i++ )
    {
    int ret=0;
    // object should be followed by resolution info
    float resol=Resolution(i);
    //bool saveMass=( i==0 );
    // new objects have mass stored in geometry level 
    //bool saveMass=!strcmpi(LODResolToName(resol),"Geometry");
    bool saveMass= (resol == 1e13);
    if( _obj[i]->SaveBinary(f,version,final,saveMass)<0 ) return -1;
    f.write((char *)&resol,sizeof(resol));
    if( f.fail() ) return -1;
    }
  return 0;
  }

//--------------------------------------------------

int LODObject::Save( WFilePath& filename, int version, bool final, SaveCallBackSt callback, void *context )
  {
  WString ext=filename.GetExtension();
  if( ext[0]=='.' && ext[1]=='a' )
    {
    return _obj[0]->SaveGenAsc(filename);
    }
  ofstream f;
  f.open(filename,ios::out|ios::binary|ios::trunc);
  if (f.fail()) return -1;
  if( Save(f,version,final)<0 ) return -1;
  // after everything is saved, save Objektiv configuration
  if( callback )
    {
    if( !callback(f,context) ) return -1;
    }
  // do not clear dirty - save may be export or save for object viewer ...
  //_dirty=false;
  #if 0 // all setup files have been removed
  // remove old setup file
  filename.SetExtension(".set3d");
  remove(filename);
  #endif
  return 0;
  }

//--------------------------------------------------

int LODObject::Load(WFilePath& filename, LoadCallBackSt callback, void *context )
  {
  DoDestruct();
  DoConstructEmpty();
  if( !strcmpi(filename.GetExtension(),".3ds") )
    {
      Assert(FALSE);
      goto Error;
    //ObjectData *obj=new ObjectData();
    //if( obj->Load3DS(filename)<0 ) goto Error;
    //AddLevel(*obj,0.0f);
    //delete obj;
    }
  else if( strcmpi(filename.GetExtension(),OBJ_EXT) )
    {
    // only p3d format can handle LOD levels
    // other formats read only one level
    ObjectData *obj=new ObjectData();
    if( obj->Load(filename)<0 ) goto Error;
    AddLevel(*obj,0.0f);
    delete obj;
    }
  else
    {
    //SetProgress(0);
    ifstream f;
    f.open(filename,ios::in|ios::binary);
    if( f.fail() ) goto Error;
    char magic[4];
    int ver=-1; // version number
    f.read(magic,sizeof(magic));
    if( f.fail() || f.eof() ) return -1;
    int n=MAX_LOD_LEVELS;
    bool isLOD=false;
    if( !strncmp(magic,"NLOD",sizeof(magic)) )
      {
      f.read((char *)&n,sizeof(n));
      isLOD=true;
      ver=0; // no version in NLOD format
      }
    else if( !strncmp(magic,"MLOD",sizeof(magic)) )
      {
      // MLOD format contains version number
      f.read((char *)&ver,sizeof(ver));
      isLOD=true;
      f.read((char *)&n,sizeof(n));
      }
    else f.seekg((size_t)f.tellg()-sizeof(magic)); // not NLOD format rewind
    for( int i=0; i<n; i++ )
      {
      //SetProgress(i*100/n);
      int ret=0;
      ObjectData *obj=new ObjectData();
      // object should be followed by resolution info
      float resol;
      streampos before=f.tellg();
      if( obj->LoadBinary(f)<0 )
        {
        delete obj;
        if( i==0 ) goto Error;
        f.seekg(before);
        break; // last LOD already loaded
        }
      f.read((char *)&resol,sizeof(resol));
      if( f.fail() || f.eof() ) resol=0;
      AddLevel(*obj,resol);
      delete obj;
      if( f.fail() || f.eof() ) break;
      }
    if( !isLOD && _n==1 && ver<0 )
      {
      // try to load setup from old .set3d file
      ifstream setup;
      filename.SetExtension(".set3d");
      setup.open(filename,ios::in|ios::binary);
      if( !f.fail() && !f.eof() )
        {
        _obj[0]->LoadSetup(f);
        }
      }
    if( callback )
      {
      if( !callback(f,context) ) return -1;
      }
    // if there is mass info in LOD 0, convert it to geometry level
    int gIndex=FindLevelExact(1e13);//LODNameToResol("Geometry"));
    if( gIndex>0 && ver==0 )
      { // geometry exists and is different from LOD 0
      bool oldMass=false,newMass=false;
      ObjectData *lod0=Level(0);
      ObjectData *lodG=Level(gIndex);
      if( lodG ) lodG->_mass.Validate(lodG->NPoints());
      int i,j;
      for( i=0; i<lod0->NPoints(); i++ )
        {
        if( lod0->_mass[i]>0 ) oldMass=true;
        }
      if( oldMass )
        {
        for( i=0; i<lodG->NPoints(); i++ )
          {
          lodG->_mass[i]=0;
          }
        // some old mass defined
        // move old mass to new mass and delete old mass information
        // distibute mass information of all lod0 vertices
        for( i=0; i<lodG->NPoints(); i++ )
          {
          if( lodG->_mass[i]>0 ) newMass=true;
          }
        for( i=0; i<lod0->NPoints(); i++ )
          {
          double sumInvDist2=0;
          const Vector3 &pos0=lod0->Point(i);
          for( j=0; j<lodG->NPoints(); j++ )
            {
            const Vector3 &posG=lodG->Point(j);
            float dist2=(posG-pos0).SquareSize();
            //float dist2=(posG-pos0).Size();
            if( dist2<=1e-3 )
              {
              // move whole mass to nearest point
              lodG->_mass[j]+=lod0->_mass[i];
              goto Distibuted;
              }
            sumInvDist2+=1/dist2;
            }
          double mass0divSumInvDist2;
          mass0divSumInvDist2=lod0->_mass[i]/sumInvDist2;
          float sumMass;
          sumMass=0;
          for( j=0; j<lodG->NPoints(); j++ )
            {
            const Vector3 &posG=lodG->Point(j);
            float invDist2=1/(posG-pos0).SquareSize();
            //float invDist2=1/(posG-pos0).Size();
            // distribute between points by distance
            lodG->_mass[j]+=invDist2*mass0divSumInvDist2;
            sumMass+=invDist2*mass0divSumInvDist2;
            }
          Distibuted:
          lod0->_mass[i]=0;
          }
        // reset mass in all lods but geometry
        for( j=1; j<NLevels(); j++ ) if( j!=gIndex )
          {
          ObjectData *lodJ=Level(j);
          for( i=0; i<lodJ->_mass.N(); i++ )
            {
            lodJ->_mass[i]=0;
            }
          }
        }
      
      }
    }
  ClearDirty();
  // after everything is loaded, load Objektiv configuration
  return 0;
  Error:
  DoDestruct();
  DoConstruct();
  return -1;
  }

//--------------------------------------------------

bool LODObject::Merge( const LODObject &src , bool createlods, const char *createSel)
  {
  if (createlods)
    {
    for (int i=0;i<src.NLevels();i++)
      {
      float level=src.Resolution(i);
      int index=FindLevelExact(level);
      if (index==-1) 
        {
        ObjectData obj;
        AddLevel(obj,level);
        }
      }
    }
  
  // merge all LOD levels
  for( int i=0; i<_n; i++ )
    {
    // find nearest LOD level of src
    int index=src.FindLevel(_resolutions[i]);
    if( index>=0 )
      {
      if( !_obj[i]->Merge(*src._obj[index],createSel) ) return false;
      }
    }
  return true;
  }

//--------------------------------------------------

void LODObject::CenterAll()
  {
  Point3D minP(+1e10,+1e10,+1e10);
  Point3D maxP(-1e10,-1e10,-1e10);
  int level;
  for( level=0; level<_n; level++ )
    {
    ObjectData *obj=_obj[level];
    for( int i=0; i<obj->NPoints(); i++ )
      {
      Point3D pos=obj->Point(i);
      CheckMinMax(minP,maxP,pos);
      // scan also all animations
      }
    }
  Point3D offset=(minP+maxP)*0.5;
  for( level=0; level<_n; level++ )
    {
    ObjectData *obj=_obj[level];
    for( int i=0; i<obj->NPoints(); i++ )
      {
      obj->Point(i)-=offset;
      // change also also all animations
      for( int a=0; a<obj->NAnimations(); a++ )
        {
        AnimationPhase *phase=obj->GetAnimation(a);
        (*phase)[i]-=offset;
        }
      }
    }
  }

//--------------------------------------------------

