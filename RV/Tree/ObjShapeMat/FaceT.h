#ifndef __TRADITIONAL_INTERFACE_FACE_T_OBJECTIV_2_
#define __TRADITIONAL_INTERFACE_FACE_T_OBJECTIV_2_


//#include "../optima/optima2mfc.h"
//#include "../optima/data3d.h"
//#include "../optima/types3d.hpp"

#include "optima2mfc.h"
#include "data3d.h"
#include "types3d.hpp"


class IArchive;

class ObjectData;
class FaceTraditional;

class FaceT
  {
    FaceTraditional *_helper;
    ObjectData *_obj;
    int _faceIndex;
  public:

    
    FaceT(ObjectData *obj,int faceIndex):_obj(obj),_faceIndex(faceIndex),_helper(NULL) {}
    FaceT(ObjectData &obj,int faceIndex):_obj(&obj),_faceIndex(faceIndex),_helper(NULL) {}
    FaceT(const ObjectData *obj,int faceIndex):_obj(const_cast<ObjectData *>(obj)),_faceIndex(faceIndex),_helper(NULL) {}
    FaceT(const ObjectData &obj,int faceIndex):_obj(const_cast<ObjectData *>(&obj)),_faceIndex(faceIndex),_helper(NULL) {}
    FaceT(ObjectData *obj):_faceIndex(-1),_helper(0) {CreateUnbound(obj);}
    FaceT(ObjectData &obj):_faceIndex(-1),_helper(0) {CreateUnbound(&obj);}
    FaceT(const ObjectData *obj):_faceIndex(-1),_helper(0) {CreateUnbound(const_cast<ObjectData *>(obj));}
    FaceT(const ObjectData &obj):_faceIndex(-1),_helper(0) {CreateUnbound(const_cast<ObjectData *>(&obj));}
    FaceT(const FaceT &other);
    FaceT():_obj(NULL),_faceIndex(-1),_helper(NULL) {}///<if you are using this constructor, remember to call SetObject soon!
    
    FaceT &operator=(const FaceT &other);
    ~FaceT() {Release();}

    void SetObject(ObjectData *obj) {_obj=obj;}
    void CreateUnbound(ObjectData *obj); ///<Creates an anonymous face
    void CreateIn(ObjectData *obj); ///<Creates new face in object
    void BindTo(ObjectData *obj, int index); ///<Binds face from object. Current face is unbend
    void BindTo(const FaceT &other); ///<Binds face from object. Current face is unbend
	void Release();  ///<releases current face, and object stays unbended
	bool IsValid() const {return (_helper!=NULL || _faceIndex!=-1) && _obj!=NULL;}

//* Direct manipulating with face

    void SetTexture(RString name,int stage=0);
    void SetMaterial(RString name, int stage=0);
    void SetTexture(const char *name, int stage=0) {SetTexture(RString(name),stage);}
    void SetMaterial(const char *name, int stage=0) {SetMaterial(RString(name),stage);}
    void SetN(int n) {SetVxCount(n);}
    void SetVxCount(int n);
    void SetPoint(int vs, int pt);
    void SetNormal(int vs, int nr);
    void SetU(int vs, float tu, int stage=0);
    void SetV(int vs, float tv, int stage=0);
    void SetUV(int vs, float tu, float tv, int stage=0);    
    void SetFlags(unsigned long flags);
    unsigned long SetFlags(unsigned long zerobits, unsigned long invertbits);

    RString GetTexture(int stage=0) const ;
    RString GetMaterial(int stage=0) const ;
    int N() const  {return GetVxCount();}
    int GetVxCount() const ;
    int GetPoint(int vs) const ;
    int GetNormal(int vs) const ;
    float GetU(int vs,  int stage=0) const ;
    float GetV(int vs, int stage=0) const ;
    unsigned long GetFlags() const ;
    

    void CopyFaceTo(int index); //copies face into another index in object
    void CopyFaceTo(FaceT &other) const; //copies face into another index in object, target acts as reference
    int GetFaceIndex() const {return _faceIndex;}

    int GetPointInfoSize() const ;   //gets total size for structure to save one point information
    void SavePointInfo(int point, void *nfostruct) const; //saves structure for one point
    void LoadPointInfo(int point, const void *nfostruct); //loads structure for one point

    void CopyPointInfo(int trgpt, int srcpt);
    void CopyPointInfo(int trgpt, FaceT &other, int srcpt);

    void Serialize(IArchive &arch);


// Other


  
    bool ContainsPoint( int vertex ) const;
    bool IsNeighbourgh( const FaceT &face ) const;
    int ContainsPoints( const bool *vertices ) const;
    bool ContainsEdge( int v1, int v2 ) const;
    int PointAt( int vertex ) const;
    bool ContainsNormal( int vertex ) const;
    void Reverse();
    void Cross();
    void Shift0To1();
    void Shift1To0();
    Point3D CalculateNormal( ) const;
    Point3D CalculateNormal(  const class AnimationPhase &phase ) const;
    Point3D CalculateRawNormal(  ) const;
    double CalculateArea(  ) const;
    double CalculatePerimeter( ) const;
    bool IsConvex(  ) const;
    
    bool PointInHalfSpace
      (
         const Point3D &p
          ) const;
    bool FaceInHalfSpace
      (
         const FaceT &face
          ) const;
    
    
    void AutoUncross(  );
    void AutoReverse(  );

    static bool PointInHalfSpace( const Point3D &normal, float d, const Point3D &p );


  private:
    void swap_pts(int pt1,int pt2);

    

  };

TypeIsMovable(FaceT);

#endif
