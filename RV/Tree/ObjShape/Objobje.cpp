// 3D model import
// (C) Ondrej Spanel, Suma, 6/1996

// import formats:
// Jaguar .C files

#include "El/elementpch.hpp"
//FLY#include "global.hpp"

#if _MSC_VER < 1300
#include <fstream.h>
#include <strstrea.h>
#else
#include <fstream>
#include <strstream>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "macros.h"
#include "fileutil.h"
//#include "pal2pac.hpp"

#include <Es/Types/pointers.hpp>

#include "objobje.hpp"

typedef unsigned char byte;
typedef unsigned short word;

#define MAX_DATA_POLY 4

FaceT::FaceT()
{
    dirty=false;
    vert.n=0;
    texture[0]=0;
    for( int i=0; i<MAX_DATA_POLY; i++ )
    {
        DataVertex &v=vert.vs[i];
        v.point=0;
        v.normal=0;
        v.mapU=0;
        v.mapV=0;
    }
}

bool FaceT::ContainsPoint( int vertex ) const
{
	for( int i=0; i<vert.n; i++ ) if( vert.vs[i].point==vertex ) return true;
	return false;
}

int FaceT::ContainsPoints( const bool *vertices ) const
{
	int count=0;
	for( int i=0; i<vert.n; i++ ) count+=vertices[vert.vs[i].point];
	return count;
}

bool FaceT::ContainsEdge( int v1, int v2 ) const
{
	for( int i=0,last=vert.n-1; i<vert.n; last=i,i++ )
	{
		if( vert.vs[last].point==v1 && vert.vs[i].point==v2 ) return true;
		//if( vs[i].point==v2 && vs[last].point==v1 ) return true;
	}
	return false;
}

bool FaceT::IsNeighbourgh( const FaceT &face ) const
{
	for( int i=0,last=vert.n-1; i<vert.n; last=i,i++ )
	{
		int actV=vert.vs[i].point;
		int lastV=vert.vs[last].point;
		if( face.ContainsEdge(actV,lastV) ) return true;
	}
	return false;
}


bool FaceT::ContainsNormal( int normal ) const
{
	for( int i=0; i<vert.n; i++ ) if( vert.vs[i].normal==normal ) return true;
	return false;
}

int FaceT::PointAt( int vertex ) const
{
	for( int i=0; i<vert.n; i++ ) if( vert.vs[i].point==vertex ) return i;
	return -1;
}

void FaceT::Reverse()
{
	if( vert.n==3 )
	{
		::swap(vert.vs[1],vert.vs[2]);
	}
	else if( vert.n==4 )
	{
		::swap(vert.vs[0],vert.vs[1]);
		::swap(vert.vs[2],vert.vs[3]);
	}
}
void FaceT::Cross()
{
	if( vert.n==4 )
	{
		static bool swap23=false;
		swap23=!swap23;
		if( swap23 ) ::swap(vert.vs[2],vert.vs[3]);
		else ::swap(vert.vs[1],vert.vs[2]);
	}
}

void FaceT::Shift1To0()
{
	::swap(vert.vs[0],vert.vs[1]);
	::swap(vert.vs[1],vert.vs[2]);
}
void FaceT::Shift0To1()
{
	::swap(vert.vs[1],vert.vs[2]);
	::swap(vert.vs[0],vert.vs[1]);
}

double FaceT::CalculatePerimeter( const ObjectData *obj ) const
{
	double perimeter=0;
	for( int i=0,last=vert.n-1; i<vert.n; last=i++ )
	{
		int iLast=vert.vs[last].point;
		int iCurr=vert.vs[i].point;
		Point3D p1=(Point3D)obj->Point(iLast)-(Point3D)obj->Point(iCurr);
		perimeter+=p1.Size();
	}
	return perimeter;
}

#define ALPHA_MINI 0x40000000

void FaceT::AutoUncross( ObjectData *obj )
{
	if( vert.n==4 )
	{
		// crossed face has bigger perimeter than non-crossed
		// from all possible permutations we will select the one with the smallest perimeter
		FaceT temp=*this;
		float minPeri=1e20;
		int i,minI=0;
		for( i=0; i<4; i++ )
		{
			float peri=temp.CalculatePerimeter(obj);
			if( minPeri>peri ) minPeri=peri,minI=i;
			temp.Cross();
		}
		for( i=0; i<minI; i++ ) Cross();
	}
}

void FaceT::AutoReverse( ObjectData *obj )
{
	// calculate topology defect for this face
	int neighbourghs=0;
	for( int j=0,last=vert.n-1; j<vert.n; last=j,j++ )
	{
		int iLast=vert.vs[last].point;
		int iCurr=vert.vs[j].point;
		for( int vs=0; vs<obj->NFaces(); vs++ )
		{
			FaceT &other=obj->Face(vs);
			if( other.ContainsEdge(iCurr,iLast) ) neighbourghs++;
			if( other.ContainsEdge(iLast,iCurr) ) neighbourghs--;
		}
	}
	// each face is its own negative neighbourgh
	// face with no neighbourghs has neighbourghs=-n
	// reversing face means all neighbourgs but the face itself will change their sign
	// i.e. -n-(neighbourghs+n)=-2n-neighbourghs;
	// we want neighbourghs to be as close to 0 as possible
	if( abs(neighbourghs)>abs(2*vert.n+neighbourghs) ) Reverse();
}

#define fx(fl) (fl) // float conversion
#define fxmetr(a) (a)

#define MF(a,b) ((a)*(b))
#define DF(a,b) ((a)/(b))
#define MKDF(a,b,c) ((a)*(b)/(c))

#define SOPEN "{"
#define SCLOSE "}"
#define COPEN '{'
#define CCLOSE '}'


void ObjectData::DoConstruct()
{
	_points.Clear();
    _normals.Clear();
    _faces.Clear();

	_sel.Clear();
	_hide.Clear();
	_lock.Clear();
	_mass.Clear();
	_sharpEdge.Clear();

	_dirty=false;
	_normalsDirty=false;
	_dirtySetup=false;
	
	_autoSaveAnimation=-1;
	
	int i;
	for( i=0; i<MAX_NAMED_SEL; i++ ) _namedSel[i]=NULL;
	for( i=0; i<MAX_NAMED_PROP; i++ ) _namedProp[i]=NULL;
	//FLY_phase.Clear();
}

void ObjectData::DoDestruct()
{
	int i;
	for( i=0; i<MAX_NAMED_SEL; i++ )
	{
		if( _namedSel[i] ) delete _namedSel[i],_namedSel[i]=NULL;
	}
	for( i=0; i<MAX_NAMED_PROP; i++ )
	{
		if( _namedProp[i] ) delete _namedProp[i],_namedProp[i]=NULL;
	}
}

// copy constructor
void ObjectData::DoCopy( const ObjectData &src )
{
	_points=src._points;
	_faces=src._faces;
	_normals=src._normals;
	
	_sel=src._sel;
	_sel.SetObject(this);

	_hide=src._hide;
	_hide.SetObject(this);

	_lock=src._lock;
	_lock.SetObject(this);

	_mass=src._mass;
	_mass.SetObject(this);

	_sharpEdge=src._sharpEdge;
	
	_dirty=src._dirty;
	_normalsDirty=src._normalsDirty;
	_dirtySetup=src._dirtySetup;

	int i;
	for( i=0; i<MAX_NAMED_SEL; i++ )
	{
		if( src._namedSel[i] ) 
		{
			_namedSel[i]=new NamedSelection(*src._namedSel[i]);
			if( _namedSel[i] ) _namedSel[i]->SetObject(this);
		}
		else _namedSel[i]=NULL;
	}
	for( i=0; i<MAX_NAMED_PROP; i++ )
	{
		if( src._namedProp[i] ) 
		{
			_namedProp[i]=new NamedProperty(*src._namedProp[i]);
		}
		else _namedProp[i]=NULL;
	}
/*FLY
	for( i=0; i<src._phase.Size(); i++ )
	{
		_phase.Add(new AnimationPhase(*src._phase[i],this));
	}
*/
	_autoSaveAnimation=src._autoSaveAnimation;
}

void ObjectData::DoCopy( const ObjectData &src, int animationPhase )
{
	DoCopy(src);
	// use data from animationPhase
	if( animationPhase>=0 )
	{
		UseAnimation(animationPhase);
		//FLY_phase.Clear();
		_autoSaveAnimation=-1;
	}
}


PosT *ObjectData::NewPoint()
{
    int index=ReservePoints(1);
	return &_points[index];
}
VecT *ObjectData::NewNormal()
{
    int index=_normals.Add(Vector3(0,1,0));
    return &_normals[index];
}
FaceT *ObjectData::NewFace()
{
    int index=ReserveFaces(1);
	return &_faces[index];
}

int ObjectData::ReservePoints( int count )
{
	int i,j;
	
    int oldPoints=_points.Size();
    int newPoints=oldPoints+count;
    _points.Resize(newPoints);
    for( j=oldPoints; j<newPoints; j++ )
    {
        _points[j][0]=0;
        _points[j][1]=0;
        _points[j][2]=0;
        _points[j].flags=0;
    }
    _sel.ValidatePoints();
    _lock.ValidatePoints();
    _hide.ValidatePoints();
    _mass.Validate();
    //FLYfor( i=0; i<NAnimations(); i++ ) _phase[i]->Validate();
    for( i=0; i<MAX_NAMED_SEL; i++ )
    {
        if( _namedSel[i] ) _namedSel[i]->ValidatePoints();
    }
	return oldPoints;
}

int ObjectData::ReserveFaces( int count )
{
	int i,j;
    int oldFaces=_faces.Size();
    int newFaces=oldFaces+count;
    _faces.Resize(newFaces);
    for( j=oldFaces; j<newFaces; j++ )
    {
        _faces[j].vert.n=0;
        _faces[j].flags=0;
    }
    _sel.ValidateFaces(newFaces);
    _lock.ValidateFaces(newFaces);
    _hide.ValidateFaces(newFaces);
    for( i=0; i<MAX_NAMED_SEL; i++ )
    {
        if( _namedSel[i] ) _namedSel[i]->ValidateFaces(newFaces);
    }
	return oldFaces;
}


int ObjectData::FindAdjacentFace( bool *scanned, int act, int next ) const
{
	// search for enabled face containing act and next
	for( int i=0; i<NFaces(); i++ ) if( !scanned[i] )
	{
		const FaceT &face=Face(i);
		//if( face.ContainsPoint(act) && face.ContainsPoint(next) ) return i;
		if( face.ContainsEdge(act,next) ) return i;
	}
	return -1;
}

inline int Modulo( int i, int n )
{
	if( i>=n ) i-=n;
	if( i<0 ) i+=n;
	return i;
}

void ObjectData::AddAdjacentNormal( Point3D &normal, bool *scanned, Point3D *normals, int act, int next, int prev, int iFace ) const
{
    //FLYconst FaceT &faceI=Face(iFace);    
	if( FindSharpEdge(act,next)<0 )
	{
		int nFace=FindAdjacentFace(scanned,act,next);
		if( nFace>=0 )
		{
			// add normal of this face - it is smooth adjacent
			normal+=normals[nFace];
			scanned[nFace]=true;
			const FaceT &face=Face(nFace);
            if( face.flags&FACE_FLATLIGHT )
            {
    			normal=normals[nFace];
                return;
            }
			// find act vertex
			int i=face.PointAt(act);
			if( i>=0 ) // condition must be true
			{
				int prevPoint=face.vert.vs[Modulo(i+face.vert.n-1,face.vert.n)].point;
				int nextPoint=face.vert.vs[Modulo(i+1,face.vert.n)].point;
				//if( nextPoint!=next )
				//AddAdjacentNormal(normal,scanned,normals,act,nextPoint,prevPoint,nFace);
				AddAdjacentNormal(normal,scanned,normals,act,prevPoint,nextPoint,nFace);
			}
		}
	}
	if( FindSharpEdge(prev,act)<0 )
	{
		int nFace=FindAdjacentFace(scanned,prev,act);
		if( nFace>=0 )
		{
			// add normal of this face - it is smooth adjacent
			normal+=normals[nFace];
			scanned[nFace]=true;
			const FaceT &face=Face(nFace);
            if( face.flags&FACE_FLATLIGHT )
            {
    			normal=normals[nFace];
                return;
            }
			// find act vertex
			int i=face.PointAt(act);
			if( i>=0 ) // condition must be true
			{
				int prevPoint=face.vert.vs[Modulo(i+face.vert.n-1,face.vert.n)].point;
				int nextPoint=face.vert.vs[Modulo(i+1,face.vert.n)].point;
				//if( nextPoint!=next )
				//AddAdjacentNormal(normal,scanned,normals,act,nextPoint,prevPoint,nFace);
				AddAdjacentNormal(normal,scanned,normals,act,prevPoint,nextPoint,nFace);
			}
		}
	}
}

Point3D ObjectData::SmoothAdjacentFaces( Point3D *faceNormals, int act, int prev, int next, int iFace )
{
	// check for special case:
	// two sides faces with smoothed edges
	const FaceT &face=Face(iFace);
	
	// scan all faces that are adjacent to given faces
	// through smooth edges beginning in point act
	Point3D normal=faceNormals[iFace];
	if( face.flags&FACE_BOTHSIDESLIGHT )
	{
		normal=Point3D(0,0,0); // used for single faces lighted from all sides
	}
	else if( face.flags&FACE_SKYLIGHT )
	{
		// normal directly from vertex positions
		normal=Point(act);
	}
	else
	{
		bool *scanned=new bool[NFaces()];
		memset(scanned,0,NFaces()*sizeof(bool));
		scanned[iFace]=true;
		
		// recursive spread to both sides
		AddAdjacentNormal(normal,scanned,faceNormals,act,prev,next,iFace);
		//AddAdjacentNormal(normal,scanned,faceNormals,prev,act,iFace);
		//AddAdjacentNormal(normal,scanned,faceNormals,next,act,iFace);
		//AddAdjacentNormal(normal,scanned,faceNormals,act,next,iFace);
		
		delete[] scanned;
	}
	// handle special case: two sided faces
	// in that case normal is very near to zero
	if( normal.SquareSize()<(0.1*0.1) )
	{
		// average all vertices
		int i;
		Point3D avg(0,0,0);
		for( i=0; i<face.vert.n; i++ )
		{
			avg=avg+(Point3D)Point(face.vert.vs[i].point);
		}
		avg=avg*(1.0/face.vert.n);
		// create normal leading from face center to current vertex
		normal=avg-Point3D(Point(act));
	}
	normal.Normalize();
    if( face.flags&FACE_REVERSELIGHT )
    {
        normal=-normal;
    }
    else if( face.flags&FACE_FLATLIGHT )
    {
        normal=faceNormals[iFace];
    }
	return normal;
}

Point3D FaceT::CalculateNormal
(
    const ObjectData *obj, const AnimationPhase &phase
) const
{
	Vector3 sum=VZero;
	// calculate average of normal in all vertices
	// (important when face is not exactly linear)
	for( int j=0; j<vert.n; j++ )
	{
		int act=vert.vs[j].point;
		int prev=vert.vs[Modulo(j+vert.n-1,vert.n)].point;
		int next=vert.vs[Modulo(j+1,vert.n)].point;
		Point3D p1=phase[act];
		Point3D p0=phase[prev];
		Point3D p2=phase[next];
		sum=sum+(p1-p0).CrossProduct(p2-p0);
	}
	return sum.Normalized();
}

Point3D FaceT::CalculateRawNormal( const ObjectData *obj ) const
{
	Vector3 sum=VZero;
	// calculate average of normal in all vertices
	// (important when face is not exactly linear)
	for( int j=0; j<vert.n; j++ )
	{
		int act=vert.vs[j].point;
		int prev=vert.vs[Modulo(j+vert.n-1,vert.n)].point;
		int next=vert.vs[Modulo(j+1,vert.n)].point;
		Point3D p1=obj->Point(act);
		Point3D p0=obj->Point(prev);
		Point3D p2=obj->Point(next);
		sum=sum+(p1-p0).CrossProduct(p2-p0);
	}
	return sum;
}

Point3D FaceT::CalculateNormal( const ObjectData *obj ) const
{
	Point3D sum=CalculateRawNormal(obj);
	sum.Normalize();
	return sum;
}

double FaceT::CalculateArea( const ObjectData *obj ) const
{
	//Point3D sum(0,0,0);
	float sum=0;
	// calculate average of normal in all vertices
	// (important when face is not exactly linear)
	for( int j=0; j<vert.n; j++ )
	{
		int act=vert.vs[j].point;
		int prev=vert.vs[Modulo(j+vert.n-1,vert.n)].point;
		int next=vert.vs[Modulo(j+1,vert.n)].point;
		Point3D p1=obj->Point(act);
		Point3D p0=obj->Point(prev);
		Point3D p2=obj->Point(next);
		//sum=sum+(p1-p0).CrossProduct(p2-p0);
		sum+=(p1-p0).CrossProduct(p2-p0).Size();
	}
	// each triangle is counted three times
	//return sum.Size()*(1.0/6);
	return sum*(1.0/6);
}

bool FaceT::IsConvex( ObjectData *obj ) const
{
	// for planar convex polygon all triangles of triangulation
	// must have the same normal
	Point3D normal=CalculateNormal(obj);
	//FLYfloat sum=0;
	// calculate average of normal in all vertices
	// (important when face is not exactly linear)
	for( int j=0; j<vert.n; j++ )
	{
		int act=vert.vs[j].point;
		int prev=vert.vs[Modulo(j+vert.n-1,vert.n)].point;
		int next=vert.vs[Modulo(j+1,vert.n)].point;
		Point3D p1=obj->Point(act);
		Point3D p0=obj->Point(prev);
		Point3D p2=obj->Point(next);
		//sum=sum+(p1-p0).CrossProduct(p2-p0);
		Point3D tNormal=(p1-p0).CrossProduct(p2-p0);
		tNormal.Normalize();
		if( (tNormal-normal).SquareSize()>1e-2 ) return false;
	}
	return true;
}


void ObjectData::DoRecalcNormals()
{
	// delete all normals previously defined
	ResetNormals();
	
	// calculate normals for all faces
	Point3D *faceNormals=new Point3D[NFaces()];
	int i,j;
	// scan all faces that should be smoothed
	for( i=0; i<NFaces(); i++ )
	{
		FaceT &face=Face(i);
		
		faceNormals[i]=face.CalculateNormal(this);
		
		// zero normal will probably be valid
		// anyway every point should be recalculated
		for( j=0; j<face.vert.n; j++ ) face.vert.vs[j].normal=0;
	}
	// scan all vertices of all faces
	for( i=0; i<NFaces(); i++ )
	{
		FaceT &face=Face(i);
		for( j=0; j<face.vert.n; j++ )
		{
			int actPoint=face.vert.vs[j].point;
			int prevPoint=face.vert.vs[Modulo(j+face.vert.n-1,face.vert.n)].point;
			int nextPoint=face.vert.vs[Modulo(j+1,face.vert.n)].point;
			// scan all faces that should be smoothed
			Point3D genNormal=SmoothAdjacentFaces(faceNormals,actPoint,prevPoint,nextPoint,i);
			int normIndex=AddNormal(genNormal);
			face.vert.vs[j].normal=normIndex;
		}
	}
	delete[] faceNormals;
	SetDirty();
    _normalsDirty=false;
}

void ObjectData::CleanNormals()
{
    if( !_normalsDirty ) return;
    DoRecalcNormals();

}

void ObjectData::RecalcNormals()
{
     _normalsDirty=true;
}


struct SortFaceT
{
    FaceT *_face;
	int _index;
    int _minI; // order by vertex index
	bool _toDelete;
	SortFaceT():_toDelete(false){}
	SortFaceT( FaceT &face, int index, int minI )
	:_face(&face),_index(index),_toDelete(false),_minI(minI)
	{
	}
};

TypeIsSimpleZeroed(SortFaceT);

static int CmpFaceVert( const void *f0, const void *f1 )
{
	// alpha faces should be drawn last
	int diff;
	int j;
	const FaceT *d0=((const SortFaceT *)f0)->_face;
	const FaceT *d1=((const SortFaceT *)f1)->_face;

	diff=d0->vert.n-d1->vert.n;
	if( diff ) return diff;
	for( j=0; j<d0->vert.n; j++ )
	{
		diff=d0->vert.vs[j].point-d1->vert.vs[j].point;
		if( diff ) return diff;
	}
	return 0;
}
static int CmpFaceText( const void *f0, const void *f1 )
{
	const SortFaceT *d0=(const SortFaceT *)f0;
	const SortFaceT *d1=(const SortFaceT *)f1;

    if( d1->_minI>d0->_minI ) return -1;
    if( d1->_minI<d0->_minI ) return +1;
    // do not sort alpha textures
    if( d1->_minI>=ALPHA_MINI ) return 0;
	// check texture name
	// different textures may have same minY
    int d = stricmp(d0->_face->texture,d1->_face->texture);
	if (d) return d;
    // the texture is same - use vertex indices
    return CmpFaceVert(f0,f1);
}

/**/
static int CmpFaceIndex( const void *f0, const void *f1 )
{
	const SortFaceT *d0=(const SortFaceT *)f0;
	const SortFaceT *d1=(const SortFaceT *)f1;
	int diff=d0->_index-d1->_index;
	return diff;
}
/**/

static void BubbleSort( void *data, int nElem, int sizeElem, int compare( const void *e1, const void *e2 ) )
{
	bool change;
	Temp<char> buf(sizeElem);
	do
	{
		char *e1=(char *)data;
		char *e2=e1+sizeElem;
		change=false;
		for( int i=1; i<nElem; i++ )
		{
			if( compare(e1,e2)>0 )
			{
				memcpy(buf,e1,sizeElem);
				memcpy(e1,e2,sizeElem);
				memcpy(e2,buf,sizeElem);
				change=true;
			}
			e1+=sizeElem;
			e2+=sizeElem;
		}
	} while( change );
}

struct SortVertex
{
    //ObjectData *obj;
    Point3D p;
    int index;
    int flags;
};

TypeIsSimpleZeroed(SortVertex);

static int CmpSortVertex( const void *vv1, const void *vv2 )
{
    const SortVertex &v1=*(const SortVertex *)vv1;
    const SortVertex &v2=*(const SortVertex *)vv2;
    // sort by lighting flags
    int light1 = v1.flags&POINT_LIGHT_MASK;
    int light2 = v2.flags&POINT_LIGHT_MASK;
    int d = light1-light2;
    if (d) return d;
    // sort by y
    float dif;
    const Point3D &p1=v1.p;
    const Point3D &p2=v2.p;
    dif=p1.Y()-p2.Y();
    if( dif<0 ) return +1;if( dif>0 ) return -1;
    dif=p1.X()-p2.X();
    if( dif<0 ) return +1;if( dif>0 ) return -1;
    dif=p1.Z()-p2.Z();
    if( dif<0 ) return +1;if( dif>0 ) return -1;
    return 0;
}

struct SortTexture
{
    const char *name;
    int minI;
};

TypeIsSimple(SortTexture);

class PreSortTextures: public AutoArray<SortTexture>
{
    public:
    void NewFace( ObjectData *obj, const FaceT &face );
    int FindTexture( const char *name );
    int MinI( const char *name );
};

int PreSortTextures::FindTexture( const char *name )
{
    for( int i=0; i<Size(); i++ )
    {
        if( !strcmpi(name,(*this)[i].name) ) return i;
    }
    return -1;
}

typedef DWORD __stdcall TextureColorARGBT( const char *s );


static TextureColorARGBT *TextureColorARGB;

static void InitPal2Pac()
{
/*FLY
    if (TextureColorARGB) return;
    HINSTANCE lib = ::LoadLibrary("pal2pac.dll");
    if (!lib) return;
    TextureColorARGB = (TextureColorARGBT *)::GetProcAddress(lib,"_TextureColorARGB@4");
*/
}

static bool TextureIsAlpha( const char *name )
{
    static const char *lastTest;
    static bool lastResult;
    if( !strcmpi(lastTest,name) ) return lastResult;
    lastTest=name;
	lastResult=!strcmpi(NajdiPExt(NajdiNazev(name)),".paa");
    if( lastResult ) return lastResult;
    // make full path
    // get 
    //WFilePath dest;
    //dest.SetDrive(_viewer.GetDrive());
    //dest.SetDirectory(_viewer.GetDirectory()+"data\\");
    

	InitPal2Pac();
	DWORD argb=0xffffffff;
	if (TextureColorARGB)
	{
		argb=::TextureColorARGB(name);
	}
    int a=(argb>>24)&0xff;
    lastResult=( a<0xe0 );
    return lastResult;
}

void PreSortTextures::NewFace( ObjectData *obj, const FaceT &face )
{
    const char *name=face.texture;
    int minI=0xffffff;
    for( int i=0; i<face.vert.n; i++ )
    {
        int v = face.vert.vs[i].point;
        if( minI>v ) minI=v;
    }
    int index=FindTexture(name);
    if( index>=0 )
    {
        // alpha can be marked once
        int oldMinI = (*this)[index].minI;
        if (oldMinI==ALPHA_MINI) return;
        if (oldMinI>minI ) (*this)[index].minI=minI;
    }
    else
    {
        bool alpha=TextureIsAlpha(name);
        if( alpha ) minI=ALPHA_MINI; // draw last
        else
        {
           // sort by material
           // this is easily achieved by adding a big number to minY
           if (face.flags&FACE_DISABLE_TEXMERGE) minI += 0x30000000;
        }
        index=Add();
        (*this)[index].minI=minI;
        (*this)[index].name=name;
    }
}

int PreSortTextures::MinI( const char *name )
{
    int index=FindTexture(name);
    if( index<0 ) return 0;
    return (*this)[index].minI;
}

void ObjectData::Optimize()
{
	// remove any unnecessary faces, ....
	SetDirty();
    CleanNormals();

/*FLY
	// remove all czech characters from all names
	int i;
	for( i=0; i<NFaces(); i++ )
	{
		FaceT &face=Face(i);
		//if( 
		char buf[1024];
		if( DeCzech(buf,face.texture) )
		{
			WFilePath fullOld,fullNew;
			fullOld=DataRoot+face.texture;
			fullNew=DataRoot+buf;
			MoveFile(fullOld,fullNew);
			strcpy(face.texture,buf);
		}
	}
*/

	// remove any identical points
	MergePoints(0,false);

	// sort all vertex indices
  int i;
    {
        // sort vertices so that first is topmost
        Temp<SortVertex> sortVertices(NPoints());
        for( i=0; i<NPoints(); i++ )
        {
            sortVertices[i].index=i;
            sortVertices[i].p=Point(i);
            sortVertices[i].flags=Point(i).flags;
        }
        
        //QSort((SortVertex *)sortVertices,NPoints(),CmpSortVertex);
        qsort((SortVertex *)sortVertices,NPoints(),sizeof(*sortVertices),CmpSortVertex);
        Temp<int> permutVertices(NPoints());
        for( i=0; i<NPoints(); i++ )
        {
            permutVertices[sortVertices[i].index]=i;
            //permutVertices[i]=sortVertices[i].index;
        }
        PermuteVertices(permutVertices);
    }

	// sort all faces so that first vertex index is the lowest one
	for( i=0; i<NFaces(); i++ )
	{
		FaceT &face=Face(i);
		int minP=INT_MAX,minJ=0;
		int j;
		for( j=0; j<face.vert.n; j++ )
		{
			if( face.vert.vs[j].point<minP ) minP=face.vert.vs[j].point,minJ=j;
		}
		FaceT temp=face;
		for( j=0; j<face.vert.n; j++ )
		{
			int src=Modulo(j+minJ,face.vert.n);
			face.vert.vs[j]=temp.vert.vs[src];
		}
	}

    // pre-sort textures
    PreSortTextures sortTextures;
	for( i=0; i<NFaces(); i++ ) sortTextures.NewFace(this,Face(i));
    
	// sort faces by texture name
	Temp<SortFaceT> sortFace(NFaces());
	for( i=0; i<NFaces(); i++ )
	{
		sortFace[i]=SortFaceT(Face(i),i,sortTextures.MinI(Face(i).texture));
	}
	// use quick sort - result is not stored anywhere
	qsort(sortFace,NFaces(),sizeof(*sortFace),CmpFaceVert);
	// remove any duplicate faces
    /**/
	for( i=0; i<NFaces(); i++ )
	{
		SortFaceT &sFace=sortFace[i];
		FaceT &face=*sFace._face;
		if( face.vert.n<3 ) sFace._toDelete=true;
	}
	for( i=1; i<NFaces(); i++ )
	{
		SortFaceT &face=sortFace[i];
		SortFaceT &prev=sortFace[i-1];
		if( !CmpFaceVert(&face,&prev) )
		{
			face._toDelete=true;
		}
	}

	// sort faces to delete to end
	qsort(sortFace,NFaces(),sizeof(*sortFace),CmpFaceIndex);
	
	for( i=NFaces(); --i>=0; )
	{
		//WASSERT( sortFace[i]._index==i );
		if( sortFace[i]._toDelete )
		{
			DeleteFace(sortFace[i]._index);
		}
	}
    /**/
	
	// prepare for sorting again
	for( i=0; i<NFaces(); i++ )
	{
		sortFace[i]=SortFaceT(Face(i),i,sortTextures.MinI(Face(i).texture));
	}
	//qsort(sortFace,nFaces,sizeof(*sortFace),CmpFaceVert);
	//BubbleSort(sortFace,nFaces,sizeof(*sortFace),CmpFaceVert);
	// sort faces by texture name
	BubbleSort(sortFace,NFaces(),sizeof(*sortFace),CmpFaceText);
	// remove any invalid sharp edges
	for( i=_sharpEdge.Size(); --i>=0; )
	{
		int a=_sharpEdge[i][0],b=_sharpEdge[i][1];
		if( a==b || a<0 || a>=NPoints() || b<0 || b>=NPoints() ) RemoveSharpEdge(i);
	}
    // remove any double sharp edges
	for( i=_sharpEdge.Size(); --i>=1; )
	{
		int a1=_sharpEdge[i][0],b1=_sharpEdge[i][1];
		int a0=_sharpEdge[i-1][0],b0=_sharpEdge[i-1][1];
		if( a1==a0 && b1==b0 ) RemoveSharpEdge(i);
	}
    

    {
        // sort results in inverse permutation
        // create inverse of inverse permutation
        Temp<int> permut(NFaces());
        for( i=0; i<NFaces(); i++ )
        {
            permut[sortFace[i]._index]=i;
        }
        // apply permutation on all face indices
        _sel.PermuteFaces(permut);
        _lock.PermuteFaces(permut);
        _hide.PermuteFaces(permut);
        for( i=0; i<MAX_NAMED_SEL; i++ )
        {
            if( _namedSel[i] ) _namedSel[i]->PermuteFaces(permut);
        }
        // permute faces
        Temp<FaceT> save(NFaces());
        //for( i=0; i<nFaces; i++ ) save[i]=*sortFace[i]._face;
        for( i=0; i<NFaces(); i++ ) save[permut[i]]=Face(i);
        for( i=0; i<NFaces(); i++ ) Face(i)=save[i];
    }
    

    
}


void ObjectData::Triangulate( bool allFaces )
{
    int nFaces=NFaces();
	for( int i=0; i<nFaces; i++ ) if( allFaces || FaceSelected(i) )
	{
		if( Face(i).vert.n<=3 ) continue;
		FaceT *nFace=NewFace(); // may change pointers into face
		if( !nFace ) return;
		FaceT &face=Face(i);
		*nFace=face;
		nFace->vert.vs[0]=face.vert.vs[0];
		nFace->vert.vs[1]=face.vert.vs[2];
		nFace->vert.vs[2]=face.vert.vs[3];
		face.vert.n=3;
		nFace->vert.n=3;
        SetDirty();
		// original face is selected - new face should be selected too
		if( FaceSelected(i) )
		{
			int iFace=nFace-_faces.Data();
			FaceSelect(iFace);
		}
	}
}

static bool CanStep( const DataVertex &v1, const DataVertex &v2 )
{
    if( v1.normal!=v2.normal ) return false;
    if( fabs(v1.mapU-v2.mapU)>1e-5 ) return false;
    if( fabs(v1.mapV-v2.mapV)>1e-5 ) return false;
    return true;
}

void ObjectData::BuildStrip
(
    AutoArray<int> &data,
    //int v0, int v1,
    const DataVertex &d0, const DataVertex &d1, bool even,
    const FaceT *face, // face requirements
    bool *freeFaces
)
{
    Temp<bool> free(freeFaces,NFaces());
    // find any face containing v0,v1 edge
    // assume there is one possibility how to continue
    const DataVertex *v0=&d0;
    const DataVertex *v1=&d1;
    for(;;)
    {
        int faceIndex=FindOrientedEdge(v0->point,v1->point,free);
        if( faceIndex<0 ) break; // end reached
        free[faceIndex]=false; // face used - no longer free
        // check if strip step is possible
        // check if vertex can be shared
        const FaceT &sFace=Face(faceIndex);
        int baseV0=sFace.PointAt(v0->point);
        int baseV1=Modulo(baseV0+1,sFace.vert.n);
        int baseV2=Modulo(baseV0-1,sFace.vert.n);
        if( !CanStep(sFace.vert.vs[baseV0],*v0) ) continue;
        if( !CanStep(sFace.vert.vs[baseV1],*v1) ) continue;
        // check if face state is not changed
        if( face )
        {
            if( face->flags!=sFace.flags ) continue;
            if( strcmpi(face->texture,sFace.texture) ) continue;
        }
        // do as many strip steps through the face as neccessary
        while( baseV2!=Modulo(baseV1+1,sFace.vert.n) )
        {
            even=!even;
            if( even )
            {
                baseV2=Modulo(baseV2-1,sFace.vert.n);
            }
            else
            {
                baseV1=Modulo(baseV1+1,sFace.vert.n);
            }
        }
        even=!even;
        if( even )
        {
            v0=&sFace.vert.vs[baseV2];
            v1=&sFace.vert.vs[baseV1];
        }
        else
        {
            v0=&sFace.vert.vs[baseV0];
            v1=&sFace.vert.vs[baseV2];
        }
        // continue building
        data.Add(faceIndex);
        face=&sFace;
    }
}

void ObjectData::BuildFan
(
    AutoArray<int> &data,
    //int v0, int v1,
    const DataVertex &d0, const DataVertex &d1,
    const FaceT *face, // face requirements
    bool *freeFaces
)
{
    Temp<bool> free(freeFaces,NFaces());
    // find any face containing v0,v1 edge
    // assume there is one possibility how to continue
    const DataVertex *v0=&d0;
    const DataVertex *v1=&d1;
    for(;;)
    {
        int faceIndex=FindOrientedEdge(v0->point,v1->point,free);
        if( faceIndex<0 ) break; // end reached
        free[faceIndex]=false; // face used - no longer free
        // check if strip step is possible
        // check if vertex can be shared
        const FaceT &sFace=Face(faceIndex);
        int baseV0=sFace.PointAt(v0->point);
        int baseV1=Modulo(baseV0+1,sFace.vert.n);
        int baseV2=Modulo(baseV0-1,sFace.vert.n);
        if( !CanStep(sFace.vert.vs[baseV0],*v0) ) continue;
        if( !CanStep(sFace.vert.vs[baseV1],*v1) ) continue;
        // check if face state is not changed
        if( face )
        {
            if( face->flags!=sFace.flags ) continue;
            if( strcmpi(face->texture,sFace.texture) ) continue;
        }
        // do strip/fan step through the face
        data.Add(faceIndex);
        // continue building
        v0=&sFace.vert.vs[baseV0];
        v1=&sFace.vert.vs[baseV2];
        face=&sFace;
    }
}

void ObjectData::StripsAndFans()
{
    //FLYint sCounter=0,fCounter=0;
    int vCounter=0,voCounter=0;
    int i;
    for( i=0; i<NFaces(); i++ ) voCounter+=Face(i).vert.n;
    // create strips and fans
    //Triangulate(true);
    #if 1
    Temp<bool> free(NFaces());
    // init: all triangles are free
    for( i=0; i<NFaces(); i++ ) free[i]=true;
    for(;;)
    {
        // iterate: start with any triangle that is free
        bool someFree=false;
        AutoArray<int> maxData;
        bool isStrip=false;
        for( i=0; i<NFaces(); i++ ) if( free[i] )
        {
            FaceT &face=Face(i);
            // check all three edges as starting points of strips/fans
            AutoArray<int> data;
            // check all six possibilities of strips/fans
            data.Clear();
            BuildStrip(data,face.vert.vs[0],face.vert.vs[1],false,NULL,free);
            if( data.Size()>maxData.Size() ) maxData=data,isStrip=true;
            data.Clear();
            BuildStrip(data,face.vert.vs[1],face.vert.vs[2],false,NULL,free);
            if( data.Size()>maxData.Size() ) maxData=data,isStrip=true;
            data.Clear();
            BuildStrip(data,face.vert.vs[2],face.vert.vs[0],false,NULL,free);
            if( data.Size()>maxData.Size() ) maxData=data,isStrip=true;
            data.Clear();
            BuildFan(data,face.vert.vs[0],face.vert.vs[1],NULL,free);
            if( data.Size()>maxData.Size() ) maxData=data,isStrip=false;
            data.Clear();
            BuildFan(data,face.vert.vs[1],face.vert.vs[2],NULL,free);
            if( data.Size()>maxData.Size() ) maxData=data,isStrip=false;
            data.Clear();
            BuildFan(data,face.vert.vs[2],face.vert.vs[0],NULL,free);
            if( data.Size()>maxData.Size() ) maxData=data,isStrip=false;
            someFree=true;
        }
        // use maximal strip/fan
        ClearSelection();
        vCounter+=2; // init takes two vertices
        for( i=0; i<maxData.Size(); i++ )
        {
            // this triangle has been checked and is no longer free
            int faceI=maxData[i];
            free[faceI]=false;
            FaceT &face=Face(faceI);
            face.flags&=FACE_FANSTRIP_MASK;
            if( isStrip )
            {
                if( i==0 ) face.flags|=FACE_BEGIN_STRIP;
                else face.flags|=FACE_CONTINUE_STRIP;
            }
            else
            {
                if( i==0 ) face.flags|=FACE_BEGIN_FAN;
                else face.flags|=FACE_CONTINUE_FAN;
            }
            // remmember strip result
            _sel.FaceSelect(faceI);
            vCounter+=face.vert.n-2; // two vertices are shared
        }
        /*
        SelectPointsFromFaces();
        WString name;
        if( isStrip ) name.Sprintf("Strip %03d",++fCounter);
        else name.Sprintf("Fan %03d",++sCounter);
        SaveNamedSel(name);
        */
        if( !someFree ) break; // terminate loop - no triangles left
    }
/*FLY
    WMessageBox::Messagef
    (
        NULL,WMBLevelInfo,WMBButtonOk,
        "Strips/Fans","Vertices to transfer: %d (was %d)",vCounter,voCounter
    );
*/
    #endif
}


void ObjectData::Squarize( bool allFaces )
{
	// restore square faces from triangles
	int i,j;
	for( i=NFaces(); --i>=0; )  if( allFaces || FaceSelected(i) )
	for( j=i; --j>=0; )  if( allFaces || FaceSelected(j) )
	{
		int k;
		//if( smoothing[i]!=smoothing[j] ) continue;
		FaceT &iFace=Face(i);
		FaceT &jFace=Face(j);
		if( iFace.vert.n!=3 ) continue;
		if( jFace.vert.n!=3 ) continue;
		// merge two triangle into one rectangle
		// there must be two common vertices
		// each triangle should contain one unique vertex
		int iC[2],jC[2];
		int cs=0;
		for( k=0; k<3; k++ )
		{
			int atInJ=jFace.PointAt(iFace.vert.vs[k].point);
			if( atInJ>=0 )
			{
				jC[cs]=atInJ;
				iC[cs]=k;
				cs++;
				if( cs>=2 ) break;
			}
		}
		if( cs<2 ) continue; // no two common points
		// check fo coplanarity of the faces using CalculateNormal() function
		Point3D iNormal=iFace.CalculateNormal(this);
		Point3D jNormal=jFace.CalculateNormal(this);

		// if plane is planar, both normals should be same
		// calculate cos(fi) - angle between normals
		double cosFi=iNormal*jNormal;
		if( cosFi<0.999 )
		{
			//double cosMin1=cosFi-1;
			continue; // 0.999 is cos(2.5 degree)
		}
		
		// following conditions are true:
		// iC[0]<iC[1], iC[0]<=1
		
		// check for some condition we need to be true
		if( jC[0]==jC[1] ) continue;
		// the points must be identical to merge
		if( iFace.vert.vs[iC[0]].normal!=jFace.vert.vs[jC[0]].normal ) continue;
		if( iFace.vert.vs[iC[1]].normal!=jFace.vert.vs[jC[1]].normal ) continue;
		if( fabs(iFace.vert.vs[iC[0]].mapU-jFace.vert.vs[jC[0]].mapU)>0.001 ) continue;
		if( fabs(iFace.vert.vs[iC[1]].mapV-jFace.vert.vs[jC[1]].mapV)>0.001 ) continue;
		// rotate faces so that iC[0],jC[0] is 0
		while( iC[0]>0 )
		{
			iFace.Shift1To0();
			iC[0]=Modulo(iC[0]+2,3);
			iC[1]=Modulo(iC[1]+2,3);
		}
		while( jC[0]>0 )
		{
			jFace.Shift1To0();
			jC[0]=Modulo(jC[0]+2,3);
			jC[1]=Modulo(jC[1]+2,3);
		}
		// four possible situations: iC[1],jC[1]==1 or 2
		// only two of them enable concatenation
		if( iC[1]==jC[1] ) continue; // different orientation
		// from j take point which is neither jC[0] nor jC[1]
		int jUnique=0;
		for( k=0; k<3; k++ ) if( jC[0]!=k &&  jC[1]!=k ) jUnique=k;
		FaceT nFace=iFace;
		if( iC[1]==1 )
		{
			// insert unique j point between iC[0] and iC[1]
			nFace.vert.vs[3]=iFace.vert.vs[2],nFace.vert.vs[2]=iFace.vert.vs[1];
			nFace.vert.vs[1]=jFace.vert.vs[jUnique];
		}
		else // iC[1]==2
		{
			nFace.vert.vs[3]=jFace.vert.vs[jUnique];
		}
		nFace.vert.n=4;
		// check if result is convex
		// note: concave would have some angle>0
		if( nFace.IsConvex(this) )
		{
			jFace=nFace;
			DeleteFace(i);
			break; // break j loop - i face is already matched
		}
	}
    if( NNormals()<=0 ) NewNormal();
	RecalcNormals();
}

static int NajdiZnak( FILE *f, const char *c )
{
	int r;
	do
	{
		r=fgetc(f);
		if( r<0 ) return r;
	}
	while( !strchr(c,r) );
	return r;
}

static float NactiMetry( FILE *f )
{
	int r;
	double d;
	r=NajdiZnak(f,"(");if( r<0 ) return r;
	fscanf(f,"%lf",&d);
	return fxmetr(d);
}
static float NactiFx( FILE *f )
{
	int r;
	double d;
	r=NajdiZnak(f,"(");if( r<0 ) return r;
	fscanf(f,"%lf",&d);
	return d;
}
static long NactiCRY( FILE *f )
{
	long r;
	r=NajdiZnak(f,"[(");if( r<0 ) return r;
	fscanf(f,"0x%lx",&r);
	r=NajdiZnak(f,")]");if( r<0 ) return r;
	return r;
}
static int NactiIndex( FILE *f )
{
	int r;
	r=NajdiZnak(f,"[(");if( r<0 ) return r;
	fscanf(f,"%d",&r);
	return r;
}

int ObjectData::LoadPointAsc( FILE *f )
{
	int r=NajdiZnak(f,"{}"); /* najdi zac. bodu nebo konec sekce bodu */
	PosT *B;
	if( r<0 ) return -1;
	if( r==CCLOSE ) return 1;
	B=NewPoint();
	if( !B ) return -1;
	(*B)[0]=NactiMetry(f);
    (*B)[1]=NactiMetry(f);
    (*B)[2]=NactiMetry(f);
	r=NajdiZnak(f,SCLOSE);if( r<0 ) {free(B);return r;}
	return 0;
}


int ObjectData::LoadNormalAsc( FILE *f )
{ // load normal
	int r=NajdiZnak(f,"{}");
	VecT *B;
	if( r<0 ) return -1;
	if( r==CCLOSE ) return 1;
	r=NajdiZnak(f,SOPEN);if( r!=COPEN ) return -1;
	B=NewNormal();
	if( !B ) return -1;
	// skip RGB color
	NactiFx(f);NactiFx(f);NactiFx(f);
	r=NajdiZnak(f,SCLOSE);if( r!=CCLOSE ) return -1;
	r=NajdiZnak(f,SOPEN);if( r!=COPEN ) return -1;
	(*B)[0]=NactiFx(f);
    (*B)[1]=NactiFx(f);
    (*B)[2]=NactiFx(f);
	r=NajdiZnak(f,SCLOSE);if( r<0 ) return -1;
	r=NajdiZnak(f,SCLOSE);if( r<0 ) return -1;
	return 0;
}
int ObjectData::LoadCRYNormalAsc( FILE *f )
{ /* obarven¡ nese informace jen o norm l ch */
	int r=NajdiZnak(f,"{}"); /* najdi zac. bodu nebo konec sekce bodu */
	VecT *B;
	if( r<0 ) return -1;
	if( r==CCLOSE ) return 1;
	B=NewNormal();
	if( !B ) return -1;
	r=NajdiZnak(f,SOPEN);if( r!=COPEN ) return -1;
	(*B)[0]=NactiFx(f);
    (*B)[1]=NactiFx(f);
    (*B)[2]=NactiFx(f);
	r=NajdiZnak(f,SCLOSE);if( r<0 ) return -1;
	r=NajdiZnak(f,SCLOSE);if( r<0 ) return -1;
	return 0;
}

int ObjectData::LoadFaceShadedAsc( FILE *f )
{
	int nb,i,r=NajdiZnak(f,"{}"); /* najdi zac. bodu nebo konec sekce bodu */
	FaceT *P;
	if( r<0 ) return -1;
	if( r==CCLOSE ) return 1;
	P=NewFace();
	if( !P ) return -1;
	nb=NactiIndex(f);if( nb<0 ) goto Error;
	P->vert.n=nb;
	r=NajdiZnak(f,SOPEN);if( r<0 ) goto Error;
	for( i=0; i<nb; i++ )
	{
		r=NactiIndex(f);if( r<0 ) goto Error;
		P->vert.vs[i].point=r;
	}
	r=NajdiZnak(f,SCLOSE);if( r<0 ) goto Error;
	r=NajdiZnak(f,SOPEN);if( r<0 ) goto Error;
	for( i=0; i<nb; i++ )
	{
		r=NactiIndex(f);if( r<0 ) goto Error;
		P->vert.vs[i].normal=r;
	}
	
	r=NajdiZnak(f,SCLOSE);if( r<0 ) goto Error;
	r=NajdiZnak(f,SCLOSE);if( r<0 ) goto Error;
	return 0;
	Error:
	return -1;
}

int ObjectData::LoadFaceCRYShadedAsc( FILE *f )
{
	int nb,i,r=NajdiZnak(f,"{}"); /* najdi zac. bodu nebo konec sekce bodu */
	FaceT *P;
	long b;
	if( r<0 ) return -1;
	if( r==CCLOSE ) return 1;
	P=NewFace();
	if( !P ) return -1;
	nb=NactiIndex(f);if( nb<0 ) goto Error;
	P->vert.n=nb;
	// skip color information
	b=NactiCRY(f);if( b<0 ) goto Error;
	r=NajdiZnak(f,SOPEN);if( r<0 ) goto Error;
	for( i=0; i<nb; i++ )
	{
		r=NactiIndex(f);if( r<0 ) goto Error;
		P->vert.vs[i].point=r;
	}
	r=NajdiZnak(f,SCLOSE);if( r<0 ) goto Error;
	r=NajdiZnak(f,SOPEN);if( r<0 ) goto Error;
	for( i=0; i<nb; i++ )
	{
		r=NactiIndex(f);if( r<0 ) goto Error;
		P->vert.vs[i].normal=r;
	}
	r=NajdiZnak(f,SCLOSE);if( r<0 ) goto Error;
	r=NajdiZnak(f,SCLOSE);if( r<0 ) goto Error;
	return 0;
	Error:
	return -1;
}

int ObjectData::LoadShadedAsc( const char *N )
{
	FILE *f=fopen(N,"r");
	if( !f ) return -1;
	
	char Buf[256];
	Flag CRYFormat=False;
	static const char MagStr[]="/* Objektiv ";
	static const char MagStrRGB[]="/* Objektiv 3/95 ";
	static const char MagStrCRY[]="/* Objektiv 9/95 ";
	if( setvbuf(f,NULL,_IOFBF,32*1024L)==EOF )
	{
		fclose(f);
		return -1;
	}
	else if( !fgetl(Buf,256,f) || strncmp(Buf,MagStr,sizeof(MagStr)-1) )
	{
		return NULL;
	}
	else if( !strncmp(Buf,MagStrRGB,sizeof(MagStrRGB)-1) )
	{
		CRYFormat=False;
	}
	else if( !strncmp(Buf,MagStrCRY,sizeof(MagStrCRY)-1) )
	{
		CRYFormat=True;
	}
	else {fclose(f);return -1;}
	
	int ret=-1;
	int r;
	/* nejprve najdi sekci s body */
	r=NajdiZnak(f,SOPEN);if( r<0 ) goto Error;
	do
	{
		r=LoadPointAsc(f);if( r<0 ) goto Error;
	} while( r==0 ) ;
	r=NajdiZnak(f,SOPEN);if( r<0 ) goto Error;
	/* najdi sekci s obarven¡mi */
	if( !CRYFormat )
	{
		//ImportN=0;
	}
	do
	{
		if( !CRYFormat )
		{
			r=LoadNormalAsc(f);if( r<0 ) goto Error;
		}
		else
		{
			r=LoadCRYNormalAsc(f);if( r<0 ) goto Error;
		}
	} while( r==0 ) ;
	/* najdi sekci s plochami */
	r=NajdiZnak(f,SOPEN);if( r<0 ) goto Error;
	do
	{
		if( !CRYFormat )
		{
			r=LoadFaceShadedAsc(f);if( r<0 ) goto Error;
		}
		else
		{
			r=LoadFaceCRYShadedAsc(f);if( r<0 ) goto Error;
		}
	} while( r==0 ) ;
	ret=0;
	Error:
	
	fclose(f);
	_dirty=false;
	_dirtySetup=false;
	return ret;
}

typedef struct
{
	char magic[4]; // "SS3D"
	int sizeVert,sizeFace,sizeNorm,sizeEdge;
} SetHead;

int ObjectData::SaveData( ostream &f, bool final )
{
	DataHeaderEx head;
	strncpy(head.magic,"SP3X",4);
	head.nPos=NPoints();
	head.nNorm=NNormals();
	head.nFace=NFaces();
	head.flags=0;
	head.version=0x099;
	head.headSize=sizeof(head);
	f.write((char *)&head,sizeof(head));
	
	int i;
	for( i=0; i<NPoints(); i++ )
	{
		DataPointEx point;
		point=Point(i);
		f.write((char *)&point,sizeof(point));
	}
	for( i=0; i<NNormals(); i++ )
	{
		DataNormal norm;
		norm=Normal(i);
		f.write((char *)&norm,sizeof(norm));
	}
	for( i=0; i<NFaces(); i++ )
	{
		DataFaceEx face=Face(i);
    	f.write((char *)&face,sizeof(face));
	}
	if( f.fail() ) return -1;
	return 0;
}

void Selection::Save( ostream &f )
{
    Validate();
    Temp<byte> vertW(vertSel.Size());
    for( int i=0; i<vertSel.Size(); i++ ) vertW[i]=-vertSel[i]; // make 0xff from 1
	f.write((char *)&vertW[0],vertSel.Size()*sizeof(byte));
	f.write((char *)faceSel.Data(),faceSel.Size()*sizeof(bool));
	//f.write((char *)norm,sizeNorm);
}
void Selection::Load( istream &f, int sizeVert, int sizeFace, int sizeNorm )
{
    int n=sizeVert;
    Temp<byte> vertW(n);
    if( sizeVert>0 )
    {
	    f.read((char *)&vertW[0],sizeVert);
    }
    ValidateFaces(sizeFace);
    ValidatePoints(sizeVert);
    if( sizeFace>0 )
    {
	    f.read((char *)faceSel.Data(),sizeFace);
    }
    for( int i=0; i<n; i++ ) vertSel[i]=-vertW[i]; // make 0xff from 1
	f.seekg(sizeNorm,ios::cur);
}


void Selection::operator += ( const Selection &src )
{
	int i;
    Validate();
	for( i=0; i<_object->NPoints(); i++ )
    {
        int vi=vertSel[i]+src.vertSel[i];
        if( vi>255 ) vi=255;
        vertSel[i]=vi;
    }
	for( i=0; i<_object->NFaces(); i++ ) faceSel[i]=faceSel[i] || src.faceSel[i];
}
void Selection::operator -= ( const Selection &src )
{
	int i;
    Validate();
	for( i=0; i<_object->NPoints(); i++ )
    {
        int vi=vertSel[i]-src.vertSel[i];
        if( vi<0 ) vi=0;
        vertSel[i]=vi;
    }
	for( i=0; i<_object->NFaces(); i++ ) faceSel[i]=faceSel[i] && !src.faceSel[i];
}

void ObjectData::SaveTag( ostream &f, const char *tag, int size )
{
	char tagBuf[64];
	strncpy(tagBuf,tag,sizeof(tagBuf));
	f.write(tagBuf,sizeof(tagBuf));
	f.write((char *)&size,sizeof(size));
}

WString ObjectData::LoadTag( istream &f, int &size )
{ // load sized tag
	char tagBuf[64+1];
	f.read(tagBuf,64);
	tagBuf[64]=0;
	f.read((char *)&size,sizeof(size));
	return tagBuf;
}

bool ObjectData::FindTag( istream &f, const char *name, int &size )
{
	int tagSize;
	for(;;)
	{
		WString tag=LoadTag(f,tagSize);
		if( f.fail() || f.eof() ) return false;
		if( !strcmpi(tag,name) )
		{
			size=tagSize;
			return true;
		}
		else f.seekg(tagSize,ios::cur);
	}
}

static WString LoadOldTag( istream &f )
{ // load old tag
	char tagBuf[64+1];
	f.read(tagBuf,64);
	tagBuf[64]=0;
	return tagBuf;
}

int ObjectData::SaveSetup( ostream &f, bool final, bool mass )
{
	f.write("TAGG",4);
	
	int selSize=NPoints()*sizeof(bool)+NFaces()*sizeof(bool);
	
	int edgeSize=_sharpEdge.Size()*sizeof(int)*2;
    // consider skip sharp edges in finalized object
    //if( !final )
    {
    	SaveTag(f,"#SharpEdges#",edgeSize);
	    f.write((char *)_sharpEdge.Data(),edgeSize);
    }
	
	if( !_sel.IsEmpty() && !final )
	{
		SaveTag(f,"#Selected#",selSize);
		_sel.Save(f);
	}
	
	int i;
	for( i=0; i<MAX_NAMED_SEL; i++ ) if( _namedSel[i] )
	{
        if( final && _namedSel[i]->Name()[0]=='.' ) continue;
        if( final && _namedSel[i]->Name()[0]=='-' ) continue;
		SaveTag(f,_namedSel[i]->Name(),selSize);
		_namedSel[i]->Save(f);
	}
	for( i=0; i<MAX_NAMED_PROP; i++ ) if( _namedProp[i] )
	{
        if( final && _namedProp[i]->Name()[0]=='.' ) continue;
        if( final && _namedProp[i]->Name()[0]=='-' ) continue;
		SaveTag(f,"#Property#",sizeof(*_namedProp[i]));
		f.write((char *)_namedProp[i],sizeof(*_namedProp[i]));
	}
	
	if( f.fail() ) return -1;
	
	if( !_lock.IsEmpty()  && !final )
	{
		SaveTag(f,"#Lock#",selSize);
		_lock.Save(f);
	}
	
	if( !_hide.IsEmpty()  && !final )
	{
		SaveTag(f,"#Hide#",selSize);
		_hide.Save(f);
	}
	
	if( mass )
	{
		int massSize=NPoints()*sizeof(float);
		SaveTag(f,"#Mass#",massSize);
		_mass.Save(f);
	}
/*FLY
	for( i=0; i<NAnimations(); i++ )
	{
		float time=_phase[i]->GetTime();
		int taggSize=sizeof(time)+sizeof(Point3D)*NPoints();
		SaveTag(f,"#Animation#",taggSize);
		f.write((char *)&time,sizeof(time));
		_phase[i]->Save(f);
	}
*/
	SaveTag(f,"#EndOfFile#",0);
	
	if( f.fail() ) return -1;
	return 0;
}

int ObjectData::SaveCHeader( ostream &f )
{
	int i;
	for( i=0; i<MAX_NAMED_SEL; i++ ) if( _namedSel[i] )
	{
	}
	if( f.fail() ) return -1;
	return 0;
}
int ObjectData::SaveCHeader( WFilePath filename )
{
	// save in binary Poseidon format
	ofstream f(filename/*FLY,ios::text|ios::out*/);
	return SaveCHeader(f);
}

int ObjectData::SaveBinary( ostream &f, bool final, bool mass )
{
	int ret=-1;
	if( _autoSaveAnimation>=0 ) RedefineAnimation(_autoSaveAnimation);
	ret=SaveData(f,final);
	if( ret==0 ) ret=SaveSetup(f,final,mass);
	// do not clear dirty - save may be export or save for object viewer ...
	return ret;
}

int ObjectData::Save( WFilePath filename, bool final, bool mass )
{
  CleanNormals();
	ofstream f(filename,ios::binary|ios::out);
  return SaveBinary(f,final,mass);

/*FLY
	// save in binary Poseidon format
    CleanNormals();
	WString ext=filename.GetExtension();
	if( !strcmpi(ext,".asc") )
	{
		return SaveGenAsc(filename);
	}
	ofstream f(filename,ios::binary|ios::out);
	return SaveBinary(f,final,mass);
*/
}

int ObjectData::LoadData( istream &f )
{
	DoDestruct();
	DoConstruct();
	
	DataHeaderEx head;
	bool extended=true;
	f.read((char *)&head,sizeof(head));
	if( strncmp(head.magic,"SP3X",4) )
	{
		extended=false;
		DataHeader oHead;
		f.seekg(-sizeof(head),ios::cur);
		f.read((char *)&oHead,sizeof(oHead));
		if( strncmp(head.magic,"SP3D",4) ) return -1; // file input error
		head.nPos=oHead.nPos;
		head.nNorm=oHead.nNorm;
		head.nFace=oHead.nFace;
		head.version=0;
		head.headSize=sizeof(oHead);
	}
	else
	{
		f.seekg(head.headSize-sizeof(head),ios::cur);
	}
	_points.Realloc(head.nPos);
	_points.Resize(head.nPos);
	_normals.Realloc(head.nNorm);
	_normals.Resize(head.nNorm);
	_faces.Realloc(head.nFace);
	_faces.Resize(head.nFace);
	
	int i;
	for( i=0; i<NPoints(); i++ )
	{
		if( !extended )
		{
			DataPoint point;
			f.read((char *)&point,sizeof(point));
			if( f.fail() ) break;
            //Assert( point.Size()>0 );
			Point(i).SetPoint(point);
			Point(i).flags=0;
		}
		else
		{
			DataPointEx point;
			f.read((char *)&point,sizeof(point));
			if( f.fail() ) break;
			Point(i)=point;
            //Assert( point.Size()>0 );
		}
	}
	for( i=0; i<NNormals(); i++ )
	{
		DataNormal norm;
		f.read((char *)&norm,sizeof(norm));
		if( f.fail() ) break;
		Normal(i)=norm;
	}
	for( i=0; i<NFaces(); i++ )
	{
		DataFaceEx face;
		if( !extended )
		{
			DataFace oFace;
			f.read((char *)&oFace,sizeof(oFace));
			if( f.fail() ) break;
			(DataFace)face=oFace;
			face.flags=0;
		}
		else
		{
			f.read((char *)&face,sizeof(face));
			if( f.fail() ) break;
		}
		Face(i)=face;
	}
	if( f.fail() ) return -1;
	return 0;
}

int ObjectData::LoadSetup( istream &f )
{
	_dirtySetup=false;
	char tagMagic[4];
	f.read(tagMagic,sizeof(tagMagic));
	if( strncmp(tagMagic,"TAGG",4) )
	{
		if( f.fail() || f.eof() ) return true; // no setup available
		SetHead setHead;
		f.seekg(-4,ios::cur);
		f.read((char *)&setHead,sizeof(setHead));
		if // check if it is old (non-tagged) format
		(
			f.fail()
            || strncmp(setHead.magic,"SS3D",4)
		)
		{
			return -1;
		}
		_sel.Load(f,setHead.sizeVert,setHead.sizeFace,setHead.sizeNorm);
		int nSEdges=setHead.sizeEdge/sizeof(SharpEdge);
        _sharpEdge.Resize(nSEdges);
		f.read((char *)_sharpEdge.Data(),setHead.sizeEdge);

		_mass.Clear(); // default mass

		int i;
		for( i=0; ;)
		{
			WString nameSel=LoadOldTag(f);
			if( f.eof() || f.fail() ) return 0;
			if( nameSel[0]=='$' ) // some tag
			{  // old tags - no size informations
				if( !strcmpi(nameSel,"$Lock$") )
				{
					_lock.Load(f,setHead.sizeVert,setHead.sizeFace,setHead.sizeNorm);
				}
				else if( !strcmpi(nameSel,"$Hide$") )
				{
					_hide.Load(f,setHead.sizeVert,setHead.sizeFace,setHead.sizeNorm);
				}
				else if( !strcmpi(nameSel,"$Mass$" ) ) _mass.Load(f,NPoints());
				else if( !strcmpi(nameSel,"$EndOfFile$" ) ) return 0;
			}
			else
			{ // no tags - it must be selection
				if( i<MAX_NAMED_SEL )
				{
					_namedSel[i]=new NamedSelection(this,nameSel);
					_namedSel[i]->Load(f,setHead.sizeVert,setHead.sizeFace,setHead.sizeNorm);
					i++;
				}
			}
		} // end of non-tagged read
	}
	else
	{ // start tagged read
		// tagged format - we can skip any unsupported tags
		int i;
		int sizeVert=NPoints()*sizeof(byte);
		int sizeFace=NFaces()*sizeof(bool);
		for( i=0; ;)
		{
			int tagSize;
			WString nameSel=LoadTag(f,tagSize);
			if( f.eof() || f.fail() ) return -1;
			if( !strcmpi(nameSel,"#Selected#") )
			{
				//WVERIFY( sizeVert+sizeFace==tagSize );
				_sel.Load(f,sizeVert,sizeFace,0);
			}
			else if( !strcmpi(nameSel,"#SharpEdges#") )
			{
				int nSEdges=tagSize/(sizeof(SharpEdge));
                _sharpEdge.Resize(nSEdges);
				f.read((char *)_sharpEdge.Data(),tagSize);
                // normalize:
                for( int i=0; i<_sharpEdge.Size(); i++ )
                {
                    SharpEdge &edge=_sharpEdge[i];
                    if( edge[0]>edge[1] ) ::swap(edge[0],edge[1]);
                }
                SortSharpEdges();
			}
			else if( !strcmpi(nameSel,"#Lock#") )
			{
				//WVERIFY( sizeVert+sizeFace==tagSize );
				_lock.Load(f,sizeVert,sizeFace,0);
			}
			else if( !strcmpi(nameSel,"#Hide#") )
			{
				//WVERIFY( sizeVert+sizeFace==tagSize );
				_hide.Load(f,sizeVert,sizeFace,0);
			}
			else if( !strcmpi(nameSel,"#Mass#" ) )
			{
				//WVERIFY(nPoints*sizeof(float)== tagSize );
				_mass.Load(f,NPoints());
			}
			else if( !strcmpi(nameSel,"#Property#" ) )
			{
				NamedProperty prop("","");
				//WVERIFY(sizeof(prop)== tagSize );
				f.read((char *)&prop,sizeof(prop));
				SetNamedProp(prop.Name(),prop.Value());
			}
			else if( !strcmpi(nameSel,"#Animation#" ) )
			{
				AnimationPhase anim(this);
				float time;
				f.read((char *)&time,sizeof(time));
				anim.SetTime(time);
				anim.Load(f,NPoints());
				AddAnimation(anim);
			}
			else if( !strcmpi(nameSel,"#EndOfFile#" ) )
			{
				f.seekg(tagSize,ios::cur);
				LoadFinished();
				return 0;
			}
			else if( nameSel[0]!='#' )
			{ // no special tag - it must be selection
				//WVERIFY( sizeVert+sizeFace==tagSize );
				if( i<MAX_NAMED_SEL )
				{
					_namedSel[i]=new NamedSelection(this,nameSel);
					_namedSel[i]->Load(f,sizeVert,sizeFace,0);
					i++;
				}
			}
			else // some unsupported tag
			{
				f.seekg(tagSize,ios::cur);
				if( f.fail() ) return -1;
			}
			if( f.fail() ) return -1;
		}
	}
	
}

int ObjectData::LoadFinished()
{
/*FLY
	if( _phase.Size()>0 )
	{
		int index=AnimationIndex(0.0);
		if( index<0 ) index=0;
		UseAnimation(index);
	}
*/
	return 0;
}

int ObjectData::LoadBinary( istream &f )
{
	// load binary Poseidon format
	//FLYint ret=-1;
	
	if( LoadData(f)<0 ) return -1;
	
	if( LoadSetup(f)<0 )
	{
		return -1;
	}
	
	_dirty=false;
	_dirtySetup=false;
	return 0;
}

int ObjectData::LoadBinary( WFilePath filename )
{
	ifstream f;
	f.open(filename,ios::binary|ios::in);
	if( f.fail() ) return -1;
	return LoadBinary(f);
}


int ObjectData::Load( WFilePath filename )
{
  return LoadBinary(filename);
/*FLY
	if( !strcmpi(filename.GetExtension(),".c") ) return LoadShadedAsc(filename);
	if( !strcmpi(filename.GetExtension(),".asc") ) return LoadGenAsc(filename);
	if( !strcmpi(filename.GetExtension(),".pst") ) return LoadStars(filename);
	return LoadBinary(filename);
*/
}

/*FLY
void FailHook( const char *text )
{
	WDebug::BreakIntoDebugger(text,false);
}
*/
/*FLY
void LogF( const char *format, ... )
{
	char buf[512];
		
	va_list arglist;
	va_start( arglist, format );

	vsprintf(buf,format,arglist);
    
	va_end( arglist );
    
	//FLYWDebug::Printf("%s\n",buf);
}

void ErrF( const char *format, ... )
{
	char buf[512];
		
	va_list arglist;
	va_start( arglist, format );

	vsprintf(buf,format,arglist);
    
	va_end( arglist );
    
	//FLYWDebug::Printf("%s\n",buf);
}
*/
struct Star
{
	float x,y,z,brightness;
};
int ObjectData::LoadStars( istream &in )
{
	DoDestruct();
	DoConstruct();
	
	int i;
	int n;
	in.read((char *)&n,sizeof(n));
	if( n>OMaxPoints ) n=OMaxPoints;
    _points.Realloc(n);
    _points.Resize(n);
	for( i=0; i<n; i++ )
	{
		Star temp;
		in.read((char *)&temp,sizeof(temp));
		if( in.eof() || in.fail() ) break;
		int user=temp.brightness*255;
		if( user<0 ) user=0;if( user>255 ) user=255;
		Point(i)[0]=temp.x;
		Point(i)[1]=temp.y;
		Point(i)[2]=temp.z;
		Point(i).flags=user*POINT_USER_STEP;
	}
	return 0;
}

int ObjectData::LoadStars( WFilePath path )
{
	ifstream f;
	f.open(path,ios::binary|ios::in);
	return LoadStars(f);
}

