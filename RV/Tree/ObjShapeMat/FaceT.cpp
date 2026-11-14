#include "objshape.h"
#include "FaceT.h"
#include <malloc.h>

inline int Modulo( int i, int n )
{
  if( i>=n ) i-=n;
  if( i<0 ) i+=n;
  return i;
}

void FaceT::swap_pts(int pt1,int pt2)
  {
  int blksz=GetPointInfoSize();
  void *block1=alloca(blksz);
  void *block2=alloca(blksz);
  SavePointInfo(pt1,block1);
  SavePointInfo(pt2,block2);
  LoadPointInfo(pt2,block1);
  LoadPointInfo(pt1,block2);
  }


bool FaceT::ContainsPoint( int vertex ) const
{
  for( int i=0; i<N(); i++ ) if( GetPoint(i)==vertex ) return true;
  return false;
}

int FaceT::ContainsPoints( const bool *vertices ) const
{
  int count=0;
  for( int i=0; i<N(); i++ ) count+=vertices[GetPoint(i)];
  return count;
}

bool FaceT::ContainsEdge( int v1, int v2 ) const
{
  for( int i=0,last=N()-1; i<N(); last=i,i++ )
  {
    if( GetPoint(last)==v1 && GetPoint(i)==v2 ) return true;
    //if( vs[i].point==v2 && vs[last].point==v1 ) return true;
  }
  return false;
}

bool FaceT::IsNeighbourgh( const FaceT &face ) const
{
  for( int i=0,last=N()-1; i<N(); last=i,i++ )
  {
    int actV=GetPoint(i);
    int lastV=GetPoint(last);
    if( face.ContainsEdge(actV,lastV) ) return true;
  }
  return false;
}

bool FaceT::ContainsNormal( int normal ) const
{
  for( int i=0; i<N(); i++ ) if( GetNormal(i)==normal ) return true;
  return false;
}

int FaceT::PointAt( int vertex ) const
{
  for( int i=0; i<N(); i++ ) if( GetPoint(i)==vertex ) return i;
  return -1;
}

void FaceT::Reverse()
{
int n=N();
for (int i=n/2;i<n;i++)
  {
  int j=n-i-1;
  if (i!=j) swap_pts(i,j);
  }
}

void FaceT::Cross()
{
  if( N()==4 )
  {
    static bool swap23=false;
    swap23=!swap23;
    if( swap23 ) swap_pts(2,3);
    else swap_pts(1,2);
  }
}

void FaceT::Shift1To0()
{
  swap_pts(0,1);
  swap_pts(1,2);
}

void FaceT::Shift0To1()
{
  swap_pts(1,2);
  swap_pts(0,1);
}

double FaceT::CalculatePerimeter(  ) const
{
  double perimeter=0;
  for( int i=0,last=N()-1; i<N(); last=i++ )
  {
    int iLast=GetPoint(last);
    int iCurr=GetPoint(i);
    Point3D p1=(Point3D)_obj->Point(iLast)-(Point3D)_obj->Point(iCurr);
    perimeter+=p1.Size();
  }
  return perimeter;
}

void FaceT::AutoUncross( )
{
  if( N()==4 )
  {
    // crossed face has bigger perimeter than non-crossed
    // from all possible permutations we will select the one with the smallest perimeter
    FaceT temp=*this;
    float minPeri=1e20;
    int i,minI=0;
    for( i=0; i<4; i++ )
    {
      float peri=temp.CalculatePerimeter();
      if( minPeri>peri ) minPeri=peri,minI=i;
      temp.Cross();
    }
    for( i=0; i<minI; i++ ) Cross();
  }
}

void FaceT::AutoReverse(  )
{
  // calculate topology defect for this face
  int neighbourghs=0;
  for( int j=0,last=N()-1; j<N(); last=j,j++ )
  {
    int iLast=GetPoint(last);
    int iCurr=GetPoint(j);
    for( int vs=0; vs<_obj->NFaces(); vs++ )
    {
      FaceT other(_obj,vs);
      if( other.ContainsEdge(iCurr,iLast) ) neighbourghs++;
      if( other.ContainsEdge(iLast,iCurr) ) neighbourghs--;
    }
  }
  // each face is its own negative neighbourgh
  // face with no neighbourghs has neighbourghs=-n
  // reversing face means all neighbourgs but the face itself will change their sign
  // i.e. -n-(neighbourghs+n)=-2n-neighbourghs;
  // we want neighbourghs to be as close to 0 as possible
  if( abs(neighbourghs)>abs(2*N()+neighbourghs) ) Reverse();
}

void FaceT::CopyPointInfo(int trgpt, int srcpt)
  {
  int blksz=GetPointInfoSize();
  void *block1=alloca(blksz);
  SavePointInfo(srcpt,block1);
  LoadPointInfo(trgpt,block1);
  }

void FaceT::CopyPointInfo(int trgpt, FaceT &other, int srcpt)
  {
  int blksz=GetPointInfoSize();
  Assert(blksz==other.GetPointInfoSize());
  void *block1=alloca(blksz);
  other.SavePointInfo(srcpt,block1);
  LoadPointInfo(trgpt,block1);
  
  }

Point3D FaceT::CalculateNormal
  (
  const AnimationPhase &phase
  ) const
      {
        Vector3 sum=VZero;
        // calculate average of normal in all vertices
        // (important when face is not exactly linear)
        for( int j=0; j<N(); j++ )
        {
          int act=GetPoint(j);
          int prev=GetPoint(Modulo(j+N()-1,N()));
          int next=GetPoint(Modulo(j+1,N()));
          Point3D p1=phase[act];
          Point3D p0=phase[prev];
          Point3D p2=phase[next];
          sum=sum+(p1-p0).CrossProduct(p2-p0);
        }
        return sum.Normalized();
      }

Point3D FaceT::CalculateRawNormal(  ) const
{
  Vector3 sum=VZero;
  // calculate average of normal in all vertices
  // (important when face is not exactly linear)
  for( int j=0; j<N(); j++ )
  {
    int act=GetPoint(j);
    int prev=GetPoint(Modulo(j+N()-1,N()));
    int next=GetPoint(Modulo(j+1,N()));
    Point3D p1=_obj->Point(act);
    Point3D p0=_obj->Point(prev);
    Point3D p2=_obj->Point(next);
    sum=sum+(p1-p0).CrossProduct(p2-p0);
  }
  return sum;
}

Point3D FaceT::CalculateNormal( ) const
{
  Point3D sum=CalculateRawNormal();
  sum.Normalize();
  return sum;
}

double FaceT::CalculateArea( ) const
{
  //Point3D sum(0,0,0);
  float sum=0;
  // calculate average of normal in all vertices
  // (important when face is not exactly linear)
  for( int j=0; j<N(); j++ )
  {
    int act=GetPoint(j);
    int prev=GetPoint(Modulo(j+N()-1,N()));
    int next=GetPoint(Modulo(j+1,N()));
    Point3D p1=_obj->Point(act);
    Point3D p0=_obj->Point(prev);
    Point3D p2=_obj->Point(next);
    //sum=sum+(p1-p0).CrossProduct(p2-p0);
    sum+=(p1-p0).CrossProduct(p2-p0).Size();
  }
  // each triangle is counted three times
  //return sum.Size()*(1.0/6);
  return sum*(1.0/6);
}

bool FaceT::IsConvex(  ) const
{
  // for planar convex polygon all triangles of triangulation
  // must have the same normal
  Point3D normal=CalculateNormal();
  float sum=0;
  // calculate average of normal in all vertices
  // (important when face is not exactly linear)
  for( int j=0; j<N(); j++ )
  {
    int act=GetPoint(j);
    int prev=GetPoint(Modulo(j+N()-1,N()));
    int next=GetPoint(Modulo(j+1,N()));
    Point3D p1=_obj->Point(act);
    Point3D p0=_obj->Point(prev);
    Point3D p2=_obj->Point(next);
    //sum=sum+(p1-p0).CrossProduct(p2-p0);
    Point3D tNormal=(p1-p0).CrossProduct(p2-p0);
    tNormal.Normalize();
    if( (tNormal-normal).SquareSize()>1e-2 ) return false;
  }
  return true;
}




bool FaceT::PointInHalfSpace( const Point3D &normal, float d, const Point3D &p )
  {
  float inside=p*normal+d;
  if( inside>=0 ) return true;
  // if the point is too out, fail
  if( inside*inside<normal.SquareSize()*1e-12 ) return true;
  return false;
  }



bool FaceT::PointInHalfSpace
  (
  const Point3D &p
  ) const
        {
        Point3D normal=CalculateRawNormal();
        Point3D p0=_obj->Point(GetPoint(0));
        float d=-(normal*p0);
        return PointInHalfSpace(normal,d,p);
        }



bool FaceT::FaceInHalfSpace
  (
   const FaceT &face
  ) const
        {
        Point3D normal=CalculateRawNormal();
        Point3D p0=_obj->Point(GetPoint(0));
        float d=-(normal*p0);
        for( int v=0; v<face.N(); v++ )
          {
          Point3D p=_obj->Point(face.GetPoint(v));
          float inside=p*normal+d;
          if( PointInHalfSpace(normal,d,p) ) continue;
          return false;
          }
        return true;
        }


struct FaceTraditionalEmptyList:public FaceTraditional
  {
  public:
    static FaceTraditionalEmptyList *fc_empty_list;
	static int autofreemem;
	FaceTraditionalEmptyList *next;

	void *operator new(size_t sz);
	void operator delete(void *ptr);
	FaceTraditionalEmptyList();
  };

FaceTraditionalEmptyList *FaceTraditionalEmptyList::fc_empty_list=NULL;
int FaceTraditionalEmptyList::autofreemem=0;


struct FaceTraditionalEmptyListCleaner
  {
  public:
	~FaceTraditionalEmptyListCleaner()
	  {
	  while (FaceTraditionalEmptyList::fc_empty_list)
		{
		FaceTraditionalEmptyList *ptr=FaceTraditionalEmptyList::fc_empty_list;
		FaceTraditionalEmptyList::fc_empty_list=ptr->next;
		free(ptr);
		}
	  }
  };
static FaceTraditionalEmptyListCleaner fccleaner;


void *FaceTraditionalEmptyList::operator new(size_t sz)
  {
  if (fc_empty_list==NULL) return malloc(sizeof(FaceTraditionalEmptyList));
  else 
	{
	FaceTraditionalEmptyList *ptr=fc_empty_list;
	fc_empty_list=ptr->next;
	return ptr;
	}
  }

void FaceTraditionalEmptyList::operator delete(void *ptr)
  {
  if (autofreemem==0) 
	{
	free(ptr);
	autofreemem=10;
	}
  else
	{
	FaceTraditionalEmptyList *ftptr=reinterpret_cast<FaceTraditionalEmptyList *>(ptr);
	ftptr->next=fc_empty_list;
	fc_empty_list=ftptr;
	if (fc_empty_list) autofreemem--;
	}
  }

FaceTraditionalEmptyList::FaceTraditionalEmptyList()
  {
  for (int i=0;i<4;i++) memset(vs+i,0,sizeof(vs[i]));
  n=0;
  flags=0;
  next=NULL;  
  }

FaceT::FaceT(const FaceT &other):_obj(other._obj),_faceIndex(-1),_helper(0)
  {
  Assert(other.IsValid());
  CreateUnbound(_obj);
  if (other._helper) *_helper=*other._helper;
  else *_helper=other._obj->Face(other._faceIndex);
  }

FaceT &FaceT::operator=(const FaceT &other)
  {
  Assert(_faceIndex==-1);
  Release();
  Assert(other.IsValid());  
  if (_obj==NULL) _obj=other._obj;
  CreateUnbound(_obj);
  if (other._helper) *_helper=*other._helper;
  else *_helper=other._obj->Face(other._faceIndex);
  return *this;
  }

void FaceT::CreateUnbound(ObjectData *obj)
  {
  Assert(!IsValid());
  _obj=obj;
  _faceIndex=-1;
  _helper=new FaceTraditionalEmptyList;
  }

void FaceT::Release()
  {
  if (_helper) delete static_cast<FaceTraditionalEmptyList *>(_helper);
  _faceIndex=-1;
  _helper=NULL;
  }

void FaceT::CreateIn(ObjectData *obj)
  {
  Assert(!IsValid());
  _obj=obj;  
  _faceIndex=const_cast<ObjectData *>(_obj)->NewFace();
  }

void FaceT::BindTo(ObjectData *obj, int index)
  {
  Release();
  _faceIndex=index;
  _obj=obj;
  }

void FaceT::BindTo(const FaceT &other)
  {
  Release();  
  if (other._helper!=NULL)
	(*this)=other;
  else
	{
	_faceIndex=other._faceIndex;
	_obj=other._obj;
	}
  }

void FaceT::SetTexture(RString name,int stage)
  {
  Assert(IsValid());
  if (_helper) _helper->vTexture=name;  
  else _obj->Face(_faceIndex).vTexture=name;
  }

void FaceT::SetMaterial(RString name, int stage)  
  {
  Assert(IsValid());
  if (_helper) _helper->vMaterial=name;  
  else _obj->Face(_faceIndex).vMaterial=name;
  }

void FaceT::SetVxCount(int n)
  {
  Assert(IsValid());
  Assert(n>=0 && n<5);
  if (_helper) _helper->n=n;
  else _obj->Face(_faceIndex).n=n;
  }

void FaceT::SetPoint(int vs, int pt)
  {
  Assert(IsValid());
  Assert(vs>=0 && vs<4);
  if (_helper) _helper->vs[vs].point=pt;
  else _obj->Face(_faceIndex).vs[vs].point=pt;
  }

void FaceT::SetNormal(int vs, int nr)
  {
  Assert(IsValid());
  Assert(vs>=0 && vs<4);
  if (_helper) _helper->vs[vs].normal=nr;
  else _obj->Face(_faceIndex).vs[vs].normal=nr;
  }

void FaceT::SetU(int vs, float tu, int stage)
  {
  Assert(IsValid());
  Assert(vs>=0 && vs<4);
  if (_helper) _helper->vs[vs].mapU=tu;
  else _obj->Face(_faceIndex).vs[vs].mapU=tu;
  }

void FaceT::SetV(int vs, float tv, int stage)
  {
  Assert(IsValid());
  Assert(vs>=0 && vs<4);
  if (_helper) _helper->vs[vs].mapV=tv;
  else _obj->Face(_faceIndex).vs[vs].mapV=tv;
  }

void FaceT::SetUV(int vs, float tu, float tv, int stage)
  {
  Assert(IsValid());
  Assert(vs>=0 && vs<4);
  if (_helper) 
	{
	_helper->vs[vs].mapU=tu;
	_helper->vs[vs].mapV=tv;
	}
  else 
	{
	FaceTraditional &face=_obj->Face(_faceIndex);
	face.vs[vs].mapU=tu;
	face.vs[vs].mapV=tv;
	}
  }

void FaceT::SetFlags(unsigned long flags)
  {
  Assert(IsValid());
  if (_helper) _helper->flags=flags;
  else _obj->Face(_faceIndex).flags=flags;
  }

unsigned long FaceT::SetFlags(unsigned long zerobits, unsigned long invertbits)
  {
  Assert(IsValid());
  unsigned long &flags=_helper?_helper->flags:_obj->Face(_faceIndex).flags;
  flags=flags & ~zerobits ^ invertbits;
  return flags;
  }

RString FaceT::GetTexture(int stage) const
  {
  Assert(IsValid());
  return _helper?_helper->vTexture:_obj->Face(_faceIndex).vTexture;
  }

RString FaceT::GetMaterial(int stage) const
  {
  Assert(IsValid());
  return _helper?_helper->vMaterial:_obj->Face(_faceIndex).vMaterial;
  }

int FaceT::GetVxCount() const 
  {
  Assert(IsValid());
  return _helper?_helper->n:_obj->Face(_faceIndex).n;
  }

int FaceT::GetPoint(int vs) const 
  {
  Assert(IsValid());
  Assert(vs>=0 && vs<4);
  return _helper?_helper->vs[vs].point:_obj->Face(_faceIndex).vs[vs].point;
  }

int FaceT::GetNormal(int vs) const 
  {
  Assert(IsValid());
  Assert(vs>=0 && vs<4);
  return _helper?_helper->vs[vs].normal:_obj->Face(_faceIndex).vs[vs].normal;
  }

float FaceT::GetU(int vs, int stage) const 
  {
  Assert(IsValid());
  Assert(vs>=0 && vs<4);
  return _helper?_helper->vs[vs].normal:_obj->Face(_faceIndex).vs[vs].mapU;
  }

float FaceT::GetV(int vs, int stage) const 
  {
  Assert(IsValid());
  Assert(vs>=0 && vs<4);
  return _helper?_helper->vs[vs].mapU:_obj->Face(_faceIndex).vs[vs].mapV;
  }

unsigned long FaceT::GetFlags() const 
  {
  Assert(IsValid());
  return _helper?_helper->flags:_obj->Face(_faceIndex).flags;
  }

void FaceT::CopyFaceTo(int index) 
  {
  Assert(IsValid());
  FaceTraditional &face=_helper?*_helper:_obj->Face(_faceIndex);
  _obj->Face(index)=face;
  }

void FaceT::CopyFaceTo(FaceT &other) const
  {
  Assert(IsValid());
  FaceTraditional &face=_helper?*_helper:_obj->Face(_faceIndex);
  FaceTraditional &trg=other._helper?*other._helper:other._obj->Face(other._faceIndex);
  trg=face;
  }

int FaceT::GetPointInfoSize() const 
  {
  FaceTraditional *p=NULL;
  Assert(IsValid());
  return sizeof(p->vs[0]);
  }

void FaceT::SavePointInfo(int point, void *nfostruct) const
  {
  Assert(IsValid());
  Assert(point>=0 && point<4);
  memcpy(nfostruct,_helper?_helper->vs+point:_obj->Face(_faceIndex).vs+point,sizeof(_helper->vs[0]));
  }

void FaceT::LoadPointInfo(int point, const void *nfostruct)
  {
  Assert(IsValid());
  Assert(point>=0 && point<4);
  memcpy(_helper?_helper->vs+point:_obj->Face(_faceIndex).vs+point,nfostruct,sizeof(_helper->vs[0]));
  }
