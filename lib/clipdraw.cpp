// Horizont - clipping and draw encapsulation
// (C) 1997, SUMA
#include "wpch.hpp"

#ifdef _MSC_VER
	// assume no aliasing
	#pragma optimize("a",on)
#endif

#include "global.hpp"
#include "types.hpp"
#include "Shape/poly.hpp"
#include "tlVertex.hpp"
#include "scene.hpp"
#include "lights.hpp"
#include "camera.hpp"
#include "landscape.hpp"
#include "engine.hpp"
#include "Shape/plane.hpp"


static int Interpolate
(
	TLVertexTable &tlMesh,
	int in, int out, Coord t, const Camera &camera
)
{
	int nTIndex=tlMesh.AddPos(); // can change pointers to TransPos
	Vector3Val iPos=tlMesh.TransPosA(in);
	Vector3Val oPos=tlMesh.TransPosA(out);
	ClipFlags iClip=tlMesh.Clip(in);
	ClipFlags oClip=tlMesh.Clip(out);
	// interpolate vertex user data
	ClipFlags hints=iClip&oClip&ClipHints;
	// interpolate vertex position
	float it = 1-t;
	//Vector3 &point=tlMesh.SetTransPos(nTIndex);
	Vector3 point = iPos*it+oPos*t;
	tlMesh.SetTransPosA(nTIndex,point);
	tlMesh.SetClip(nTIndex,NeedsClipping(point,camera)|hints);
	// interpolate vertex data
	const TLVertex &iVert = tlMesh.GetVertex(in);
	const TLVertex &oVert = tlMesh.GetVertex(out);
	TLVertex &d = tlMesh.SetVertex(nTIndex);
	// conversion necessary from/to PackedColor
	Color inColor((ColorVal)iVert.color),inSpecu((ColorVal)iVert.specular);
	Color outColor((ColorVal)oVert.color),outSpecu((ColorVal)oVert.specular);
	d.t0.u=iVert.t0.u*it+oVert.t0.u*t;
	d.t0.v=iVert.t0.v*it+oVert.t0.v*t;
	d.t1.u=iVert.t1.u*it+oVert.t1.u*t;
	d.t1.v=iVert.t1.v*it+oVert.t1.v*t;
	d.color=PackedColor(inColor*it+outColor*t);
	d.specular=PackedColor(inSpecu*it+outSpecu*t);
	return nTIndex;
}
static int Interpolate( TLVertexTable &tlMesh, int in, int out, Coord t )
{
	int nTIndex=tlMesh.AddPos(); // can change pointers to TransPos
	Vector3Val iPos=tlMesh.TransPosA(in);
	Vector3Val oPos=tlMesh.TransPosA(out);
	ClipFlags iClip=tlMesh.Clip(in);
	ClipFlags oClip=tlMesh.Clip(out);
	// interpolate vertex user data
	ClipFlags hints=iClip&oClip&ClipHints;
	// interpolate vertex position
	float it = 1-t;
	tlMesh.SetTransPosA(nTIndex,iPos*it+oPos*t);
	tlMesh.SetClip(nTIndex,hints);

	const TLVertex &iVert = tlMesh.GetVertex(in);
	const TLVertex &oVert = tlMesh.GetVertex(out);
	TLVertex &d = tlMesh.SetVertex(nTIndex);
	// conversion necessary from/to PackedColor
	Color inColor((ColorVal)iVert.color),inSpecu((ColorVal)iVert.specular);
	Color outColor((ColorVal)oVert.color),outSpecu((ColorVal)oVert.specular);
	d.t0.u=iVert.t0.u*it+oVert.t0.u*t;
	d.t0.v=iVert.t0.v*it+oVert.t0.v*t;
	d.t1.u=iVert.t1.u*it+oVert.t1.u*t;
	d.t1.v=iVert.t1.v*it+oVert.t1.v*t;
	d.color=PackedColor(inColor*it+outColor*t);
	d.specular=PackedColor(inSpecu*it+outSpecu*t);

	return nTIndex;
}

#pragma warning(disable:68)

int Interpolate(Shape &mesh, int in, int out, Coord t, bool quickAdd=false);

bool PolyVertices::InsideFromTop
(
	const Array<Vector3> &posArray, const Plane &plane, Vector3Par pos,
	float *y, float *dX, float *dZ
) const
{
	if( _n<3 ) return false;
	// perform tests from above
	int i;
	Vector3 lPos = posArray[_vertex[_n-1]];
	for( i=0; i<_n; i++ )
	{
		Vector3 aPos = posArray[_vertex[i]];
		// half-plane equation has form ax+by+c<=0
		// (a,b) is line normal
		//Vector3 aMinusL=aPos-lPos;
		// note: Visual C++ removes multiply by Y==0
		Vector3 lineNormal(lPos.Z()-aPos.Z(),0,aPos.X()-lPos.X());
		float checkIn=lineNormal*(pos-aPos);
		if( checkIn>0 ) return false;
		lPos=aPos;
	}

	if( y )
	{
		// planar approach:
		// use face plane equation
		Vector3Val p0 = posArray[_vertex[0]];
		Vector3Val normal=plane.Normal();
		// c=-normal*p0
		// equation is n*p+c=0
		// nx*px+ny*py+nz*pz+c=0
		// nx*px+nz*pz+c=-ny*py
		// py=(-c-nx*px-nz*pz)/ny
		if( fabs(normal.Y())<1e-2 ) return false; // avoid some kind of faces
		float invNY=1/normal.Y();
		float yRes=(normal*p0-normal.X()*pos.X()-normal.Z()*pos.Z())*invNY;
		*y=yRes;
		// calculate dX and dZ
		if( dX ) *dX=-normal.X()*invNY;
		if( dZ ) *dZ=-normal.Z()*invNY;
	}
	return true;
}


#if _VBS3 //used for new roads
bool PolyVertices::InsideFromBottom
(
  const Array<Vector3> &posArray, const Plane &plane, Vector3Par pos,
  float *y, float *dX, float *dZ
) const
{
  if( _n<3 ) return false;
  // perform tests from above
  int i;
  const Vector3 *lPos=&posArray[_vertex[_n-1]];
  for( i=0; i<_n; i++ )
  {
    const Vector3 *aPos=&posArray[_vertex[i]];
    // half-plane equation has form ax+by+c<=0
    // (a,b) is line normal
    //Vector3 aMinusL=*aPos-*lPos;
    // note: Visual C++ removes multiply by Y==0
    Vector3 lineNormal(lPos->Z() - aPos->Z(),0,aPos->X()-lPos->X());
    float checkIn=lineNormal*(pos - *aPos);
    if( checkIn<0 ) return false;
    lPos=aPos;
  }
  if( y )
  {
    // planar approach:
    // use face plane equation
    Vector3Val p0=posArray[_vertex[0]];
    Vector3Val normal=plane.Normal();
    // c=-normal*p0
    // equation is n*p+c=0
    // nx*px+ny*py+nz*pz+c=0
    // nx*px+nz*pz+c=-ny*py
    // py=(-c-nx*px-nz*pz)/ny
    if( fabs(normal.Y())<1e-2 ) return false; // avoid some kind of faces
    float invNY=1/normal.Y();
    float yRes=(normal*p0-normal.X()*pos.X()-normal.Z()*pos.Z())*invNY;
    *y=yRes;
    // calculate dX and dZ
    if( dX ) *dX=-normal.X()*invNY;
    if( dZ ) *dZ=-normal.Z()*invNY;
  }
  return true;
}
#endif
float PolyVertices::DistanceFromTop
(
	const VertexTable &mesh, Vector3Par pos
) const
{
	// note: returns Square(distance)
	if( _n<3 ) return 1e10;
	int i;
	// then perform exact check
	Vector3 lPos=mesh.Pos(_vertex[_n-1]);
	//Assert( fabs(tPos.Z()-lPos.Z())<1e-1 );
	bool orIn=false;
	bool andIn=true;
	for( i=0; i<_n; i++ )
	{
		Vector3 aPos=mesh.Pos(_vertex[i]);
		// half-plane equation has form ax+by+c<=0
		// (a,b) is line normal

		Vector3 lineNormal(aPos.Z()-lPos.Z(),0,lPos.X()-aPos.X());
		float checkIn=lineNormal*(pos-aPos);
		if( checkIn<0 ) andIn=false;
		else orIn=true;
		lPos=aPos;
	}
	if( orIn==andIn )
	{
		DoAssert( orIn ); // it cannot be outside of all half-spaces
		// is in all half spaces or out of all half spaces
		// this can only mean it is inside (with no respect to face orientation)
		return 0;
	}
	else
	{
		// is outside - distance is distance to the nearest vertex
		float minDist2=1e30;
		for( i=0; i<_n; i++ )
		{
			Vector3Val aPos=mesh.Pos(_vertex[i]);
			float dist2=(aPos-pos).SquareSizeXZ();
			saturateMin(minDist2,dist2);
		}
		return minDist2;
	}
}

bool PolyVertices::Inside(Vector3Par dir, const Array<Vector3> &posArray, const Plane &plane, Vector3Par pos, float *under) const
{
	if( _n<3 ) return false;

	// perform all tests in the space given by dir
	// use cross product with face normal to get line normal in the dir space
	Vector3 lPos=posArray[_vertex[_n-1]];
	for (int i=0; i<_n; i++)
	{
		Vector3 aPos=posArray[_vertex[i]];
		// half-plane equation has form ax+by+c<=0
		// (a,b) is line normal
		Vector3Val aml = (aPos-lPos);
		Vector3 lineNormal=aml.CrossProduct(dir);
		float checkIn=lineNormal*(pos-aPos);
		if( checkIn>0 ) return false;
		lPos=aPos;
	}

	if (under)
	{
		// planar approach:
		// use face plane equation
		// find intersection of plane with (pos,dir) line
		// plane.n*x+plane.d = 0, x = pos+dir*t
		// plane.n*(pos+dir*t)+plane.d = 0
		// plane.n*pos+plane.n*dir*t = -plane.d
		// t = (-plane.d-plane.n*pos)/plane.n*dir
		float denom = plane.Normal()*dir;
		if (fabs(denom)<1e-2)
		{
		  LogF("Singular inside test");
	return false;
}
		float t = (-plane.D()-plane.Normal()*pos)/denom;
		Assert(plane.Distance(pos+dir*t)<1e-3f);
		*under = t;
	}

	return true;
}

float PolyVertices::SquareDistance(
	const VertexTable &mesh, const Plane &plane, Vector3Par pos, Vector3 *normal
) const
{
	if( _n<3 ) return false;
	if( normal ) *normal=plane.Normal();
	// perform all tests in face plane space
	// use cross product with face normal to get line normal in face plane
	int i;
	// then perform exact check
	Vector3 lPos=mesh.Pos(_vertex[_n-1]);
	//Assert( fabs(tPos.Z()-lPos.Z())<1e-1 );
	bool orIn=false;
	bool andIn=true;
	for( i=0; i<_n; i++ )
	{
		Vector3 aPos=mesh.Pos(_vertex[i]);
		// half-plane equation has form ax+by+c<=0
		// (a,b) is line normal
		Vector3Val aml = (aPos-lPos);
		Vector3 lineNormal=aml.CrossProduct(plane.Normal());
		float checkIn=lineNormal*(pos-aPos);
		if( checkIn<0 ) andIn=false;
		else orIn=true;
		lPos=aPos;
	}
	if( orIn==andIn )
	{
		DoAssert( orIn ); // it cannot be outside of all half-spaces
		// is in all half spaces or out of all half spaces
		// this can only mean it is inside (with no respect to face orientation)
		//note: Normal() is normalized
		Vector3Val p0=mesh.Pos(_vertex[0]);
		return Square(plane.Normal()*(pos-p0));
	}
	else
	{
		// is outside - distance is distance to the nearest vertex
		float minDist2=1e30;
		for( i=0; i<_n; i++ )
		{
			Vector3Val aPos=mesh.Pos(_vertex[i]);
			float dist2=aPos.Distance2(pos);
			if( minDist2>dist2 ) minDist2=dist2;
		}
		return minDist2;
	}
}

inline int Dominance( Vector3Par a, Vector3Par b )
{
	float d;
	d=a[2]-b[2];
	if( d<0 ) return -1;
	if( d>0 ) return +1;
	d=a[1]-b[1];
	if( d<0 ) return -1;
	if( d>0 ) return +1;
	d=a[0]-b[0];
	if( d<0 ) return -1;
	if( d>0 ) return +1;
	return 0;
}

/**
@return return the position of the intesection (in terms of "t")
Say return value is t. Then the intersection point is: 
   Pt = in+(out-in)*t = out*t+in*(1-t) = lerp(in,out,t)
*/
inline Coord Intersect( Vector3Par in, Vector3Par out, Vector3Par normal, Coord d )
{
	Vector3 AmB=in-out;
	return ( in*normal+d ) / ( AmB*normal );
}

bool PolyVertices::Clip
(
	TLVertexTable &mesh,
	PolyVertices &res, Vector3Par normal, Coord d, ClipFlags test, const Camera &camera
)
{
	// initialize resulting polygon
	if( _n<3 ) {res._n=0;return true;}

	// search for first vertex inside clipping half-space
	int i;
	VertexIndex *pVertex; // previous vertex
	VertexIndex *aVertex; // actual vertex

	bool change=false;
	pVertex=_vertex+_n-1;
	ClipFlags pOut=mesh.Clip(*pVertex)&test;
	
	int nClipped=0;
	
	for( i=0; i<_n; i++ )
	{
		aVertex=_vertex+i;
		ClipFlags aOut=mesh.Clip(*aVertex)&test;
		// four possible situations
		if( aOut!=pOut )
		{
			// edge going in or out
			int a=*aVertex;
			int p=*pVertex;
			Vector3 aPos=mesh.TransPosA(a);
			Vector3 pPos=mesh.TransPosA(p);
			if( Dominance(aPos,pPos)<0 )
			{ // this should guarantee same rounding error on clipping
				Swap(a,p);
				Swap(aPos,pPos);
			}
			Coord t=Intersect(aPos,pPos,normal,d);
			res._vertex[nClipped++]=Interpolate(mesh,a,p,t,camera);
			change=true;
		}
		if( !aOut )
		{
			// point in
			res._vertex[nClipped++]=*aVertex;
		}
		else change=true;
		pVertex=aVertex;
		pOut=aOut;
	}

	Assert( nClipped<=MaxPoly );
	if( nClipped<3 ) res._n=0; // polygon completely out
	else res._n=nClipped;
	return change;
}

// some operations require that inside test is performed, not pre-calculated
bool PolyVertices::Clip
(
	TLVertexTable &mesh, PolyVertices &res, Vector3Par normal, Coord d
)
{
	// initialize resulting polygon
	if( _n<3 ) {res._n=0;return true;}
	
	// search for first vertex inside clipping half-space
	int i;
	VertexIndex *pVertex; // previous vertex
	VertexIndex *aVertex; // actual vertex
	
	pVertex=_vertex+_n-1;
	bool pOut=normal*mesh.TransPosA(*pVertex)+d<0;
	
	int nClipped=0;
	bool change=false;

	for( i=0; i<_n; i++ )
	{
		aVertex=_vertex+i;
		bool aOut=normal*mesh.TransPosA(*aVertex)+d<0;
		// four possible situations
		if( aOut!=pOut )
		{
			// edge going in or out
			int a=*aVertex;
			int p=*pVertex;
			Vector3 aPos=mesh.TransPosA(a);
			Vector3 pPos=mesh.TransPosA(p);
			if( Dominance(aPos,pPos)<0 )
			{ // this should guarantee same rounding error on clipping
				Swap(a,p);
				Swap(aPos,pPos);
			}
			Coord t=Intersect(aPos,pPos,normal,d);
			// note: aPos and pPos may become invalid during Interpolate
			res._vertex[nClipped++]=Interpolate(mesh,a,p,t);
			change=true;
		}
		if( !aOut )
		{
			// point in
			res._vertex[nClipped++]=*aVertex;
		}
		else change=true;
		pVertex=aVertex;
		pOut=aOut;
	}

	Assert( nClipped<=MaxPoly );
	if( nClipped<3 ) res._n=0; // polygon completely out
	else res._n=nClipped;
	return change;
}

void PolyVertices::Clip( TLVertexTable &mesh, const Camera &camera, ClipFlags clipFlags )
{
	int i;
	// this function should be called only if neccessary
	Assert( clipFlags );
	ClipFlags clipOr=0,clipAnd=clipFlags;
	for( i=0; i<_n; i++ )
	{
		ClipFlags clip=mesh.Clip(_vertex[i]);
		clipOr|=clip;
		clipAnd&=clip;
	}
	if( !clipOr ) return;
	if( clipAnd ) {_n=0;return;}
	clipFlags&=clipOr;

	PolyVertices tempResult;
	PolyVertices *source=this;
	PolyVertices *result=&tempResult;
	if( clipFlags&ClipFront )
	{
		if( source->Clip(mesh,*result,Vector3(0,0,+1),-camera.ClipNear(),ClipFront,camera ) )
		{
			Swap(source,result);
		}
	}
	if( clipFlags&ClipBack )
	{
		if( source->Clip(mesh,*result,Vector3(0,0,-1),camera.ClipFar(),ClipBack,camera) )
		{
			Swap(source,result);
		}
	}
	
	if( clipFlags&ClipLeft )
	{
		if( source->Clip(mesh,*result,Vector3(+1,0,+1),0,ClipLeft,camera) )
		{
			Swap(source,result);
		}
	}
	if( clipFlags&ClipRight )
	{
		if( source->Clip(mesh,*result,Vector3(-1,0,+1),0,ClipRight,camera) )
		{
			Swap(source,result);
		}
	}

	if( clipFlags&ClipTop )
	{
		if( source->Clip(mesh,*result,Vector3(0,+1,+1),0,ClipTop,camera) )
		{
			Swap(source,result);
		}
	}
	if( clipFlags&ClipBottom )
	{
		if( source->Clip(mesh,*result,Vector3(0,-1,+1),0,ClipBottom,camera) )
		{
			Swap(source,result);
		}
	}

  /*
	if( clipFlags&ClipUser0 )
	{
		// we perform user clipping after view frustum clipping
		// therefore we are sure no more view frustum clipping will occur
		// and we may safely use Clip without camera argument
		if( source->Clip(mesh,*result,camera.UserClipDir(),camera.UserClipVal(),ClipUser0,camera) )
		{
			Swap(source,result);
		}
	}
	*/

	if( source!=this ) *this=*source;

	Assert( _n<=MaxPoly );
	// Due to rounding errors it might seem that the new vertex must be clipped.
	// We are sure it is clipped - clear clipping flags
	// note: we cannot use SetTransPos, because it invalidates ScreenPos
	for( i=0; i<_n; i++ )
	{
		int index=_vertex[i];
		mesh.SetClip(index,mesh.Clip(index)&~ClipAll);
	}
}

void PolyVertices::CheckClip( TLVertexTable &mesh, const Camera &camera, ClipFlags clipFlags )
{
	PolyVertices tempResult;
	PolyVertices *source=this;
	PolyVertices *result=&tempResult;
	if( clipFlags&ClipFront )
	{
		if( source->Clip(mesh,*result,Vector3(0,0,+1),-camera.ClipNear()) )
		{
			Swap(source,result);
		}
	}
	if( clipFlags&ClipBack )
	{
		if( source->Clip(mesh,*result,Vector3(0,0,-1),camera.ClipFar()) )
		{
			Swap(source,result);
		}
	}
	
	if( clipFlags&ClipLeft )
	{
		if( source->Clip(mesh,*result,Vector3(+1,0,+1),0) )
		{
			Swap(source,result);
		}
	}
	if( clipFlags&ClipRight )
	{
		if( source->Clip(mesh,*result,Vector3(-1,0,+1),0) )
		{
			Swap(source,result);
		}
	}
	
	if( clipFlags&ClipTop )
	{
		if( source->Clip(mesh,*result,Vector3(0,+1,+1),0) )
		{
			Swap(source,result);
		}
	}
	if( clipFlags&ClipBottom )
	{
		if( source->Clip(mesh,*result,Vector3(0,-1,+1),0) )
		{
			Swap(source,result);
		}
	}

	/*	
	if( clipFlags&ClipUser0 )
	{
		if( source->Clip(mesh,*result,camera.UserClipDir(),camera.UserClipVal()) )
		{
			Swap(source,result);
		}
	}
	*/

	// result may be left in temporary
	if( source!=this ) *this=*source;

	if( _n<3 )
	{
		Assert( _n==0 );
		return;
	}

	// Due to rounding errors it might seem that the new vertex must be clipped.
	// We are sure it is clipped - clear clipping flags
	for( int i=0; i<_n; i++ )
	{
		int index=_vertex[i];
		mesh.SetClip(index,mesh.Clip(index)&~ClipAll);
		#if _DEBUG
			// verify vertex is within clipping range
			const Vector3 &v=mesh.TransPosA(index);
			Assert( v*Vector3(0,0,+1)-camera.ClipNear()>=-1 );
			Assert( v*Vector3(0,0,-1)+camera.ClipFar()>=-1 );
			Assert( v*Vector3(+1,0,+1)>=-1 );
			Assert( v*Vector3(-1,0,+1)>=-1 );
			Assert( v*Vector3(0,+1,+1)>=-1 );
			Assert( v*Vector3(0,-1,+1)>=-1 );
		#endif
	}
}

// some operations require that inside test is performed, not pre-calculated
void PolyVertices::Split
(
	TLVertexTable &mesh, PolyVertices &clip, PolyVertices &rest,
	Vector3Par normal, Coord d
)
{
	// initialize resulting polygon
	if( _n<3 ) {rest._n=clip._n=0;return;}
	
	// search for first vertex inside clipping half-space
	int i;
	VertexIndex *pVertex; // previous vertex
	VertexIndex *aVertex; // actual vertex
	
	pVertex=_vertex+_n-1;
	bool pOut=normal*mesh.TransPosA(*pVertex)+d<0;
	
	int nClipped=0;
	int nRest=0;
	
	for( i=0; i<_n; i++ )
	{
		aVertex=_vertex+i;
		bool aOut=normal*mesh.TransPosA(*aVertex)+d<0;
		// four possible situations
		if( aOut!=pOut )
		{
			// edge going in or out
			Coord t=Intersect(mesh.TransPosA(*aVertex),mesh.TransPosA(*pVertex),normal,d);
			rest._vertex[nRest++]=clip._vertex[nClipped++]=Interpolate(mesh,*aVertex,*pVertex,t);
		}
		if( !aOut ) clip._vertex[nClipped++]=*aVertex; // point in
		else rest._vertex[nRest++]=*aVertex; // point out (in rest)
		pVertex=aVertex;
		pOut=aOut;
	}

	Assert( nClipped<=MaxPoly );
	Assert( nRest<=MaxPoly );
	
	if( nClipped<3 ) clip._n=0; // polygon completely out
	else clip._n=nClipped;
	if( nRest<3 ) rest._n=0; // polygon completely out
	else rest._n=nRest;
}

#if _VBS3_CRATERS_FIXEDPOLYGONSPLIT

void PolyVertices::Split(const Matrix4 &toWorld, Shape &mesh, PolyVertices &clip, PolyVertices &rest,	Vector3Par normal, Coord d, bool quickAdd)
{
  // Initialize output polygons
  rest._n = clip._n = 0;

  // If there are not enough vertices in the source then quit
  if (_n < 3) return;

  // Epsilon used in calculations
  const float epsilon = 1e-3;

  // Preprocess - go through all the source vertices - if they all lie on one side or on a border (with certain epsilon) don't do anything more
  {
    // Find the min and max distance to plane
    float minDist = FLT_MAX;
    float maxDist = -FLT_MAX;
    for (int i = 0; i < _n; i++)
    {
      Vector3 pos;
      pos.SetFastTransform(toWorld, mesh.Pos(_vertex[i]));
      float dist = pos * normal + d;
      if (minDist > dist) minDist = dist;
      if (maxDist < dist) maxDist = dist;
    }

    // Throw away polygons that lie on the splitting plane
    if ((maxDist <= epsilon) && (minDist >= -epsilon))
    {
      return;
    }

    // We know whole polygon can be in clip
    if (minDist >= -epsilon)
    {
      DoAssert(maxDist > epsilon);
      clip = *this;
      return;
    }

    // We know whole polygon can be in rest
    if (maxDist <= epsilon)
    {
      DoAssert(minDist < -epsilon);
      rest = *this;
      return;
    }
  }

  // Go through the vertices and copy them either to the clip or to the rest array
  {
    // Get the initial p (previous) vertex - last vertex of the polygon
    VertexIndex pVertex = _vertex[_n - 1];
    Vector3 pPos; pPos.SetFastTransform(toWorld, mesh.Pos(pVertex));

    // Go through the vertices and split them between clip and rest
    for (int i = 0; i < _n; i++)
    {
      // Get the n (next) vertex
      VertexIndex nVertex = _vertex[i];
      Vector3 nPos; nPos.SetFastTransform(toWorld, mesh.Pos(nVertex));

      // Get vector from P to N
      Vector3 pnVector = nPos - pPos;

      // Get the dot product with the normal
      float pnDotN = pnVector * normal;

      // If the current line (between P and N) is parallel with the clipping plane, then don't try to look for intersection
      //if (fabs(pnDotN) < epsilon)
      if (pnDotN == 0.0f)
      {
        // Get distance of vertex from the plane
        float dist = nPos * normal + d;

        // Put it either to clip or the rest or both
        if (dist >= -epsilon) clip._vertex[clip._n++] = nVertex;
        if (dist <= epsilon) rest._vertex[rest._n++] = nVertex;
      }
      else
      {
        // Determine what space are the points in
        bool pInClippedSpace = (pPos * normal + d > 0.0f);
        bool nInClippedSpace = (nPos * normal + d > 0.0f);

        // If they lie in separate spaces, create a point in between
        if (pInClippedSpace != nInClippedSpace)
        {
          // Get intersection coefficient (-rest, +clip)
          float t = (nPos * normal + d) / pnDotN;

          // Clip the coefficient, to be in a reasonable distance from both points
          const float tEpsilon = 1e-4;
          saturate(t, tEpsilon, 1.0f - tEpsilon);

          // Add new vertex
          int newVertex = Interpolate(mesh, nVertex, pVertex, t, false, quickAdd);

          // Convert the new vertex to world, add only in case it is not equal to the previous or next vertex
          // Note this code may result in the model to be vertices unreferenced by faces
          {
            Vector3 newPos;
            newPos.SetFastTransform(toWorld, mesh.Pos(newVertex));
            if ((newPos.Distance2(pPos) > Square(epsilon)) && (newPos.Distance2(nPos) > Square(epsilon)))
            {
              rest._vertex[rest._n++] = clip._vertex[clip._n++] = newVertex;
            }
          }

// #if _ENABLE_REPORT
//           // Make sure the new vertex lies within epsilon distance from the plane. Otherwise the epsilon must be extended.
//           // (Preprocess would fail to catch the already divided polygon otherwise - which does't matter much, as we never divide divided polygons (in craters case)
//           // - we use always the original one)
//           Vector3 pos;
//           pos.SetFastTransform(toWorld, mesh.Pos(newVertex));
//           float dist = pos * normal + d;
//           if (dist >= epsilon)
//           {
//             Fail("Error: Just created vertex is too far from the plane it should lie on. Make epsilon bigger or tEpsilon smaller.");
//           }
// #endif
        }

        // Get distance of vertex from the plane
        float dist = nPos * normal + d;

        // Put current vertex either to clip or the rest or both
        if (dist >= -epsilon) clip._vertex[clip._n++] = nVertex;
        if (dist <= epsilon) rest._vertex[rest._n++] = nVertex;
      }

      // Update the previous vertex and position
      pVertex = nVertex;
      pPos = nPos;
    }
  }

  // Verify we are still in range
  DoAssert(clip._n <= MaxPoly);
  DoAssert(rest._n <= MaxPoly);

  // Remove polygons that are not at least a triangles
  if (clip._n < 3) clip._n = 0;
  if (rest._n < 3) rest._n = 0;

  // Split supposed to be performed and in such case the following should be valid
  Assert((clip._n + rest._n) > _n);

// #if _ENABLE_REPORT
//   // If normal of the splitted faces doesn't match normal of the original, then there is something suspicious
//   // Consider model space (not world) coordinates
//   // If the assert looks like a bogus one, this whole area can be commented out
//   {
//     // Original normal
//     Vector3 pPos0 = mesh.Pos(_vertex[0]);
//     Vector3 pPos1 = mesh.Pos(_vertex[1]);
//     Vector3 pPos2 = mesh.Pos(_vertex[2]);
//     Vector3 vA = (pPos1 - pPos0).Normalized();
//     Vector3 vB = (pPos2 - pPos0).Normalized();
//     Vector3 n = vA.CrossProduct(vB).Normalized();
// 
//     // Test clip normal
//     if (clip._n > 2)
//     {
//       Vector3 pPos0 = mesh.Pos(clip._vertex[0]);
//       Vector3 pPos1 = mesh.Pos(clip._vertex[1]);
//       Vector3 pPos2 = mesh.Pos(clip._vertex[2]);
//       Vector3 vA = (pPos1 - pPos0).Normalized();
//       Vector3 vB = (pPos2 - pPos0).Normalized();
//       Vector3 nClip = vA.CrossProduct(vB).Normalized();
//       if (nClip.Distance(n) > 0.2f)
//       {
//         Fail("Error: Normal changed during split");
//       }
//     }
// 
//     // Test rest normal
//     if (rest._n > 2)
//     {
//       Vector3 pPos0 = mesh.Pos(rest._vertex[0]);
//       Vector3 pPos1 = mesh.Pos(rest._vertex[1]);
//       Vector3 pPos2 = mesh.Pos(rest._vertex[2]);
//       Vector3 vA = (pPos1 - pPos0).Normalized();
//       Vector3 vB = (pPos2 - pPos0).Normalized();
//       Vector3 nRest = vA.CrossProduct(vB).Normalized();
//       if (nRest.Distance(n) > 0.2f)
//       {
//         Fail("Error: Normal changed during split");
//       }
//     }
//   }
// #endif
}

#else

void PolyVertices::Split
(
	const Matrix4 &toWorld,
	Shape &mesh, PolyVertices &clip, PolyVertices &rest,
	Vector3Par normal, Coord d, bool quickAdd
)
{
	// source level splitting is used to create splitted shapes
	if( _n<3 ) {rest._n=clip._n=0;return;}
	
	// search for first vertex inside clipping half-space
	int i;
	VertexIndex *pVertex; // previous vertex
	VertexIndex *aVertex; // actual vertex
	
	Vector3 pPos,aPos;

	pVertex=_vertex+_n-1;
	pPos.SetFastTransform(toWorld,mesh.Pos(*pVertex));

	bool pOut=pPos*normal+d<0;
	
	int nClipped=0;
	int nRest=0;
	
	for( i=0; i<_n; i++ )
	{
		aVertex=_vertex+i;
		aPos.SetFastTransform(toWorld,mesh.Pos(*aVertex));
		bool aOut=aPos*normal+d<0;
		// four possible situations
		if( aOut!=pOut )
		{
			// edge going in or out
			Coord t=Intersect(aPos,pPos,normal,d);
      saturate(t, 0, 1);
			rest._vertex[nRest++]=clip._vertex[nClipped++]=Interpolate(mesh,*aVertex,*pVertex,t, quickAdd);
		}
		if( !aOut ) clip._vertex[nClipped++]=*aVertex; // point in
		else rest._vertex[nRest++]=*aVertex; // point out (in rest)
		pVertex=aVertex;
		pOut=aOut;
		pPos=aPos;
	}

	Assert( nClipped<=MaxPoly );
	Assert( nRest<=MaxPoly );
	
	if( nClipped<3 ) clip._n=0; // polygon completely out
	else clip._n=nClipped;
	if( nRest<3 ) rest._n=0; // polygon completely out
	else rest._n=nRest;
}

#endif

bool PolyVertices::InsideFromX
(
	const VertexTable &mesh, Vector3Par pos
) const
{
	if( _n<3 ) return false;

	Vector3 normal(-1,0,0); // test only incidence from one direction
	Vector3 pPos(0,pos[1],pos[2]);
	// perform all tests in face plane space
	// use cross product with face normal to get line normal in face plane
	int i;
	Vector3 lPos=mesh.Pos(_vertex[_n-1]);
	bool andIn=true;
	bool orIn=false;
	for( i=0; i<_n; i++ )
	{
		Vector3 aPos=mesh.Pos(_vertex[i]);
		// half-plane equation has form ax+by+c<=0
		// (a,b) is line normal
		//Vector3 aMinusL=aPos-lPos;
		//aMinusL[0]=0;
		//Vector3 lineNormal=aMinusL.CrossProduct(normal);
		// optimize: CrossProduct with (-1,0,0) is:
		// return Vector3(0,-Z(),+Y());
		Vector3 lineNormal(0,lPos.Z()-aPos.Z(),aPos.Y()-lPos.Y());
		Vector3 aPPos(0,aPos.Y(),aPos.Z());
		//float checkIn=lineNormal*pPos-lineNormal*aPPos;
		float checkIn=lineNormal*(pPos-aPPos);
		if( checkIn<0 ) andIn=false;
		else orIn=true;
		lPos=aPos;
	}
	if( andIn==orIn ) return true;
	return false;
	//return true;
}

void PolyVertices::FitToLandscape( TLVertexTable &mesh, Scene &scene, float y )
{
	int i;
	for( i=0; i<_n; i++ )
	{
		int index=_vertex[i];
		ClipFlags clip=mesh.Clip(index);
		// note: 
		Vector3 pos = mesh.TransPosA(index);
		Vector3 tPos(VFastTransform,scene.CamInvTrans(),pos);
		float tY=scene.GetLandscape()->SurfaceY(tPos[0],tPos[2]);
		ClipFlags landClip=clip&ClipLandMask;
		if( landClip==ClipLandOn ) tPos[1]=tY+y;
		else if( landClip==ClipLandUnder ) tPos[1]=floatMin(tY,tPos[1]);
		else if( landClip==ClipLandAbove ) tPos[1]=floatMax(tY+y,tPos[1]);
		pos.SetFastTransform(scene.ScaledInvTransform(),tPos);
		mesh.SetTransPosA(index,pos);
	}
}
void PolyVertices::FitToLandscape( const Matrix4 &toWorld, VertexTable &mesh, const Landscape &land, float y )
{
	int i;
	//Matrix4 fromWorld(MInverseScaled,toWorld);
	Matrix4 fromWorld(MInverseGeneral,toWorld);
	for( i=0; i<_n; i++ )
	{
		Vector3 &pos=mesh.SetPos(_vertex[i]);
		ClipFlags clip=mesh.Clip(_vertex[i]);
		Vector3 tPos(VFastTransform,toWorld,pos);
		float tY=land.SurfaceY(tPos[0],tPos[2]);
		ClipFlags landClip=clip&ClipLandMask;
		if( landClip==ClipLandOn ) tPos[1]=tY+y;
		else if( landClip==ClipLandUnder ) tPos[1]=floatMin(tY,tPos[1]);
		else if( landClip==ClipLandAbove ) tPos[1]=floatMax(tY+y,tPos[1]);
		pos.SetFastTransform(fromWorld,tPos);
	}
}

