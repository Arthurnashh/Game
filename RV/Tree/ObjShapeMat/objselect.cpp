#include "objshape.h"
#include "wpch.hpp"

#include <strstream>
using namespace std;
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "objobje.hpp"

Selection::Selection( const Selection &src )
  {
  _object=src._object;
  vertSel=src.vertSel;
  faceSel=src.faceSel;
  }

//--------------------------------------------------

Selection::Selection( const Selection &src, ObjectData *object )
  {
  _object=object;
  vertSel=src.vertSel;
  faceSel=src.faceSel;
  }

//--------------------------------------------------

void Selection::DeletePoint( int index )
  {
  Validate();
  vertSel.Delete(index);
  }

//--------------------------------------------------

void Selection::DeleteFace( int index )
  {
  Validate();
  faceSel.Delete(index);
  }

//--------------------------------------------------

void Selection::Clear()
  {
  vertSel.Clear();
  faceSel.Clear();
  Validate();
  }

//--------------------------------------------------

void Selection::ValidatePoints( int nPoints )
  {
  int i;
  int oldVert=vertSel.Size();
  int newVert=_object->NPoints();
  if( newVert<nPoints ) newVert=nPoints;
  if( newVert!=oldVert )
    {
    vertSel.Resize(newVert);
    for( i=oldVert; i<newVert; i++ ) vertSel[i]=0;
    }
  }

//--------------------------------------------------

void Selection::ValidateFaces( int nFaces )
  {
  int i;
  int oldFace=faceSel.Size();
  int newFace=_object->NFaces();
  if( newFace<nFaces ) newFace=nFaces;
  if( newFace!=oldFace )
    {
    faceSel.Resize(newFace);
    for( i=oldFace; i<newFace; i++ ) faceSel[i]=false;
    }
  }

//--------------------------------------------------

void Selection::Validate()
  {
  ValidatePoints();
  ValidateFaces();
  }

//--------------------------------------------------

int Selection::NPoints() const
  {
  int i;
  int count=0;
  for( i=0; i<vertSel.Size(); i++ ) if( vertSel[i] ) count++;
  return count;
  }

//--------------------------------------------------

int Selection::NFaces() const
  {
  int i;
  int count=0;
  for( i=0; i<faceSel.Size(); i++ ) if( faceSel[i] ) count++;
  return count;
  }

//--------------------------------------------------

bool Selection::IsEmpty() const
  {
  if( NPoints()>0 ) return false;
  if( NFaces()>0 ) return false;
  return true;
  }

//--------------------------------------------------

void Selection::PermuteFaces( const int *permutation )
  {
  Selection newSel=*this;
  int i;
  for( i=0; i<_object->NFaces(); i++ )
    {
    faceSel[permutation[i]]=newSel.faceSel[i];
    }
  }

//--------------------------------------------------

void Selection::PermuteVertices( const int *permutation )
  {
  Selection newSel=*this;
  int i;
  for( i=0; i<_object->NPoints(); i++ )
    {
    vertSel[permutation[i]]=newSel.vertSel[i];
    }
  }

//--------------------------------------------------

void ObjectData::DeletePoint( int index )
  {
  // point indices are important in selections and edges
  _sel.DeletePoint(index);
  int i;
  
  // delete in all selections
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( _namedSel[i] ) _namedSel[i]->DeletePoint(index);
    }
  _lock.DeletePoint(index);
  _hide.DeletePoint(index);
  _mass.DeletePoint(index);
  
  // delete in all animations
  for( i=0; i<NAnimations(); i++ )
    {
    AnimationPhase &anim=*GetAnimation(i);
    anim.DeletePoint(index);
    }
  
  
  for( int j=_sharpEdge.Size(); --j>=0; )
    {
    if( _sharpEdge[j][0]==index || _sharpEdge[j][1]==index )
      {
      _sharpEdge.Delete(j);
      }
    else
      {
      if( _sharpEdge[j][0]>index ) _sharpEdge[j][0]--;
      if( _sharpEdge[j][1]>index ) _sharpEdge[j][1]--;
      }
    }
  
  // scan all faces, delete all containing point index
  for( i=NFaces(); --i>=0; )
    {
    FaceT face(this,i);
    if( face.ContainsPoint(index) ) DeleteFace(i);
    else
      {
      for( int j=0; j<face.N(); j++ )
        {
        if( face.GetPoint(j)>index ) face.SetPoint(j,face.GetPoint(j)-1);
        }
      }
    }
  _points.Delete(index);
  // update point attributes
  _sel.ValidatePoints();
  _lock.ValidatePoints();
  _hide.ValidatePoints();
  _mass.Validate();
  for( i=0; i<NAnimations(); i++ ) _phase[i]->Validate();
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( _namedSel[i] ) _namedSel[i]->ValidatePoints();
    }
  }

//--------------------------------------------------

void ObjectData::DeleteNormal( int index )
  {
  // normal indices are important in selections
  int i;
  // scan all faces, delete all containing normal index
  for( i=NFaces(); --i>=0; )
    {
    FaceT face(this,i);
    if( face.ContainsNormal(index) ) DeleteFace(i);
    else
      {
      for( int j=0; j<face.N(); j++ )
        {
        if( face.GetNormal(j)>index ) face.SetNormal(j,face.GetNormal(j)-1);
        }
      }
    }
  _normals.Delete(index);
  }

//--------------------------------------------------

void ObjectData::DeleteFace( int index )
  {
  // face indices are important only in selections
  int i;
  _sel.DeleteFace(index);
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( _namedSel[i] ) _namedSel[i]->DeleteFace(index);
    }
  _lock.DeleteFace(index);
  _hide.DeleteFace(index);
  // delete face
  _faces.Delete(index);
  // update point attributes
  _sel.ValidateFaces();
  _lock.ValidateFaces();
  _hide.ValidateFaces();
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( _namedSel[i] ) _namedSel[i]->ValidateFaces();
    }
  }

//--------------------------------------------------

void ObjectData::ReplacePoint( int newIndex, int oldIndex )
  {
  // point indices are important in selections and edges
  for( int j=_sharpEdge.Size(); --j>=0; )
    {
    if( _sharpEdge[j][0]==oldIndex ) _sharpEdge[j][0]=newIndex;
    if( _sharpEdge[j][1]==oldIndex ) _sharpEdge[j][1]=newIndex;
    }
  
  SortSharpEdges();
  
  // scan all faces, replace all containing point index
  int i;
  for( i=NFaces(); --i>=0; )
    {
    FaceT face(this,i);
    bool degraded=false;
    for( int j=0; j<face.N(); j++ )
      {
      if( face.GetPoint(j)==newIndex ) degraded=true;
      }
    if( !degraded )
      {
      for( int j=0; j<face.N(); j++ )
        {
        if( face.GetPoint(j)==oldIndex ) face.SetPoint(j,newIndex);
        }
      }
    else
      {
      // point is already contained in face - we will only remove it
        int k =0;
      for( int j=0; j<face.N(); j++ )
        {
        if( face.GetPoint(j)!=oldIndex ) face.SetPoint(k++,face.GetPoint(j));
        }
      face.SetN(k);
      if( face.N()<3 ) DeleteFace(i);
      }
    }
  // what happens in animations and selections?
  // this should be handled by merge
  }

//--------------------------------------------------

void ObjectData::PermuteVertices( const int *permutation )
  {
  // point indices are important in selections and edges
  for( int j=_sharpEdge.Size(); --j>=0; )
    {
    _sharpEdge[j][0]=permutation[_sharpEdge[j][0]];
    _sharpEdge[j][1]=permutation[_sharpEdge[j][1]];
    if( _sharpEdge[j][0]>_sharpEdge[j][1] )
      {
      __swap(_sharpEdge[j][0],_sharpEdge[j][1]);
      }
    }
  
  SortSharpEdges();
  
  // scan all faces, replace all containing point index
  int i;
  for( i=NFaces(); --i>=0; )
    {
    FaceT face(this,i);
    for( int j=0; j<face.N(); j++ )
      {
      face.SetPoint(j,permutation[face.GetPoint(j)]);
      }
    }
  
  // permute selections
  _sel.PermuteVertices(permutation);
  _hide.PermuteVertices(permutation);
  _lock.PermuteVertices(permutation);
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( _namedSel[i] ) _namedSel[i]->PermuteVertices(permutation);
    }
  
  // permute animations
  for( i=0; i<_phase.Size(); i++ )
    {
    _phase[i]->PermuteVertices(permutation);
    }
  
  // permute per-vertex attributes
  _mass.PermuteVertices(permutation);
  
  // permute object data
  Temp<PosT> newPos(NPoints());
  for( i=0; i<NPoints(); i++ ) newPos[i]=Point(i);
  for( i=0; i<NPoints(); i++ )
    {
    Point(permutation[i])=newPos[i];
    }
  
  }

//--------------------------------------------------

int ObjectData::FindNormal( const VecT &norm, double maxDist )
  {
  double maxDist2=maxDist*maxDist;
  int ret=-1;
  for( int i=0; i<NNormals(); i++ )
    {
    Vector3 df=norm-Normal(i);
    double dist2=df.SquareSize();
    if( dist2<maxDist2 ) maxDist2=dist2,ret=i;
    }
  return ret;
  }

//--------------------------------------------------

  int ObjectData::FindPoint( const VecT &point)
  {       
    for( int i=0; i<NPoints(); i++ )
    {      
      if( point == Point(i))
        return i;
    }
    return -1;
  }

//--------------------------------------------------

int ObjectData::FindOrientedEdge( int a, int b ) const
  {
  // return index of face containing points a,b
  for( int i=0; i<NFaces(); i++ )
    {
    const FaceT face(this,i);
    if( face.ContainsEdge(a,b) ) return i;
    }
  return -1;
  }

//--------------------------------------------------

int ObjectData::FindOrientedEdge( int a, int b, bool *searchFaces ) const
  {
  for( int i=0; i<NFaces(); i++ ) if( searchFaces[i] )
    {
    const FaceT face(this,i);
    if( face.ContainsEdge(a,b) ) return i;
    }
  return -1;
  }

//--------------------------------------------------

int ObjectData::FindEdge( int a, int b ) const
  {
  // return index of face containing points a,b
  for( int i=0; i<NFaces(); i++ )
    {
    const FaceT face(this,i);
    if( face.ContainsEdge(a,b) ) return i;
    if( face.ContainsEdge(b,a) ) return i;
    }
  return -1;
  }

//--------------------------------------------------

int ObjectData::AddNormal( const VecT &norm )
  {
  // if given normal does not exist, create it
  int index=FindNormal(norm,1.0/16);
  if( index>=0 ) return index;
  index=NNormals();
  *NewNormal()=norm;
  return index;
  }

//--------------------------------------------------

int ObjectData::SingleSelectedFace() const
  {
  // if one and only one face is selected, return its index
  int i;
  int count=0;
  int select=-1;
  for( i=0; i<NFaces(); i++ )
    {
    if( FaceSelected(i) ) select=i,count++;
    }
  if( count!=1 ) return -1;
  return select;
  }

//--------------------------------------------------

int ObjectData::FirstSelectedFace() const
  {
  int i;
  for( i=0; i<NFaces(); i++ )
    {
    if( FaceSelected(i) ) return i;
    }
  return -1;
  }

//--------------------------------------------------

int ObjectData::CountSelectedFaces() const
  {
  int i;
  int count=0;
  for( i=0; i<NFaces(); i++ )
    {
    if( FaceSelected(i) ) count++;
    }
  return count;
  }

//--------------------------------------------------

int ObjectData::SingleSelectedPoint() const
  {
  int i;
  int count=0;
  int select=-1;
  for( i=0; i<NPoints(); i++ )
    {
    if( PointSelected(i) ) select=i,count++;
    }
  if( count!=1 ) return -1;
  return select;
  }

//--------------------------------------------------

int ObjectData::FirstSelectedPoint() const
  {
  int i;
  for( i=0; i<NPoints(); i++ )
    {
    if( PointSelected(i) ) return i;
    }
  return -1;
  }

//--------------------------------------------------

int ObjectData::CountSelectedPoints() const
  {
  int i;
  int count=0;
  for( i=0; i<NPoints(); i++ )
    {
    if( PointSelected(i) ) count++;
    }
  return count;
  }

//--------------------------------------------------

inline int CompareF( const SharpEdge *edge0, const SharpEdge *edge1 )
  {
  int ret=(*edge0)[0]-(*edge1)[0];
  if( ret ) return ret;
  return (*edge0)[1]-(*edge1)[1];
  }

//--------------------------------------------------

template <class Type>
int FullSearch( const Type *base, int num, const Type &key )
  {
  // robust non-binary implementation
  const Type *data=base;
  int i=num;
  while( i>0 )
    {
    if( !CompareF(data,&key) ) return num-i;
    data++;
    i--;
    }
  return -1;
  }

//--------------------------------------------------

#if 0
template <class Type>
int BSearch( const Type *base, int num, const Type &key )
  {
  return FullSearch(base,num,key);
  }

//--------------------------------------------------

#else
template <class Type>
int BSearch( const Type *base, int num, const Type &key )
  {
  const Type *lo = base;
  const Type *hi = base + (num - 1);
  const Type *mid;
  unsigned int half;
  int result;
  
  while (lo <= hi)
    {
    
    half = num / 2;
    if (half!=0)
      {
      mid = lo + (num & 1 ? half : (half - 1));
      
      result = CompareF(&key,mid);
      if (!result)
        return(mid-base);
      else if (result < 0)
        {
        hi = mid - 1;
        num = num & 1 ? half : half-1;
        }
      else    
        {
        lo = mid + 1;
        num = half;
        }
      }
    else if( num ) return CompareF(lo,&key) ? -1 : lo-base; // it should be here
    else break;
    }
  
  return -1;
  }

//--------------------------------------------------

#endif
int ObjectData::FindSharpEdge( int a, int b ) const
  {
  if( a>b ) __swap(a,b);
  const SharpEdge *data=_sharpEdge.Data();
  int size=_sharpEdge.Size();
  SharpEdge key;
  key[0]=a;
  key[1]=b;
  // find a usign binary search
  int index=BSearch(data,size,key);
  #if 0
  if( index<0 )
    {
    index=FullSearch(data,size,key);
    if( index>=0 )
      {
      Fail("Edge found late.");
      }
    }
  #endif
  return index;
  
  }

//--------------------------------------------------

#include <Es/Algorithms/qsort.hpp>

void ObjectData::SortSharpEdges()
  {
  QSort(_sharpEdge.Data(),_sharpEdge.Size(),CompareF);
  // optimize sharp edge list
  for( int i=1; i<_sharpEdge.Size(); )
    {
    const SharpEdge &curr=_sharpEdge[i-1];
    const SharpEdge &next=_sharpEdge[i];
    if( !CompareF(&curr,&next) ) _sharpEdge.Delete(i);
    else i++;
    }
  }

//--------------------------------------------------

void ObjectData::RemoveSharpEdge( int a, int b )
  {
  int index=FindSharpEdge(a,b);
  if( index>=0 ) RemoveSharpEdge(index);
  }

//--------------------------------------------------

void ObjectData::AddSharpEdge( int a, int b )
  {
  int index=FindSharpEdge(a,b);
  if( index<0 )
    {
    if( a>b ) __swap(a,b);
    SharpEdge edge;
    edge[0]=a;
    edge[1]=b;
    int i;
    for( i=0; i<_sharpEdge.Size(); i++ )
      {
      SharpEdge &cur=_sharpEdge[i];
      if( CompareF(&cur,&edge)>=0 ) break;
      }
    //_sharpEdge.Add(edge);
    _sharpEdge.Insert(i,edge);
    _dirtySetup=true;
    }
  }

//--------------------------------------------------

void ObjectData::RemoveSharpEdge( int index )
  {
  _sharpEdge.Delete(index);
  _dirtySetup=true;
  }

//--------------------------------------------------

void ObjectData::RemoveAllSharpEdges()
  {
  if( _sharpEdge.Size()>0 )
    {
    _dirtySetup=true;
    _sharpEdge.Clear();
    }
  }

//--------------------------------------------------

void ObjectData::SelectFacesFromPoints( int vertex, SelMode mode )
  {
  // test all faces adajcent to vertex and select them as appropriate
  int i;
  for( i=0; i<NFaces(); i++ )
    {
    const FaceT face(this,i);
    if( vertex<0 || face.ContainsPoint(vertex) )
      {
      bool allSel=true;
      for( int j=0; j<face.N(); j++ )
        {
        allSel=allSel && PointSelected(face.GetPoint(j));
        }
      if( mode==SelAdd && !allSel ) continue; 
      if( mode==SelSub && allSel ) continue; 
      FaceSelect(i,allSel);
      }
    }
  }

//--------------------------------------------------

void ObjectData::SelectPointsFromSelectedFaces( int faceIndex, SelMode mode )
  {
  int i;
  for( i=0; i<NFaces(); i++ )
    if( faceIndex<0 || faceIndex==i )
      if( FaceSelected(i) )
        {
        const FaceT face(this,i);
        for( int j=0; j<face.N(); j++ )
          {
          PointSelect(face.GetPoint(j),true);
          }
        }
  }

//--------------------------------------------------

void ObjectData::UnselectPointsFromUnselectedFaces( int faceIndex, SelMode mode )
  {
  int i;
  for( i=0; i<NFaces(); i++ )
    if( faceIndex<0 || faceIndex==i )
      if( !FaceSelected(i) )
        {
        const FaceT face(this,i);
        for( int j=0; j<face.N(); j++ )
          {
          PointSelect(face.GetPoint(j),false);
          }
        }
  }

//--------------------------------------------------

void ObjectData::SelectPointsFromFaces( int faceIndex, SelMode mode )
  {
  // test all points contained in face and select them as appropriate
  // could be much faster for faceIndex>=0, but this way it is easier
  // two stages: one selects all points of selected faces
  // second unselect all points of unselected faces
  // order of stages can vary depending on mode
  switch( mode )
    {
    case SelAdd:
    SelectPointsFromSelectedFaces(faceIndex,mode);
    break;
    case SelSub:
    UnselectPointsFromUnselectedFaces(faceIndex,mode);
    break;
    default: // SelSet
    UnselectPointsFromUnselectedFaces(faceIndex,mode);
    SelectPointsFromSelectedFaces(faceIndex,mode);
    break;
    }
  }

//--------------------------------------------------

void ObjectData::PromoteComponent( bool *visited, EdgeMatrix &edges, int point, bool state )
  {
  int i;
  for( i=0; i<NPoints(); i++ ) if( !visited[i] )
    {
    if( edges(i,point) )
      {
      visited[i]=true;
      PointSelect(i,state);
      PromoteComponent(visited,edges,i,state);
      }
    }
  }

//--------------------------------------------------

void ObjectData::SelectComponent( int point, bool state )
  {
  Temp<bool> visited(NPoints());
  if( !visited ) return;
  int i;
  for( i=0; i<NPoints(); i++ ) visited[i]=false;
  visited[point]=true;
  SRef<EdgeMatrix> edges=new EdgeMatrix(NPoints());
  if( !edges ) return;
  for( i=0; i<NFaces(); i++ )
    {
    const FaceT face(this,i);
    int j;
    for( j=0; j<face.N(); j++ )
      {
      int i0=face.GetPoint(j);
      int i1=face.GetPoint((j+1)%face.N());
      edges->AddEdge(i0,i1);
      }
    }
  PointSelect(point,state);
  PromoteComponent(visited,*edges,point,state);
  }

//--------------------------------------------------

bool ObjectData::SelectionDelete()
  {
  int i;
  bool del=false;
  // go backward - indexes become invalid after deletion
  // delete all selected faces - no point removal
  for( i=NFaces(); --i>=0; ) if( FaceSelected(i) )
    {
    DeleteFace(i);
    del=true;
    }
  // delete all selected points with associated faces
  for( i=NPoints(); --i>=0; ) if( PointSelected(i) )
    {
    DeletePoint(i);
    del=true;
    }
  if( del )
    {
    RecalcNormals();
    //ClearSelection();
    SetDirty();
    }
  return true;
  }

//--------------------------------------------------

bool ObjectData::SelectionDeleteFaces()
  {
  int i;
  bool del=false;
  // go backward - indexes become invalid after deletion
  // delete all selected faces - no point removal
  for( i=NFaces(); --i>=0; ) if( FaceSelected(i) )
    {
    DeleteFace(i);
    del=true;
    }
  if( del )
    {
    RecalcNormals();
    //ClearSelection();
    SetDirty();
    }
  return true;
  }

//--------------------------------------------------

void ObjectData::ClearSelection()
  {
  _sel.Clear();
  }

//--------------------------------------------------

void ObjectData::ExtractSelection()
  {
  // copy whole object
  // delete all unselected faces
  int i,j;
  for( i=NFaces(); --i>=0; )
    {
    if( !FaceSelected(i) ) DeleteFace(i);
    }
  // only selected faces left now.
  //  delete all unselected points not contained in any face
  // delete all normals not contained in any face
  Temp<bool> pointsUsed(NPoints());
  Temp<bool> normalsUsed(NNormals());
  memset(pointsUsed,0,NPoints()*sizeof(*pointsUsed));
  memset(normalsUsed,0,NNormals()*sizeof(*normalsUsed));
  for( i=0; i<NFaces(); i++ )
    {
    FaceT face(this,i);
    for( j=0; j<face.N(); j++ )
      {
      pointsUsed[face.GetPoint(j)]=true;
      normalsUsed[face.GetNormal(j)]=true;
      }
    }
  for( i=NPoints(); --i>=0; )
    {
    if( !pointsUsed[i] && !PointSelected(i) ) DeletePoint(i);
    }
  for( i=NNormals(); --i>=0; )
    {
    if( !normalsUsed[i] ) DeleteNormal(i);
    }
  // delete all empty named selections
  for( i=0; i<MAX_NAMED_SEL; i++ ) if( _namedSel[i] )
    {
    if( _namedSel[i]->IsEmpty() ) delete _namedSel[i],_namedSel[i]=NULL;
    }
  }

//--------------------------------------------------

bool ObjectData::Merge( const ObjectData &src,const char *createSel)
  {
  int i,j;
  // merged data will be selected
  ClearSelection();
  // copy points
  // allocate new points
  int pIndex=ReservePoints(src.NPoints());
  int fIndex=ReserveFaces(src.NFaces());
  int nIndex=_normals.Size();
  _normals.Resize(nIndex+src.NNormals());
  
  memcpy(_points.Data()+pIndex,src._points.Data(),src.NPoints()*sizeof(PosT));
  // copy points to all animation phases
  for( i=0; i<NAnimations(); i++ )
    {
    AnimationPhase &phase=*GetAnimation(i);
    phase.Validate(NPoints());
    for( j=0; j<src.NPoints(); j++ )
      {
      phase[pIndex+j]=src.Point(j);
      }
    }
  // copy normals
  
  
  if( src.NNormals()>0 )
    {
    memcpy(&_normals[nIndex],src._normals.Data(),src.NNormals()*sizeof(VecT));
    }
  for( i=0; i<src.NFaces(); i++ )
    {    
    FaceT face(this);
    face=FaceT(src,i);
    for( j=0; j<face.N(); j++ )
      {
      face.SetPoint(j,face.GetPoint(j)+pIndex);
      face.SetNormal(j,face.GetNormal(j)+nIndex);
      }
    face.CopyFaceTo(fIndex+i);
    }
  // import mass information
  if( src._mass.N()>0 )
    {
    _mass.Validate();
    for( i=0; i<src._mass.N(); i++ ) _mass[pIndex+i]=src._mass[i];
    }
  // copy and reindex edges
  int oldEdges=_sharpEdge.Size();
  _sharpEdge.Resize(_sharpEdge.Size()+src._sharpEdge.Size());
  for( i=0; i<src._sharpEdge.Size(); i++ )
    {
    _sharpEdge[oldEdges+i][0]=src._sharpEdge[i][0]+pIndex;
    _sharpEdge[oldEdges+i][1]=src._sharpEdge[i][1]+pIndex;
    }
  
  
  if (createSel==NULL)
    {
    // define all non-empty named selections from src
    int si;
    for( si=0; si<MAX_NAMED_SEL; si++ ) if( src._namedSel[si] )
      {
      const NamedSelection &srcSel=*src._namedSel[si];
      if( srcSel.IsEmpty() ) continue;
      // define sel with sel
      // use old named selection in target (if any)
      if( !UseNamedSel(srcSel.Name()) ) _sel.Clear();
      // add any points selected in source
      for( i=0; i<src.NPoints(); i++ )
        {
        _sel.SetPointWeight(pIndex+i,srcSel.PointWeight(i));
        }
      for( i=0; i<src.NFaces(); i++ )
        {
        _sel.FaceSelect(fIndex+i,srcSel.FaceSelected(i));
        }
      // save to target
      SaveNamedSel(srcSel.Name());
      }
    }
  
  // set selection
  _sel.Clear();
  for( i=0; i<src.NPoints(); i++ ) _sel.PointSelect(pIndex+i,true);
  for( i=0; i<src.NFaces(); i++ ) _sel.FaceSelect(fIndex+i,true);
  if (createSel!=NULL) SaveNamedSel(createSel);
  SetDirty();
  return true;
  }

//--------------------------------------------------

bool ObjectData::SelectionCopy( UINT format )
  {
  bool ret=false;
  // open clipboard
  ::EmptyClipboard();
  // copy selected data
  SRef<ObjectData> obj=new ObjectData(*this,_autoSaveAnimation);
  if( !obj ) return false;
  obj->ExtractSelection();
  return obj->SaveClipboard(format);
  }

//--------------------------------------------------

bool ObjectData::SaveClipboard( UINT format )
  {
  bool ret=false;
  ostrstream f;
//  f.rdbuf()->setbuf(NULL,2048);
  SaveData(f,false,9999);
  SaveSetup(f,false,true,false);
  
  if( !::OpenClipboard(NULL) ) return false;
  f.seekp(0,ios::end);
  int size=f.tellp();
  const char *strData=f.str();
  if( strData )
    {
    // data valid
    HGLOBAL handle=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,size);
    if( handle )
      {
      void *memData=GlobalLock(handle);
      if( memData )
        {
        memcpy(memData,strData,size);
        ret=true;
        GlobalUnlock(handle);
        ::SetClipboardData(format,(HANDLE)handle);
        }
      }
    }
  f.rdbuf()->freeze(0); // unlock data - destructor will deallocate
  //	free(f.str());
  ::CloseClipboard();
  return ret;
  }

//--------------------------------------------------

bool ObjectData::SelectionCut( UINT format )
  {
  if( SelectionCopy(format) ) return SelectionDelete();
  return false;
  }

//--------------------------------------------------

bool ObjectData::LoadClipboard( UINT format )
  {
  bool ret=false;
  if( !::OpenClipboard(NULL) ) return false;
  // get memory handle to object
  HGLOBAL handle=::GetClipboardData(format);
  if( handle )
    {
    void *data=GlobalLock(handle);
    if( data )
      {
      // data available
      DWORD size=GlobalSize(handle);
      // size can be larger than actual data size
      istrstream f((char *)data,size);
      LoadData(f);
      LoadSetup(f);
      ret=true;
      }
    GlobalUnlock(handle);
    }
  ::CloseClipboard();
  return ret;
  }

//--------------------------------------------------

bool ObjectData::SelectionPaste( UINT format )
  {
  SRef<ObjectData> obj=new ObjectData();
  if( obj )
    {
    if( obj->LoadClipboard(format) )
      {
      return Merge(*obj);
      }
    }
  return false;
  }

//--------------------------------------------------

void ObjectData::CheckFaces()
  {
  // check all faces for planarity
  // stop on:
  // non-planar geometry    
  // non-linear texture coordinates
  int i;
  for( i=0; i<NFaces(); i++ )
    {
    FaceT face(this,i);
    if( face.N()==3 )
      {
      FaceSelect(i,false);
      continue;
      }
    if( face.N()<3 )
      {
      FaceSelect(i,true);
      continue;
      }
    // 4 point face - calculate plane normal and test for planarity
    const PosT &p0=Point(face.GetPoint(0));
    const PosT&p1=Point(face.GetPoint(1));
    const PosT &p2=Point(face.GetPoint(2));
    const PosT &p3=Point(face.GetPoint(3));
    Point3D p10=(Point3D)p1-(Point3D)p0;
    Point3D p20=(Point3D)p2-(Point3D)p0;
    Point3D p13=(Point3D)p1-(Point3D)p3;
    Point3D p23=(Point3D)p2-(Point3D)p3;
    Point3D normal0=p10.CrossProduct(p20);
    Point3D normal3=p13.CrossProduct(p23);
    // if plane is planar, both normals should be same
    // calculate cos(fi) - angle between normals
    double cosFi=normal0*normal3/(normal0.Size()*normal3.Size());
    if( cosFi<0.996 ) // 0.996 is cos(5 degree)
      {
      FaceSelect(i,true);
      continue;
      }
    FaceSelect(i,false);
    }
  SelectPointsFromFaces();
  }

struct UVPair
  {
  float u,v;
  };

static bool GenerateST(Vector3Par anormal, Vector3Par apos, Vector3Par bpos, Vector3Par cpos,
                const UVPair &at, const UVPair &bt, const UVPair &ct,
                Vector3 &outS, Vector3 &outT)
{
  float k = bt.u - at.u;
  float l = bt.v - at.v;
  float m = ct.u - at.u;
  float n = ct.v - at.v;
  Vector3 v1 = bpos - apos;
  Vector3 v2 = cpos - apos;
  float kn = k * n;
  float lm = l * m;
  float d = lm - kn;

  // Test the vectors to be lineary dependent
  if (fabs(d) < FLT_MIN)
  {
    // First try s to be (1,0,0) vector
    Vector3 s(1, 0, 0);
    float t = s * anormal;
    s = s - anormal * t;
    if (fabs(s.SquareSize()) < FLT_MIN)
    {
      s.Normalize();
    }
    else
    {
      // (1,0,0) failed, so (0,1,0) must work
      s = Vector3(0, 1, 0);
      t = s * anormal;
      s = (s - anormal * t).Normalized();
    }

    // Write out some reasonable values
    outS = s;
    outT = anormal.CrossProduct(s);

    // The vectors in the texture are lineary dependent
    return false;
  }

  // U to S
  outS = (-n * v1 * d + l * v2 * d).Normalized();

  // V to T
  outT = (m * v1 * d - k * v2 * d).Normalized();
  // Normalize the outS and outT using the anormal
  // The reference vector is the normal - the outS and outT must be orthogonal to it

  // Get the middleST vector and make it orthogonal to normal
  Vector3 middleST = (outS + outT) * 0.5f;
  float t = middleST * anormal;
  middleST = middleST - anormal * t;
  if (fabs(middleST.SquareSize()) < FLT_MIN)
  {
    // The middle ST is lineary dependent on normal
    return false;
  }
  middleST.Normalize();

  // Get vector orthogonal to the middleST
  Vector3 orthoMiddleST = anormal.CrossProduct(middleST);

  if (orthoMiddleST.DotProduct(outS) > 0.0f)
  {
    // Get outS and outT from the middle of the middleST and orthoModdleST
    outS = (orthoMiddleST + middleST).Normalized();
    outT = (-orthoMiddleST + middleST).Normalized();
  }
  else
  {
    // Get outS and outT from the middle of the middleST and orthoModdleST
    outS = (-orthoMiddleST + middleST).Normalized();
    outT = (orthoMiddleST + middleST).Normalized();
  }

  return true;
}

void ObjectData::CheckSTVectors()
  {
  for (int i=0;i<NFaces();i++)
    {
    FaceT fc(this,i);
    if (fc.N()==3)
      {
      PosT p1=Point(fc.GetPoint(0));
      PosT p2=Point(fc.GetPoint(1));
      PosT p3=Point(fc.GetPoint(2));
      VecT anorm=Normal(fc.GetNormal(0));
      UVPair at,bt,ct;
      at.u=fc.GetU(0);at.v=fc.GetV(0);
      bt.u=fc.GetU(1);bt.v=fc.GetV(1);
      ct.u=fc.GetU(2);ct.v=fc.GetV(2);
      Vector3 out1,out2;
      bool res=GenerateST(anorm,p1,p2,p3,at,bt,ct,out1,out2);
      if (!res) FaceSelect(i);
      continue;
      }
    else
      {
      int a=2;
      int b=3;
      int c;
      for (c=0;c<4;c++)
        {
        PosT p1=Point(fc.GetPoint(a));
        PosT p2=Point(fc.GetPoint(b));
        PosT p3=Point(fc.GetPoint(c));
        VecT anorm=Normal(fc.GetNormal(0));
        UVPair at,bt,ct;
        at.u=fc.GetU(a);at.v=fc.GetV(a);
        bt.u=fc.GetU(b);bt.v=fc.GetV(b);
        ct.u=fc.GetU(c);ct.v=fc.GetV(c);
        Vector3 out1,out2;
        bool res=GenerateST(anorm,p1,p2,p3,at,bt,ct,out1,out2);
        if (!res) 
          {
          FaceSelect(i);
          break;
          }
        a=b;
        b=c;
        }   
      if (c!=4) continue;
      }
    FaceSelect(i,false);
    }
  SelectPointsFromFaces();
  }


//--------------------------------------------------

void ObjectData::IsolatedPoints()
  {
  ClearSelection();
  int i;
  for( i=0; i<NFaces(); i++ ) FaceSelect(i,true);
  SelectPointsFromFaces();
  
  // invert selection
  for( i=0; i<NPoints(); i++ ) PointSelect(i,!PointSelected(i));
  //for( i=0; i<NFaces(); i++ ) FaceSelect(i,!FaceSelected(i));
  for( i=0; i<NFaces(); i++ ) FaceSelect(i,false);
  }

//--------------------------------------------------

#define UV_COS_LIMIT 0.999 // cos(2 degree)

void ObjectData::CheckMapping()
  {
  int i;
  for( i=0; i<NFaces(); i++ )
    {
    FaceT face(this,i);
    if( face.N()<=3 )
      {
      FaceSelect(i,false);
      continue;
      }
    // check if u,v are linear
    // all calculations are performed in planar space defined by face normal
    Point3D normal=face.CalculateNormal();
    double sqrtArea=sqrt(face.CalculateArea());
    Point3D p0=Point(face.GetPoint(0));
    Point3D p1=Point(face.GetPoint(1));
    Point3D p2=Point(face.GetPoint(2));
    Point3D p3=Point(face.GetPoint(3));
    // normal defines direction up
    // direction forward is defined by (p1-p0)
    Point3D dir=p1-p0;
    dir.Normalize();
    Point3D aside=dir.CrossProduct(normal);
    // aside is lineX, normal is lineY, dir is lineZ
    // convert to linear space
    // y is assumed to be same for all vertices (face should be planar)
    Point3D l0(p0*aside,0,p0*dir);
    Point3D l1(p1*aside,0,p1*dir);
    Point3D l2(p2*aside,0,p2*dir);
    Point3D l3(p3*aside,0,p3*dir);
    Point3D normal0,normal3;
    // check planarity for u
    l0[1]=face.GetU(0)*sqrtArea;
    l1[1]=face.GetU(1)*sqrtArea;
    l2[1]=face.GetU(2)*sqrtArea;
    l3[1]=face.GetU(3)*sqrtArea;
    normal0=(l1-l0).CrossProduct(l2-l0);
    normal3=(l1-l3).CrossProduct(l2-l3);
    // if plane is planar, both normals should be same
    // calculate cos(fi) - angle between normals
    double cosFi=normal0*normal3/(normal0.Size()*normal3.Size());
    if( cosFi<UV_COS_LIMIT )
      {
      FaceSelect(i,true);
      continue;
      }
    // check planarity for v
    l0[1]=face.GetV(0)*sqrtArea;
    l1[1]=face.GetV(1)*sqrtArea;
    l2[1]=face.GetV(2)*sqrtArea;
    l3[1]=face.GetV(3)*sqrtArea;
    normal0=(l1-l0).CrossProduct(l2-l0);
    normal3=(l1-l3).CrossProduct(l2-l3);
    // if plane is planar, both normals should be same
    // calculate cos(fi) - angle between normals
    cosFi=normal0*normal3/(normal0.Size()*normal3.Size());
    if( cosFi<UV_COS_LIMIT )
      {
      FaceSelect(i,true);
      continue;
      }
    FaceSelect(i,false);
    }
  SelectPointsFromFaces();
  }

//--------------------------------------------------

bool ObjectData::MergePoints( double limit, bool selected )
  {
  // merge all selected points that are nearer than limit
  int i,j;
  bool changeTotal=false;
  bool change;
  do
    {
    change=false;
    for( i=0; i<NPoints(); i++ ) if( !selected || PointSelected(i) )
      {
      for( j=0; j<i; j++ ) if( !selected || PointSelected(j) )
        {
        int flagsI=Point(i).flags;
        int flagsJ=Point(j).flags;
        Point3D pointI=Point(i);
        Point3D pointJ=Point(j);
        if( flagsI==flagsJ ) // merge only points with same flags
        if( (pointI-pointJ).SquareSize()<=limit*limit )
          {
          // if there is mass, use weighed average and sum mass
          float iMass=0,jMass=0;
          float sumMass=0;
          if( i<_mass.N() && j<_mass.N() )
            {
            iMass=_mass[i];
            jMass=_mass[j];
            sumMass=iMass+jMass;
            _mass[i]=sumMass;
            }
          if( sumMass<=0 ) iMass=jMass=1,sumMass=2;
          Point3D average=(pointI*iMass+pointJ*jMass)/sumMass;
          Point(i).SetPoint(average);
          Point(i).flags=flagsJ|flagsI; // merge flags
          // calculate average for all animation phases
          for( int p=0; p<NAnimations(); p++ )
            {
            AnimationPhase &anim=*GetAnimation(p);
            anim[i]=(anim[i]*iMass+anim[j]*jMass)/sumMass;
            }
          // be clever as for selections
          // merged point should be in all selections any of mergers is
          for( int s=0; s<MAX_NAMED_SEL; s++ )
            {
            NamedSelection *sel=_namedSel[s];
            if( sel )
              {
              sel->SetPointWeight
                (
                  i,
              (sel->PointWeight(i)+sel->PointWeight(j))*0.5
                );
              }
            }
          if( _lock.PointSelected(j) ) _lock.PointSelect(i);
          if( !_hide.PointSelected(j) ) _hide.PointSelect(i,false);
          
          ReplacePoint(i,j);
          DeletePoint(j);
          //SetProgress(j*100/NPoints());
          _dirty=true;
          change=true;
          goto forBreak;
          }
        }
      }
    forBreak:
    if( change ) changeTotal=true;
    // repeat while there is some improvement
    } while( change );
  if( changeTotal ) SortSharpEdges();
  return changeTotal;
  }

//--------------------------------------------------

// named selection

bool ObjectData::SaveNamedSel( const char *name )
  {
  int i;
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( _namedSel[i] && !strcmp(_namedSel[i]->Name(),name) )
      {
      _dirtySetup=true;
      *_namedSel[i]=NamedSelection(_sel,name);
      return false;
      }
    }
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( !_namedSel[i] )
      {
      // free slot - save
      _dirtySetup=true;
      _namedSel[i]=new NamedSelection(_sel,name);
      return true;
      }
    }
  return false;
  }

//--------------------------------------------------

bool ObjectData::UseNamedSel( const char *name, SelMode mode )
  {
  int i;
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( _namedSel[i] && !strcmp(_namedSel[i]->Name(),name) )
      {
      const NamedSelection &nam=*_namedSel[i];
      switch( mode )
        {
        default: // case SelSet:
        _sel=nam;
        break;
        case SelAdd:
        _sel+=nam;
        break;
        case SelSub:
        _sel-=nam;
        break;
        }
      return true;
      }
    }
  return false;
  }

//--------------------------------------------------

bool ObjectData::DeleteNamedSel( const char *name )
  {
  int i;
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( _namedSel[i] && !strcmp(_namedSel[i]->Name(),name) )
      {
      _dirtySetup=true;
      delete _namedSel[i],_namedSel[i]=NULL;
      return true;
      }
    }
  return false;
  }

//--------------------------------------------------

bool ObjectData::RenameNamedSel( const char *name, const char *newname )
  {
  int i;
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( _namedSel[i] && !strcmp(_namedSel[i]->Name(),newname) )
      {
      return false;
      }
    }
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( _namedSel[i] && !strcmp(_namedSel[i]->Name(),name) )
      {
      _dirtySetup=true;
      _namedSel[i]->SetName(newname);
      return true;
      }
    }
  return false;
  }

//--------------------------------------------------

int ObjectData::FindNamedSel( const char *name ) const
  {
  int i;
  for( i=0; i<MAX_NAMED_SEL; i++ )
    {
    if( _namedSel[i] && !strcmp(_namedSel[i]->Name(),name) )
      {
      return i;
      }
    }
  return -1;
  }

//--------------------------------------------------

const char *ObjectData::NamedSel( int i ) const
  {
  if( i<0 || i>=MAX_NAMED_SEL ) return NULL;
  if( !_namedSel[i] ) return NULL;
  return _namedSel[i]->Name();
  }

//--------------------------------------------------

const NamedSelection *ObjectData::GetNamedSel( int i ) const
  {
  return _namedSel[i];
  }

//--------------------------------------------------

NamedSelection *ObjectData::GetNamedSel( int i )
  {
  return _namedSel[i];
  }

//--------------------------------------------------

const NamedSelection *ObjectData::GetNamedSel( const char *name ) const
  {
  int i=FindNamedSel(name);
  if( i>=0 ) return _namedSel[i];
  return NULL;
  }

//--------------------------------------------------

NamedSelection *ObjectData::GetNamedSel( const char *name )
  {
  int i=FindNamedSel(name);
  if( i>=0 ) return _namedSel[i];
  return NULL;
  }

//--------------------------------------------------

