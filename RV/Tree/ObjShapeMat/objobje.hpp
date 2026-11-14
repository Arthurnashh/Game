#ifndef _OBJOBJE_H
#define _OBJOBJE_H

#include <stdio.h>
#include <fstream>
using namespace std;
//#include <class\quickSort.hpp>
#include <Es/essencePch.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Types/pointers.hpp>
#include "data3d.h"
#include "types3d.hpp"
//#include "../traditionalInterface/FaceT.h"
#include "FaceT.h"

typedef float coor;

typedef DataPointEx PosT;
typedef DataVec VecT;
typedef DataVertex VertexT;

#define OMaxPoints (4*1024)
#define OMaxFaces (2*1024)

#define OBJ_EXT ".p3d"

class ObjectData;


class FaceTraditional: public DataFaceEx
  {
  public:
    
    FaceTraditional() 
	  {
	  //ASSERT(1);
	  }
    FaceTraditional( const DataFaceEx &data )
      :DataFaceEx(data)
        {}
  };

//--------------------------------------------------

TypeIsMovable(FaceTraditional);

typedef enum 
  {SelSet,SelAdd,SelSub,SelAnd} SelMode;

//--------------------------------------------------

template <class AType>
class PointAttrib: public RefCount
  {
  friend class ObjectData;
  
  protected:
    
    ObjectData *_object;
    
    AutoArray<AType> _attrib;
    
  public:
    PointAttrib( ObjectData *object )
      {
      _object=object;
      }
    PointAttrib( const PointAttrib &src );
    PointAttrib( const PointAttrib &src, ObjectData *object );
    
    AType &operator [] ( int i )
      {
      return _attrib[i];
      }
    const AType &operator [] ( int i ) const
      {
      return _attrib[i];
      }
    
    void Validate( int i=-1 );
    
    void Clear();
    void Clear( int i );
    
    void DeletePoint( int index );
    
    void PermuteVertices( const int *permutation );
    
    void Save( ostream &f );
    void Load( istream &f, int nVert );
    
    int N() const;
    
    void SetObject( ObjectData *object ) 
      {_object=object;}
  };

//--------------------------------------------------

template<class AType>
void PointAttrib<AType>::DeletePoint( int index )
  {
  int n=N();
  for( int i=index+1; i<n; i++ ) _attrib[i-1]=_attrib[i];
  }

//--------------------------------------------------

template <class AType>
inline void PointAttrib<AType>::Validate( int i )
  {
  int max=_object->NPoints();
  if( max<i ) max=i;
  int oldSize=_attrib.Size();
  if( oldSize!=max )
    {
    _attrib.Resize(max);
    for( i=oldSize; i<max; i++ ) Clear(i);
    }
  }

//--------------------------------------------------

template <class AType>
inline void PointAttrib<AType>::Clear()
  {
  _attrib.Realloc(_object->NPoints());
  int i,n=N();
  for( i=0; i<n; i++ ) Clear(i);
  }

//--------------------------------------------------

template <class AType>
inline int PointAttrib<AType>::N() const
  {
  return _attrib.Size();
  }

//--------------------------------------------------

template <class AType>
PointAttrib<AType>::PointAttrib( const PointAttrib &src )
  {
  _object=src._object;
  Validate();
  int n=N();
  if( n>src.N() ) n=src.N();
  for( int i=0; i<n; i++ ) _attrib[i]=src._attrib[i];
  }

//--------------------------------------------------

template <class AType>
PointAttrib<AType>::PointAttrib( const PointAttrib &src, ObjectData *obj )
  {
  _object=src._object;
  Validate();
  int n=N();
  if( n>src.N() ) n=src.N();
  for( int i=0; i<n; i++ ) _attrib[i]=src._attrib[i];
  _object=obj;
  }

//--------------------------------------------------

template <class AType>
void PointAttrib<AType>::Save( ostream &f )
  {
  Validate();
  f.write((char *)_attrib.Data(),N()*sizeof(AType));
  }

//--------------------------------------------------

template <class AType>
void PointAttrib<AType>::Load( istream &f, int nPoint )
  {
  Validate(nPoint);
  f.read((char *)_attrib.Data(),nPoint*sizeof(AType));
  }

//--------------------------------------------------

template <class AType>
void PointAttrib<AType>::PermuteVertices( const int *permutation )
  {
  Validate();
  PointAttrib<AType> save=*this;
  int i;
  for( i=0; i<_object->NPoints(); i++ )
    {
    (*this)[permutation[i]]=save[i];
    }
  }

//--------------------------------------------------

class Selection
  {
  friend class ObjectData;
  
  private:
    
    ObjectData *_object;
    
    //byte vert[OMaxPoints];
    //bool norm[OMaxPoints];
    //bool face[OMaxFaces];
    AutoArray<byte> vertSel;
    AutoArray<bool> faceSel;
    
    void Validate();
    void ValidatePoints( int nPoints=-1 );
    void ValidateFaces( int nFaces=-1 );
    
  public:
    Selection( ObjectData *object )
      {
      _object=object;
      Validate();
      }
    Selection( const Selection &src );
    Selection( const Selection &src, ObjectData *object );
    
    void operator += ( const Selection &src );
    void operator -= ( const Selection &src );
    
    float PointWeight( int i ) const 
      {return (float)(vertSel[i]*(1.0/255));}
    void SetPointWeight( int i, float w )
      {
      int iw=toInt(w*255);
      if( iw<0 ) iw=0;else if( iw>255 ) iw=255;
      vertSel[i]=iw;
      }
    
    bool PointSelected( int i ) const 
      {return vertSel[i]!=0;}
    void PointSelect( int i, bool sel=true )
      {
      Validate();
      vertSel[i]=sel ? 255 : 0;
      }
    
    bool FaceSelected( int i ) const 
      {return faceSel[i];}
    void FaceSelect( int i, bool sel=true )
      {
      Validate();
      faceSel[i]=sel;
      }
    
    void Clear();
    bool IsEmpty() const;
    
    void DeletePoint( int index );
    //void DeleteNormal( int index );
    void DeleteFace( int index );
    
    void Save( ostream &f );
    void Load( istream &f, int sizeVert, int sizeFace, int sizeNorm );
    
    void PermuteFaces( const int *permutation );
    void PermuteVertices( const int *permutation );
    
    int NPoints() const;
    int NFaces() const;
    
    void SetObject( ObjectData *object )
      {
      _object=object;
      Validate();
      }
    ObjectData *GetObject() 
      {return _object;}
  };

//--------------------------------------------------

class NamedSelection: public Selection
  {
  private:
    RString _name;
    
  public:
    NamedSelection( ObjectData *object, const char *name )
      :Selection(object),_name(name)
        {
        
        }
    NamedSelection( const Selection &sel, const char *name )
      :Selection(sel),_name(name)
        {
        
        }
    const char *Name() const 
      {return _name;}
    void SetName( const char *name ) 
      {_name=name;}
  };

//--------------------------------------------------

class NamedProperty
  {
  private:
    char _name[64];
    char _value[64];
    
  public:
    NamedProperty( const char *name, const char *value="" )
      {
      strncpy(_name,name,sizeof(_name));
      strncpy(_value,value,sizeof(_value));
      }
    
    const char *Name() const 
      {return _name;}
    const char *Value() const 
      {return _value;}
    void SetName( const char *name ) 
      {strncpy(_name,name,sizeof(_name));}
    void SetValue( const char *value ) 
      {strncpy(_value,value,sizeof(_value));}
  };

//--------------------------------------------------

class EdgeMatrix
  {
  private:
    bool *_data;
    int _nVertices;
    bool &Edge( int i, int j )
      {
      return _data[i*_nVertices+j];
      }
    
  public:
    EdgeMatrix( int nVertices )
      {
      _nVertices=nVertices;
      int size=_nVertices*_nVertices;
      _data=new bool[size];
      //for( int i=0; i<size; i++ ) _data[i]=false;
      memset(_data,0,size);
      }
    ~EdgeMatrix()
      {
      if( _data ) delete[] _data;
      }
    bool operator () ( int i, int j )
      {
      return Edge(i,j);
      }
    void AddEdge( int i, int j )
      {
      Edge(i,j)=true;
      Edge(j,i)=true;
      }
  };

//--------------------------------------------------

#define MAX_NAMED_SEL 512
#define MAX_NAMED_PROP 128

class AnimationPhase: public PointAttrib<Point3D>
  {
  float _time;
  
  public:
    AnimationPhase(  ObjectData *object )
      :PointAttrib<Point3D>(object),_time(0)
        {}
    AnimationPhase(  const AnimationPhase &src )
      :PointAttrib<Point3D>(src),_time(src._time)
        {}
    
    AnimationPhase(  const AnimationPhase &src, ObjectData *object )
      :PointAttrib<Point3D>(src,object),_time(src._time)
        {}
    
    void SetTime( float time ) 
      {_time=time;}
    float GetTime() const  
      {return _time;}
    
    void Redefine( ObjectData *data ); // import external data
  };

//--------------------------------------------------

struct CoordSpace
  {
  int pt[4];
  };

//--------------------------------------------------

struct SharpEdge
  {
  int edge[2];
  int &operator[] ( int i ) 
    {return edge[i];}
  int operator[] ( int i ) const 
    {return edge[i];}
  ClassIsSimple(SharpEdge);
  };

//--------------------------------------------------

#pragma warning (disable : 4355)
class ObjectData
  {
  private:
    friend class Selection;
    friend class LODObject;
    
    // object data
    //PosT points[OMaxPoints];
    //VecT normals[OMaxPoints];
    //FaceT faces[OMaxFaces];
    //int nPoints,nNormals,nFaces;
    AutoArray<PosT> _points;
    AutoArray<VecT> _normals;
    AutoArray<FaceTraditional> _faces;
    
    bool _dirty; // some data was changed and not saved
    bool _normalsDirty; // normals must be recalculated
    
    // object setup (selection, edges...)
    Selection _sel;
    Selection _hide,_lock; // hidden, locked vertices
    NamedSelection *_namedSel[MAX_NAMED_SEL];
    NamedProperty *_namedProp[MAX_NAMED_PROP];
    
    PointAttrib<float> _mass;
    AutoArray< Ref<AnimationPhase> > _phase;
    
    AutoArray<SharpEdge> _sharpEdge;
    
    bool _dirtySetup;
    
    int _autoSaveAnimation; // currently edited data is taken from some animation
    
    
  private:
    // initializers and deinitilizers
    void DoConstruct(); // constructor
    void DoConstructEmpty(); // constructor
    void DoCopy( const ObjectData &src ); // copy constructor
    void DoCopy( const ObjectData &src, int animationPhase ); // copy constructor
    void DoDestruct(); // destructor
    
  public:
    // cannonical interface
    ObjectData()
      :_sel(this),_hide(this),_lock(this),
    _mass(this)
      {
      DoConstruct();
      }
    ~ObjectData()
      {DoDestruct();}
    
  public:
    ObjectData( const ObjectData &src )
      :_sel(this),_hide(this),_lock(this),
    _mass(this)
      {
      DoCopy(src);
      }
    ObjectData( const ObjectData &src, int animationPhase )
      :_sel(this),_hide(this),_lock(this),
    _mass(this)
      {
      DoCopy(src,animationPhase);
      }
    void operator = ( const ObjectData &src )
      {
      DoDestruct();
      DoCopy(src);
      }
    bool Merge( const ObjectData &src ,const char *createSel=NULL);
    
    // access to data 
    void SetDirty()
      {_dirty=true;}
    void ClearDirty()
      {_dirty=false;}
    bool Dirty()
      {return _dirty;}
    
    void SetDirtySetup()
      {_dirtySetup=true;}
    void ClearDirtySetup()
      {_dirtySetup=false;}
    bool DirtySetup()
      {return _dirtySetup;}
    
    int NPoints() const 
      {return _points.Size();}
    const PosT &Point( int i ) const 
      {return _points[i];}
    PosT &Point( int i ) 
      {return _points[i];}
    
    int NNormals() const 
      {return _normals.Size();}
    const VecT &Normal( int i ) const 
      {return _normals[i];}
    VecT &Normal( int i ) 
      {return _normals[i];}
    void ResetNormals() 
      {_normals.Clear();}
    
    int NFaces() const 
      {return _faces.Size();}
    const FaceTraditional &Face( int i ) const 
      {return _faces[i];}
    FaceTraditional &Face( int i ) 
      {return _faces[i];}
    
    bool SaveNamedSel( const char *name );
    bool UseNamedSel( const char *name, SelMode mode=SelSet );
    bool DeleteNamedSel( const char *name );
    bool RenameNamedSel( const char *name, const char *newname );
    const char *NamedSel( int i ) const;
    
    int FindNamedSel( const char *name ) const;
    
    const NamedSelection *GetNamedSel( int i ) const;
    const NamedSelection *GetNamedSel( const char *name ) const;
    NamedSelection *GetNamedSel( int i );
    NamedSelection *GetNamedSel( const char *name );
    
    
    const char *GetNamedProp( const char *name ) const; // NULL if not existing
    bool SetNamedProp( const char *name, const char *value );
    bool DeleteNamedProp( const char *name );
    bool RenameNamedProp( const char *name, const char *newname );
    
    const char *GetNamedProp( int i ) const;
    //int NNamedProps() const;
    
    
    int NAnimations() const;
    AnimationPhase *GetAnimation( int i ) const;
    int AnimationIndex( float time ) const;
    int NearestAnimationIndex( float time ) const;
    bool AddAnimation( const AnimationPhase &src );
    bool RenameAnimation( float oldTime, float newTime );
    bool DeleteAnimation( float time );
    void DeleteAllAnimations();
    bool SortAnimations(); // retain current animation
    void AutoTimeAnimations(bool last1); // rename positive time animations
    
    void HalfRateAnimation();
    void MirrorAnimation();
    
    int CurrentAnimation() const 
      {return _autoSaveAnimation;}
    
    void RedefineAnimation( AnimationPhase &anim );
    void UseAnimation( AnimationPhase &anim );
    
    void SoftenAnimation
      (
        float valueX, float valueY, float valueZ,
    bool actualVsAverage,
    float timeRange, bool interpolate, bool looped
      );
    
    int FindCoordSpaces
      (
        CoordSpace coord[MAX_NAMED_SEL], const AnimationPhase &src0
          ) const;
    
    void AutoAnimation( ObjectData *src );
    void AutoAnimation( const char *file );
    
    Vector3 GetStep(const char *selection) const; // return step length
    float StepLength(const char *selection) const 
      {return GetStep(selection).Z();} // return step length
    float XStepLength(const char *selection) const 
      {return GetStep(selection).X();} // return step length
    
    void RemoveStep( Vector3 vec ); // perform step correction
    
    void RemoveZStep( float len )
      {RemoveStep(Vector3(0,0,len));} // perform step correction
    void RemoveXStep( float len )
      {RemoveStep(Vector3(len,0,0));} // perform step correction
    void NormalizeCenter( bool normX, bool normZ, const char *centername);
    
    void ExportAnimation( const char *file, float step, float xStep ) const;
    // measure step length and perform correction before looped export
    void ExportAnimationLooped( const char *file,const char *stepsel ) const;
    
    void RedefineAnimation( int i );
    void UseAnimation( int i );
    
    WString Description() const;
    
    const Selection *GetSelection() const 
      {return &_sel;}
    const Selection *GetHidden() const 
      {return &_hide;}
    const Selection *GetLocked() const 
      {return &_lock;}
    
    void UseLocked() 
      {_sel=_lock;}
    void UseHidden() 
      {_sel=_hide;}
    
    bool PointSelected( int i ) const 
      {return _sel.PointSelected(i);}
    bool FaceSelected( int i ) const 
      {return _sel.FaceSelected(i);}
    void PointSelect( int i, bool sel=true )
      {_sel.PointSelect(i,sel);}
    void FaceSelect( int i, bool sel=true )
      {_sel.FaceSelect(i,sel);}
    
    int ReservePoints( int count );
    int ReserveFaces( int count );
    
    PosT *NewPoint();
    VecT *NewNormal();
    int NewFace();
    
    int AddNormal( const VecT &norm );
    int FindNormal( const VecT &norm, double maxDist=0.01 );
    int FindEdge( int a, int b ) const;
    int FindOrientedEdge( int a, int b ) const;
    int FindOrientedEdge( int a, int b, bool *searchFaces ) const;
    int FindPoint( const VecT &point);

    void DeletePoint( int index );
    void DeleteFace( int index );
    void DeleteNormal( int index );
    void ReplacePoint( int newIndex, int oldIndex );
    
    void PermuteVertices( const int *permutation );
    
    // normals recalculation
  protected:
    int FindAdjacentFace( bool *scanned, int act, int next ) const;
    void AddAdjacentNormal( Point3D &normal, bool *scanned, Point3D *normals, int act, int next, int prev, int iFace ) const;
    Point3D SmoothAdjacentFaces( Point3D *faceNormals, int act, int prev, int next, int iFace );
    
  public:
    void DoRecalcNormals(); // actual normal recalculation
    void RecalcNormals(); // set normals as dirty
    void CleanNormals(); // recalc if dirty
    
  protected:
    void BuildStrip
      (
        AutoArray<int> &data,
    const DataVertex &d0, const DataVertex &d1, bool even,
    const FaceTraditional *face, // face requirements
    bool *freeFaces
      );
    void BuildFan
      (
        AutoArray<int> &data,
    const DataVertex &d0, const DataVertex &d1,
    const FaceTraditional *face, // face requirements
    bool *freeFaces
      );
    
  public:
    void StripsAndFans();
    
  public:
    // face optimization
    void CheckFaces();
    void IsolatedPoints();
    void CheckMapping();
    void CheckSTVectors();
    
    void BackfaceCull( const Vector3 &pin );
    // result in current selection
    
    void CheckClosedTopology();
    void CloseTopology();
    
    int CalculateInsideFaces( int face, bool onlySelected ) const;
    
    void SelectionToConvexComponents();
    void CheckConvexComponent(); // check selection
    
    void RemoveComponents();
    void CreateComponents();
    void CreateConvexComponents();
    void CheckConvexComponents();
    
    bool FaceCanBeInConvexHull
      (
        int i, int j, int k, bool allPoints
          );
    void CreateConvexHull( bool allVertices=false );
    void ComponentConvexHull();
    
    void Triangulate( bool allFaces=false );
    void Squarize( bool allFaces=false );
    
  protected:
    // internal functions for old (ASCII) formats conversion
    int LoadPointAsc( FILE *f );
    int LoadNormalAsc( FILE *f );
    int LoadCRYNormalAsc( FILE *f );
    int LoadFaceFlatAsc( FILE *f );
    int LoadFaceShadedAsc( FILE *f );
    int LoadFaceCRYShadedAsc( FILE *f );
    
    // load ASCII object
    int LoadShadedAsc( const char *N );
    int LoadBinary( WFilePath filename );
    
    
    int Load3DS( const char *name );
    
  public: // public I/O routines
    // binary format stream operation
    void Optimize(); // remove any unnecessary faces, ....
    
    int SaveData( ostream &f, bool final, int version );
    int SaveSetup( ostream &f, bool final, bool mass, bool nolongtags);
    int LoadData( istream &f );
    int LoadSetup( istream &f );
    
    int LoadFinished();
    
    int LoadBinary( istream &f );
    int SaveBinary( ostream &f, int version, bool final=false, bool mass=true );
    
    int SaveCHeader( ostream &f ); // export data for C compiler
    int SaveCHeader( WFilePath f );
    
    int LoadStars( istream &f ); // star
    int LoadStars( WFilePath name );
    
    int LoadGenAsc( istream &f ); // 3d studio ascii import/export
    int SaveGenAsc( ostream &f );
    int LoadGenAsc( WFilePath name );
    int SaveGenAsc( WFilePath  name );
    
    // autodetect format and load
    int Load( WFilePath filename );
    // save in new (binary) format
    int Save( WFilePath filename, bool final=false, bool mass=true );
    
    // selection operations
  public:
    
    void ClearSelection();
    
    int SingleSelectedFace() const;
    int FirstSelectedFace() const;
    int CountSelectedFaces() const;
    
    int SingleSelectedPoint() const;
    int FirstSelectedPoint() const;
    int CountSelectedPoints() const;
    
    // sharp/smoothed edge operations
    int FindSharpEdge( int a, int b ) const;
    void SortSharpEdges();
    void RemoveSharpEdge( int a, int b );
    void RemoveSharpEdge( int index );
    void RemoveAllSharpEdges();
    void AddSharpEdge( int a, int b );
    bool EnumEdges(int &enm, int &from, int &to);
    
    // autoselection
    void SelectFacesFromPoints( int vertex=-1, SelMode mode=SelSet );
    void SelectPointsFromSelectedFaces( int faceIndex, SelMode mode );
    void UnselectPointsFromUnselectedFaces( int faceIndex, SelMode mode );
    void SelectPointsFromFaces( int face=-1, SelMode mode=SelSet );
    
    void SelectHalfSpace( const FaceT &face );
    
  protected:
    //void PromoteComponent( bool *visited, int point, bool state );
    void PromoteComponent( bool *visited, EdgeMatrix &edges, int point, bool state );
    
  public:
    void SelectComponent( int point, bool state=true );
    
    void HideSelection()  
      {_hide+=_sel;}
    void UnhideSelection() 
      {_hide-=_sel;}
    void LockSelection() 
      {_lock+=_sel;}
    void UnlockSelection() 
      {_lock-=_sel;}
    
    void SetPointMass( int i, double mass ) 
      {_mass[i]=(float)mass;}
    double GetPointMass( int i ) const 
      {return i<_mass.N()?_mass[i]:0;}
    
    void SetSelectionMass( double mass, bool constTotal=false );
    double GetSelectionMass() const;
    double GetTotalMass() const;
    double GetMaxMass() const;
    VecT GetMassCentre(bool selection);
    
    // animation weighting operations
    
    
    // clipboard operations
  public:
    bool LoadClipboard( UINT format );
    bool SaveClipboard( UINT format );
    
    void ExtractSelection();
    
    bool SelectionDelete();
    bool SelectionDeleteFaces();
    bool SelectionCopy( UINT format );
    bool SelectionCut( UINT format );
    bool SelectionPaste( UINT format );
    
    bool SelectionSplit();
    
    // different operations on selection
    bool MergePoints( double limit, bool selected );
    
    
    // general helper function export
    
    static void SaveTag( ostream &f, const char *tag, int size , bool nolongtag);
    static WString LoadTag( istream &f, int &size );
    static bool FindTag( istream &f, const char *name, int &size );
  };

//--------------------------------------------------

template <class T> 
void __swap(T &v1, T& v2)
  {
  T tmp=v1;v1=v2;v2=tmp;
  }

#endif

