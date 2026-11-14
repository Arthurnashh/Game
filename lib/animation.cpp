// Poseidon - shape animation support
// (C) 1997, SUMA
#include "wpch.hpp"

#include "Shape/shape.hpp"
#include "Shape/material.hpp"
#include "animation.hpp"
#include <Es/Strings/bString.hpp>
#include <El/Common/randomGen.hpp>
#include "serializeBinExt.hpp"

#include "global.hpp"
#include "rtAnimation.hpp"
#include "paramFileExt.hpp"

#if _ENABLE_PHYSICS
#include "Physics\enf_worldphysics.h"
#include "Physics\enf_physicsbullet.h"
#include "BulletCollision/CollisionShapes/btConvexHullShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#endif

#include <Es/Files/filenames.hpp>

//#pragma optimize("", off)

Offset ConvertOffset(SerializeBinStream &f, Offset o);

// named selections

int IndexSelection::Find( int index ) const
{
  for( int i=0; i<Size(); i++ )
  {
    if( (*this)[i]==index ) return i;
  }
  return -1;
}

bool IndexSelection::IsSelected( int index ) const
{
  for( int i=0; i<Size(); i++ )
  {
    if( (*this)[i]==index ) return true;
  }
  return false;
}

bool IndexSelection::IsSubset(const IndexSelection &sel) const
{
  // check: sel is subset of this
  // (all points selected in sel  are also selected in this)
  int t = 0;
  int s = 0;
  // each member of sel must be in this

  while (s<sel.Size())
  {
    int si = sel[s];
    // check if si is it t
    for(;;)
    {
      if (t>=Size()) return false; // si is not contained - end of set
      int ti = (*this)[t];
      if (si<ti) return false; // si not contained - missing in set
      t++;
      if (si==ti) break; // si found - advance to next s
    }
    s++;
  }
  return true;
}


void Selection::DoConstruct( const Selection &src )
{
  _sel.Init(src._sel);
  _weights.Init(src._weights);
  //_intervals.Init(src._intervals);
}

Selection::Selection( const SelInfo *sel, int nSel )
{
  if( nSel>0 )
  {
    _sel.Init(nSel);
    // check if weight is singular
    bool singular = true;
    for (int i=0; i<nSel; i++)
    {
      if (sel[i].weight<254) singular = false;
    }
    if (singular)
    {
      // all weights one - do not store
      for( int i=0; i<nSel; i++ )
      {
        _sel[i]=sel[i].index;
      }
    }
    else
    {
      _weights.Init(nSel);
      for( int i=0; i<nSel; i++ )
      {
        _sel[i]=sel[i].index;
        _weights[i]=sel[i].weight;
      }
    }
    //CreateIntervals(); // preprocess intervals
  }
  //else _sel.Init(),_weigths.Init();
}


SectionSelection::SectionSelection()
{
}
SectionSelection::~SectionSelection()
{
}

void SectionSelection::SerializeBin(SerializeBinStream &f)
{
  // when loading version 7 we should load FaceSelection
  if (f.IsLoading() && f.GetVersion()<=7)
  {
    // load FaceSelection member, but ignore them
    Buffer<int> faces;
    bool needsSections;
    f.TransferBinaryArray(faces);
    f.Transfer(needsSections);
  }

  // do not attempt to save version 7
  DoAssert(f.IsLoading() || f.GetVersion()>7);

  f.TransferBinaryArray(_sections);
}

FaceSelection::FaceSelection()
{
  _needsSections = false;
}

FaceSelection::~FaceSelection()
{
}

void FaceSelection::SerializeBin(SerializeBinStream &f)
{
  if (f.IsLoading() && f.GetVersion()<13)
  {
    Buffer<Offset> dummy;
    f.SkipBinaryArray(dummy);
  }
  f.Transfer(_needsSections);
  f.TransferBinaryArray(_sections);
}

void FaceSelection::RescanSections(Shape *shape, const Selection &faces, const char *debugName)
{
  AUTO_STATIC_ARRAY(int,sections,256);

  // we need to be able to work with face indices here - face offsets are no longer present
  Buffer<Offset> offsets;
  shape->BuildFaceIndexToOffset(offsets);
  for (int s=0; s<shape->NSections(); s++)
  {
    const ShapeSection &sec = shape->GetSection(s);
    // check what is sec relation to offsets
    // scan which sections are wholly included in this selection
    // warn if some section is included partially
    // we are sure to have _faces
    // _faceSel need not be present
    bool some = false;
    bool all = true;
    for (Offset so=sec.beg; so<sec.end; shape->NextFace(so))
    {
      bool found = false;
      for (int oo=0; oo<faces.Size(); oo++)
      {
        int face = faces[oo];
        Offset selOff = offsets[face];
        if (selOff==so) {found=true;break;}
      }
      if (found) 
      {
        some = true; // face is there
      }
      else
      {
        all = false; // face is not there
      }
    }
    if (some!=all)
    {
      LogF
      (
        "Section %d - %s (%d..%d) partially contained in %s",
        s,(const char *)sec.GetDebugText(),sec.beg,sec.end,
        debugName
      );
      #if 0
      for (Offset so=sec.beg; so<sec.end; shape->NextFace(so))
      {
        bool found = false;
        for (int oo=0; oo<Size(); oo++)
        {
          if ((*this)[oo]==so) {found=true;break;}
        }
        if (found) 
        {
          LogF("  %d contained",so);
        }
        else
        {
          LogF("  %d not contained",so);
        }
      }
      #endif

    }
    if (all)
    {
      sections.Add(s);
    }
  }
  _sections.Init(sections.Data(),sections.Size());
  //
  //shape->
}

Selection::Selection( const VertexIndex *sel, int nSel )
{
  if( nSel>0 )
  {
    _sel.Init(sel,nSel);
  }
}

void Selection::DoDestruct()
{
  _sel.Delete();
  _weights.Delete();
}


double Selection::GetMemoryUsed() const
{
  return _sel.GetMemoryUsed()+_weights.GetMemoryUsed();
}
size_t Selection::GetMemoryControlled() const
{
  return _sel.GetMemoryAllocated()+_weights.GetMemoryAllocated();
}


NamedProperty::NamedProperty( const char *name, const char *value )
{
  RString nameLow = name;
  RString valueLow = value;
  nameLow.Lower();
  valueLow.Lower();
  _name=nameLow;
  _value=valueLow;
}

void Selection::Add(int vi, byte weigth)
{
  bool weighted = _weights.Size()>0;

  // add to selection
  // note: we may want to keep indices ordered
  // it is not necessary, but it may be nice
  Buffer<VertexIndex> temp(_sel.Size()+1);
  Buffer<byte> tempW(_sel.Size()+1);
  int i=0;
  while (i<_sel.Size())
  {
    if (_sel[i]>vi) break;
    temp[i] = _sel[i];
    if (weighted) tempW[i] = _weights[i];
    i++;
  }
  temp[i] = vi;
  if (weighted) tempW[i] = weigth;
  while (i<_sel.Size())
  {
    temp[i+1] = _sel[i];
    if (weighted) tempW[i+1] = _weights[i];
    i++;
  }

  _sel = temp;
  if (weighted) _weights = tempW;

}

#if NAMED_SEL_STATS
#include <El/Statistics/statistics.hpp>
static StatisticsByName NamedSelMemory;
static StatisticsByName NamedSelLodMemory;

extern RString NamedSelCreateContext;

static void ReportNamedSelStats()
{
  NamedSelMemory.Report(1);
  NamedSelLodMemory.Report(1);
}

#endif


NamedSelection::NamedSelection(
  RString name,
  const SelInfo *points, int nPoints,
  const VertexIndex *faces, int nFaces
)
:Selection(points,nPoints),_faces(faces,nFaces)
{
  name.Lower();
  _name=name;
  #if _ENABLE_REPORT
    _wasUsed = false;
    _wasContentUsed = false;
  #endif
  #if NAMED_SEL_STATS
    if (_name.GetLength()>0)
    {
      _info = NamedSelCreateContext;
      size_t used = GetMemoryControlled()+sizeof(*this);
      NamedSelMemory.Count(_name,used);
      NamedSelLodMemory.Count(_info,used);
      //LogF("%s:%d",cc_cast(_info+":"+_name),used);
    }
  #endif
}

double NamedSelection::GetMemoryUsed() const
{
  return base::GetMemoryUsed()+_faces.GetMemoryUsed();
}
size_t NamedSelection::GetMemoryControlled() const
{
  return base::GetMemoryControlled()+_faces.GetMemoryControlled();
}

void NamedSelection::DoConstruct( const NamedSelection &src )
{
  Selection::DoConstruct(src);
  _faces = src.Faces();
  _name = src._name;
  _faceSel = src._faceSel;
  #if _ENABLE_REPORT
    _wasUsed = false;
    _wasContentUsed = false;
  #endif
  #if NAMED_SEL_STATS
    if (_name.GetLength()>0)
    {
      _info = NamedSelCreateContext;
      size_t used = GetMemoryControlled()+sizeof(*this);
      NamedSelMemory.Count(_name,used);
      NamedSelLodMemory.Count(_info,used);
      //LogF("%s:%d",cc_cast(_info+":"+_name),used);
    }
  #endif
}

void NamedSelection::DoConstruct()
{
  _faceSel = FaceSelection();
  #if _ENABLE_REPORT
    _wasUsed = false;
    _wasContentUsed = false;
  #endif
}

void Selection::SerializeBin(SerializeBinStream &f)
{
  f.TransferBinaryArray(_sel);
  f.TransferBinaryArray(_weights);
}

void NamedSelection::SerializeBin(SerializeBinStream &f)
{
  f.Transfer(_name);
  _faces.SerializeBin(f);
  #if _DEBUG
    for (int i=0; i<_faces.Size(); i++)
    {
      int index = _faces[i];
      Assert(index>=0);
    }
  #endif
  _faceSel.SerializeBin(f);
  Selection::SerializeBin(f);
  #if NAMED_SEL_STATS
    if (_name.GetLength()>0)
    {
      _info = NamedSelCreateContext;
      size_t used = GetMemoryControlled()+sizeof(*this);
      NamedSelMemory.Count(_name,used);
      NamedSelLodMemory.Count(_info,used);
      //LogF("%s:%d",cc_cast(_info+":"+_name),used);
    }
  #endif
}

void NamedSelection::RemoveAllButSections()
{
  _faces.Clear();
  _sel.Clear();
  _weights.Clear();
}

void NamedSelection::DoDestruct()
{
  #if NAMED_SEL_STATS
    if (_name.GetLength()>0)
    {
      size_t used = GetMemoryControlled()+sizeof(*this);
      NamedSelMemory.Count(_name,-(int)used);
      NamedSelLodMemory.Count(_info,-(int)used);
      //LogF("%s:%d~",cc_cast(_info+":"+_name),used);
    }
    static bool reportStats = false;
    if (reportStats)
    {
      ReportNamedSelStats();
    }
  #endif
  Selection::Clear();
  _faces.Clear();
}

void NamedSelection::CalculateFaceOffsets(Buffer<Offset> &offsets, Shape *shape) const
{
  Buffer<Offset> faceOffsets;
  shape->BuildFaceIndexToOffset(faceOffsets);
  offsets.Resize(_faces.Size());
  for (int i=0; i<_faces.Size(); i++)
  {
    int face = _faces[i];
    offsets[i] = faceOffsets[face];
  }
}
  
// maintain named selections
int Shape::FindNamedSel( const char *name ) const
{
  int i;
  // if name is NULL or, empty, do not search
  if (!name || !*name) return -1;
  for( i=0; i<_sel.Size(); i++ )
  {
    const NamedSelection &sel=_sel[i];
    if( !strcmpi(name,sel.Name()) )
    {
#if _ENABLE_REPORT
      sel.ReportSelectionUsed();
#endif
      return i;
    }
  }
  //Log("No selection '%s'.",name);
  return -1;
}

int Shape::FindNamedSel( const char *name, const char *altName ) const
{
  int index=FindNamedSel(name);
  if( index>=0 ) return index;
  return FindNamedSel(altName);
}

void Shape::AddNamedSel( const NamedSelection &sel )
{
  _sel.Add(sel);
}

const Plane &ConvexComponent::GetPlane(const Array<Plane> &planes, int i) const
{ 
  int index = _planes[i];
  return planes[index];
}
const Poly &ConvexComponent::GetFace( const Shape *shape, int i ) const
{
  // note: this function needs BuildFaceIndexToOffset
  // called on shape before using
  int index=_planes[i];
  Offset o = shape->FaceIndexToOffset(index);
  return shape->Face(o);
}

void ConvexComponent::Init(Shape *shape, const NamedSelection::Access &sel, int nameIndex)
{
  _minMax[0]=VZero;
  _minMax[1]=VZero;
  _minMax[0]=Vector3(+1e10,+1e10,+1e10);
  _minMax[1]=Vector3(-1e10,-1e10,-1e10);
  
  // source selection
  sel.CopyVertexData(_sel);
  _name = sel.GetName();
  _nameIndex = nameIndex;

  int size = sel.Size();
  {
    int index=sel[0];
    
    Vector3Val p=shape->Pos(index);
    _minMax[0]=p;
    _minMax[1]=p;
    _center = shape->Pos(index);
    for( int si=1; si<size; si++ )
    {
      int index= sel[si];
      CheckMinMaxIter(_minMax[0],_minMax[1],shape->Pos(index));
      _center += shape->Pos(index);
    }
  }

  // find bounding center
  _center /= size;
  // find bounding sphere radius
  float maxRadius2=0;
  for( int si=0; si<size; si++ )
  {
    int index = sel[si];
    float radius2=shape->Pos(index).Distance2(_center);
    saturateMax(maxRadius2,radius2);
  }
  _radius=sqrt(maxRadius2);
  // convert face representation to half space
  if (sel.NSections()>0)
  {
    const ShapeSection &ss = shape->GetSection(sel.GetSection(0));
    // get texture of any face
    _texture = ss.GetTexture();
    const TexMaterial *mat = ss.GetMaterialExt();
    _surfaceInfo = mat ? mat->GetSurfaceInfo() : NULL;
#if _ENABLE_PHYSICS
    _physMat = mat ? mat->GetPhysMaterial() : NULL;
#endif
  }
  else
  {
    // if there is only one section for whole object, it must be it
    // otherwise we have to search for the section that encapsulates this component
    if (shape->NSections()==1)
    {
      const ShapeSection &ss = shape->GetSection(0);
      // get texture of any face
      _texture = ss.GetTexture();
      const TexMaterial *mat = ss.GetMaterialExt();
      _surfaceInfo = mat ? mat->GetSurfaceInfo() : NULL;
#if _ENABLE_PHYSICS
      _physMat = mat ? mat->GetPhysMaterial() : NULL;
#endif
    }
    else
    {
      Offset omin=shape->EndFaces(),omax=shape->BeginFaces();
      Buffer<Offset> offsets;
      sel.CalculateFaceOffsets(offsets,shape);
      for (int i=0; i<offsets.Size(); i++)
      {
        Offset o = offsets[i];
        if (omin>o) omin=o;
        if (omax<o) omax=o;
      }
      bool found = false;
      // check if obeg..oend is fully included in some section
      for (int s=0; s<shape->NSections(); s++)
      {
        const ShapeSection &ss = shape->GetSection(s);
        if (ss.beg<=omin && ss.end>omax)
        {
          // selection fully included .. chance of full hit
          bool allIncluded = true;
          for (int i=0; i<offsets.Size(); i++)
          {
            Offset o = offsets[i];
            if (ss.beg>o || ss.end<=o) {allIncluded=false;break;}
          }
          if (allIncluded)
          {
            _texture = ss.GetTexture();
            const TexMaterial *mat = ss.GetMaterialExt();
            _surfaceInfo = mat ? mat->GetSurfaceInfo() : NULL;
#if _ENABLE_PHYSICS
            _physMat = mat ? mat->GetPhysMaterial() : NULL;
#endif
            found = true;
            break; // break for (s)
          }
        }
      }
      if (!found)
      {

        // find section that contains most of this selection
        int maxIncluded = 0;
        for (int s=0; s<shape->NSections(); s++)
        {
          const ShapeSection &ss = shape->GetSection(s);
          // selection fully included .. chance of full hit
          int included = 0;
          for (int i=0; i<offsets.Size(); i++)
          {
            Offset o = offsets[i];
            if (ss.beg>o || ss.end<=o) continue;
            included++;
          }
          if (included>=maxIncluded)
          {
            _texture = ss.GetTexture();
            const TexMaterial *mat = ss.GetMaterialExt();
            _surfaceInfo = mat ? mat->GetSurfaceInfo() : NULL;
#if _ENABLE_PHYSICS
            _physMat = mat ? mat->GetPhysMaterial() : NULL;
#endif
            maxIncluded = included;
          }
        }
        /*
        RptF
        (
          "No section found containing %s, best guess is %s",
          name,_texture ? (const char *)_texture->GetName() : "<null>"
        );
        */
      }
    }
  }
  const Selection &faces = sel.Faces();
  _planes.Clear();
  _planes.Realloc(faces.Size());
  for( int fi=0; fi<faces.Size(); fi++ )
  {
    _planes.Add(faces[fi]);
  }
  _planes.Compact();
  shape->BuildFaceIndexToOffset();
  InitPlanes(shape);

  //check out if we have box here
  if(NPlanes() == 6)
  {
    int mask = 0;
    static const Vector3 axials[] = {
      Vector3(1,0,0),Vector3(-1,0,0),
      Vector3(0,1,0),Vector3(0,-1,0),
      Vector3(0,0,1),Vector3(0,0,-1)
    };

    for(int n = 0; n < 6; n++)
    {
      const Plane& pl = GetPlane(shape->GetPlanesArray(), n);
      for(int p = 0; p < 6; p++)
      {
        if((mask & (1 << p)) == 0) //don't repeat the same axes
          mask |= (fabs(1.0f - pl.Normal().DotProduct(axials[p])) < 0.001f) ? (1 << p) : 0;
      }
    }
    //we have all major axes. It must be a box
    if(mask == ((1 << 6) - 1))
      _isBox = true;
  }
  CalculateOOB(shape);  
}

#if _ENABLE_PHYSICS
bool ConvexComponent::IsPrepared()
{
  return _sel.Size() ? true : false;
}

bool ConvexComponent::GetPhysGeom(Shape *shape, enf::PhysicsGeomDef& def)
{
	bool matDefined = (_physMat) ? true : false;
	
	Vector3 len = Max() - Min();
  if(_isBox)
  {
    btBoxShape* colShape = new btBoxShape(enf::BulletWorld::Point2btVector(len * 0.5f));
		colShape->setUserPointer(this);
		enf::Matrix43 mat(MTranslation, _center);
    enf::PhysicsGeometry* physGeom = (enf::PhysicsGeometry*)colShape;
    
    def.Init(GetName().Data(), physGeom, mat, 0, _physMat, 8);
    return matDefined;
  }
  else
  {
    Assert(_sel.Size());
    
    btConvexHullShape *hull = new btConvexHullShape();
    for(int i = 0; i < _sel.Size(); ++i)
    {
      VertexIndex id = _sel[i];
      btVector3 vx = enf::BulletWorld::Point2btVector(shape->Pos(id));
      hull->addPoint(vx);
    }
		hull->setUserPointer(this);
    enf::PhysicsGeometry* physGeom = (enf::PhysicsGeometry*)hull;
    def.Init(GetName().Data(), physGeom, M4Identity, 0, _physMat, 8);
    return matDefined;
  }
}

#endif

void ConvexComponent::CalculateOOB(Shape *shape)
{
  int size = _sel.Size();
  if (size > 0)
  {
    // Compute OBB
    _centerOBB = _center;
    _whAngle[2] = 0.0f;

    float minArea = FLT_MAX;

    // calculate minimum area rectangle in the xz plane containing all vertices
    for (int i = 0, j = size - 1; i < size; j = i, i++)
    {
      int ii = _sel[i];
      int ij = _sel[j];

      // edge e0
      Vector3 e0 = shape->Pos(ii) - shape->Pos(ij);
      if (e0.SizeXZ() < 0.0001f) continue;
      e0[1] = 0.0f;
      e0.Normalize();

      // orthogonal edge to edge e0
      Vector3 e1(-e0.Z(), 0.0f, e0.X());

      float min0 = 0.0f, min1 = 0.0f, max0 = 0.0f, max1 = 0.0f;
      for (int k = 0; k < size; k++)
      {
        int ik = _sel[k];

        Vector3 tmp = shape->Pos(ik) - shape->Pos(ij);
        float dot = tmp.X() * e0.X() + tmp.Z() * e0.Z();

        if (dot < min0) min0 = dot;
        if (dot > max0) max0 = dot;

        dot = tmp.X() * e1.X() + tmp.Z() * e1.Z();

        if (dot < min1) min1 = dot;
        if (dot > max1) max1 = dot;
      }

      float area = (max0 - min0) * (max1 - min1);

      // If best so far, remember area, center and axes
      Assert(area > 0.0f);

      if (area < minArea)
      {
        minArea = area;
        // rectangle width & height
        _whAngle[0] = (max0 - min0) * 0.5f;
        _whAngle[1] = (max1 - min1) * 0.5f;
        _centerOBB = shape->Pos(ij) + (e0 * (min0 + max0) + e1 * (min1 + max1)) * 0.5f;
        float e0x = e0.X();
        saturate(e0x, -1.0f, 1.0f);
        _whAngle[2] = e0.Z() < 0.0f ? (H_PI - acosf(e0x)) : acosf(e0x);
      }
    }
  }
  else
  {
    _centerOBB = VZero;
    _whAngle = VZero;
  }
}

void ConvexComponent::GetSkinning(BlendSkinning &skinning, const Shape *s, const LODShape *lShape) const
{
  if (!s->IsVertexBoneRefPresent())
  {
    return;
  }

  VertexIndex vi = _sel[0];

  const AutoArray<AnimationRTWeight> &a = s->GetVertexBoneRef();

  // select dominant bone
  SkeletonIndex domBone = SkeletonIndexNone;
  float domW = 0;

  for (int i=0; i<a[vi].Size(); i++)
  {
    float w = a[vi][i].GetWeight();
    SubSkeletonIndex bone = a[vi][i].GetSel();

    SkeletonIndex boneSI = s->GetSkeletonIndexFromSubSkeletonIndex(bone); 
    if(boneSI == SkeletonIndexNone)
      continue;
    BlendBone& blend = skinning.bones.Append();
    blend.bone = boneSI;
    blend.boneName = lShape->GetSkeleton()->GetBone(boneSI);
    blend.weight = w;
    if (w>domW)
    {
      domW = w;
      domBone = skinning.bones[i].bone;
    }
  }
}

void ConvexComponent::CalcOBB(Vector3 *rect, float xResize, float zResize) const
{
  float dx = _whAngle[0] + xResize;
  float dz = _whAngle[1] + zResize;

  Assert(dx > 0.0f);
  Assert(dz > 0.0f);

  rect[0] = Vector3(-dx, _minMax[0].Y(), -dz);
  rect[1] = Vector3(-dx, _minMax[0].Y(), +dz);
  rect[2] = Vector3(+dx, _minMax[0].Y(), +dz);
  rect[3] = Vector3(+dx, _minMax[0].Y(), -dz);

  float s = sinf(_whAngle[2]);
  float c = cosf(_whAngle[2]);

  // rotation and translation OBB in plane XZ
  for (int i = 0; i < 4; i++)
  {
    float tmpX = rect[i].X();
    float tmpZ = rect[i].Z();

    rect[i][0] = _centerOBB.X() + (tmpX * c - tmpZ * s);
    rect[i][2] = _centerOBB.Z() + (tmpX * s + tmpZ * c);
  }
}

void ConvexComponent::Reverse()
{
  ReverseMinMax(_minMax);
  ReverseVector(_center);
  ReverseVector(_centerOBB);
  _whAngle[2] += H_PI;
  if (_whAngle[2]>=2*H_PI) _whAngle[2] -= 2*H_PI;

}

void ConvexComponent::Recalculate(Shape *shape)
{
  int size=_sel.Size();
  if( size>0 )
  {
    int index=_sel[0];
    
    Vector3Val p=shape->Pos(index);
    _minMax[0]=p;
    _minMax[1]=p;

    _center = shape->Pos(index); 
    
    for( int si=1; si<size; si++ )
    {
      int index=_sel[si];
      CheckMinMaxIter(_minMax[0],_minMax[1],shape->Pos(index));
      _center += shape->Pos(index);
    }
    // find bounding center
    _center /= size;
    // find bounding sphere radius
    float maxRadius2=0;
    for( int si=0; si<size; si++ )
    {
      int index=_sel[si];
      float radius2=shape->Pos(index).Distance2(_center);
      saturateMax(maxRadius2,radius2);
    }
    _radius=sqrt(maxRadius2);
  }
  else
  {
    _minMax[0]=VZero;
    _minMax[1]=VZero;
    _center=VZero;
    _radius=0;
  }
  CalculateOOB(shape);
}

void ConvexComponent::RecalculateNormals(Shape *shape) const
{
  for( int i=0; i<NPlanes(); i++ )
  {    
    Plane& plane = shape->GetPlane(_planes[i]);
    const Poly &face=GetFace(shape,i);

    face.CalculateNormal(plane, shape->GetPosArray());
  }
}

bool ConvexComponent::IsInside(const Array<Plane> &planes, Vector3Val point) const
{
  Assert(NPlanes() >= 4);
  for (int i=0; i<NPlanes(); i++)
  {
    const Plane &plane = GetPlane(planes, i);
    float dist = plane.Distance(point);
    if (dist < 0 ) return false;
  }
  return true;
}

int Shape::FindProperty( const RStringB &name ) const
{
  #if _ENABLE_REPORT
    BString<1024> buf;
    strcpy(buf,name);
    strlwr(buf);
    if (strcmp(name,buf))
    {
      Fail("Upper case property find");
    }
  #endif
  for( int i=0; i<_prop.Size(); i++ )
  {
    if( _prop[i].Name()==name) return i;
  }
  return -1;
}

int Shape::FindProperty( const char *name ) const
{
  #if _ENABLE_REPORT
    BString<1024> buf;
    strcpy(buf,name);
    strlwr(buf);
    if (strcmp(name,buf))
    {
      Fail("Upper case property search");
    }
  #endif
  for( int i=0; i<_prop.Size(); i++ )
  {
    //if( !strcmpi(_prop[i].Name(),name) ) return i;
    if( !strcmp(_prop[i].Name(),name) ) return i;
  }
  return -1;
}

void Shape::SetProperty( const char *name, const char *value )
{
  int index = FindProperty(name);
  if (index>=0)
  {
    _prop[index] = NamedProperty(name,value);
    return;
  }
  _prop.Add(NamedProperty(name,value));
}

/**
RStringB searching is faster than const char *
*/
const RStringB &Shape::PropertyValue( const RStringB &name ) const
{
  int index=FindProperty(name);
  if( index<0 ) return RStringBEmpty;
  const NamedProperty &prop=NamedProp(index);
  return prop.Value();
}

const RStringB &Shape::PropertyValue( const char *name ) const
{
  int index=FindProperty(name);
  if( index<0 ) return RStringBEmpty;
  const NamedProperty &prop=NamedProp(index);
  return prop.Value();
}


Vector3 Shape::CalculateCenter( const Selection &sel ) const
{
  Assert(IsGeometryLoaded());
  // calculate geometrical center
  Vector3 sum(VZero);
  if( sel.Size()<=0 ) return sum;
  for( int i=0; i<sel.Size(); i++ ) sum+=Pos(sel[i]);
  return sum*(1/float(sel.Size()));
}

// maintain animations

void AnimationBase::DoConstruct()
{
  for( int level=0; level<MAX_LOD_LEVELS; level++ )
  {
    _bone[level] = SkeletonIndexNone;
  }
}

AnimationBase::AnimationBase()
{
  _initDone = false;
  DoConstruct();
}

int AnimationBase::FindSelection(LODShape *shape, int level) const
{
  const AnimationDef *def = GetDef();
  if (def)
  {
    return FindSelection(shape,level,def->name,def->altName);
  }
  return -1;
}

int AnimationBase::FindSelection(LODShape *shape, int level, const char *nameSel, const char *altName ) const
{
  ShapeUsed lShape=shape->Level(level);
  // check if we got some nonempty name
  if (*nameSel==0 && (!altName || *altName==0))
  {
    // animation disabled - not used at all
    return -1;  
  }
  return shape->FindNamedSel(lShape,nameSel,altName);
}

static inline RStringB SelName(const char *str)
{
  if (!str) return RStringB();
  RString low = str;
  low.Lower();
  return low;
}


void AnimationBase::InitLevel( LODShape *shape, int level, const AnimationDef &def)
{
}
void AnimationBase::DeinitLevel(LODShape *shape, int level)
{
}

void AnimationBase::InitZero()
{
  for( int level=0; level<MAX_LOD_LEVELS; level++ )
  {
    _bone[level] = SkeletonIndexNone;
  }
}

void AnimationBase::Init( LODShape *shape, const char *nameSel, const char *altName )
{
  InitZero();
  // save original positions
  AnimationDef def;
  def.name = SelName(nameSel);
  def.altName = SelName(altName);
  _initDone = true;
}

void AnimationBase::Deinit(LODShape *shape)
{
  Assert(_initDone);
  _initDone = false;
}

void AnimationSectionBase::DoConstruct()
{
  InitZero();
}

AnimationSectionBase::AnimationSectionBase()
{
  DoConstruct();
}

void AnimationSection::Init(LODShape *shape, const char *name, const char *altName)
{
  _def.name = SelName(name);
  _def.altName = SelName(altName);
  _initDone = true;
}

#if _ENABLE_REPORT

void NamedSelection::CheckSectionsNeeded(LODShape *shape, const char *nameSel, const char *altName) const
{
  if (!GetNeedsSections())
  {
    //ParamFile sections;
    //sections.Parse();

    char shortName[256];
    GetFilename(shortName,shape->GetName());
    while (strpbrk(shortName," -/()"))
    {
      *strpbrk(shortName," -/()")='_';
    }

    RptF("Selection missing in CfgModels");
    RptF("  class %s",shortName);
    RptF("    \"%s\",",nameSel);
    if (altName && *altName) LogF("\"%s\",",altName);
  }
  
  //DoAssert(GetNeedsSections());
}
#endif

void AnimationSectionBase::CheckSection( LODShape *shape, int level, const char *nameSel, const char *altName )
{
  #if _ENABLE_REPORT
    ShapeUsed lod = shape->Level(level);
    int si = GetSelection(level);
    if (si<0) return;
    const NamedSelection &sel = lod->NamedSel(si);
    // check if sections are already registered
    sel.CheckSectionsNeeded(shape,nameSel,altName);
  #endif
}

void AnimationSectionBase::ScanIndex(LODShape *shape, int level, const char *nameSel, const char *altName )
{
  ShapeUsed lShape=shape->Level(level);
  // check if we got some nonempty name
  if (*nameSel==0 && (!altName || *altName==0))
  {
    // animation disabled - not used at all
    _selection[level] = -1;
    return;  
  }
  _selection[level]=shape->FindNamedSel(lShape,nameSel,altName);
}

void AnimationSectionBase::InitLevel( LODShape *shape, int level, const AnimationDef &def)
{
  #if CHECK_INIT_SHAPE
    DoAssert(++_initCount[level]>0);
  #endif
  ScanIndex(shape,level,def.name,def.altName);
  CheckSection(shape,level,def.name,def.altName);
}

void AnimationSectionBase::DeinitLevel(LODShape *shape, int level)
{
  #if CHECK_INIT_SHAPE
    DoAssert(--_initCount[level]>=0);
  #endif
  _selection[level]=-1;
}

void AnimationSectionBase::InitZero()
{
  for( int level=0; level<MAX_LOD_LEVELS; level++ )
  {
    _selection[level]=-1;
  }
}

void AnimationSectionBase::Init(LODShape *shape, const char *nameSel, const char *altName)
{
  InitZero();
  AnimationDef def;
  def.name = SelName(nameSel);
  def.altName = SelName(altName);
}

void AnimationSectionBase::Deinit(LODShape *shape)
{
  Assert(_initDone);
  _initDone = false;
}

Texture *AnimationSectionBase::GetTexture(LODShape *shape) const
{
  for (int level=0; level<MAX_LOD_LEVELS; level++)
  {
    if (_selection[level]<0) continue;
    ShapeUsed lShape=shape->Level(level);
    const NamedSelection &sel=lShape->NamedSel(_selection[level]);

    if (sel.NSections()<=0) continue;
    const ShapeSection &sec = lShape->GetSection(sel.GetSection(0));
    return sec.GetTexture();
  }
  return NULL;
}

TexMaterial *AnimationSectionBase::GetMaterial(LODShape *shape) const
{
  for (int level=0; level<MAX_LOD_LEVELS; level++)
  {
    if (_selection[level]<0) continue;
    ShapeUsed lShape=shape->Level(level);
    const NamedSelection &sel=lShape->NamedSel(_selection[level]);

    if (sel.NSections()<=0) continue;
    const ShapeSection &sec = lShape->GetSection(sel.GetSection(0));
    return sec.GetMaterialExt();
  }
  return NULL;
}

void AnimationSectionBase::SetTexture(
  AnimationContext &animContext, LODShape *shape, int level, Texture *texture
) const
{
  #if CHECK_INIT_SHAPE
    DoAssert(_initCount[level]>0);
  #endif
  if( _selection[level]<0 ) return;
  Assert(shape->IsLevelLocked(level));
  const Shape *lShape = shape->GetLevelLocked(level);
  const NamedSelection &sel = lShape->NamedSel(_selection[level]);
  // 
  for (int i=0; i<sel.NSections(); i++)
  {
    int sec = sel.GetSection(i);
    PolyProperties &ss = animContext.SetSection(lShape, sec);
    ss.SetTexture(texture);
  }
}

void AnimationSectionBase::SetMaterial(AnimationContext &animContext, LODShape *shape, int level, TexMaterial *mat ) const
{
  #if CHECK_INIT_SHAPE
    DoAssert(_initCount[level]>0);
  #endif
  if( _selection[level]<0 ) return;
  Assert(shape->IsLevelLocked(level));
  const Shape *lShape = shape->GetLevelLocked(level);
  const NamedSelection &sel=lShape->NamedSel(_selection[level]);
  // 
  for (int i=0; i<sel.NSections(); i++)
  {
    int sec = sel.GetSection(i);
    PolyProperties &ss = animContext.SetSection(lShape, sec);
    ss.SetMaterialExt(mat);
  }
}

void AnimationSectionBase::Hide(AnimationContext &animContext, LODShape *shape, int level ) const
{
  #if CHECK_INIT_SHAPE
    DoAssert(_initCount[level]>0);
  #endif
  if( _selection[level]<0 ) return;
  Assert(shape->IsLevelLocked(level));
  const Shape *lShape = shape->GetLevelLocked(level);
  const NamedSelection &sel=lShape->NamedSel(_selection[level]);
  for (int i=0; i<sel.NSections(); i++)
  {
    int sec = sel.GetSection(i);
    PolyProperties &ss = animContext.SetSection(lShape, sec);
    ss.OrSpecial(IsHidden);
  }
}

void AnimationSectionBase::Unhide(AnimationContext &animContext, LODShape *shape, int level) const
{
  #if CHECK_INIT_SHAPE
    DoAssert(_initCount[level]>0);
  #endif
  if( _selection[level]<0 ) return;
  Assert(shape->IsLevelLocked(level));
  const Shape *lShape = shape->GetLevelLocked(level);
  const NamedSelection &sel=lShape->NamedSel(_selection[level]);
  for (int i=0; i<sel.NSections(); i++)
  {
    int sec = sel.GetSection(i);
    PolyProperties &ss = animContext.SetSection(lShape, sec);
    ss.AndSpecial(~IsHidden);
  }
}

AnimationRotation::AnimationRotation()
{
}

static bool SelectionPresent(LODShape *shape, int level, const RStringB &axis, bool inMemory)
{
  if( axis.GetLength()==0 ) return false;
  // one point axis, direction is implicit
  const Shape *sShape = NULL;
  ShapeUsed lock;
  if (inMemory) sShape = shape->MemoryLevel();
  else
  {
    lock = shape->Level(level);
    sShape = lock;
  }
  if (!sShape) return false;
  int index = shape->FindNamedSel(sShape,axis);
  return index >= 0;
}

/// definition for two-point axis
struct AnimationRotationAxisDef: public AnimationRotationDef
{
  RStringB axis;
  RStringB altAxis;
  bool inMemory;

  virtual void InitLevel(AnimationRotation &anim, LODShape *shape, int level)
  {
    anim.InitLevel(shape,level,*this);
  }
  virtual bool BoneSupported(LODShape *shape, int level) const
  {
    return SelectionPresent(shape,level,axis,inMemory) || SelectionPresent(shape,level,altAxis,inMemory);
  }
};

/// definition for one-point axis-aligned axis
struct AnimationRotationAxisAlignedAxisDef: public AnimationRotationDef
{
  RStringB axis;
  RStringB altAxis;
  bool inMemory;

  virtual void InitLevel(AnimationRotation &anim, LODShape *shape, int level)
  {
    anim.InitLevel(shape,level,*this);
  }
  virtual bool BoneSupported(LODShape *shape, int level) const
  {
    return SelectionPresent(shape,level,axis,inMemory) || SelectionPresent(shape,level,altAxis,inMemory);
  }
};

/// definition for two-point oriented axis
struct AnimationRotationOrientedAxisDef: public AnimationRotationDef
{
  RStringB begin;
  RStringB end;
  bool inMemory;

  virtual void InitLevel(AnimationRotation &anim, LODShape *shape, int level)
  {
    anim.InitLevel(shape,level,*this);
  }
  virtual bool BoneSupported(LODShape *shape, int level) const
  {
    return SelectionPresent(shape,level,begin,inMemory) && SelectionPresent(shape,level,end,inMemory);
  }
};

/**
@return level used
*/
static ShapeUsedGeometryLock<> FindAxisSelection(
  int &index,
  LODShape *shape, int level,
  bool inMemory, const char *name, const char *altName
)
{
  Assert( name && *name );
  if (!inMemory || shape->FindMemoryLevel()<0)
  {
    // search given level first
    ShapeUsedGeometryLock<> sShape(shape,level);
    index = shape->FindNamedSel(sShape,name,altName);
    if (index>=0)
    {
      return sShape;
    }
    // if not found, proceed to memory level
    if (shape->FindMemoryLevel()<0)
    {
      return ShapeUsedGeometryLock<>();
    }
  }
  // search memory level
  ShapeUsedGeometryLock<> sShape(shape,shape->FindMemoryLevel());
  index = shape->FindNamedSel(sShape,name,altName);
  return sShape;
}

void AnimationRotation::InitLevel(
  LODShape *shape, int level, const AnimationRotationAxisAlignedAxisDef &def
)
{
  // one point axis, direction is implicit
  base::InitLevel(shape,level,def);
  base::RegisterLevel(shape,level);
  if (SkeletonIndexValid(_bone[level]))
  {
    if (def.axis.GetLength()>0)
    {
      int index;
      ShapeUsedGeometryLock<> sShape = FindAxisSelection(index,shape,level,def.inMemory,def.axis,def.altAxis);
      if (index < 0)
      {
        RptF(
          "Missing axis in model %s:%s, sel %s, axis %s",
          (const char *)shape->GetName(),shape->LevelName(shape->Resolution(level)),
          cc_cast(def.name),cc_cast(def.axis)
        );
        _center[level] = VZero;
        _direction[level] = VUp;
        return;
      }
      const NamedSelection::Access &axisSel = sShape->NamedSel(index,shape);
      if (axisSel.Size()<1)
      {
        RptF(
          "Axis has no point in model %s:%s, sel %s",
          (const char *)shape->GetName(),shape->LevelName(shape->Resolution(level)),
          cc_cast(def.axis)
        );
        _center[level]=VZero;
        _direction[level]=VAside;
      }
      else
      {
        // axis should be used as an axis-aligned
        // we need one point - calculate a center of the selection
        _center[level]=sShape->CalculateCenter(axisSel);
        _direction[level]=VAside;
      }
    }
    else
    {
      int selection = FindSelection(shape,level,def.name,def.altName);
      // center of the selection should be used as an axis
      if (selection>=0)
      {
        ShapeUsedGeometryLock<> sShape(shape,level);
        const NamedSelection::Access &sel = sShape->NamedSel(selection,shape);
        if( sel.Size()>0 )
        {
          _center[level]=sShape->CalculateCenter(sel);
        }
        else
        {
          _center[level] = VZero;
        }
        
        _direction[level]=VAside;
      }
      else
      {
        RptF(
          "Implicit axis selection missing %s:%s, sel %s",
          (const char *)shape->GetName(),shape->LevelName(shape->Resolution(level)),
          cc_cast(def.name)
        );
        _center[level] = VZero;
        _direction[level]=VAside;
      }
    }
  }
  else
  {
    _center[level] = VZero;
    _direction[level] = VForward;
  }
}

void AnimationRotation::InitLevel(LODShape *shape, int level, const AnimationRotationAxisDef &def)
{
  base::InitLevel(shape,level,def);
  base::RegisterLevel(shape,level);
  if (SkeletonIndexValid(_bone[level]))
  {
    Assert( def.axis.GetLength()>0 );
    // explicit axis
    Vector3 beg,end;
    if (!FindAxis(beg,end,shape,level,def.inMemory,def.axis,def.altAxis))
    {
      _center[level] = VZero;
      _direction[level] = VAside;
    }
    else
    {
      _center[level] = beg;
      Vector3 direction = end - beg;
      if (direction.SquareSize() > 0.0f)
      {
        _direction[level] = direction.Normalized();
      }
      else
      {
        _direction[level] = VAside;
        RptF("Error: level %f of model %s doesn't have axis %s properly defined", shape->Resolution(level), cc_cast(shape->GetName()), cc_cast(def.name));
      }
    }
  }
  else
  {
    _center[level] = VZero;
    _direction[level] = VForward;
  }
}

void AnimationRotation::InitLevel(LODShape *shape, int level)
{
  Assert(_def);
  if (_def)
  {
    _def->InitLevel(*this,shape,level);
  }
}

void AnimationRotation::InitLevel(
  LODShape *shape, int level, const AnimationRotationOrientedAxisDef &def
)
{
  base::InitLevel(shape,level,def);
  base::RegisterLevel(shape,level);
  int selection = FindSelection(shape,level,def.name,def.altName);
  if (selection >= 0)
  {
    int sLevel = -1;
    ShapeUsed lock;
    if (def.inMemory) sLevel = shape->FindMemoryLevel();
    else
    {
      lock = shape->Level(level);
      sLevel = level;
    }
    if (sLevel>=0)
    {
      Vector3 begP = shape->NamedPoint(sLevel,def.begin);
      Vector3 endP = shape->NamedPoint(sLevel,def.end);

      _center[level] = begP;
      Vector3 direction = endP - begP;
      if (direction.SquareSize() > 0.0f)
      {
        _direction[level] = direction.Normalized();
      }
      else
      {
        _direction[level] = VAside;
        RptF("Error: level %f of model %s doesn't have axis %s properly defined", shape->Resolution(level), cc_cast(shape->GetName()), cc_cast(def.name));
      }
    }
    else
    {
      _center[level] = VZero;
      _direction[level] = VForward;
    }
  }
  else
  {
    _center[level] = VZero;
    _direction[level] = VForward;
  }
}

void AnimationRotation::DeinitLevel(LODShape *shape, int level)
{
  base::DeinitLevel(shape,level);
}
void AnimationRotation::Init(
  LODShape *shape, const char *nameD, const char *altNameD,
  const char *axisD, const char *altAxisD, bool inMemoryD
)
{
  AnimationRotationAxisDef *def = new AnimationRotationAxisDef;
  def->name = SelName(nameD);
  def->altName = SelName(altNameD);
  def->axis = SelName(axisD);
  def->altAxis = SelName(altAxisD);
  def->inMemory = inMemoryD;
  _def = def;
  _initDone = true;
}

void AnimationRotation::InitAxisAlignedAxis(
  LODShape *shape, const char *nameD, const char *altNameD,
  const char *axisD, const char *altAxisD, bool inMemoryD
)
{
  AnimationRotationAxisAlignedAxisDef *def = new AnimationRotationAxisAlignedAxisDef;
  def->name = SelName(nameD);
  def->altName = SelName(altNameD);
  def->axis = SelName(axisD);
  def->altAxis = SelName(altAxisD);
  def->inMemory = inMemoryD;
  _def = def;
  _initDone = true;
}

void AnimationRotation::InitOrientedAxis
(
  LODShape *shape, const char *name, const char *begin, const char *end, bool inMemory
)
{
  AnimationRotationOrientedAxisDef *def = new AnimationRotationOrientedAxisDef;
  def->name = SelName(name);
  def->altName = RStringB();
  def->begin = SelName(begin);
  def->end = SelName(end);
  def->inMemory = inMemory;
  _def = def;
  _initDone = true;
}

void AnimationRotation::Deinit(LODShape *shape)
{
  base::Deinit(shape);
}

void AnimationRotation::GetRotation( Matrix4 &mat, float angle, int level ) const
{
  if (!SkeletonIndexValid(_bone[level]))
  {
    // selection does not exist in LOD
    mat = M4Identity;
    return;
  }

  // rotate around given axis
  // align axis with z-axis or y-axis
  Matrix4 align(MIdentity);
  Vector3Val dir=_direction[level];
  if( fabs(dir*VForward)>0.9 )
  {
    align.SetDirectionAndUp(dir,VUp);
  }
  else
  {
    align.SetDirectionAndUp(dir,VForward);
  }
  Matrix4 invAlign(MInverseRotation,align);
  mat = (
    Matrix4(MTranslation,_center[level])
    *align
    *Matrix4(MRotationZ,angle)
    *invAlign
    *Matrix4(MTranslation,-_center[level])
  );
}

void AnimationRotation::GetRotationX( Matrix4 &mat, float angle, int level ) const
{
  if (!SkeletonIndexValid(_bone[level]))
  {
    // selection does not exist in LOD
    mat = M4Identity;
    return;
  }

  // rotate around given axis
  mat = (
    Matrix4(MTranslation,_center[level])
    *Matrix4(MRotationX,-angle)
    *Matrix4(MTranslation,-_center[level])
  );
}
void AnimationRotation::GetRotationY( Matrix4 &mat, float angle, int level ) const
{
  if (!SkeletonIndexValid(_bone[level]))
  {
    // selection does not exist in LOD
    mat = M4Identity;
    return;
  }

  // rotate around given axis
  mat = (
    Matrix4(MTranslation,_center[level])
    *Matrix4(MRotationY,angle)
    *Matrix4(MTranslation,-_center[level])
  );
}
void AnimationRotation::GetRotationZ( Matrix4 &mat, float angle, int level ) const
{
  if (!SkeletonIndexValid(_bone[level]))
  {
    // selection does not exist in LOD
    mat = M4Identity;
    return;
  }

  // rotate around given axis
  mat = (
    Matrix4(MTranslation,_center[level])
    *Matrix4(MRotationZ,-angle)
    *Matrix4(MTranslation,-_center[level])
  );
}

Vector3Val AnimationRotation::GetCenter(int level) const
{
  if (SkeletonIndexValid(_bone[level])) return _center[level];
  return GetCenter();
}

Vector3Val AnimationRotation::GetCenter() const
{
  for( int i=0; i<MAX_LOD_LEVELS; i++ )
  {
    // TODO: make sure _center is valid
    if (SkeletonIndexValid(_bone[i])) return _center[i];
  }
  return VZero;
}
void AnimationRotation::SetCenter( Vector3Par center, Vector3Par direction )
{
  for( int i=0; i<MAX_LOD_LEVELS; i++ ) _center[i]=center,_direction[i]=direction;
}

void AnimationRotation::SerializeBin(SerializeBinStream &f)
{
  base::SerializeBin(f);
}


void AnimationRotation::Reverse(LODShape *shape)
{
  for (int level=0; level<shape->NLevels(); level++)
  {
    if (SkeletonIndexValid(_bone[level]))
    {
      ReverseVector(_center[level]);
      ReverseVector(_direction[level]);
    }
  }
}

void AnimationRotation::SerializeBin(SerializeBinStream &f, int level)
{
  base::SerializeBin(f,level);
  if (SkeletonIndexValid(_bone[level]))
  {
    f.Transfer(_center[level]);
    f.Transfer(_direction[level]);
  }
}

SkeletonIndex AnimationRotation::BoneSupported(LODShape *shape, int level) const
{
  SkeletonIndex bone = base::BoneSupported(shape,level);
  if (!SkeletonIndexValid(bone)) return bone;
  if (!_def->BoneSupported(shape,level)) return SkeletonIndexNone;
  return bone;
}

AnimationTranslation::AnimationTranslation()
{
}

struct AnimationTranslationAxisDef: public AnimationTranslationDef
{
  RStringB axis;
  RStringB altAxis;
  bool inMemory;
  
  virtual void InitLevel(AnimationTranslation &anim, LODShape *shape, int level)
  {
    anim.InitLevel(shape,level,*this);
  }
  virtual bool BoneSupported(LODShape *shape, int level) const
  {
    return SelectionPresent(shape,level,axis,inMemory) || SelectionPresent(shape,level,altAxis,inMemory);
  }
};

struct AnimationTranslationOrientedAxisDef: public AnimationTranslationDef
{
  RStringB begin;
  RStringB end;
  bool inMemory;

  virtual void InitLevel(AnimationTranslation &anim, LODShape *shape, int level)
  {
    anim.InitLevel(shape,level,*this);
  }
  virtual bool BoneSupported(LODShape *shape, int level) const
  {
    return SelectionPresent(shape,level,begin,inMemory) && SelectionPresent(shape,level,end,inMemory);
  }
};

/// definition for axis-aligned axis
struct AnimationTranslationAxisAlignedAxisDef: public AnimationTranslationDef
{
  RStringB axis;
  bool inMemory;
  
  // axis aligned translation does not need any additional info
  virtual void InitLevel(AnimationTranslation &anim, LODShape *shape, int level)
  {
    anim.InitLevel(shape,level,*this);
  }
  virtual bool BoneSupported(LODShape *shape, int level) const
  {
    // axis aligned translation always supported
    return true;
  }
};

bool AnimationBase::FindAxis(
  Vector3 &beg, Vector3 &end, LODShape *shape, int level,
  bool inMemory, RString axis, RString altAxis
) const
{
  int index;
  ShapeUsedGeometryLock<> sShape = FindAxisSelection(index,shape,level,inMemory,axis,altAxis);
  if (index < 0)
  {
    RptF(
      "Missing axis in model %s:%s, axis %s",
      (const char *)shape->GetName(),shape->LevelName(shape->Resolution(level)),
      cc_cast(axis)
    );
    return false;
  }
  const NamedSelection::Access &axisSel = sShape->NamedSel(index,shape);
  if (axisSel.Size()<2)
  {
    RptF(
      "Axis has less than two points, in model %s:%s, sel %s",
      (const char *)shape->GetName(),shape->LevelName(shape->Resolution(level)),
      cc_cast(axis)
    );
    // we can get one point, but not two
    beg = sShape->Pos(axisSel[0]);
    end = beg+Vector3(0.001f,0,0); // almost no translation, but enough to give rotation axis
    return true;
  }
  else
  {
    beg = sShape->Pos(axisSel[0]);
    end = sShape->Pos(axisSel[1]);
    // if there are more points, use the two most distant one
    float maxDist2 = beg.Distance2(end);
    for (int ei=2; ei<axisSel.Size(); ei++)
    {
      Vector3 e = sShape->Pos(axisSel[ei]);
      float dist2 = beg.Distance2(e);
      if (maxDist2<dist2)
      {
        maxDist2 = dist2;
        end = e;
      }
    }
    return true;
  }
}

void AnimationTranslation::InitLevel(LODShape *shape, int level, const AnimationTranslationAxisDef &def)
{
  base::InitLevel(shape, level, def);
  base::RegisterLevel(shape, level);
  if (SkeletonIndexValid(_bone[level]))
  {
    Assert(def.axis.GetLength() > 0);
    Vector3 beg,end;
    if (!FindAxis(beg,end,shape,level,def.inMemory,def.axis,def.altAxis))
    {
      _direction[level] = VZero;
    }
    else
    {
      _direction[level] = end - beg;
    }
  }
  else
  {
    _direction[level] = VZero;
  }
  _center[level] = VZero;
}

void AnimationTranslation::InitLevel(LODShape *shape, int level)
{
  Assert(_def);
  if (_def)
  {
    _def->InitLevel(*this,shape,level);
  }
}

void AnimationTranslation::InitLevel(
  LODShape *shape, int level, const AnimationTranslationOrientedAxisDef &def
)
{
  base::InitLevel(shape,level,def);
  base::RegisterLevel(shape,level);
  if (SkeletonIndexValid(_bone[level]))
  {
    int sLevel = -1;
    ShapeUsed lock;
    if (def.inMemory) sLevel = shape->FindMemoryLevel();
    else
    {
      lock = shape->Level(level);
      sLevel = level;
    }
    if (sLevel>=0)
    {
      Vector3 beg = shape->NamedPoint(sLevel,def.begin);
      Vector3 end = shape->NamedPoint(sLevel,def.end);
      _direction[level] = end - beg;
    }
    else
    {
      _direction[level] = VZero;
    }
  }
  else
  {
    _direction[level] = VZero;
  }
  _center[level] = VZero;
}

void AnimationTranslation::DeinitLevel(LODShape *shape, int level)
{
  base::DeinitLevel(shape,level);
}

void AnimationTranslation::Init
(
  LODShape *shape, const char *nameD, const char *altNameD,
  const char *axisD, const char *altAxisD, bool inMemoryD
)
{
  AnimationTranslationAxisDef *def = new AnimationTranslationAxisDef;
  def->name = SelName(nameD);
  def->altName = SelName(altNameD);
  def->axis = SelName(axisD);
  def->altAxis = SelName(altAxisD);
  def->inMemory = inMemoryD;
  _def = def;
  _initDone = true;
}

void AnimationTranslation::InitOrientedAxis
(
  LODShape *shape, const char *name, const char *begin, const char *end, bool inMemory
)
{
  AnimationTranslationOrientedAxisDef *def = new AnimationTranslationOrientedAxisDef;
  def->name = SelName(name);
  def->altName = RStringB();
  def->begin = SelName(begin);
  def->end = SelName(end);
  def->inMemory = inMemory;
  _def = def;
  _initDone = true;
}

void AnimationTranslation::InitAxisAlignedAxis(
  LODShape *shape, const char *name, const char *axisName, bool inMemory
)
{
  AnimationTranslationAxisAlignedAxisDef *def = new AnimationTranslationAxisAlignedAxisDef;
  def->name = SelName(name);
  def->altName = RStringB();
  def->axis = SelName(axisName);
  def->inMemory = inMemory;
  _def = def;
  _initDone = true;
}

void AnimationTranslation::InitLevel(
  LODShape *shape, int level, const AnimationTranslationAxisAlignedAxisDef &def
)
{
  base::InitLevel(shape,level,def);
  base::RegisterLevel(shape,level);

  // direction is not important here
  _direction[level] = VZero;
  if (def.axis.GetLength()>0)
  {
    int selection;
    ShapeUsedGeometryLock<> sShape = FindAxisSelection(selection,shape,level,def.inMemory,def.axis,NULL);
    if (selection>=0)
    {
      const NamedSelection::Access &sel = sShape->NamedSel(selection,shape);
      if( sel.Size()>0 )
      {
        _center[level]=sShape->CalculateCenter(sel);
      }
    }
    else
    {
      _center[level]=VZero;
    }
  }
  else
  {
    int selection = FindSelection(shape,level,def.name,def.altName);
    if (selection>=0)
    {
      ShapeUsedGeometryLock<> lShape(shape,level);
      const NamedSelection::Access &sel = lShape->NamedSel(selection,shape);
      if( sel.Size()>0 )
      {
        _center[level]=lShape->CalculateCenter(sel);
      }
    }
    else
    {
      _center[level]=VZero;
    }
  }
}

SkeletonIndex AnimationTranslation::BoneSupported(LODShape *shape, int level) const
{
  SkeletonIndex bone = base::BoneSupported(shape,level);
  if (!SkeletonIndexValid(bone)) return bone;
  if (!_def->BoneSupported(shape,level)) return SkeletonIndexNone;
  return bone;
}

void AnimationTranslation::Deinit(LODShape *shape)
{
  base::Deinit(shape);
}

void AnimationTranslation::GetTranslation(Matrix4 &mat, float offset, int level) const
{
  if (!SkeletonIndexValid(_bone[level]))
  {
    // selection does not exist in LOD
    mat = M4Identity;
    return;
  }
  // translate in given direction
  mat = Matrix4(MTranslation, offset * _direction[level]);
}


Vector3Val AnimationTranslation::GetCenter() const
{
  for( int i=0; i<MAX_LOD_LEVELS; i++ )
  {
    if (SkeletonIndexValid(_bone[i])) return _center[i];
  }
  return VZero;
}
Vector3Val AnimationTranslation::GetCenter(int level) const
{
  if (SkeletonIndexValid(_bone[level])) return _center[level];
  return GetCenter();
}

void AnimationTranslation::SerializeBin(SerializeBinStream &f)
{
  base::SerializeBin(f);
}

void AnimationTranslation::SerializeBin(SerializeBinStream &f, int level)
{
  base::SerializeBin(f,level);
  if (SkeletonIndexValid(_bone[level]))
  {
    f.Transfer(_center[level]);
    f.Transfer(_direction[level]);
  }
}

void AnimationTranslation::Reverse(LODShape *shape)
{
  for (int level=0; level<shape->NLevels(); level++)
  {
    if (SkeletonIndexValid(_bone[level]))
    {
      ReverseVector(_center[level]);
      ReverseVector(_direction[level]);
    }
  }
}

void AnimationTranslationAxisAlignedBaseType::Init(ParamEntryPar cls, LODShape *shape)
{
  base::Init(cls, shape);
  
  RString selection = cls >> "selection";
  bool memory = true;
  ConstParamEntryPtr entry = cls.FindEntry("memory");
  if (entry) memory = *entry;

  entry = cls.FindEntry("axis");
  if (entry)
  {
    RString axis = *entry;
    _animation.InitAxisAlignedAxis(shape, selection, axis, memory);
  }
  else
  {
    _animation.InitAxisAlignedAxis(shape, selection, NULL, memory);
  }  
  
  // offset not used for axis aligned animation
  _offset0 = _offset1 = 0;
}

/**
Common implementations check if axis is know for given LOD level.
*/

SkeletonIndex AnimationBase::BoneSupported(LODShape *shape, int level) const
{
  const AnimationDef *def = GetDef();
  if (def)
  {
    SkeletonIndex bone = shape->GetSkeleton()->FindBone(def->name);
    if (!SkeletonIndexValid(bone) && def->altName.GetLength()>0)
    {
      bone = shape->GetSkeleton()->FindBone(def->altName);
    }
    return bone;
  }
  return SkeletonIndexNone;
}


void AnimationBase::SerializeBin(SerializeBinStream &f)
{
}

void AnimationBase::SerializeBin(SerializeBinStream &f, int level)
{
  // if serializing, init count is not used
  int selection = -1;
  if (f.GetVersion()<32)
  {
    f.Transfer(selection);
    
  }
  f.TransferLittleEndian(_bone[level]);

  // make sure no bone is invalid when selection is valid - some animations rely on this

  // old condition: (SkeletonIndexValid(_bone[level]) || _selection[level]>=0)
  // new condition (SkeletonIndexValid(_bone[level]))
  // old!=new when !SkeletonIndexValid(_bone[level]) && _selection[level]>=0
  DoAssert(SkeletonIndexValid(_bone[level]) || selection<0);
}


void AnimationBase::RegisterLevel(LODShape *shape, int level)
{
  if (shape->GetAnimationType()!=AnimTypeHardware) return;
  int selection = FindSelection(shape,level);
  if (selection>=0)
  {
    ShapeUsed sShape = shape->Level(level);
    const NamedSelection &ns = sShape->NamedSel(selection);
    SkeletonIndex si = shape->FindBone(ns.GetName());
    _bone[level] = si;
  }
  else
  {
    _bone[level] = BoneSupported(shape,level);
  }
  #if _ENABLE_REPORT
  if (SkeletonIndexValid(_bone[level]) && shape->GetAnimationType()==AnimTypeNone)
  {
    RptF("Shape %s not marked as animated",(const char *)shape->GetName());
  }
  #endif
}

void Animation::Init(LODShape *shape, const char *nameSel, const char *altName )
{
  InitZero();
  // save original positions
  _def.name = SelName(nameSel);
  _def.altName = SelName(altName);
  _initDone = true;
}

void Animation::InitLevel(LODShape *shape, int level)
{
  base::InitLevel(shape,level,_def);
  base::RegisterLevel(shape,level);
}

void Animation::Reverse(LODShape *shape)
{
  // nothing to reverse on this level
}

// texture animations
void AnimationUV::DoConstruct()
{
}

void AnimationUV::InitLevel(LODShape *shape, int level)
{
  ShapeUsed lock = shape->Level(level);
  if (lock.IsNull()) return;
  Shape *lShape = lock.InitShape();

  _selection[level]=shape->FindNamedSel(lShape,_def.name,_def.altName);
      }

void AnimationUV::Init( LODShape *shape, const char *nameSel )
{
  _def.name = SelName(nameSel);
  _def.altName = RStringB();

  for( int level=0; level<MAX_LOD_LEVELS; level++ ) _selection[level]=-1;
  }

void AnimationUV::DeinitLevel( LODShape *shape, int level)
{
}

void AnimationUV::UVOffset(
  AnimationContext &animContext, LODShape *shape, float offsetU, float offsetV, int level
) const
{
  if (_selection[level] < 0) return;

      ShapeUsed lShape = shape->Level(level);
  if( lShape.IsNull() ) return;

  if (!shape->IsShadowVolume(level))
  {
    if (GetSelection(level) >= 0)
    {
      const NamedSelection &namedSel = lShape->NamedSel(GetSelection(level));
      for (int i = 0; i < namedSel.NSections(); i++)
      {
        animContext.SetUVOffset(lShape, namedSel.GetSection(i),offsetU,offsetV);
      }
    }
  }
}


void AnimationAnimatedTexture::DoConstruct()
{
}

void AnimationAnimatedTexture::InitLevel(LODShape *shape, int level, const AnimationDef &def)
{
  base::InitLevel(shape,level, def);
  int sel = _selection[level];
  if (sel < 0) return;
  ShapeUsed lShape = shape->Level(level);
  const NamedSelection &ns = lShape->NamedSel(sel);
  _animTexS[level].Realloc(ns.NSections());
  // section-based
  // note: section must already be initialized
  // CfgModels is needed for this (and corresponding FindSections call)
  for (int i=0; i<ns.NSections(); i++)
  {
    int si = ns.GetSection(i);
    const ShapeSection &sec = lShape->GetSection(si);
    _animTexS[level][i] = sec.GetTexture();
  }
}

void AnimationAnimatedTexture::DeinitLevel(LODShape *shape, int level)
{
  _animTexS[level].Free();
  base::DeinitLevel(shape,level);
}

void AnimationAnimatedTexture::Init(LODShape *shape, const char *nameSel, const char *altNameSel)
{
  InitZero();
  // init animated textures
  // scan faces
  _def.name = SelName(nameSel);
  _def.altName = SelName(altNameSel);
  /*
  for (int level=0; level<shape->NLevels(); level++)
  {
    InitLevel(shape,level);
  }
  */
  _initDone = true;
}

void AnimationAnimatedTexture::Deinit(LODShape *shape)
{
  /*
  for (int level=0; level<shape->NLevels(); level++)
  {
    DeinitLevel(shape,level);
  }
  */
  Assert(_initDone);
  _initDone = false;
}

void AnimationAnimatedTexture::SetPhaseLoadedOnly(AnimationContext &animContext, LODShape *shape, int level, int phase) const
{
  Assert(phase >= 0);
  if (_selection[level] < 0) return;
  Assert(shape->IsLevelLocked(level));
  const Shape *lShape = shape->GetLevelLocked(level);
  const NamedSelection &sel = lShape->NamedSel(_selection[level]);
  // section based
  for (int i=0; i<sel.NSections(); i++)
  {
    int si = sel.GetSection(i);
    PolyProperties &ss = animContext.SetSection(lShape, si);

    Texture *texture = _animTexS[level][i];
    if (!texture || phase >= texture->AnimationLength()) continue;

    ss.SetTexture(texture->GetAnimationLoaded(phase));
    
  }
}

bool AnimationAnimatedTexture::RequestPhase(LODShape *shape, int level, int phase) const
{
  Assert(phase >= 0);
  if (_selection[level] < 0) return true;
  Assert(shape->IsLevelLocked(level));
  const Shape *lShape = shape->GetLevelLocked(level);
  const NamedSelection &sel = lShape->NamedSel(_selection[level]);
  // section based
  bool ret = true;
  for (int i=0; i<sel.NSections(); i++)
  {
    Texture *texture = _animTexS[level][i];
    if (!texture || phase >= texture->AnimationLength()) continue;

    bool ready = texture->RequestAnimation(phase,0);
    if (!ready) ret = false;
  }
  return ret;
}

void AnimationAnimatedTexture::SetPhase(AnimationContext &animContext, LODShape *shape, int level, int phase) const
{
  Assert(phase >= 0);
  if (_selection[level] < 0) return;
  Assert(shape->IsLevelLocked(level));
  const Shape *lShape = shape->GetLevelLocked(level);
  const NamedSelection &sel = lShape->NamedSel(_selection[level]);
  // section based
  for (int i=0; i<sel.NSections(); i++)
  {
    int si = sel.GetSection(i);
    PolyProperties &ss = animContext.SetSection(lShape, si);

    Texture *texture = _animTexS[level][i];
    if (!texture || phase >= texture->AnimationLength()) continue;

    ss.SetTexture(texture->GetAnimation(phase));
  }
}

void AnimationAnimatedTexture::AnimateTexture(AnimationContext &animContext, LODShape *shape, int level, float anim) const
{
  if (_selection[level] < 0) return;
  Assert(shape->IsLevelLocked(level));
  const Shape *lShape = shape->GetLevelLocked(level);
  const NamedSelection &sel = lShape->NamedSel(_selection[level]);
  for (int i=0; i<sel.NSections(); i++)
  {
    int si = sel.GetSection(i);
    PolyProperties &ss = animContext.SetSection(lShape, si);

    Texture *texture = _animTexS[level][i];
    if (!texture) continue;
    int n = texture->AnimationLength();
    if (n <= 1) continue; // no animation
    int phase = toIntFloor(anim * n);
    ss.SetTexture(texture->GetAnimation(phase));
  }
}

bool AnimationAnimatedTexture::PreloadTexture(LODShape *shape, float z2) const
{
  for (int level=0; level<shape->NLevels(); level++)
  {
    if (_selection[level] < 0) continue;
    // use only levels which are loaded
    if (!shape->IsLevelLocked(level)) continue;
    // we could verify other levels use the same texture
    return PreloadTexture(shape,level,z2);
  }
  return true;
}

bool AnimationAnimatedTexture::PreloadTexture(LODShape *shape, int level, float z2) const
{
  if (_selection[level] < 0) return true;

  bool ret = true;
  
  Assert(shape->IsLevelLocked(level));
  const Shape *lShape = shape->GetLevelLocked(level);
  const NamedSelection &sel = lShape->NamedSel(_selection[level]);
  AnimatedTexture *lastTex = NULL;
  for (int i=0; i<sel.NSections(); i++)
  {
    int si = sel.GetSection(i);
    const ShapeSection &ss = lShape->GetSection(si);

    Texture *texture = _animTexS[level][i];
    if (!texture) continue;
    
		AnimatedTexture *anim = texture->GetAnimatedTexture();
		// it is quite common to have multiple sections with the same texture
		// avoid preloading it multiple times
		if (lastTex==anim) continue;
		lastTex = anim;
		for (int i=0; i<anim->Size(); i++)
		{
			Texture *atex = anim->Get(i);
    	if (!atex->PreloadTexture(z2,ss.GetAreaOverTex(0),false)) ret = false;
		}

  }  
  return ret;
}


void AnimationType::Init(ParamEntryPar cls, LODShape *shape)
{
  _name = cls.GetName();
  ConstParamEntryPtr sourceEntry = cls.FindEntry("source");
  if (sourceEntry)
  {
    _source = *sourceEntry;
  }
  
  _minValue = 0;
  _maxValue = 1;
  ConstParamEntryPtr minValue = cls.FindEntry("minValue");
  ConstParamEntryPtr maxValue = cls.FindEntry("maxValue");
  if (minValue)
  {
    _minValue = *minValue;  
  }
  if (maxValue)
  {
    _maxValue = *maxValue;
  }

  _minPhase = _minValue;
  _maxPhase = _maxValue;
  ConstParamEntryPtr minPhase = cls.FindEntry("phaseBeg");
  ConstParamEntryPtr maxPhase = cls.FindEntry("phaseEnd");
  if (minPhase)
  {
    _minPhase = *minPhase;  
  }
  if (maxPhase)
  {
    _maxPhase = *maxPhase;
  }

  _address = AnimClamp;
  ConstParamEntryPtr sourceAddress = cls.FindEntry("sourceAddress");
  if (sourceAddress)
  {
    int address = GetEnumValue<AnimAddress>(*sourceAddress);
    if (address<0 || address>=NAnimAddress)
    {
      address = AnimClamp;
    }
    _address = AnimAddress(address);
  }
}

const EnumName AnimAddressNames[]=
{
  EnumName(AnimationType::AnimClamp,"Clamp"),
  EnumName(AnimationType::AnimLoop,"Loop"),
  EnumName(AnimationType::AnimMirror,"Mirror"),
  EnumName()
};

template <>
const EnumName *GetEnumNames<AnimationType::AnimAddress>(AnimationType::AnimAddress dummy)
{
  return AnimAddressNames;
}

void AnimationType::MinMaxValue(float &minVal, float &maxVal) const
{
  switch (_address)
  {
    case AnimMirror:
      minVal = -FLT_MAX;
      maxVal = +FLT_MAX;
      return;
    case AnimLoop:
      minVal = -FLT_MAX;
      maxVal = +FLT_MAX;
      return;
    default: //case AnimClamp:
      minVal = floatMax(_minPhase, _minValue);
      maxVal = floatMax(_maxPhase, _maxValue);
      return;
  }
}

// time domain transformation
float AnimationType::GetPhase(float phase, float minResult, float maxResult) const
{
  switch (_address)
  {
    case AnimMirror:
      // loop in double  interval
      phase = fastFmod(phase-_minValue,(_maxValue-_minValue)*2)+_minValue;
      // when over limit, mirror
      if (phase>_maxValue) phase = _maxValue - (phase-_maxValue);
      // 
      saturate(phase, _minPhase, _maxPhase);
      return Interpolativ(phase, _minValue, _maxValue, minResult, maxResult);
    case AnimLoop:
      phase = fastFmod(phase-_minValue,_maxValue-_minValue)+_minValue;
    // pass through - no break
    default: //case AnimClamp:
      saturate(phase, _minPhase, _maxPhase);
      return Interpolativ(phase, _minValue, _maxValue, minResult, maxResult);
  }
}

void AnimationRotationType::Init(ParamEntryPar cls, LODShape *shape)
{
  base::Init(cls, shape);
  
  RString selection = cls >> "selection";
  bool memory = true;
  ConstParamEntryPtr entry = cls.FindEntry("memory");
  if (entry) memory = *entry;
  entry = cls.FindEntry("axis");
  if (entry)
  {
    RString axis = *entry;
    _animation.Init(shape, selection, NULL, axis, NULL, memory);
  }
  else
  {
    RString begin = cls >> "begin";
    RString end = cls >> "end";
    _animation.InitOrientedAxis(shape, selection, begin, end, memory);
  }
  _angle0 = cls >> "angle0";
  _angle1 = cls >> "angle1";
}

void AnimationRotationAxisAlignedBaseType::Init(ParamEntryPar cls, LODShape *shape)
{
  base::Init(cls, shape);
  
  RString selection = cls >> "selection";
  bool memory = true;
  ConstParamEntryPtr entry = cls.FindEntry("memory");
  if (entry) memory = *entry;
  RString axis = cls>>"axis";
  _animation.InitAxisAlignedAxis(shape, selection, NULL, axis, NULL, memory);
  _angle0 = cls >> "angle0";
  _angle1 = cls >> "angle1";
}

void AnimationRotationType::InitLevel(LODShape *shape, int level)
{
  _animation.InitLevel(shape, level);
}
void AnimationRotationType::DeinitLevel(LODShape *shape, int level)
{
  _animation.DeinitLevel(shape, level);
}

SkeletonIndex AnimationRotationType::GetBone(int level) const
{
  return _animation.GetBone(level);
}

Vector3Val AnimationRotationType::GetCenter(int level) const
{
  return _animation.GetCenter(level);
}

Vector3Val AnimationRotationType::GetCenter() const
{
  return _animation.GetCenter();
}


void AnimationRotationType::SerializeBin(SerializeBinStream &f)
{
  base::SerializeBin(f);
  f.Transfer(_angle0);
  f.Transfer(_angle1);
  _animation.SerializeBin(f);
}
void AnimationRotationType::SerializeBin(SerializeBinStream &f, int level)
{
  base::SerializeBin(f,level);
  _animation.SerializeBin(f,level);
}
void AnimationRotationType::Reverse(LODShape *shape)
{
  _animation.Reverse(shape);
}

Matrix4 AnimationRotationType::GetAnimMatrix(int level, float phase) const
{
  float angle = GetPhase(phase, _angle0, _angle1);
  Matrix4 rot;
  _animation.GetRotation(rot,angle,level);
  return rot;
}

void AnimationTranslationType::Init(ParamEntryPar cls, LODShape *shape)
{
  base::Init(cls, shape);

  RString selection = cls >> "selection";
  bool memory = true;
  ConstParamEntryPtr entry = cls.FindEntry("memory");
  if (entry) memory = *entry;
  entry = cls.FindEntry("axis");
  if (entry)
  {
    RString axis = *entry;
    _animation.Init(shape, selection, NULL, axis, NULL, memory);
  }
  else
  {
    RString begin = cls >> "begin";
    RString end = cls >> "end";
    _animation.InitOrientedAxis(shape, selection, begin, end, memory);
  }
  _offset0 = cls >> "offset0";
  _offset1 = cls >> "offset1";
}

void AnimationTranslationType::InitLevel(LODShape *shape, int level)
{
  _animation.InitLevel(shape, level);
}

void AnimationTranslationType::DeinitLevel(LODShape *shape, int level)
{
  _animation.DeinitLevel(shape, level);
}

SkeletonIndex AnimationTranslationType::GetBone(int level) const
{
  return _animation.GetBone(level);
}

Matrix4 AnimationTranslationType::GetAnimMatrix(int level, float phase) const
{
  float offset = GetPhase(phase, _offset0, _offset1);
  
  Matrix4 transform;
  _animation.GetTranslation(transform, offset, level);
  return transform;
}

Vector3Val AnimationTranslationType::GetCenter() const
{
  return _animation.GetCenter();
}

Vector3Val AnimationTranslationType::GetCenter(int level) const
{
  return _animation.GetCenter(level);
}


void AnimationTranslationType::SerializeBin(SerializeBinStream &f)
{
  base::SerializeBin(f);
  f.Transfer(_offset0);
  f.Transfer(_offset1);
  _animation.SerializeBin(f);
}
void AnimationTranslationType::SerializeBin(SerializeBinStream &f, int level)
{
  base::SerializeBin(f,level);
  _animation.SerializeBin(f,level);
}

void AnimationTranslationType::Reverse(LODShape *shape)
{
  _animation.Reverse(shape);
}


/// generic matrix animation
class AnimationGenericType : public AnimationType
{
protected:
  typedef AnimationType base;

  Animation _animation;
  /// axis position
  Vector3 _axisPos;
  /// axis direction
  Vector3 _axisDir;
  /// angle of rotation around the axis
  float _angle;
  /// translation along the axis
  float _offset;

public:
  void Init(ParamEntryPar cls, LODShape *shape);

  virtual void InitLevel(LODShape *shape, int level);
  virtual void DeinitLevel(LODShape *shape, int level);

  SkeletonIndex GetBone(int level) const;
  Matrix4 GetAnimMatrix(int level, float phase) const;
  Vector3Val GetCenter() const;
  Vector3Val GetCenter(int level) const;
  virtual TypeId GetType() const {return TIdDirect;}
  virtual void SerializeBin(SerializeBinStream &f);
  virtual void SerializeBin(SerializeBinStream &f, int level);
  virtual void Reverse(LODShape *shape);
  virtual size_t GetMemoryControlled() const {return sizeof(*this);}
};


void AnimationGenericType::Init(ParamEntryPar cls, LODShape *shape)
{
  base::Init(cls, shape);

  RString selection = cls >> "selection";
  _animation.Init(shape, selection, NULL);
  Quaternion<float> q;
  Vector3 pos;
  GetValue(_axisDir,cls >>"axisDir");
  GetValue(_axisPos,cls >>"axisPos");
  _offset = cls >> "axisOffset";
  _angle = HDegree(cls>>"angle");
  // convert axis-angle to quaternion?
}

void AnimationGenericType::InitLevel(LODShape *shape, int level)
{
  _animation.InitLevel(shape, level);
}

void AnimationGenericType::DeinitLevel(LODShape *shape, int level)
{
  _animation.DeinitLevel(shape, level);
}

SkeletonIndex AnimationGenericType::GetBone(int level) const
{
  return _animation.GetBone(level);
}

Matrix4 AnimationGenericType::GetAnimMatrix(int level, float phase) const
{
  float factor = GetPhase(phase, 0, 1);

  // perform rotation and translation in axis space
  Matrix4 rotation(MRotationZ,-_angle*factor);
  rotation.SetPosition(Vector3(0,0,_offset*factor));
  // convert from axis space to transform space
  Matrix4 axisSpace;
  Vector3 up = _axisDir.CrossProduct(VUp)+_axisDir.CrossProduct(VForward);
  axisSpace.SetDirectionAndUp(_axisDir,up);
  axisSpace.SetPosition(_axisPos);
  
  return axisSpace*rotation*axisSpace.InverseRotation();
}

Vector3Val AnimationGenericType::GetCenter() const
{
  Fail("No center for generic matrix animation");
  return VZero;
}

Vector3Val AnimationGenericType::GetCenter(int level) const
{
  Fail("No center for generic matrix animation");
  return VZero;
}
void AnimationGenericType::SerializeBin(SerializeBinStream &f)
{
  base::SerializeBin(f);
  f.Transfer(_axisPos);
  f.Transfer(_axisDir);
  f.Transfer(_angle);
  f.Transfer(_offset);
  _animation.SerializeBin(f);
}
void AnimationGenericType::SerializeBin(SerializeBinStream &f, int level)
{
  base::SerializeBin(f,level);
  _animation.SerializeBin(f,level);
}
void AnimationGenericType::Reverse(LODShape *shape)
{
  ReverseVector(_axisPos);
  ReverseVector(_axisDir);
  _animation.Reverse(shape);
}
// hide animation
class AnimationHideType : public AnimationType
{
protected:
  typedef AnimationType base;

  float _hideValue;

  Animation _animation;

public: 
  void Init(ParamEntryPar cls, LODShape *shape);

  virtual void InitLevel(LODShape *shape, int level);
  virtual void DeinitLevel(LODShape *shape, int level);

  Matrix4 GetAnimMatrix(int level, float phase) const;
  virtual SkeletonIndex GetBone(int level) const;

  virtual void SerializeBin(SerializeBinStream &f);
  /// binary save/load - per-level info
  virtual void SerializeBin(SerializeBinStream &f, int level);
    
  /// reverse shape - mirror X/Z, leave Y
  /** needed for LODShape::Reverse */
  virtual void Reverse(LODShape *shape) {}; // nothing to reverse

  virtual TypeId GetType() const {return TIdHide;}  
  virtual size_t GetMemoryControlled() const {return sizeof(*this);}
};

void AnimationHideType::Init(ParamEntryPar cls, LODShape *shape)
{
  base::Init(cls, shape);
  RString selection = cls >> "selection";
  ConstParamEntryPtr hideValue = cls.FindEntry("hideValue");
  if (hideValue)
    _hideValue = *hideValue;  
  else
    _hideValue = _maxPhase;
  _animation.Init(shape, selection, NULL);
}

void AnimationHideType::InitLevel(LODShape *shape, int level)
{
  _animation.InitLevel(shape, level);
}

void AnimationHideType::DeinitLevel(LODShape *shape, int level)
{
  _animation.DeinitLevel(shape, level);
}

Matrix4 AnimationHideType::GetAnimMatrix(int level, float phase) const 
{
  float hide = GetPhase(phase, 0, 1);

#if _VBS2 //neg. hidevalue changes direction of hide controller edge
  if(_hideValue < 0)
  { 
    if(-_hideValue < hide)
      return M4Identity;  
  }else
#endif
  if (hide < _hideValue)
    return M4Identity;  

  return M4Zero; 
}

SkeletonIndex AnimationHideType::GetBone(int level) const
{
  return _animation.GetBone(level);
}

void AnimationHideType::SerializeBin(SerializeBinStream &f)
{
  base::SerializeBin(f);
  f.Transfer(_hideValue);
  _animation.SerializeBin(f);
}
/// binary save/load - per-level info
void AnimationHideType::SerializeBin(SerializeBinStream &f, int level)
{
  base::SerializeBin(f, level);
  _animation.SerializeBin(f, level);
}


AnimationHolder AnimationHolder::_empty;


void AnimationHolder::InitBonesToAnimations(LODShape *shape, int level)
{
    AutoArray< AutoArray<int> > &animToBones = _bonesToAnimations[level];
    Skeleton *sk = shape->GetSkeleton();
    if (sk)
    {
      animToBones.Realloc(sk->NBones());
      animToBones.Resize(sk->NBones());
      for (int i=0; i<sk->NBones(); i++)
      {
        animToBones[i].Clear();
      }
      // create a mapping from SkeletonIndex to animation index
      for (int i=0; i<_animations.Size(); i++)
      {
        SkeletonIndex boneIndex = _animations[i]->GetBone(level);
        if (boneIndex!=SkeletonIndexNone)
        {
          animToBones[GetSkeletonIndex(boneIndex)].Add(i);
        }
      }
      for (int i=0; i<sk->NBones(); i++)
      {
        AutoArray<int> &anims = animToBones[i];
        anims.Compact();
        }
      }
}

void AnimationHolder::InitLevel(LODShape *shape, int level)
{
  for (int i=0; i<_animations.Size(); i++)
  {
    _animations[i]->InitLevel(shape,level);
    }

  if (_animations.Size()>0 /*&& shape->GetSkeleton() && _bonesToAnimations[level].Size()==0*/)
  {
    InitBonesToAnimations(shape,level);
  }
}

void AnimationHolder::DeinitLevel(LODShape *shape, int level)
{
  for (int i=0; i<_animations.Size(); i++)
  {
    _animations[i]->DeinitLevel(shape,level);
  }
  if (_animations.Size()>0)
  {
    AutoArray< AutoArray<int> > &animToBones = _bonesToAnimations[level];
    animToBones.Clear();
  }
}

void AnimationHolder::LODShapeLoaded(LODShape *shape)
{
  // resides in the shape - no need to do anything
}

void AnimationHolder::LODShapeUnloaded(LODShape *shape)
{
  // resides in the shape - no need to do anything
}

void AnimationHolder::ShapeLoaded(LODShape *shape, int level)
{
  InitLevel(shape,level);
}
void AnimationHolder::ShapeUnloaded(LODShape *shape, int level)
{
  DeinitLevel(shape,level);
}

/// helper for AnimationHolder::Load
class AnimationHolder::LoadAnimationFromEntry
{
  LODShape *_shape;
  AnimationHolder &_holder;
  
  public:
  LoadAnimationFromEntry(LODShape *shape, AnimationHolder &holder)
  :_shape(shape),_holder(holder)
  {
  }
  bool operator () (ParamEntryPar entry) const
  {
    AnimationType *animType = AnimationType::CreateObject(entry, _shape);
    if (animType) _holder._animations.Add(animType);
    return false;
  }
};

void AnimationHolder::Load(LODShape *shape, ParamEntryPar array)
{
  _animations.Clear();
  array.GetClassInterface()->ForEachEntry(LoadAnimationFromEntry(shape,*this));
  _animations.Compact();
  if (_animations.Size()>0)
  {
    _bonesToAnimations.Realloc(shape->NLevels());
    // make sure all arrays get destructed and constructed again - in case some already exist
    _bonesToAnimations.Resize(0);
    _bonesToAnimations.Resize(shape->NLevels());
  }
  else
  {
    _bonesToAnimations.Clear();
  }
}

void AnimationHolder::Clear()
{
  _animations.Clear();
}

int AnimationHolder::FindIndex(RString name) const
{
  for (int i=0; i<_animations.Size(); i++)
  {
    if (stricmp(_animations[i]->GetName(), name) == 0)
    {
      return i;
    }
  }
  return -1;
}

/**
while removing we need to adjust _bonesToAnimations
*/
void AnimationHolder::Remove(int index)
{
  for (int i=0; i<_bonesToAnimations.Size(); i++) // levels
  {
    AutoArray< AutoArray<int> > &level = _bonesToAnimations[i];
    for (int b=0; b<level.Size(); b++)
    {
      AutoArray<int> &bone = level[b];
      for (int a=0; a<bone.Size(); a++)
      {
        int &aIndex = bone[a];
        Assert(aIndex!=index);
        if (aIndex>index) aIndex--;
      }
    }
  }
  _animations.DeleteAt(index);
}


void AnimationHolder::SerializeBin(SerializeBinStream &f)
{
  // TODO: move as much as possible into levels
  f.TransferRefArray(_animations);
  // transfer bone mapping info
  f.Transfer(_bonesToAnimations);
  
}

size_t AnimationHolder::GetMemoryControlled() const
{
  size_t ret = sizeof(*this);
  ret += _animations.GetMemoryAllocated();
  // not accurate - different types may be used instead of AnimationType
  for (int i=0; i<_animations.Size(); i++)
  {
    const AnimationType *type = _animations[i];
    if (type)
    {
      ret += type->GetMemoryControlled();
    }
  }
  
  ret += _bonesToAnimations.GetMemoryAllocated();
  
  if (_bonesToAnimations.Size()>0)
  {
    // not accurate - various bones may bind to multiple animations
    size_t singleBone = sizeof(AutoArray<int>)+sizeof(int);
    int nBones = _bonesToAnimations[0].Size();
    // each _bonesToAnimations[i] stores AutoArray for each bone containing one index
    ret += _bonesToAnimations.Size()*nBones*singleBone;
  }
  return ret;
}
void AnimationHolder::SerializeBin(SerializeBinStream &f, int level)
{
  for (int i=0; i<_animations.Size(); i++)
  {
    _animations[i]->SerializeBin(f,level);
  }
}

size_t AnimationHolder::GetMemoryControlled(int level) const
{
  return 0;
}

AnimationType *AnimationType::CreateObject(ParamEntryPar cls, LODShape *shape)
{
//  if (!cls.FindEntry("selection"))
//  {
//    // if there is no selection, it is no real animation, and we can discard it
//    return NULL;
//  }
  RString type = cls >> "type";
  if (!strcmpi(type, "disabled"))
  {
    return NULL;
  }
  if (stricmp(type, "rotation") == 0)
  {
    AnimationType *object = new AnimationRotationType();
    object->Init(cls, shape);
    return object;
  }
  else if (stricmp(type, "rotationX") == 0)
  {
    AnimationType *object = new AnimationRotationXType();
    object->Init(cls, shape);
    return object;
  }
  else if (stricmp(type, "rotationY") == 0)
  {
    AnimationType *object = new AnimationRotationYType();
    object->Init(cls, shape);
    return object;
  }
  else if (stricmp(type, "rotationZ") == 0)
  {
    AnimationType *object = new AnimationRotationZType();
    object->Init(cls, shape);
    return object;
  }
  else if (stricmp(type, "translation") == 0)
  {
    AnimationType *object = new AnimationTranslationType();
    object->Init(cls, shape);
    return object;
  }
  else if (stricmp(type, "translationX") == 0)
  {
    AnimationType *object = new AnimationTranslationXType();
    object->Init(cls, shape);
    return object;
  }
  else if (stricmp(type, "translationY") == 0)
  {
    AnimationType *object = new AnimationTranslationYType();
    object->Init(cls, shape);
    return object;
  }
  else if (stricmp(type, "translationZ") == 0)
  {
    AnimationType *object = new AnimationTranslationZType();
    object->Init(cls, shape);
    return object;
  }
  else if (stricmp(type, "direct") == 0)
  {
    AnimationType *object = new AnimationGenericType();
    object->Init(cls, shape);
    return object;
  }
  else if (stricmp(type, "hide") == 0)
  {
    AnimationType *object = new AnimationHideType();
    object->Init(cls, shape);
    return object;
  }

  ErrF("Unknown animation type %s",cc_cast(type));
  return NULL;
}

AnimationType *AnimationType::CreateObject(SerializeBinStream &f)
{
  int type = f.LoadInt();
  switch (type)
  {
    case TIdRotation: return new AnimationRotationType();
    case TIdRotationX: return new AnimationRotationXType();
    case TIdRotationY: return new AnimationRotationYType();
    case TIdRotationZ: return new AnimationRotationZType();
    case TIdTranslation: return new AnimationTranslationType();
    case TIdTranslationX: return new AnimationTranslationXType();
    case TIdTranslationY: return new AnimationTranslationYType();
    case TIdTranslationZ: return new AnimationTranslationZType();
    case TIdDirect: return new AnimationGenericType();
    case TIdHide: return new AnimationHideType();
  }
  ErrF("Unknown animation type %d",type);
  return NULL;
}

/*
template <>
struct EndianTraits<AnimAddress>: public EndianTraitsSwap<AnimAddress>
{
};
*/
void AnimationType::SerializeBin(SerializeBinStream &f)
{
  if (f.IsSaving())
  {
    int type = GetType();
    f.SaveInt(type);
  }
  f.Transfer(_name);
  f.Transfer(_source);
  f.Transfer(_minPhase),f.Transfer(_maxPhase);
  f.Transfer(_minValue),f.Transfer(_maxValue);
  f.TransferLittleEndian(_address);
  Assert(_address>=0 && _address<NAnimAddress);
}

void AnimationType::SerializeBin(SerializeBinStream &f, int level)
{
  // no per-level information on this level
}

