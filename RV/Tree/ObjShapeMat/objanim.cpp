// keyframe animations
// (C) Ondrej Spanel, Suma

#include "wpch.hpp"
#include "global.hpp"

#include <fstream>
#include <strstream>
using namespace std;
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <Es/Types/pointers.hpp>
#include <Es/Containers/array.hpp>
#include <Es/Common/mathND.hpp>

#include "objshape.h"
//#include "objobje.hpp"

void PointAttrib<Point3D>::Clear( int i )
  {
  _attrib[i]=VZero;
  }

//--------------------------------------------------

void AnimationPhase::Redefine( ObjectData *data )
  {
  // import external data
  Validate();
  if( N()!=data->NPoints() ) return;
  for( int i=0; i<N(); i++ )
    {
    (*this)[i]=data->Point(i);
    }
  }

//--------------------------------------------------

int ObjectData::NAnimations() const
  {
  return _phase.Size();
  }

//--------------------------------------------------

AnimationPhase *ObjectData::GetAnimation( int i ) const
  {
  return _phase[i];
  }

//--------------------------------------------------

int ObjectData::NearestAnimationIndex( float time ) const
  {
  int i,iMin=-1;
  float minDiff=1e10;
  for( i=0; i<_phase.Size(); i++ )
    {
    float diff=fabs(_phase[i]->GetTime()-time);
    if( minDiff>diff ) minDiff=diff,iMin=i;
    }
  return iMin;
  }

//--------------------------------------------------

int ObjectData::AnimationIndex( float time ) const
  {
  int i,iMin=0;
  float minDiff=1e10;
  for( i=0; i<_phase.Size(); i++ )
    {
    float diff=fabs(_phase[i]->GetTime()-time);
    if( minDiff>diff ) minDiff=diff,iMin=i;
    }
  if( minDiff<1e-4 ) return iMin;
  return -1;
  //return iMin;
  }

//--------------------------------------------------

bool ObjectData::AddAnimation( const AnimationPhase &src )
  {
  if( AnimationIndex(src.GetTime())>=0 ) return false;
  AnimationPhase *phase=new AnimationPhase(src);
  phase->Validate();
  _phase.Add(phase);
  SetDirty();
  return true;
  }

//--------------------------------------------------

#if 1
static int CmpPhaseBin( const void *p0, const void *p1 )
  {
  const Ref<AnimationPhase> phase0=*(const Ref<AnimationPhase> *)p0;
  const Ref<AnimationPhase> phase1=*(const Ref<AnimationPhase> *)p1;
  if( phase0->GetTime()>phase1->GetTime() ) return +1;
  if( phase0->GetTime()<phase1->GetTime() ) return -1;
  return 0;
  }

//--------------------------------------------------

#else
static int CmpPhase
  (
  const Ref<AnimationPhase> &p0,
  const Ref<AnimationPhase> &p1
  )
    {
    if( p0->GetTime()>p1->GetTime() ) return +1;
    if( p0->GetTime()<p1->GetTime() ) return -1;
    return 0;
    }

//--------------------------------------------------

#endif

bool ObjectData::SortAnimations()
  {
  bool changed=false;
  for( int i=0; i<_phase.Size()-1; i++ )
    {
    if( CmpPhaseBin(&_phase[i],&_phase[i+1])<0 ) changed=true;
    //if( CmpPhase(_phase[i],_phase[i+1])<0 ) changed=true;
    }
  if( changed )
    {
    float current=0;
    if( _autoSaveAnimation>=0 )
      {
      current=_phase[_autoSaveAnimation]->GetTime();
      }
    //qsort(_phase.Data(),_phase.Size(),sizeof(*_phase.Data()),CmpPhase);
    _phase.QSortBin(CmpPhaseBin);
    if( _autoSaveAnimation>=0 )
      {
      _autoSaveAnimation=AnimationIndex(current);
      current=_phase[_autoSaveAnimation]->GetTime();
      }
    }
  return changed;
  }

//--------------------------------------------------

const float MinPositive=-1e-3;

void ObjectData::HalfRateAnimation()
  {
  // reduce rate to half
  // keep first and last frame
  int p,nPositive=0;
  for( p=1; p<_phase.Size()-1; )
    {
    // skip p
    p++;
    // delete animation p+1
    _phase.Delete(p);
    }
  SortAnimations();
  }

//--------------------------------------------------

void ObjectData::AutoTimeAnimations( bool last1 )
  {
  // ignore phases with negative time
  int p,nPositive=0;
  for( p=0; p<NAnimations(); p++ )
    {
    if( GetAnimation(p)->GetTime()>=MinPositive ) nPositive++;
    }
  int nPhase=0;
  float invN=last1 ? 1.0/(nPositive-1) : 1.0/nPositive;
  for( p=0; p<NAnimations(); p++ )
    {
    AnimationPhase *anim=GetAnimation(p);
    float time=anim->GetTime();
    if( time<MinPositive ) continue;
    float autoTime=nPhase*invN;
    nPhase++;
    anim->SetTime(autoTime);
    }
  }

//--------------------------------------------------

void ObjectData::MirrorAnimation()
  {
  // ignore phases with negative time
  int p,nPositive=0;
  for( p=0; p<NAnimations(); p++ )
    {
    if( GetAnimation(p)->GetTime()>=MinPositive ) nPositive++;
    }
  int nPhase=0;
  float invN=1.0/nPositive;
  for( p=0; p<NAnimations(); p++ )
    {
    AnimationPhase *anim=GetAnimation(p);
    float time=anim->GetTime();
    if( time<MinPositive ) continue;
    float autoTime=nPhase*invN;
    nPhase++;
    anim->SetTime(1-invN-autoTime);
    }
  SortAnimations();
  }

//--------------------------------------------------

bool ObjectData::RenameAnimation( float oldTime, float newTime )
  {
  if( AnimationIndex(newTime)>=0 ) return false;
  int index=AnimationIndex(oldTime);
  if( index<0 ) return false;
  _phase[index]->SetTime(newTime);
  SetDirty();
  return true;
  }

//--------------------------------------------------

bool ObjectData::DeleteAnimation( float time )
  {
  int index=AnimationIndex(time);
  if( index<0 ) return false;
  _phase.Delete(index);
  if( _autoSaveAnimation==index ) _autoSaveAnimation=-1;
  if( _autoSaveAnimation>index ) _autoSaveAnimation--;
  SetDirty();
  return true;
  }

//--------------------------------------------------

void ObjectData::DeleteAllAnimations()
  {
  UseAnimation(-1);
  _phase.Clear();
  }

//--------------------------------------------------

void ObjectData::RedefineAnimation( AnimationPhase &anim )
  {
  int i;
  anim.Validate();
  for( i=0; i<anim.N(); i++ )
    {
    anim[i]=Point(i);
    }
  }

//--------------------------------------------------

void ObjectData::RedefineAnimation( int i )
  {
  SetDirty();
  RedefineAnimation(*_phase[i]);
  _autoSaveAnimation=i;
  }

//--------------------------------------------------

void ObjectData::UseAnimation( int index )
  {
  if( _autoSaveAnimation>=0 )
    {
    RedefineAnimation(*_phase[_autoSaveAnimation]);
    }
  if( index>=0 )
    {
    AnimationPhase &anim=*_phase[index];
    UseAnimation(anim);
    }
  _autoSaveAnimation=index;
  }

//--------------------------------------------------

void ObjectData::UseAnimation( AnimationPhase &anim )
  {
  for( int i=0; i<anim.N(); i++ )
    {
    if( anim[i]!=Point(i) )
      {
      if( _autoSaveAnimation<0 ) SetDirty();
      Point(i).SetPoint(anim[i]);
      }
    }
  }

//--------------------------------------------------

void ObjectData::SoftenAnimation
  (
  float valueX, float valueY, float valueZ,
  bool actualVsAverage,
  float timeRange, bool interpolate, bool looped
  )
    {
    // save current data
    int current=_autoSaveAnimation;
    if (current<0 && actualVsAverage) return;
    float actTime = current>=0 ? _phase[current]->GetTime() : 0;
    UseAnimation(-1);
    // soften animation (all phases)
    int p,i;
    float averageCoef=1.0/_phase.Size();
    float invTimeRange = ( timeRange>0 && interpolate ) ? 1.0/timeRange : 0;
    for( i=0; i<NPoints(); i++ ) if( PointSelected(i) )
      {
      float weight=_sel.PointWeight(i);
      Vector3 average=VZero;
      if (actualVsAverage)
        {
        average=(*_phase[current])[i];
        }
      else
        {
        for( p=0; p<_phase.Size(); p++ )
          {
          const AnimationPhase &phase=(*_phase[p]);
          // ignore phases with negative time (special)
          if( phase.GetTime()<MinPositive ) continue;
          average+=phase[i];
          }
        average*=averageCoef;
        }
      float invSoftValueX=(1-valueX)*weight;
      float invSoftValueY=(1-valueY)*weight;
      float invSoftValueZ=(1-valueZ)*weight;
      for( p=0; p<_phase.Size(); p++ )
        {
        AnimationPhase &phase=(*_phase[p]);
        float time = phase.GetTime();
        if (time<MinPositive) continue;
        
        float timeDist=fabs(time-actTime);
        if( looped )
          {
          if( timeDist>0.5 ) timeDist=1-timeDist;
          if( timeDist<0 ) timeDist=0;
          }
        if( timeDist>timeRange ) continue;
        float factor=1-timeDist*invTimeRange;
        float factorX = factor*invSoftValueX;
        float factorY = factor*invSoftValueY;
        float factorZ = factor*invSoftValueZ;
        phase[i][0]=phase[i][0]*(1-factorX)+average[0]*factorX;
        phase[i][1]=phase[i][1]*(1-factorY)+average[1]*factorY;
        phase[i][2]=phase[i][2]*(1-factorZ)+average[2]*factorZ;
        }
      }
    // restore current data
    UseAnimation(current);
    }

//--------------------------------------------------

static void FindCoordSpace
  (
  CoordSpace &coord,
  const ObjectData *source, const NamedSelection &sel, const AnimationPhase &beg
  )
    {
    int i,j,k,l;
    // create a list of all selected points
    AutoArray<int> selPoints;
    for( i=0; i<source->NPoints(); i++ ) if( sel.PointSelected(i) )
      {
      selPoints.Add(i);
      }
    int nSel=selPoints.Size();
    //if( nSel<1 ) return MIdentity; // no point
    // find four selected points
    // they should define coordinate space
    // i.e for any (i,j,k) min(begP[i]-begP[j])*(begP[k]-begP[j]) should be maximal
    float minMax=1e10;
    int iMax=-1,jMax=-1,kMax=-1,lMax=-1;
    for( i=0; i<nSel; i++ )
      for( j=0; j<nSel; j++ ) if( j!=i )
        for( k=0; k<nSel; k++ ) if( k!=i && k!=j )
          for( l=0; l<nSel; l++ ) if( l!=i && l!=j && l!=k )
            {
            // all possible combinations are tested here
            Vector3 jMi=(beg[selPoints[j]]-beg[selPoints[i]]).Normalized();
            Vector3 kMi=(beg[selPoints[k]]-beg[selPoints[i]]).Normalized();
            Vector3 lMi=(beg[selPoints[l]]-beg[selPoints[i]]).Normalized();
            //float jMiSize2=jMi.SquareSize();
            //float kMiSize2=kMi.SquareSize();
            //float lMiSize2=lMi.SquareSize();
            float jk=jMi*kMi;
            float jl=jMi*lMi;
            float kl=kMi*lMi;
            float maxCosA=floatMax(jk,floatMax(jl,kl));
            //float minSP=fabs(jk*jl*kl)/(jMiSize2*kMiSize2*lMiSize2);
            if( minMax>maxCosA )
              {
              minMax=maxCosA;
              iMax=selPoints[i],jMax=selPoints[j],kMax=selPoints[k],lMax=selPoints[l];
              // if we have some good solution, we need not to search for the best
              if( minMax<0.1 ) goto Solution;
              }
            }
    if( iMax<0 || minMax>0.8 )
      {
      // not enough points to define whole transformation matrix
      coord.pt[0]=coord.pt[1]=coord.pt[2]=coord.pt[3]=-1;
      LogF
        ("No coordinate points in %s",
      sel.Name()
        );
      }
    else
      {
      Solution:
      coord.pt[0]=iMax;
      coord.pt[1]=jMax;
      coord.pt[2]=kMax;
      coord.pt[3]=lMax;
      }
    
    }

//--------------------------------------------------

static Matrix4P CalcTransform
  (
  const CoordSpace &coord, const ObjectData *source, 
  const AnimationPhase &beg, const AnimationPhase &end
  )
    {
    if( coord.pt[0]<0 )
      {
      // not enough points to define whole transformation matrix
      //return Matrix4(MTranslation,end[coord.pt[0]]-beg[coord.pt[0]]);
      return MIdentity;
      }
    
    Vector3 pBeg[4],pEnd[4];
    int i,j;
    for( i=0; i<4; i++ )
      {
      pBeg[i]=beg[coord.pt[i]];
      pEnd[i]=end[coord.pt[i]];
      }
    // create a lin. equation system matrix
    // we search for a matrix M (3x4) (rows * columns):
    // M*pBeg[i]=pEnd[i]
    Matrix<12,12> left;
    Vector<12> right;
    left.InitZero();
    // M(i,j) corresponds to solution(i*4+j) (column index)
    for( int k=0; k<4; k++ )
      {
      // insert condition below into the system
      // M*pBeg[i]=pEnd[i]
      for( i=0; i<3; i++ )
        {
        int row=k*3+i;
        Assert( row<12 );
        for( j=0; j<3; j++ )
          {
          int col=i*4+j;
          Assert( col<12 );
          left(row,col)=pBeg[k][j];
          }
        left(row,i*4+3)=1;
        right[row]=pEnd[k][i];
        }
      }
    Vector<12> solution;
    // solve lin. equation system
    Matrix<12,12> invLeft;
    bool regular=invLeft.InitInversed(left);
    Assert( regular );
    Multiply(solution,invLeft,right);
    #if _DEBUG
      {
      // verify solution
      Vector<12> verify;
      Multiply(verify,left,solution);
      for( int i=0; i<12; i++ )
        {
        if( fabs(verify[i]-right[i])>1e-5 )
          {
          char line[256];
          sprintf(line,"%10s ","!Solution");
          for( int j=0; j<12; j++ )
            {
            sprintf(line+strlen(line),"%6.3f ",solution[j]);
            }
          Log(line);
          break;
          }
        }
      }
    #endif
    // fill matrix with the solution
    Matrix4P transform(MIdentity);
    Vector3P translate(VZero);
    for( i=0; i<3; i++ )
      {
      for( j=0; j<3; j++ ) transform(i,j)=solution[i*4+j];
      translate[i]=solution[i*4+3];
      }
    transform.SetPosition(translate);
    return transform;
    }

//--------------------------------------------------

struct PointWeight
  {
  int selIndex;
  float weight;
  };

//--------------------------------------------------

TypeIsSimple(PointWeight);

class PointWeights: public AutoArray<PointWeight>
  {
  public:
    void AddWeight( int selIndex, float weight );
  };

//--------------------------------------------------

void PointWeights::AddWeight( int selIndex, float weight )
  {
  // note: we are sure each selection is added just once
  PointWeight w;
  w.selIndex=selIndex;
  w.weight=weight;
  Add(w);
  }

//--------------------------------------------------

#define MagicLength 8
static const char RTMMagic100[MagicLength+1]="RTM_0100";
static const char RTMMagic101[MagicLength+1]="RTM_0101";

#define Put(o,v) o.write((const char *)&v,sizeof(v))
#define Get(o,v) o.read((char *)&v,sizeof(v))

static ostream &operator << ( ostream &o, const Vector3P &v )
  {
  o << v.X() << ',' << v.Y() << ',' << v.Z() << ',';
  return o;
  }

//--------------------------------------------------

int ObjectData::FindCoordSpaces
  (
  CoordSpace coord[MAX_NAMED_SEL], const AnimationPhase &src0
  ) const
        {
        int ns;
        // find neutral coordinate spaces
        int nsel=0;
        for( ns=0; ns<MAX_NAMED_SEL; ns++ ) if( _namedSel[ns] )
          {
          if( *_namedSel[ns]->Name()=='*' ) continue;
          const NamedSelection *srcSel=_namedSel[ns];
          // scan source for some coordinate space points
          FindCoordSpace(coord[ns],this,*srcSel,src0);
          nsel++;
          // weighting information is assigned real time
          }
        return nsel;
        }

//--------------------------------------------------

void ObjectData::ExportAnimation( const char *file, float zStep, float xStep ) const
  {
  float yStep=0;
  bool binary=true;
  const char *ext=strrchr(file,'.');
  if( ext && !strcmpi(ext,".txt") ) binary=false;
  ofstream o;
  if( NAnimations()<2 ) return; // nothing to save
  if( binary )
    {
    o.open(file,ios::binary|ios::out);
    o.write(RTMMagic101,MagicLength);
    Put(o,xStep);
    Put(o,yStep);
    Put(o,zStep);
    }
  else
    {
    o.open(file,ios::out);
    o << "XStep length=" << xStep << '\n';
    o << "YStep length=" << xStep << '\n';
    o << "ZStep length=" << zStep << '\n';
    }
  const AnimationPhase &src0=*_phase[0];
  // for all named selections of source find points that can be used as coordinate space
  CoordSpace coord[MAX_NAMED_SEL];
  
  int nsel=FindCoordSpaces(coord,src0);
  if( binary )
    {
    int na=NAnimations()-1; // first is ignored (linking)
    Put(o,na);
    Put(o,nsel);
    }
  
  int ns;
  for( ns=0; ns<MAX_NAMED_SEL; ns++ ) if( _namedSel[ns] )
    {
    if( *_namedSel[ns]->Name()=='*' ) continue;
    const NamedSelection &aSel=*_namedSel[ns];
    if( binary )
      {
      char name[32];
      strcpy(name,aSel.Name());
      // save all transformed selection names
      o.write(name,sizeof(name));
      }
    else
      {
      o << aSel.Name() << '\n';
      }
    }
  
  for( int p=1; p<NAnimations(); p++ )
    {
    AnimationPhase &srcP=*_phase[p];
    // create transformations for all named selections
    if( binary )
      {
      float time=srcP.GetTime();
      Put(o,time);
      }
    else
      {
      o << "Key "<< srcP.GetTime() << '\n';
      }
    int ns;
    for( ns=0; ns<MAX_NAMED_SEL; ns++ ) if( _namedSel[ns] )
      {
      if( *_namedSel[ns]->Name()=='*' ) continue;
      const NamedSelection &aSel=*_namedSel[ns];
      const char *nsName=aSel.Name();
      // scan for corresponding named selection in source
      // create transformation matrix from neutral to current position
      // from any three points we can generate transformation matrix
      Matrix4P transform=CalcTransform(coord[ns],this,src0,srcP);
      // save transforms together with selection names
      if( binary )
        {
        char name[32];
        strncpy(name,nsName,sizeof(name)-1);
        name[sizeof(name)-1]=0;
        o.write(name,sizeof(name));
        Put(o,transform);
        }
      else
        {
        o << nsName << "={";
        // save orientation
        o << transform.DirectionAside();
        o << transform.DirectionUp();
        o << transform.Direction();
        // save position
        o << transform.Position();
        o << "};\n";
        }
      }
    
    }
  }

//--------------------------------------------------

void ObjectData::ExportAnimationLooped( const char *file ,const char *stepsel) const
  {
  SRef<ObjectData> temp=new ObjectData(*this);
    { // if there is step removed, insert it back
    const char *step=temp->GetNamedProp("Step");
    float length=0;
    if( step )
      {
      length=atof(step);
      temp->RemoveZStep(-length);
      }
    }
    { // if there is step removed, insert it back
    const char *xStep=temp->GetNamedProp("XStep");
    float length=0;
    if( xStep )
      {
      length=atof(xStep);
      temp->RemoveXStep(-length);
      }
    }
  float zStepLength=temp->StepLength(stepsel);
  float xStepLength=temp->XStepLength(stepsel);
  temp->RemoveZStep(zStepLength);
  temp->RemoveXStep(xStepLength);
  temp->ExportAnimation(file,zStepLength,xStepLength);
  }

//--------------------------------------------------

void ObjectData::AutoAnimation( const char *file )
  {
  ifstream in;
  in.open(file,ios::binary|ios::in);
  float xStep=0,yStep=0,zStep=0;
  int nAnim,nSel;
  char magic[MagicLength+1];
  in.read(magic,MagicLength);
  magic[MagicLength]=0;
  if( !strcmp(RTMMagic100,magic) )
    {
    // file format error
    // compare with magic value
    Get(in,zStep); // get basic characeristics
    Get(in,nAnim);
    Get(in,nSel);
    }
  else if( !strcmp(RTMMagic101,magic) )
    {
    Get(in,xStep); // get basic characeristics
    Get(in,yStep);
    Get(in,zStep);
    Get(in,nAnim);
    Get(in,nSel);
    }
  
  if( _phase.Size()>0 && _autoSaveAnimation!=0 )
    {
    // reset current data to neutral position
    UseAnimation(*_phase[0]);
    }
  // delete all animation phases
  if( _phase.Size()>0 ) SetDirty();
  _phase.Clear();
  _autoSaveAnimation=-1;
  // for each phasis of source create a new phasis in object
  int ns;
  
  FindArray<int> transformed;
  // load all transformed selection names
  for( ns=0; ns<nSel; ns++ )
    {
    char name[32];
    in.read(name,sizeof(name));
    int index=FindNamedSel(name);
    if( index>=0 ) transformed.Add(index);
    }
  
  // create weighting information for all points
  Temp<PointWeights> weights(NPoints());
  for( ns=0; ns<MAX_NAMED_SEL; ns++ ) if( _namedSel[ns] )
    {
    // use only selections that have corresponding transformed selection
    if( transformed.Find(ns)<0 ) continue;
    const NamedSelection &aSel=*_namedSel[ns];
    for( int i=0; i<NPoints(); i++ ) if( aSel.PointSelected(i) )
      {
      // start with weights taken from selections
      weights[i].AddWeight(ns,aSel.PointWeight(i));
      }
    }
  // normalize weights
  for( int i=0; i<NPoints(); i++ ) if( weights[i].Size()>0 )
    {
    PointWeights &wg=weights[i];
    float sum=0;
    int w;
    for( w=0; w<wg.Size(); w++ ) sum+=wg[w].weight;
    if( sum==0 )
      {
      float avg=1/wg.Size();
      for( w=0; w<wg.Size(); w++ ) wg[w].weight=avg;
      }
    else
      {
      float coef=1/sum;
      for( w=0; w<wg.Size(); w++ ) wg[w].weight*=coef;
      }
    }
  
  AnimationPhase phase0(this);
  phase0.Redefine(this);
  phase0.SetTime(-0.5);
  //RedefineAnimation(phase0);
  AddAnimation(phase0);
  for( int p=0; p<nAnim; p++ )
    {
    float time;
    Get(in,time); // get basic characeristics
    AnimationPhase phase=phase0;
    // load keyframe time
    phase.SetTime(time);
    // fill animation with neutral position
    //RedefineAnimation(phase);
    int ns,s;
    Matrix4 transforms[MAX_NAMED_SEL];
    //for( ns=0; ns<MAX_NAMED_SEL; ns++ ) transforms[ns].InitIdentity();
    for( ns=0; ns<MAX_NAMED_SEL; ns++ ) transforms[ns].SetZero();
    // load all transformations, check corresponding named selections
    for( s=0; s<nSel; s++ )
      {
      char name[32];
      Matrix4P transform;
      in.read(name,sizeof(name));
      Get(in,transform);
      int index=FindNamedSel(name);
      if( index<0 ) continue; // no corresponding selection
      transforms[index]=transform;
      }
    
    // apply transformations to current animation phasis
    for( int i=0; i<NPoints(); i++ )
      {
      const PointWeights &wg=weights[i];
      if( wg.Size()<=0 ) continue; // not animated
      Vector3 &pos=phase[i];
      // do compromises between weighted transformations
      Vector3 res=VZero;
      for( int w=0; w<wg.Size(); w++ )
        {
        const PointWeight &pw=wg[w];
        res+=transforms[pw.selIndex]*pos*pw.weight;
        }
      pos=res;
      }
    AddAnimation(phase);
    }
  WString sLength;
  sLength.Sprintf("%f",zStep);
  SetNamedProp("step",sLength);
  sLength.Sprintf("%f",xStep);
  SetNamedProp("xStep",sLength);
  
  // select first animation phasis
  if( _phase.Size()>0 )
    {
    _autoSaveAnimation=0;
    UseAnimation(*_phase[_autoSaveAnimation]);
    }
  }

//--------------------------------------------------

Vector3 ObjectData::GetStep(const char *selection) const
  {
  // measure step length
  // there are two methods:
  // 1st - measure half step length(in time 0.5)
  int startIndex=NearestAnimationIndex(0);
  int halfIndex=NearestAnimationIndex(0.5);
  int endIndex=NearestAnimationIndex(1.0);
  if( startIndex<0 ) return VZero;
  if( startIndex==halfIndex && startIndex==endIndex ) return VZero;
  const AnimationPhase *start=GetAnimation(startIndex);
  const AnimationPhase *half=GetAnimation(halfIndex);
  const AnimationPhase *end=GetAnimation(endIndex);
  float startTime=start->GetTime();
  float halfTime=half->GetTime();
  float endTime=end->GetTime();
  // check position of selection
/*  static const char com1[]="teziste";
  static const char com2[]="bricho";*/
  const NamedSelection *comSel=GetNamedSel(selection);
  if( !comSel ) return VZero;
  // calculate com of comSel
  int i;
  Vector3 startCom=VZero,halfCom=VZero,endCom=VZero;
  int nCom=0;
  //comSel->ValidatePoints(-1);
  for( i=0; i<NPoints(); i++ ) if( comSel->PointSelected(i) )
    {
    startCom+=(*start)[i];
    halfCom+=(*half)[i];
    endCom+=(*end)[i];
    nCom++;
    }
  if( nCom<=0 ) return VZero;
  startCom/=nCom;
  halfCom/=nCom;
  endCom/=nCom;
  // assume x,y step is zero
  if( (endTime-startTime)>0.99 )
    {
    // full step available
    return (endCom-startCom)*(1/(endTime-startTime));
    }
  else
    {
    // half step calculation
    return (halfCom-startCom)*(1/(halfTime-startTime));
    }
  }

//--------------------------------------------------

void ObjectData::RemoveStep( Vector3 stepLength )
  {
  int current=_autoSaveAnimation;
  // reset current data to neutral position
  UseAnimation(-1);
  int i;
  int index=0;
  for( i=0; i<NAnimations(); i++ )
    {
    AnimationPhase *phase=GetAnimation(i);
    if( phase->GetTime()<MinPositive ) continue;
    Vector3 offset=stepLength*phase->GetTime();
    for( int v=0; v<NPoints(); v++ )
      {
      (*phase)[v]-=offset;
      }
    }
  // select first animation phasis
  UseAnimation(current);
  }

//--------------------------------------------------

void ObjectData::NormalizeCenter( bool normX, bool normZ ,const char *centername)
  {
  // step should be already removed
  // move all animation phases so that COM is at Z-coordinate 0
  int current=_autoSaveAnimation;
  // reset current data to neutral position
  UseAnimation(-1);
  int i;
  int index=0;
  
  int startIndex=NearestAnimationIndex(0);
  if( startIndex<0 ) return;
  const AnimationPhase *start=GetAnimation(startIndex);
/*  static const char com1[]="teziste";
  static const char com2[]="bricho";*/
  const NamedSelection *comSel=GetNamedSel(centername);
  if( !comSel ) return;
  // calculate com of comSel
  Vector3 startCom=VZero;
  int nCom=0;
  for( i=0; i<NPoints(); i++ ) if( comSel->PointSelected(i) )
    {
    startCom+=(*start)[i];
    nCom++;
    }
  if( nCom<=0 ) return;
  startCom/=nCom;
  for( i=0; i<NAnimations(); i++ )
    {
    AnimationPhase *phase=GetAnimation(i);
    if( phase->GetTime()<MinPositive ) continue;
    for( int v=0; v<NPoints(); v++ )
      {
      if (normZ) (*phase)[v][2]-=startCom[2];
      if (normX) (*phase)[v][0]-=startCom[0];
      }
    }
  // select first animation phasis
  UseAnimation(current);
  
  }

//--------------------------------------------------

WString ObjectData::Description() const
  {
  if( _autoSaveAnimation<0 ) return "";
  WString ret;
  ret.Sprintf(", %d:%.6f",_autoSaveAnimation,_phase[_autoSaveAnimation]->GetTime());
  return ret;
  }

//--------------------------------------------------

