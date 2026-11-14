#include "../wpch.hpp"
#include "poly.hpp"
//#include "tlVertex.hpp"
#include "shape.hpp"
#include "material.hpp"

#if _ENABLE_PHYSICS
#include "Physics\enf_worldphysics.h"
#endif

void PolyProperties::Init()
{
	_texture=NULL;
	_special=0;
	_areaOverTex.Init();
}

bool PolyProperties::operator == (const PolyProperties &sec) const
{
	return
	(
		_texture==sec._texture &&
		_special==sec._special &&
		_surfMat==sec._surfMat
	);
}
bool PolyProperties::operator != (const PolyProperties &sec) const
{
	return
	(
		_texture!=sec._texture ||
		_special!=sec._special ||
		_surfMat!=sec._surfMat
	);
}


float Poly::CalculateUDensity(const VertexTable &mesh, int uvStage) const
{
	int ii=_vertex[0];
  float ic = mesh.U(uvStage, ii);
	Vector3Val iPos = mesh.Pos(ii);
	float cDistanceSum = 0;
	float distanceSum = 0;
	for( int i=1; i<_n; i++ )
	{
		int vi=_vertex[i];
    float vc = mesh.U(uvStage, vi);
		
		float cDistance = fabs(vc-ic);
		float distance = mesh.Pos(vi).Distance(iPos);
		distanceSum += distance;
		cDistanceSum += cDistance;
	}
	float density = cDistanceSum>1e-6 ? distanceSum/cDistanceSum : 1e6;
	return density;
}

float Poly::CalculateVDensity(const VertexTable &mesh, int uvStage) const
{
	int ii=_vertex[0];
  float ic= mesh.V(uvStage, ii);
	Vector3Val iPos = mesh.Pos(ii);
	float cDistanceSum = 0;
	float distanceSum = 0;
	for( int i=1; i<_n; i++ )
	{
		int vi=_vertex[i];
    float vc = mesh.V(uvStage, vi);
		
		float cDistance = fabs(vc-ic);
		float distance = mesh.Pos(vi).Distance(iPos);
		distanceSum += distance;
		cDistanceSum += cDistance;
	}
	float density = cDistanceSum>1e-6 ? distanceSum/cDistanceSum  : 1e6;
	return density;
}

float Poly::CalculateUVArea(const VertexTable &mesh, int uvStage) const
{
  // assume 2D UV space, calculate area of given poly in that space
  // triangulate the poly as needed
	int i;
	float area=0;
	for( i=2; i<_n; i++ )  // triangulate the poly as needed
	{
		int ii=_vertex[0];
		int ll=_vertex[i-1];
		int rr=_vertex[i];
		Vector3 ip(mesh.U(uvStage,ii),mesh.V(uvStage,ii),0);
		Vector3 lp(mesh.U(uvStage,ll),mesh.V(uvStage,ll),0);
		Vector3 rp(mesh.U(uvStage,rr),mesh.V(uvStage,rr),0);
		Vector3Val lmi = (lp-ip);
		Vector3 normal=lmi.CrossProduct(rp-ip);
		area+=normal.Size();
	}
	return area*0.5;
}

Vector3 PolyVertices::GetNormal(const Array<Vector3> &pos) const
{
  // find best fit using Newell method (http://cs.haifa.ac.il/~gordon/plane.pdf)
  float a = 0, b = 0, c = 0;
  for (int curr=0; curr<_n; curr++)
  {
    int next = curr+1>=_n ? 0 : curr+1;
    Vector3Val posC = pos[_vertex[curr]];
    Vector3Val posN = pos[_vertex[next]];
    
    a += (posN.Y()-posC.Y())*(posC.Z()+posN.Z()); // sign opposite to usual
    b += (posN.Z()-posC.Z())*(posC.X()+posN.X());
    c += (posN.X()-posC.X())*(posC.Y()+posN.Y());
  }
#if _DEBUG
  if (_n==3)
  {
    Vector3Val p0 = pos[_vertex[0]];
    Vector3Val p1 = pos[_vertex[1]];
    Vector3Val p2 = pos[_vertex[2]];
    // TODO: implement template cross product?
    Vector3Val p20 = (p2 - p0);
    Vector3 check = p20.CrossProduct(p1 - p0);
    Vector3 res(a,b,c);
    Assert(check.Distance(res)<0.01f);
  }
#endif
  return Vector3(a,b,c);
}


void PolyVertices::CalculateNormal(Plane &dst, const Array<Vector3> &pos) const
{
  // find three vertices so that all vertices are inside
  Assert(_n<=4);
	Vector3 normal = GetNormal(pos).Normalized();
  // compute be
  Vector3 center(VZero);
  for (int curr=0; curr<_n; curr++)
  {
    center += pos[_vertex[curr]];
}
  center /= _n;
  dst.SetNormal(normal,-normal*center);
}

void PolyVertices::CalculateD(Plane &dst, const Array<Vector3> &pos) const
{
  Vector3 center(VZero);
  for (int curr=0; curr<_n; curr++)
  {
    center += pos[_vertex[curr]];
}
  center /= _n;
  Vector3Val normal = dst.Normal();
  dst.SetD(-normal*center);
}

void PolyProperties::AnimateStageTexture(int stage, float phase)
{
  TexMaterial *mat = _surfMat;
	if( !mat ) return;
	mat->Load();
	Assert(stage<mat->_stageCount);

  Texture *tex = mat->_stage[stage]._tex;
  if (!tex) return;
	int n=tex->AnimationLength();
	if (n<=1) return; // no animation
	int i=toIntFloor(phase*n);
	if( i<0 ) i+=n;
	else if( i>=n ) i-=n;
	Assert( i>=0 && i<n );  // avoid clamping animation too much
	mat->_stage[stage]._tex = tex->GetAnimation(i);
}

#if _ENABLE_PHYSICS
enf::PhysicsMaterial* PolyProperties::GetPhysMaterial() const
{
  enf::PhysicsMaterial *ret = GetMaterialExt() ? GetMaterialExt()->GetPhysMaterial() : NULL;
  if(ret)
    return ret;
  return GetTexture() ? GetTexture()->GetPhysMaterial() : GLOB_ENGINE->TextBank()->GetPhysMaterial("material/default");
}
#endif

const SurfaceInfo *PolyProperties::GetSurfaceInfo() const
{
  const SurfaceInfo *ret = GetMaterialExt() ? GetMaterialExt()->GetSurfaceInfo() : NULL;
  if (ret) return ret;
  return GetTexture() ? &GetTexture()->GetSurfaceInfo() : &SurfaceInfo::_default;
}


void PolyProperties::AnimateTexture( float phase )
{
	if( !_texture ) return;
	int n=_texture->AnimationLength();
	if (n<=1) return; // no animation
	int i=toIntFloor(phase*n);
	if( i<0 ) i+=n;
	else if( i>=n ) i-=n;
	Assert( i>=0 && i<n );  // avoid clamping animation too much
	SetTexture(_texture->GetAnimation(i));
}

TexMaterial *PolyProperties::GetMaterialExt() const
{
	return _surfMat;
}

void PolyProperties::SetMaterialExt(TexMaterial *m)
{
	_surfMat=m;
}

RString PolyProperties::GetDebugText() const
{
	char buf[1024];
	if (_texture) strcpy(buf,_texture->Name());
	else strcpy(buf,"<null>");
	if (_surfMat) {strcat(buf,",");strcat(buf,_surfMat->GetName());}
	if (_special)
	{
		// TODO: consider text description of _special
		sprintf(buf+strlen(buf)," - %x",_special);
	}
	return buf;
}

void PolyVertices::GetMinMax(Vector3 &minV, Vector3 &maxV, const VertexTable &mesh ) const
{
  minV = Vector3(+FLT_MAX,+FLT_MAX,+FLT_MAX);
  maxV = Vector3(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	for( int i=0; i<_n; i++ )  // triangulate the poly as needed
	{
		int ii=_vertex[i];
		CheckMinMax(minV,maxV,mesh.Pos(ii));
	}
}

Vector3 PolyVertices::GetAverage( const VertexTable &mesh ) const
{
  Vector3 center(VZero);
  for (int i=0; i<N(); i++) center += mesh.Pos(GetVertex(i));
  return center / N();

}

float PolyVertices::GetArea( const VertexTable &mesh ) const
{
	float area=0;
	for( int i=2; i<_n; i++ )  // triangulate the poly as needed
	{
		int ii=_vertex[0];
		int ll=_vertex[i-1];
		int rr=_vertex[i];
		Vector3Val ip=mesh.Pos(ii);
		Vector3Val lp=mesh.Pos(ll);
		Vector3Val rp=mesh.Pos(rr);
		Vector3Val lmi = (lp-ip);
		Vector3 normal=lmi.CrossProduct(rp-ip);
		area+=normal.Size();
	}
	return area*0.5;
}

float PolyVertices::GetAreaTop( const VertexTable &mesh ) const
{
	float area=0;
	for( int i=2; i<_n; i++ )
	{
		int ii=_vertex[0];
		int ll=_vertex[i-1];
		int rr=_vertex[i];
		Vector3Val ip=mesh.Pos(ii);
		Vector3Val lp=mesh.Pos(ll);
		Vector3Val rp=mesh.Pos(rr);
		Vector3Val lmi = (lp-ip);
		Vector3 normal = lmi.CrossProduct(rp-ip);
		area+=floatMax(normal.Y(),0);
	}
	return area*0.5;
}

inline float UVTest(float a, float b, float c, const UVPair &uv)
{
	return a*uv.u + b*uv.v + c;
}

inline Coord Intersect( const UVPair &in, const UVPair &out, float a, float b, float c)
{
	//UVPair imo(in.u-out.u),in.v-out.v);
	//return ( in*normal+d ) / ( AmB*normal );
	return ( in.u*a+in.v*b+c ) / ( (in.u-out.u)*a+(in.v-out.v)*b );
}

int Interpolate(Shape &mesh, int in, int out, Coord t, bool quickAdd=false)
{
	Vector3Val iPos=mesh.Pos(in);
	Vector3Val oPos=mesh.Pos(out);
	ClipFlags iClip=mesh.Clip(in);
	ClipFlags oClip=mesh.Clip(out);
	Vector3 iNorm = mesh.Norm(in).Get();
	Vector3 oNorm = mesh.Norm(out).Get();
	// interpolate vertex user data
	ClipFlags hints=iClip&oClip&ClipHints;
	float it = 1-t;
	// interpolate vertex position
	Vector3 point(iPos*it+oPos*t);
	Vector3 norm(iNorm*it+oNorm*t);
	norm.Normalize();
	// interpolate vertex data
	float inU=mesh.U(in),outU=mesh.U(out);
	float inV=mesh.V(in),outV=mesh.V(out);
	// create new vertex
  // optimization: minUV, maxUV will be unchanged, calculate invUV directly
  UVPair invUV(1 / (mesh.MaxUV().u - mesh.MinUV().u), 1 / (mesh.MaxUV().v - mesh.MinUV().v));
	// TODO: determine when to use fast/slow versions of AddPos
	float resU=inU*it+outU*t;
	float resV=inV*it+outV*t;
	Assert(!mesh.IsVertexBoneRefPresent());
  Assert(mesh.NUVStages() == 1);

  bool isST = mesh.IsSTPresent();
  int nTIndex=0;
  if (quickAdd)
    nTIndex = mesh.AddVertex(point,norm,hints|ClipAll,invUV,resU,resV,NULL,-1,-1.0/*noSearch*/,-1.0);
  else
    nTIndex = mesh.AddVertex(point,norm,hints|ClipAll,invUV,resU,resV);
	// if some optional attributes are present, we should interpolate them as well

	if (isST)
	{
	  Vector3 inS = mesh.ST(in).s.Get(), outS = mesh.ST(out).s.Get();
	  Vector3 inT = mesh.ST(in).t.Get(), outT = mesh.ST(out).t.Get();
	  Vector3 resS = inS*it+outS*t;
	  Vector3 resT = inT*it+outT*t;
	  // normalize could make results more accurate
	  // however road normals are usually very flat
	  // moreover, interpolation without normalization is equivalent to
	  // what triangle interpolator would do
    // resS.Normalize();
    // resT.Normalize();
	  mesh.SetAndStretchST(nTIndex,STPair(resS,resT));
	}
  if (mesh.IsAlphaPresent())
  {
    int alpha = toInt(mesh.Alpha8(in)*it + mesh.Alpha8(out)*t);
    saturate(alpha, 0, 255);
    mesh.SetAndStretchAlpha(nTIndex,alpha);
  }

	return nTIndex;
}



