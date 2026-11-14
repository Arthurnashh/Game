// Poseidon - shape management
// (C) 1998, SUMA

#include "wpch.hpp"
#include "rtAnimation.hpp"
#include <El/QStream/qbStream.hpp>
#include <El/ParamFile/paramFile.hpp>
#include <Es/Types/scopeLock.hpp>
#include <El/Math/math3dP.hpp>
#include <Es/ReportStack/reportStack.hpp>
#include "serializeBinExt.hpp"
#include "diagModes.hpp"

#if 0 // _DEBUG
# pragma optimize("",off)
#endif

void AnimationRTWeight::Normalize()
{
  int n=Size();
  if( n==0 ) return;
  else if( n==1 )
  {
    Set(0).SetWeight(1);
  }
  else
  {
    int sum=0;
    for( int i=0; i<n; i++ )
    {
      sum +=Get(i).GetWeight8b();
    }
    if( sum>0 )
    {
      if (sum!=255)
      {
        float invSum=255.0f/sum;
        int rest=255;
        for( int i=1; i<n; i++ )
        {
          AnimationRTPair &pair = Set(i);
          float w = pair.GetWeight8b()*invSum;
          //pair.SetWeight(w);
          // SetWeight may incure rounding error
          int wi = toInt(w);
          saturate(wi,0,255);
          pair.SetWeight8b(wi);
          rest -= wi;
        }
        AnimationRTPair &pair = Set(0);
        pair.SetWeight8b(rest);
      }

      // verify normalization

      #ifdef _DEBUG
      int sumT=0;
      for( int i=0; i<n; i++ ) sumT += Get(i).GetWeight8b();
      Assert( sumT==255 );
      #endif
    }
    else
    {
      Fail("Sum zero");
    }
    // avoid rounding errors
  }
}

extern ParamFile Pars;

SkeletonSourceName::SkeletonSourceName(ParamEntryPar cls, const char *name)
:_class(safe_cast<const ParamEntry *>(&cls)),_name(name)
{
}

void SkeletonSource::Load(ParamEntryPar cfg)
{
  ParamEntryVal inherit = cfg>>"skeletonInherit";
  RStringB inheritName = inherit;
  if (inheritName.GetLength()>0)
  {
    if (!strcmpi(inheritName,cfg.GetName()))
    {
      RptF("Error: Infinite inheritance recursion in %s",(const char *)cfg.GetContext());
      return;
    }
    // read inherited information fist
    ConstParamEntryPtr parent = cfg.FindParent();
    if (parent)
    {
      Load((*parent)>>inheritName);
    }
  }
  // load skeleton bones
  ParamEntryVal bones = cfg>>"skeletonBones";
  if (bones.GetSize()%2!=0)
  {
    RptF("Error: Bad entry count in %s skeleton",(const char *)cfg.GetContext());
  }
  // make sure currently added bones will fit without reallocation
  // loading is done recursively - do not shrink (therefore Reserve, not Realloc)
  _structure.Reserve(_structure.Size() + bones.GetSize()/2);
  for (int i=0; i<bones.GetSize()-1; i+=2)
  {
    RStringB bone = bones[i];
    RStringB parent = bones[i+1];
    RString b = bone;
    RString p = parent;
    b.Lower();
    p.Lower();
    if (_structure.FindKey(RStringB(b))>=0)
    {
      RptF(
        "Warning: bone %s already listed in %s skeleton",
        cc_cast(b),cc_cast(cfg.GetContext())
      );
      continue;
    }
    if (!strcmp(p,b))
    {
      RptF(
        "Error: bone %s is parent of itself",
        cc_cast(b),cc_cast(cfg.GetContext())
      );
      continue;
    }
    // Add bone pair to structure definition
    SBoneRelation br;
    br._bone = b;
    br._parent = p;
    _structure.Add(br);
  }
  // used recursively, do not compact now - let OrderHierarchy handle it later
}


void SkeletonSource::RebuildParents()
{
  // build back-pointers to the parents
  _parent.Realloc(_structure.Size());
  _parent.Resize(_structure.Size());
  // save names, construct a hierarchy information
  for (int i = 0; i < _structure.Size(); i++)
  {
    const RStringB &parentName = _structure[i]._parent;
    const RStringB &boneName = _structure[i]._bone;
    SkeletonIndex parent;
    if (parentName.GetLength()==0)
    {
      parent = SkeletonIndexNone;
    }
    else
    {
      parent = SetSkeletonIndex(FindBone(parentName));
      if (!SkeletonIndexValid(parent))
      {
        RptF(
          "%s: Invalid parent bone '%s' for '%s'",
          cc_cast(GetName()),cc_cast(parentName),cc_cast(boneName)
          );
      }
    }
    _parent[i] = parent;
  }
}

void SkeletonSource::OrderHierarchy()
{
  RebuildParents();
  FindArrayKey<SBoneRelation> ordered;
  ordered.Realloc(_structure.Size());
  _tree.Build(this,ordered);

#if 0 // _DEBUG
  // now we can verify the structure is OK?
  struct LogBone
  {
    const Array<RStringB> &_names;

    LogBone(const Array<RStringB> &names):_names(names){}

    void operator () (int bone, int &parent) const
    {
      // do not log root - no real bone
      if (bone<=0) return;
      LogF(" \"%s\",\"%s\",",bone>0 ? cc_cast(_names[bone-1]) : "@",parent>0 ? cc_cast(_names[parent-1]) : "@");
      parent = bone;
    }
  };
  bones.Process(LogBone(names));
#endif
  // now we need to reorder the _structure based on the hierarchy we have
  
  
  if (ordered.Size()!=_structure.Size())
  {
    RptF("Skeleton %s contains bones unaccessible by hierarchy chain (cycles?)",cc_cast(_name));
  }
  _structure = ordered;
  // order changed, need to rebuild parent list
  // caution: no SkeletonIndex data may be stored at this moment, as they would be invalid
  RebuildParents();
}

void SkeletonSource::BuildHierarchy()
{
  RebuildParents();
  _tree.Build(this,_treeSI);
  _tree.Compact();
}

void SkeletonSource::RescanHierarchy()
{
  RebuildParents();
  // traverse the hierarchy
  // for each parent add all his children recursively
  #if 0
  _tree.AddChildrenOf(this,SkeletonIndexNone);
  _tree.Compact();
  #else
  _tree.Build(this,_treeSI);
  // the hierarchy was stored and should already be ordered well, we should get identity mapping now
  for (int i=0; i<_treeSI.Size(); i++)
  {
    Assert(_treeSI[i]==SetSkeletonIndex(i));
  }
  #endif
#if _ENABLE_REPORT
  // TODO: make sure the ordering did not change number of bones
  // this can be done by traversing one more time while verifying names
  // -1 because tree contains root not included in _structure
#endif
}

SkeletonSource::SkeletonSource(const SkeletonSourceName &name)
{
  _name = name._name;
  if (name._class && name._class->FindEntry(name._name))
  {
    ParamEntryVal cfg = (*name._class)>>name._name;
    Load(cfg);
    _isDiscrete = cfg>>"isDiscrete";
  }
  else
  {
    if (name._name[0] != '$')
    {
      RptF("Obsolete CfgSkeletons for %s, move to model.cfg",cc_cast(name._name));
      // no model config known - use global config
      ParamEntryVal notes = Pars>>"CfgSkeletons";
      ParamEntryVal cfg = notes>>name._name;
      Load(cfg);
      _isDiscrete = cfg>>"isDiscrete";
    }
  }

  // load 
  LoadConfig();

  // make sure bone ordering conforms to the hierarchy
  OrderHierarchy();
}

//SkeletonSource::SkeletonSource(ParamEntryPar cfg)
//{
//  Load(cfg);
//  _isDiscrete = cfg>>"isDiscrete";
//}

SkeletonSource::SkeletonSource(RStringB name, SerializeBinStream &f)
{
  _name = name;
  SerializeBin(f);

  LoadConfig();
}

void SkeletonSource::LoadConfig()
{
  ParamEntryVal sk_par=Pars>>"CfgSkeletonParameters";
  ConstParamEntryPtr sk_par_name=sk_par.FindEntry(_name);
  if (sk_par_name)
  {
    ParamEntryVal pivots_file=(*sk_par_name)>>"pivotsModel";
    _pivotsName = pivots_file;

    ConstParamEntryPtr sk_par_weap=sk_par_name->FindEntry("weaponBone");
    if (sk_par_weap)
    {
      _weaponName = (*sk_par_weap);
      _weaponName.Lower();
    }
  }
  else
    LogF("Skeleton %s does not have pivot file defined. Fallback to matrix based animation blending. Setup CfgSkeletonParameters/%s/pivotsModel.",_name.Data(),_name.Data());
}

void operator << (SerializeBinStream &f, SBoneRelation &br);
bool SkeletonSource::SerializeBin(SerializeBinStream &f)
{
  if (f.GetVersion() >= 23)
  {
    f.TransferBinary(_isDiscrete);
  }
  else
  {
    if (f.IsLoading())
    {
      _isDiscrete = true;
    }
  }
  f.TransferBasicArray(_structure);
  if (f.IsLoading())
  {
#if _VBS3
    if  ((f.GetVersion()<=40) || ((f.GetVersion() >= 10042) && (f.GetVersion() <= 10043)))
#else
    if  (f.GetVersion()<=40)
#endif
    {
      // older files have not hierarchy ordered yet
      // we cannot order it, we can only build a mapping
      BuildHierarchy();
    }
    else
    {
      f.Transfer(_pivotsNameObsolote);
      RescanHierarchy();
    }
  }
  else
  {
    f.Transfer(_pivotsNameObsolote);
  }
  return true;
}

void SkeletonSource::SkipBin(SerializeBinStream &f)
{
  SkeletonSource temp;
  temp.SerializeBin(f);
#if _ENABLE_REPORT
  // verify the one we skip is identical to the one we use
  int different = 0;
  if (_isDiscrete!=temp._isDiscrete) different|=1;
  if (_pivotsNameObsolote!=temp._pivotsNameObsolote) different|=2;
  /*
  if (_tree.Size()!=temp._tree.Size()) different++;
  else for (int i=0; i<_tree.Size(); i++)
  {
    if (_tree[i]!=temp._tree[i]) different++;
  }
  */
  if (_structure.Size()!=temp._structure.Size()) different|=4;
  else for (int i=0; i<_structure.Size(); i++)
  {
    if (_structure[i]._bone!=temp._structure[i]._bone) different|=8;
    if (_structure[i]._parent!=temp._structure[i]._parent) different|=16;
  }
  if (different>0)
  {
    static const char *diffNames[] = {"isDiscrete","pivots name","bones count","bone name","bone parent"};
    RString diffText;
    for (int i=0; i < sizeof(diffNames)/sizeof(diffNames[0]);++i)
    {
      if ((different&(1<<i))!=0)
      {
        if (diffText.GetLength()==0)
          diffText = diffNames[i];
        else
          diffText = diffText + "|" +diffNames[i];
  }
    }
    RptF("Embedded skeleton %s different in different p3d files, diff=%s",cc_cast(_name),cc_cast(diffText));
  }

#endif
}

int SkeletonSource::FindBone(const RStringB &name) const
{
  for (int i=0; i<_structure.Size(); i++)
  {
    if (_structure[i]._bone==name) return i;
  }
  return -1;
}

SkeletonSource *SkeletonBank::NewEmbedded(const RStringB &name, SerializeBinStream &f)
{
  // Check if the skeleton is presented in the bank first
  SkeletonSource *skeleton = Find(SkeletonSourceName(name));
  if (skeleton)
  {
    skeleton->SkipBin(f);
    return skeleton;
  }

  // Create the skeleton
  skeleton = new SkeletonSource(name, f);
  BankArrayTraitsExt::AddItem(*this, skeleton);
  return skeleton;
}

SkeletonBank SkeletonSources;

Skeleton::Skeleton()
{
  _isDiscrete = false;
  _reportMissingBones = true;
}

Skeleton::Skeleton(RStringB name)
{
  _name = name;
  if (name.GetLength() > 0)
  {
    Ref<SkeletonSource> skeletonSource = SkeletonSources.New(name);
    LoadFromSkeletonSource(skeletonSource);
  }
  else
  {
    _isDiscrete = true;
  }
  _reportMissingBones = true;
}

Skeleton::~Skeleton()
{
}

SkeletonIndex Skeleton::FindBone(RStringB name) const
{
  DoAssert(name.IsLowCase());
  // check if bone already exists
  for (int i=0; i<_matrixNames.Size(); i++)
  {
    if (name==_matrixNames[i])
    {
      return SetSkeletonIndex(i);
    }
  }
  return SkeletonIndexNone;
}


void Skeleton::LoadFromSkeletonSource(Ref<SkeletonSource> skeletonSource)
{
  _source = skeletonSource;
  _isDiscrete = skeletonSource->IsDiscrete();

  _matrixNames.Realloc(_source->Structure().Size());
  _matrixNames.Resize(0);
  // save names, construct a hierarchy information
  for (int i = 0; i < _source->Structure().Size(); i++)
  {
    RStringB parentName = _source->Structure()[i]._parent;
    RStringB boneName = _source->Structure()[i]._bone;
    _matrixNames.Add(boneName);
  }
  // matrix ordering in the skeleton is arbitrary
  _matrixNames.Compact();

  RString pivots = _source->GetPivotsName();
  if (!pivots.IsEmpty())
  {
    _weapBone = FindBone(_source->GetWeaponName());

    // we assume all pivoted models are reversed
    Ref<LODShape> pivotsShape = Shapes.New(pivots,true,false);
    // read points from the memory LOD
    if (pivotsShape)
    {
      const Shape *memory = pivotsShape->MemoryLevel();
      if (memory)
      {
        // for each bone find its pivot

        _pivots.Realloc(_matrixNames.Size());
        _pivots.Resize(_matrixNames.Size());

        for (int i=0; i<_matrixNames.Size(); i++)
        {
          RStringB name = _matrixNames[i];
          int point = pivotsShape->PointIndex(memory,name);
          if (point<0)
          {
            // when pivot not found, use a parent
            // we know parent is already computed, because of tree ordering
            int parent = GetSkeletonIndex(GetParent(SetSkeletonIndex(i)));
            if (parent>=0)
            {
              _pivots[i] = _pivots[parent];
            }
            else
            {
              // no pivot found, and there is no parent - initialize with any value
              // this probably indicates an error in pivot definition?
              _pivots[i] = VZero;
            }
          }
          else
          {
            _pivots[i] = memory->Pos(point);
          }
        }
      }
      else RptF("Missing Memory level in skeleton pivots file %s.",pivots.Data());
    }
  }
}

RStringB Skeleton::GetBone(SkeletonIndex i) const
{
  if(GetSkeletonIndex(i) >= 0 && GetSkeletonIndex(i) < _matrixNames.Size())
    return _matrixNames[GetSkeletonIndex(i)];

  return "";
}

void Skeleton::LoadCollPatterns(ParamEntryPar cfg)
{
  if (_collisionVertexPattern.Size() != 0 && _collisionGeomCompPattern.Size() != 0)
    return; //already loaded

  // gestures may be loaded before moves, with no pattern definition
  // TODO: find some cleaner solution
  if (!cfg.FindEntry("collisionVertexPattern")) return;
  
  ParamEntryVal entry = cfg>>"collisionVertexPattern";
  if (entry.IsArray())
  {
    int n = entry.GetSize();
    _collisionVertexPattern.Realloc(n);
    _collisionVertexPattern.Resize(n);
    for(int i = 0; i < n; i++)
    {
      _collisionVertexPattern[i] = entry[i];
    }
  }

  entry = cfg>>"collisionGeomCompPattern";
  if (entry.IsArray())
  {
    int n = entry.GetSize();
    _collisionGeomCompPattern.Realloc(n);
    _collisionGeomCompPattern.Resize(n);
    for(int i = 0; i < n; i++)
    {
      _collisionGeomCompPattern[i] = entry[i];
    }
  }
}

SkeletonIndex Skeleton::NewBone(RStringB name)
{
  // the bone must exists in the skeleton
  for (int i = 0; i < _matrixNames.Size(); i++)
  {
    if (!strcmpi(name, _matrixNames[i]))
    {
      return SetSkeletonIndex(i);
    }
  }

  // Adding bones dynamically is not possible
  if (_reportMissingBones)
  {
    RPTSTACK();
    RptF("Error: Bone %s doesn't exist in some skeleton ", name.Data());
  }
  return SkeletonIndexNone;
}


/// animation represented in a hierarchy
/**
hierarchical animation contains only orientations for individual bones + additional offsets
For bones fixed on a joint the offsets are mostly zero.
*/

template <class LStore, class MStore>
struct ConvertToLocal
{
  Array<LStore> &_local;
  const Array<MStore> &_model;
  const Array<int> &_boneMap;

  ConvertToLocal(Array<LStore> &local, const Array<MStore> &model, const Array<int> &boneMap)
  :_local(local),_model(model),_boneMap(boneMap)
  {
  }

  Matrix4 operator () (int bone, Matrix4Par parentSpace) const
  {
    // process this bone in a current space
    if (bone>0)
    {
      // bone is SkeletonIndex here
      int boneIndex = _boneMap[bone-1];
      // if the bone is not present, parent space is unaffected
      if (boneIndex>=0)
      {
        // for multiplication we always need Matrix4
        Matrix4Val anim = _model[boneIndex];
        // convert the animation matrix into the parent space
        // consider: inversion might better be done while setting the parent space
        _local[boneIndex] = parentSpace.InverseGeneral() * anim;
        // parent space for all children is our original animation matrix
        return anim;
      }
    }
    return parentSpace;
  }
};

template <class MStore, class LStore>
struct ConvertToModel
{
  const Array<LStore> &_local;
  Array<MStore> &_model;
  const Array<int> &_boneMap;

  ConvertToModel(Array<MStore> &model, const Array<LStore> &local, const Array<int> &boneMap)
  :_local(local),_model(model),_boneMap(boneMap)
  {
  }

  Matrix4 operator () (int bone, Matrix4Par parentSpace) const
  {
    // process this bone in a current space
    if (bone>0)
    {
      // bone is SkeletonIndex here
      int boneIndex = _boneMap[bone-1];
      // if the bone is not present, parent space is unaffected
      if (boneIndex>=0)
      {
        // for multiplication we always need Matrix4
        Matrix4Val anim = _local[boneIndex];
        // convert the animation matrix from the parent space
        _model[boneIndex] = parentSpace * anim;
        // parent space for all children is our animation matrix
        return _model[boneIndex];
      }
    }
    return parentSpace;
  }
};

template <class LocalStore, class ModelStore>
void AnimationPhaseToLocal(Array<LocalStore> &local, const Array<ModelStore> &model, const Skeleton *sk, const Array<int> &boneMap)
{
  sk->GetTree().ProcessCtx(ConvertToLocal<LocalStore,ModelStore>(local,model,boneMap),0,MIdentity);
}

template <class ModelStore, class LocalStore>
void AnimationPhaseToModel(Array<ModelStore> &model, const Array<LocalStore> &local, const Skeleton *sk, const Array<int> &boneMap)
{
  sk->GetTree().ProcessCtx(ConvertToModel<ModelStore,LocalStore>(model,local,boneMap),0,MIdentity);
}

AnimationRTPhase::AnimationRTPhase()
{
}

AnimationRTPhase::~AnimationRTPhase()
{
}


void AnimationRTPhase::SerializeBin(SerializeBinStream &f,const Skeleton *sk,const Array<int> *boneMap)
{
  // consider using one common number of matrices, not storing it in each frame separately
  f.TransferBinaryArray(_local);
  if (f.IsLoading() && sk)
    ConvertLocalWithPivots(sk,boneMap);
}

void AnimationRTPhase::ConvertLocalWithPivots(const Skeleton *sk,const Array<int> *boneMap)
{
  if (sk->GetPivots().Size()>0)
  {
    // convert translations of bones using skeleton pivots to bone origin local space
    const Vector3 *pivot;
    int idx;
    for(int bone=0;bone<sk->GetPivots().Size();bone++)
    {
      idx=(*boneMap)[bone];
      if (idx>=0 && bone != GetSkeletonIndex(sk->GetWeaponBone()))
      {
        pivot=&sk->GetPivots().Get(bone);
        //Matrix4(_local[idx]).FastTransform(pivot);
        Vector3Par pos=Matrix4(_local[idx]).FastTransform(*pivot);
        _local[idx].SetPosition(pos);
      }
    }
  }
}

const AnimationRTMatrix AnimationRTMatrixIdentity = MIdentity;

/// when 1, each animation contains all bones from the skeleton, even when not used in the animation
#define MATRICES_ALL_BONES 0

float AnimationRTPhase::Load(QIStream &in, Skeleton *skelet, int nSel, const Array<int> &boneMap, bool reversed)
{
  // load single phase
  float time = 0;
  in.read(&time,sizeof(time));
  SerializeBinStream::ProcessLittleEndian32b(&time,sizeof(time));
  if (in.fail())
  {
    // cannot load - skip and return
    RptF("Broken animation file");
    return 0;
  }
  Vector3 pnp(+1,-1,+1);
  Vector3 npn(-1,+1,-1);
  AutoArray<Matrix4, MemAllocDataStack<Matrix4,256,AllocAlign16> > origMatrix;
  // set all matrices to identity - some matrices may be skipped while loading
  #if MATRICES_ALL_BONES
    int nMatrices = skelet->NBones();
    origMatrix.Resize(nMatrices);
    for (int i=0; i<nMatrices; i++) origMatrix[i] = MIdentity;
    AutoArray<bool, MemAllocLocal<bool,256> > usedMatrix;
    usedMatrix.Resize(nMatrices);
    for (int i=0; i<nMatrices; i++) usedMatrix[i] = false;
  #else
    origMatrix.Resize(0);
  #endif
  for( int i=0; i<nSel; i++ )
  {
    char name[32];
    Matrix4P transformP;
    in.read(name,sizeof(name));
    // get matrix index from corresponding skeleton

    strlwr(name);
    
    in.read((char *)&transformP,sizeof(transformP));
    SerializeBinStream::ProcessLittleEndian32b(&transformP,sizeof(transformP));
    
    // if the bone does not exist in the skeleton, skip it
    int index = GetSkeletonIndex(skelet->FindBone(name));
    if (index>=0)
    {
      Matrix4 transform = ConvertToM(transformP);
      if( reversed )
      {
        //const static Matrix4P swapMatrix(MRotationY,H_PI);
        //const static Matrix4P swapMatrix(MScale,-1,1,-1);
        //transformP = swapMatrix*transformP*swapMatrix;
        // optimization: replaced swap with algebraic simplification
        //transformP(0,1) = -transformP(0,1);
        //transformP(1,0) = -transformP(1,0);
        //transformP(1,2) = -transformP(1,2);
        //transformP(2,1) = -transformP(2,1);
        transform.SetDirectionAside(transform.DirectionAside().Modulate(pnp));
        transform.SetDirectionUp   (transform.DirectionUp   ().Modulate(npn));
        transform.SetDirection     (transform.Direction     ().Modulate(pnp));
        transform.SetPosition      (transform.Position      ().Modulate(npn));
      }
      #if MATRICES_ALL_BONES
      origMatrix[index] = transform;
      usedMatrix[index] = true;
      #else
      origMatrix.Add(transform);
      #endif
    }
//    else
//    { // some bones may be disabled in the skeleton config
//      RptF("Bad bone %s",name);
//    }
  }
  _local.Realloc(origMatrix.Size());
  _local.Resize(origMatrix.Size());
  AnimationPhaseToLocal<AnimationRTMatrix,Matrix4>(_local,origMatrix,skelet,boneMap);
  #if MATRICES_ALL_BONES
  // set all unused matrices to identity (this will make sure they inherit from their parents)
  const AnimationRTMatrix &rtIdentity = MIdentity;
  for (int i=0; i<origMatrix.Size(); i++) if (!usedMatrix[i])
  {
    _local[i] = rtIdentity;
  }
  #endif
  
  ConvertLocalWithPivots(skelet,&boneMap);

  return time;
}

void AnimationRTPhase::Skip(QIStream &in, int nSel)
{
  // load single phase
  in.seekg(sizeof(float)+nSel*(32+sizeof(Matrix4P)),QIOS::cur);
}

/**
120 KB was too little, even common animations were streamed
Even 256 KB causes streaming of some common (idle) animations
*/

const int AnimationRTPhases::_bufferSize = 512 * 1024;

void AnimationRTPhases::Init()
{
  // invalidate all content
  _streamOffsets.Clear();
  _buffer.Init();
  _phasesCount = 0;
  _streamed = false;
  _firstValid = false;
}

void AnimationRTPhases::InitIdentity(Skeleton *skelet)
{
  // only the _firstPhase will be initialized
  _streamOffsets.Clear();
  _buffer.Init();
  _streamed = false;

  // scan animation data
  int nBones = skelet->NBones();
  _firstPhase = new AnimationRTPhase;
  _firstPhase->_local.Realloc(nBones);
  _firstPhase->_local.Resize(nBones);
  // set all matrices to identity
  const AnimationRTMatrix &rtIdentity = MIdentity;
  for (int i=0; i<nBones; i++)
  {
    _firstPhase->_local[i] = rtIdentity;
  }
  _phasesCount = 1;
  _firstValid = true;
}

void AnimationRTPhases::LimitCount(int count)
{
  if (count > _phasesCount) _phasesCount = count;
}

void AnimationRTPhases::SaveBin(SerializeBinStream &f)
{
  // assert the structure is trivial
  Assert(CanBeSaved());
  Assert(_buffer._phasesStart == 0);
  Assert(_buffer._phasesOffset == 1);

  if (_firstValid) _firstPhase->SerializeBin(f,NULL,NULL);
  for (int i=0; i<_buffer._phases.Size(); i++) _buffer._phases[i]->SerializeBin(f,NULL,NULL);
}

void AnimationRTPhases::LoadHeaderBin(SerializeBinStream &f,const Skeleton *sk,const Array<int> *boneMap)
{
  // the first frame is always loaded (as a fall back)
  _firstPhase = new AnimationRTPhase;
  _firstPhase->SerializeBin(f,sk,boneMap);
  _firstValid = true;
  _phasesCount = 1;

  // store the offset of frame 1
  DoAssert(_streamOffsets.Size() == 0);
  _streamOffsets.Realloc(1);
  _streamOffsets.Resize(1);
  _streamOffsets[0] = f.TellG();
}

void AnimationRTPhases::LoadPhasesBin(SerializeBinStream &f, int count, bool streamingEnabled,const Skeleton *sk,const Array<int> *boneMap)
{
  Assert(_firstValid); // first frame already loaded
  Assert(count > 0);
  Assert(_streamOffsets.Size() > 0);

  // prepare the structure for count items
  _phasesCount = count;
  _buffer._phasesStart = 0; // beginning of _phases
  _buffer._phasesOffset = 1; // frame #0 stored in _firstPhase
#if DIAG_EXTERNAL_BUFFER
  _buffer._animLoadCount++;
#endif
  {
    // initialize _streamOffsets (only needed items)
    int n = _streamOffsets.Size();
    if (count != n)
    {
      DoAssert(count > n)
      _streamOffsets.Realloc(count);
      _streamOffsets.Resize(count);
      for (int i=n; i<count; i++) _streamOffsets[i] = -1; // offsets not set yet
    }
  }

  // set file pointer to the item #1
  f.SeekG(_streamOffsets[0]);
  // first item handled
  count--;

  _streamed = false;
  if (streamingEnabled)
  {
    // calculate the maximal number of frames which can be stored in buffer
    int itemSize = sizeof(int) + _firstPhase->_local.Size() * sizeof(AnimationRTMatrix); // size + bones * matrix
    int maxCount = _bufferSize / itemSize;
    if (count > maxCount)
    {
      _streamed = true;
      count = maxCount;
    }
    // count is now number of frames we want to read immediately
  }

  // load the requested items
  _buffer._phases.Realloc(count);
  _buffer._phases.Resize(count);
  for (int i=0; i<count; i++)
  {
    _buffer._phases[i] = new AnimationRTPhase;
    _buffer._phases[i]->SerializeBin(f,sk,boneMap);
    _streamOffsets[i + 1] = f.TellG();
  }
}

/**
Validation - extracted from AnimationRTPhases::LoadNeededPhases
*/
bool AnimationRTPhases::CheckBufferValid(const AnimationRTBuffer &buffer) const
{
  if (buffer._phases.Size() == 0)
  {
    // empty buffer is valid
    return true;
  }
  int offset = buffer._phasesOffset + 1;
  if (offset >= _phasesCount)
  {
    // use the start of animation again
    offset -= _phasesCount;
  }
  DoAssert(offset>=0 && offset < _phasesCount);
  // what phase need to be loaded to do it
  int phase = offset + buffer._phases.Size() - 1;
  if (phase >= _phasesCount)
  {
    // use the start of animation again
    phase -= _phasesCount;
  }
  DoAssert(phase < _phasesCount);
  if (phase != 0)
  {
    return _streamOffsets[phase - 1] >= 0;
  }
  return true;
}

void AnimationRTPhases::LoadNeededPhases(SerializeBinStream &f, AnimationRTBuffer &buffer, int phase, int phasePastNeeded, const Skeleton *sk,const Array<int> *boneMap) const
{
  Assert(_firstValid); // first frame already loaded
  Assert(_streamOffsets.Size() > 0);
  Assert(_buffer._phases.Size() > 0);

  if (buffer._phases.Size() == 0)
  {
    // external buffer not yet initialized
    buffer = _buffer;
  }
  DoAssert(buffer._phases.Size()>0);
#if DIAG_EXTERNAL_BUFFER
  DoAssert(stricmp(buffer._animName, _buffer._animName) == 0);
  DoAssert(buffer._animLoadCount == _buffer._animLoadCount);
#endif

  // upper limit of single frame size in the binarized file
  int itemSize = sizeof(int) + _firstPhase->_local.Size() * sizeof(AnimationRTMatrix); // size + bones * matrix

  // what frames we want
  int wantedOffsetMin = phasePastNeeded - 1; // keep some more frames prior phase so that Render VS can access it
  int wantedOffsetMax = phase - 1;

  // buffer end is buffer._phasesOffset+_phasesCount
  // verify this contains both phases we need
  
  int wantedOffset = wantedOffsetMin;
  Assert(wantedOffsetMax<wantedOffsetMin+_phasesCount);(void)wantedOffsetMax;

  saturate(wantedOffset, 1, _phasesCount - 1);

  // now we can read files only sequentially, maybe we would be able to solve some exceptions as well
  // we want the buffer to contain a much forward as possible, but still contain phase and phasePastNeeded
  if (wantedOffset != buffer._phasesOffset)
  {
    if (CHECK_DIAG(DEAnimation))
    {
      LogF("Streaming animation %d <- %d",wantedOffset,buffer._phasesOffset);
    }
//    LogF("AnimationRTPhases::LoadNeededPhases: current offset %d, wanted %d", _phasesOffset, wantedOffset);
//    LogF(" (buffer %d / %d)", _phases.Size(), _phasesCount);
    // frames after the currently loaded frames needed
    while (buffer._phasesOffset != wantedOffset)
    {
      // try to make (_phasesOffset + 1) the new _phasesOffset
      int offset = buffer._phasesOffset + 1;
      if (offset >= _phasesCount)
      {
        // use the start of animation again
        offset -= _phasesCount;
      }
      DoAssert(offset>=0 && offset < _phasesCount);
      // what phase need to be loaded to do it
      int phase = offset + buffer._phases.Size() - 1;
      if (phase >= _phasesCount)
      {
        // use the start of animation again
        phase -= _phasesCount;
      }
      DoAssert(phase < _phasesCount);
      if (phase == 0)
      {
        // use frame #0 to avoid too complicated calculations
        buffer._phases[buffer._phasesStart] = _firstPhase;
        DoAssert(buffer._phases[buffer._phasesStart]->_local.Size()>0);
        // _streamOffsets[0] should be already set
      }
      else
      {
        int pos = _streamOffsets[phase - 1];
        if (pos < 0)
        {
          Fail("Bad animation offset");
          break;
        }
        if (!f.GetLoadStream()->PreRead(FileRequestPriority(10), pos, itemSize))
        {
          // file not ready, wait a bit
          // it would be nice to preload a few frames more here, but we cannot rely on _streamOffsets for this, as this is not initialized yet
          // we can use heuristic only instead, therefore we do the preload in one piece to keep request matching simple
          int toPreloadMax = _streamOffsets.Size()-phase;
          // preload at least to the phase we wanted
          int toPreload = wantedOffset-buffer._phasesOffset;
          if (toPreload<0) toPreload += _phasesCount;
          // preload at least 5 frames
          if (toPreload<5) toPreload = 5;
          if (toPreload>toPreloadMax) toPreload = toPreloadMax;
          if (toPreload>0) f.GetLoadStream()->PreRead(FileRequestPriority(20), pos, itemSize*toPreload);
          break;
        }
        DoAssert(pos>=0);
        f.SeekG(pos);
        DoAssert(buffer._phasesStart>=0 && buffer._phasesStart<buffer._phases.Size());
        buffer._phases[buffer._phasesStart] = new AnimationRTPhase;
        buffer._phases[buffer._phasesStart]->SerializeBin(f,sk,boneMap);
        DoAssert(buffer._phases[buffer._phasesStart]->_local.Size()>0);
        DoAssert(buffer._phases[buffer._phasesStart]->_local.Size()==_firstPhase->_local.Size());
        _streamOffsets[phase] = f.TellG();
      }
//      LogF(" - frame %d loaded to slot %d", phase, _phasesStart);
      buffer._phasesStart++;
      if (buffer._phasesStart >= buffer._phases.Size()) buffer._phasesStart -= buffer._phases.Size();
      buffer._phasesOffset = offset;
//      LogF("  (new offset %d, new start %d", _phasesOffset, _phasesStart);
    }
  }
}

void AnimationRTPhases::Release()
{
  // set the structure to the state before LoadPhasesBin
  _buffer.Init();
  // shrink data
  if (_firstValid)
    _phasesCount = 1;
  else
    _phasesCount = 0;
  _streamed = false;
}

int AnimationRTPhases::GetMemoryControlled() const
{
  int nPhases = _buffer._phases.Size();
  if (nPhases == 0) return 0;
  return nPhases * _buffer._phases[0]->_local.Size() * sizeof(AnimationRTMatrix);
}

double AnimationRTPhases::GetMemoryUsed() const
{
  int countMatrices = _firstValid ? _buffer._phases.Size() * _firstPhase->_local.Size() : 0;
  return _streamOffsets.GetMemoryUsed() + _buffer._phases.MaxSize() * sizeof(AnimationRTPhase) + countMatrices * sizeof(AnimationRTMatrix);
}

void AnimationRTPhases::LoadPhases(
  QIStream &in, Skeleton *skelet, int nSel, int startAnim, int endAnim,
  AutoArray<int> &boneMap, bool reversed, AutoArray<float> &times)
{
  _phasesCount = endAnim;
  _buffer._phasesStart = 0; // beginning of _phases
  _buffer._phasesOffset = 1; // frame #0 stored in _firstPhase
  if (endAnim <= 0) return;

  _buffer._phases.Realloc(endAnim - 1);
  _buffer._phases.Resize(endAnim - 1);
  times.Realloc(endAnim);
  times.Resize(endAnim);

  // load animation data
  if (startAnim == 0)
  {
    _firstPhase = new AnimationRTPhase;
    float time = _firstPhase->Load(in, skelet, nSel, boneMap, reversed);
    times[0] = time;
    _firstValid = true;
    startAnim++;
  }
  else
  {
    // skip animation data which are already loaded
    for (int i=0; i<startAnim; i++)
    {
      AnimationRTPhase::Skip(in, nSel);
    }
  }
  for (int i=startAnim; i<endAnim; i++)
  {
    _buffer._phases[i - 1] = new AnimationRTPhase;
    float time = _buffer._phases[i - 1]->Load(in, skelet, nSel, boneMap, reversed);
    times[i] = time;
  }
}

void AnimationRTPhases::Request(
  const AnimationRTPhase* &prevPhase, const AnimationRTPhase* &nextPhase,
  int &prevIndex, int &nextIndex, bool looped, const AnimationRTBuffer *buffer) const
{
  Assert(_firstValid);

  if (_buffer._phases.Size() == 0)
  {
    // no frames loaded yet - only the _firstFrame is valid
    prevIndex = 0;
    nextIndex = 0;
    prevPhase = _firstPhase;
    nextPhase = _firstPhase;
    return;
  }

  const AnimationRTBuffer *buf = &_buffer;
  if (_streamed)
  {
    //DoAssert(buffer);
    if (buffer && buffer->_phases.Size() > 0) // external buffer exists and initialized
      buf = buffer;
  }

  // check the interval covered by the buffer
  int minIndex = buf->_phasesOffset;
  int maxIndex = buf->_phasesOffset + buf->_phases.Size() - 1;
  // when cyclic buffer cover also start of animation, check it as well
  int maxIndex2 = -1;
  if (maxIndex >= _phasesCount)
  {
    maxIndex2 = maxIndex - _phasesCount;
    maxIndex = (_phasesCount - 1);
// LogF("Covered %d .. %d, 0 .. %d", minIndex, maxIndex, maxIndex2);
  }

  // when prevIndex / nextIndex is out of bounds, try to interpolate between loaded item and 0 when possible

  if (prevIndex < minIndex)
  {
    if (prevIndex > maxIndex2) // check also the second interval (frame beginning)
      prevIndex = 0; // frame 0 is always loaded
  }
  else if (prevIndex > maxIndex) prevIndex = maxIndex;

  if (nextIndex < minIndex && nextIndex != 0)
  {
    if (nextIndex > maxIndex2) // check also the second interval (frame beginning)
      nextIndex = minIndex;
  }
  else if (nextIndex > maxIndex)
  {
    if (looped) nextIndex = 0;
    else nextIndex = maxIndex;
  }

  // correct access to the cyclic buffer
  if (prevIndex == 0) prevPhase = _firstPhase;
  else if (prevIndex < minIndex)
  {
// LogF("- reading phase %d from slot %d", prevIndex, ((prevIndex + _phasesCount) - _phasesOffset + _phasesStart) % _phases.Size());
    prevPhase = buf->_phases[((prevIndex + _phasesCount) - buf->_phasesOffset + buf->_phasesStart) % buf->_phases.Size()];
  }
  else prevPhase = buf->_phases[(prevIndex - buf->_phasesOffset + buf->_phasesStart) % buf->_phases.Size()];

  if (nextIndex == 0) nextPhase = _firstPhase;
  else if (nextIndex < minIndex)
  {
// LogF("- reading phase %d from slot %d", nextIndex, ((nextIndex + _phasesCount) - _phasesOffset + _phasesStart) % _phases.Size());
    nextPhase = buf->_phases[((nextIndex + _phasesCount) - buf->_phasesOffset + buf->_phasesStart) % buf->_phases.Size()];
  }
  else nextPhase = buf->_phases[(nextIndex - buf->_phasesOffset + buf->_phasesStart) % buf->_phases.Size()];
}

const int PersistantLimit = 3;

bool AnimationRTPhases::Preload(QIStream &in, FileRequestPriority priority, bool fullLoad)
{
  // calculate preload size for the binarized animation
  const int nBones = 128; // number of bones (127 for ArmA 2 character)
  int headerSize = 64; // version, sizes etc.
  headerSize += nBones * 16; // names of selections
  headerSize += 4000 * sizeof(float); // phaseTimes - max. 4000 frames (over 2 minutes with 30 fps)

  if (fullLoad)
  {
    // fill the whole cyclic buffer
    return in.PreRead(priority, 0, headerSize + _bufferSize);
  }
  else
  {
    // if the animation is short, we may need PersistantLimit frames
    return in.PreRead(priority, 0, headerSize + PersistantLimit * (nBones * sizeof(AnimationRTMatrix)));
  }
}

#define RTMMagicLen 8
static const char RTMMagic100[RTMMagicLen+1]="RTM_0100";
static const char RTMMagic101[RTMMagicLen+1]="RTM_0101";

DEFINE_FAST_ALLOCATOR(AnimationRT)

AnimationRT::AnimationRT()
{
  _loadCount = 0;
  _preloadCount=0;
  _nPhases = 0;
  _version = 0;
  #ifdef _M_PPC
  _endianState = EndianUnknown;
  #endif
  _lzoCompression = false;
}

AnimationRT::AnimationRT(const AnimationRTName &name)
{
  REPORTSTACKITEM(name.name);
  _loadCount = 0;
  _preloadCount=0;
  _name = name;
  _nPhases = 0;
  _version = 0;
  #ifdef _M_PPC
  _endianState = EndianUnknown;
  #endif
  _lzoCompression = false;
  Load(name.skeleton,name.name,name.reversed);

#if DIAG_EXTERNAL_BUFFER
  _phasesBuffer.SetName(name.name);
#endif
}

void AnimationRT::SerializeBin(Skeleton *skelet, SerializeBinStream &f)
{
  if (f.IsSaving())
  {
    // streamed animation cannot be saved
    if (!_phasesBuffer.CanBeSaved())
    {
      f.SetError(SerializeBinStream::EGeneric);
      return;
    }
  }
  // do we need the skeleton at all?
  // transfer complete header
#ifdef _MSC_VER
  if (!f.Version('RTMB'))
#else
  if (!f.Version(StrToInt("BMTR")))
#endif
  {
    f.SetError(SerializeBinStream::EFileStructure);
    return;
  }

  // version 3 - LZO compression used for compressed arrays
  const int actVersion = 3;
  const int minVersion = 2;
  if (!f.Version(minVersion, actVersion))
  {
    f.SetError(SerializeBinStream::EBadVersion);
    return;
  }
  if (f.GetVersion() >= 3) f.UseLZOCompression();

  f.Transfer(_reversed);
  f.Transfer(_step);
  f.Transfer(_nPhases);
  f.Transfer(_preloadCount);
  int nAnim;
  if (f.IsSaving())
  {
    nAnim = _phasesBuffer.NAnimatedBones();
    f.SaveInt(nAnim);
  }
  else
  {
    _version = f.GetVersion();
    nAnim = f.LoadInt();
    #ifdef _M_PPC
    _endianState = f.GetEndianState();
    #endif
    _lzoCompression = f.IsUsingLZOCompression();
  }
  f.TransferBasicArray(_selections);
  // scan all selection and update weights as necessary
  if (f.IsLoading())
  {
    // we prefer building boneMap runtime to be independent on skeleton bone ordering
    _boneMap.Realloc(skelet->NBones());
    _boneMap.Resize(skelet->NBones());
    #if MATRICES_ALL_BONES
      // prototype: create 1:1 mapping for all bones in the skeleton
      for (int i=0; i<_boneMap.Size(); i++) _boneMap[i] = i;
    #else
      // unless a bone is found, assume it is not touched by this animation
      for (int i=0; i<_boneMap.Size(); i++) _boneMap[i] = -1;
      for (int i=0; i<_selections.Size(); i++)
      {
        SkeletonIndex bone = skelet->NewBone(_selections[i]);
        if (SkeletonIndexValid(bone))
        {
          _boneMap[GetSkeletonIndex(bone)]=i;
        }
      }
    #endif
  }

  f.TransferBinaryArray(_phaseTimes);
  
  if (f.IsSaving())
  {
    _phasesBuffer.SaveBin(f);
  }
  else
  {
    if (_nPhases > 0)
    {
      _phasesBuffer.LoadHeaderBin(f,skelet,&_boneMap);
      if (_preloadCount > 0)
      {
        _phasesBuffer.LoadPhasesBin(f, _nPhases, _name.streamingEnabled,skelet,&_boneMap);

        if (_phasesBuffer.IsStreamed())
        {
          _stream = new QIFStreamB;
          _stream->AutoOpen(_name.name);
          if (_stream->fail() || _stream->rest() <= 0)
          {
            // if there is any failure, discard the stream, as it would be unusable
            _stream.Free();
          }
        
        }


//         if (_phasesBuffer.IsStreamed())
//         {
//           LogF("Animation %s is streamed", cc_cast(_name.name));
//         }
      }
    }
    _loadCount = _preloadCount > 0;
  }
}

/**
@return true when OK
*/
bool AnimationRT::LoadOptimized(Skeleton *skelet, const char *name)
{
  QIFStreamB in;
  in.AutoOpen(name);
  return LoadOptimized(skelet,in);
}

bool AnimationRT::LoadOptimized(Skeleton *skelet, QIStream &in)
{
  SerializeBinStream f(&in);
  SerializeBin(skelet,f);
  return f.GetError()==SerializeBinStream::EOK;
}

void AnimationRT::SaveOptimized(Skeleton *skelet, const char *name)
{
  // before saving we need to make sure we have all data loaded
  AddLoadCount(true);
  QOFStream out;
  out.open(name);
  SerializeBinStream f(&out);
  SerializeBin(skelet,f);
  out.close();
  ReleaseLoadCount();
}

#include "El/FreeOnDemand/memFreeReq.hpp"

class AnimationRTCache: public MemoryFreeOnDemandHelper
{
friend class AnimationRTLock;

  TListBidir<AnimationRT> _loaded;

#if ANIMATION_RT_SLIST
  /// collected requests for loading from the current frame
  LockFreeGrowingList<AnimationRT> _loadRequested;
#endif

  int _count;
  int _totalSize;

  public:
  AnimationRTCache();
  ~AnimationRTCache();

  void Load(AnimationRT *anim);
  void Delete(AnimationRT *anim);

  size_t FreeOneItem();
  float Priority() const;
  size_t MemoryControlled() const;
  RString GetDebugName() const;
  bool CheckIntegrity() const;

#if ANIMATION_RT_SLIST
  /// handle _loadRequested list, load wanted animations to the cache
  void PerformLoading();
  /// clean-up _loadRequested list
  void CleanUp();
#endif
};

RString AnimationRTCache::GetDebugName() const
{
  return "Anim";
}

size_t AnimationRTCache::FreeOneItem()
{
  AnimationRT *item = _loaded.Last();
  if (!item) return 0;
  size_t size = item->GetMemoryControlled();
  Delete(item);
  return size;
}

float AnimationRTCache::Priority() const
{
  const int AnimationRTCacheItemSize = 50*1024;
  const float AnimationRTCacheItemTime = 10000;

  return AnimationRTCacheItemTime/AnimationRTCacheItemSize;
}

bool AnimationRTCache::CheckIntegrity() const
{
  size_t total = 0;
  for (AnimationRT *a = _loaded.Start(); _loaded.NotEnd(a); a= _loaded.Advance(a))
  {
    total += a->GetMemoryControlled();
  }
  return _totalSize == total;
}

size_t AnimationRTCache::MemoryControlled() const
{
  Assert(CheckIntegrity());
  return _totalSize;
}

/*!
\patch 1.45 Date 2/18/2002 by Ondra
- Optimized: Memory usage for MC animations optimized.
Most MC Data loaded only when needed.
*/


//! max. number of animations held in cache
const int MaxAnimationRTItemCount = 8*1024;
//! max. number of animation matrices held in cache
const int MaxAnimationRTMatrixCount = 1024*1024;

AnimationRTCache::AnimationRTCache()
{
  _count = 0;
  _totalSize = 0;
  RegisterFreeOnDemandMemory(this);
}
AnimationRTCache::~AnimationRTCache()
{
  Assert(_count==_loaded.Size());
}

/*!
\patch_internal 1.59 Date 5/23/2002 by Ondra
- Fixed: Bug in MC data cache: Animation size was underestimated.
\patch_internal 1.59 Date 5/23/2002 by Ondra
- Fixed: Bug in MC data cache: Releasing animation sometimes accessed freed memory.
*/
void AnimationRTCache::Delete(AnimationRT *anim)
{
  Assert(CheckIntegrity());
  Assert(anim->IsInList());
  //Log("Del anim %s: %dx%d",(const char *)anim->GetName().name,anim->GetKeyframeCount(),anim->GetSkeleton()->NBones());
  _totalSize -= anim->GetMemoryControlled();
  _count--;
#if _DEBUG
  if (_count==0)
  {
    Assert(_totalSize==0);
  }
#endif
  anim->ReleaseLoadCount();
  _loaded.Delete(anim);
  Assert(CheckIntegrity());
}

void AnimationRTCache::Load(AnimationRT *anim)
{
  if (anim->IsInList())
  {
    // at this point the animation should already be loaded
    // verify, and increase ref count to make sure 
    anim->AddLoadCount();
    _loaded.Delete(anim);
    _loaded.Insert(anim);
    anim->ReleaseLoadCount();
  }
  else
  {
    Assert(CheckIntegrity());
    // check if there is some free space in animation cache
    if
    (
      _count>MaxAnimationRTItemCount ||
      _totalSize>MaxAnimationRTMatrixCount*sizeof(AnimationRTMatrix)
    )
    {
      if (_loaded.Last()) Delete(_loaded.Last());
    }
    
    // when added to list, increase LoadCount
    anim->AddLoadCount();
    _loaded.Insert(anim);

    //Log("Add anim %s: %dx%d",(const char *)anim->GetName().name,anim->GetKeyframeCount(),anim->GetSkeleton()->NBones());
    _totalSize += anim->GetMemoryControlled();
    _count++;
    Assert(_count==_loaded.Size());

    Assert(CheckIntegrity());
  }
}

static AnimationRTCache GAnimationRTCache;

#if ANIMATION_RT_SLIST

void AnimationRTCache::PerformLoading()
{
  // process all items of _loadRequested
  while (Ref<AnimationRT> get = _loadRequested.Get())
  {
    // mark as not in the SList
    get->RemoveFromSList();
    // load, place in the cache
    if (get->AddLoadCount() > 0)
    {
      // but handle load count correctly
      get->ReleaseLoadCount();
    }
  }
  _loadRequested.Clear();
}

void AnimationRTCache::CleanUp()
{
  // process all items of _loadRequested
  while (Ref<AnimationRT> get = _loadRequested.Get())
  {
    // mark as not in the SList
    get->RemoveFromSList();
  }
  _loadRequested.Clear();
}

void PerformAnimationRTLoading()
{
  GAnimationRTCache.PerformLoading();
}

void PerformAnimationRTCleanUp()
{
  GAnimationRTCache.CleanUp();
}

AnimationRTLock::AnimationRTLock(AnimationRT &lock)
: _anim(&lock)
{
  // add to the SLL once
  if (lock.AddIntoSList())
  {
    GAnimationRTCache._loadRequested.Add(_anim);
  }
}

#else
void PerformAnimationRTCleanUp()
{

}


#endif

/*!
\patch_internal 1.43 Date 2/1/2002 by Ondra
- Optimized: animation loading changed to be more suitable for delayed loading.
*/

#define LOG_ANIM_LOAD 0

bool AnimationRT::FullLoad(Skeleton *skelet, const char *file, bool sync)
{
  QIFStreamB in;
  in.AutoOpen(file);
  if (in.fail() || in.rest()<=0)
  {
    RptF("Animation %s not found or empty",file);
    return true;
  }
  if (!sync && !AnimationRTPhases::Preload(in, FileRequestPriority(10), true))
  {
    return false;
  }
  if (_version>=2)
  {
    // binary full load needed
    // we should skip all headers
    SerializeBinStream f(&in);
    f.SetVersion(_version);
    #ifdef _M_PPC
    f.SetEndianState(_endianState);
    #endif
    if (_lzoCompression) f.UseLZOCompression();

    // we may need to remove the looped frame
    int endAnim = _nPhases;
    if (GetLooped())
    {
      // remove all keyframes with time>1
      for (int i=0; i<_phaseTimes.Size(); i++)
      {
        if (_phaseTimes[i]>0.999 && i>0 )
        {
          endAnim = i;
          break;
        }
      }
    }

    _phasesBuffer.LoadPhasesBin(f, endAnim, _name.streamingEnabled,skelet,&_boneMap);
    if (_phasesBuffer.IsStreamed())
    {
      _stream = new QIFStreamB;
      _stream->AutoOpen(_name.name);
      if (_stream->fail() || _stream->rest() <= 0)
      {
        // if there is any failure, discard the stream, as it would be unusable
        _stream.Free();
      }
    
      LogF("Animation %s is streamed", cc_cast(file));
    }

//     if (_phasesBuffer.IsStreamed())
//     {
//       LogF("Animation %s is streamed", cc_cast(_name.name));
//     }
    return true;
  }

  #if LOG_ANIM_LOAD
  LogF("Animation %s fully loaded",file);
  #endif
  #ifdef _DEBUG
    char magic[RTMMagicLen+1];
    in.read(magic,RTMMagicLen);
    magic[RTMMagicLen]=0;
    Assert(!strcmp(magic,RTMMagic101));
  #else
    in.seekg(RTMMagicLen,QIOS::cur);
  #endif
  // skip stepX, stepY, stepZ
  in.seekg(sizeof(float)*3,QIOS::cur);
  // some selections may be skipped  - memory may contain different data then the file
  int nAnim = in.getil();
  int nSel = in.getil();
  Assert(_nPhases==nAnim);
  (void)nAnim;
  // skip all selection names
  in.seekg(32*nSel,QIOS::cur);

  Assert(_nPhases >= 1);
  Assert(_phaseTimes.Size() == 1);
  // very short animations should be preloaded
  LoadPhases(in,skelet,nSel,1,_nPhases);
  RemoveLoopFrame();
  return true;
}

AnimationRT::~AnimationRT()
{
  if (IsInList())
  {
    GAnimationRTCache.Delete(this);
  }
}

void AnimationRT::FullRelease()
{
  #if LOG_ANIM_LOAD
  LogF("Animation %s released",(const char *)_name.name);
  #endif

  // when discarding non-binarized animations, we need to handle times array as needed
  if (_version<2 && _phaseTimes.Size() > 1)
  {
    _phaseTimes.Resize(1);
    _phaseTimes.Realloc(1);
  }
  _phasesBuffer.Release();
}

double AnimationRT::GetMemoryUsed() const
{
  return _phasesBuffer.GetMemoryUsed() + _phaseTimes.GetMemoryUsed() + _selections.GetMemoryUsed();
}

void AnimationRT::AddPreloadCount()
{
  // small animations are always persistant - we cannot change it
  if (_nPhases<=PersistantLimit) return;
  _preloadCount++;
  #if LOG_ANIM_LOAD>=2
    LogF("Animation %s preloaded - AddPreloadCount()",Name());
  #endif
  // caution: we need to make sure LoadCount is successful
  AddLoadCount(true);
}

void AnimationRT::ReleasePreloadCount()
{
  if (_nPhases<=PersistantLimit) return;
  _preloadCount--;
  #if LOG_ANIM_LOAD
    LogF("Animation %s unpreloaded - ReleasePreloadCount()",Name());
  #endif
  ReleaseLoadCount();
}

bool AnimationRT::Preload(FileRequestPriority priority, const char *name, bool fullLoad)
{
  QIFStreamB in;
  in.AutoOpen(name);
  return AnimationRTPhases::Preload(in, priority, fullLoad);
}

void AnimationRT::LoadNeededPhases(AnimationRTBufferCopyOnWrite &buffer, float time, float timePastNeeded, const Skeleton *sk) const
{
  if (_phasesBuffer.IsStreamed() && _stream)
  {
    Assert(_version >= 2);

    SerializeBinStream f(_stream);
    f.SetVersion(_version);
#ifdef _M_PPC
    f.SetEndianState(_endianState);
#endif
    if (_lzoCompression) f.UseLZOCompression();

    int phase = FindNearest(time);
    int phasePastNeeded = FindNearest(timePastNeeded);

    // TODO: pass AnimationRTBufferCopyOnWrite into LoadNeededPhases
    _phasesBuffer.LoadNeededPhases(f, buffer, phase, phasePastNeeded, sk,&_boneMap);
  }
}

void AnimationRT::LoadPhases(
  QIStream &in, Skeleton *skelet, int nSel, int startAnim, int endAnim
)
{
  _phasesBuffer.LoadPhases(in, skelet, nSel, startAnim, endAnim, _boneMap, _reversed, _phaseTimes);
}

int AnimationRT::Match(QIStream &in, Skeleton *skelet,const char *name)
{
  // if there is any bone in the animation not present in the skeleton, we should report a non-match?
  char magic[RTMMagicLen+1];
  in.read(magic,RTMMagicLen);
  magic[RTMMagicLen]=0;
  if( !strcmp(magic,RTMMagic100) )
  {
    float stepZ=0;
    in.read((char *)&stepZ,sizeof(stepZ));
    RptF("Old format used in animation %s",name);
  }
  else if( !strcmp(magic,RTMMagic101) )
  {
    float stepX=0, stepY=0, stepZ=0;
    in.read((char *)&stepX,sizeof(stepX));
    in.read((char *)&stepY,sizeof(stepY));
    in.read((char *)&stepZ,sizeof(stepZ));
  }
  else
  {
    return -1;
  }
  int nAnim = in.getil();
  int nSel = in.getil();
  (void)nAnim;
  int matched = 0;
  // scan all selection and update weights as necessary
  for( int i=0; i<nSel; i++ )
  {
    char name[32];
    in.read(name,sizeof(name));
    // avoid buffer overrun
    name[sizeof(name) - 1] = 0;
    strlwr(name);
    SkeletonIndex bone = skelet->FindBone(name);
    if (SkeletonIndexValid(bone))
    {
      matched++;
    }
    else
    {
      // currently there are some bones which do not match - this seems to be normal
    }
  }
  return matched;
}

void AnimationRT::Load(QIStream &in, Skeleton *skelet,const char *name, bool reversed)
{
  REPORTSTACKITEM(GetName().name);
  
  _loadCount = 0;
  _looped = LoopedNotInit;
  _reversed = reversed;
  _step = VZero;
  _phasesBuffer.Init();
  char magic[RTMMagicLen+1];
  int startFile = in.tellg();
  in.read(magic,RTMMagicLen);
  magic[RTMMagicLen]=0;
  bool streaming = true;
  if( !strcmp(magic,RTMMagic100) )
  {
    float stepZ=0;
    in.read((char *)&stepZ,sizeof(stepZ));
    SerializeBinStream::ProcessLittleEndian32b(&stepZ,sizeof(stepZ));
    _step=Vector3(0,0,stepZ);
    RptF("Old format used in animation %s",name);
    streaming = false;
  }
  else if( !strcmp(magic,RTMMagic101) )
  {
    float stepX=0, stepY=0, stepZ=0;
    in.read((char *)&stepX,sizeof(stepX));
    in.read((char *)&stepY,sizeof(stepY));
    in.read((char *)&stepZ,sizeof(stepZ));
    SerializeBinStream::ProcessLittleEndian32b(&stepX,sizeof(stepX));
    SerializeBinStream::ProcessLittleEndian32b(&stepY,sizeof(stepY));
    SerializeBinStream::ProcessLittleEndian32b(&stepZ,sizeof(stepZ));
    _step=Vector3(stepX,stepY,stepZ);
  }
  else
  {
    in.seekg(startFile,QIOS::beg);
    if (LoadOptimized(skelet,in))
    {
      return;
    }
    _nPhases = 0;
    ErrorMessage("Bad animation file format in file '%s'.",name);
    return;
  }
  int nAnim = in.getil();
  int nSel = in.getil();
  _nPhases = nAnim;
  _boneMap.Realloc(skelet->NBones());
  _boneMap.Resize(skelet->NBones());
  #if MATRICES_ALL_BONES
    // prototype: create 1:1 mapping for all bones in the skeleton
    for (int i=0; i<_boneMap.Size(); i++) _boneMap[i] = i;
  #else
    // unless a bone is found, assume it is not touched by this animation
    for (int i=0; i<_boneMap.Size(); i++) _boneMap[i] = -1;
    // store only bones which are used
    int mapIndex = 0;
  #endif
  // scan all selection and update weights as necessary
  for( int i=0; i<nSel; i++ )
  {
    char name[32];
    in.read(name,sizeof(name));
    // avoid buffer overrun
    name[sizeof(name) - 1] = 0;
    strlwr(name);
    // when animation is using a bone not defined by the skeleton, we ignore it in
    SkeletonIndex bone = skelet->NewBone(name);
    #if MATRICES_ALL_BONES
    _selections.Add(name);
    #else
    if (SkeletonIndexValid(bone))
    {
      _boneMap[GetSkeletonIndex(bone)] = mapIndex++;
      _selections.Add(name);
    }
    #endif
  }

  if (_preloadCount>0 || nAnim<=PersistantLimit || !streaming)
  {
    // very short animations can always be preloaded
    LoadPhases(in,skelet,nSel,0,nAnim);

    // mark it as it load count in increased
    _loadCount = 1;
    #if 0 // LOG_ANIM_LOAD
      LogF("Animation %s preloaded %s",Name(),_preloadCount>0 ? "(Explicit)" :"");
    #endif
    if (_preloadCount==0) _preloadCount++;
  }
  else
  {
    LoadPhases(in,skelet,nSel,0,1);
  }
  // note: we do not use some bones from the rtm file at all, as they are not present in the skeleton
    
}

int AnimationRT::AddLoadCount(bool sync) const
{
  Assert(_loadCount>=0);
  if (_loadCount++!=0) return _loadCount;
  // was zero - we have to load it
  if (!unconst_cast(this)->FullLoad(_name.skeleton,_name.name,sync))
  {
    // when failed, do not keep loaded
    _loadCount--;
    Assert(_loadCount==0);
  }
  else
  {
    GAnimationRTCache.Load(unconst_cast(this));
  }
  return _loadCount;
}
void AnimationRT::ReleaseLoadCount() const
{
  Assert(_loadCount>=1);
  if (--_loadCount!=0) return;
  // is now zero - we have to release it
  unconst_cast(this)->FullRelease();
}

void AnimationRT::Load(Skeleton *skelet, const char *file, bool reversed)
{
  QIFStreamB in;
  in.AutoOpen(file);
  if (in.fail() || in.rest()<=0)
  {
    RptF("Animation %s not found or empty",file);
    // create an empty (all identities) animation
    _phaseTimes.Realloc(1);
    _phaseTimes.Resize(1);
    _phaseTimes[0] = 0;

    _phasesBuffer.InitIdentity(skelet);
    // we have to initialize bones
    _step = VZero;
    _looped = LoopedFalse;
    _nPhases = 1;
    _loadCount = 0;

    return;
  }
  //_name=file;
  Load(in,skelet,file,reversed);
}

void AnimationRT::CombineTransform(
  LODShape *lShape, int level,
  Matrix4Array &matrices, Matrix4Par trans, const BlendAnimInfo *blend, int nBlend
)
{
  // Get the shape and make sure it already exists
  DoAssert(lShape->IsLevelLocked(level));
  const Shape *shape = lShape->GetLevelLocked(level);

  // transformation is given in actual coordinates
  // convert it to original (bounding center) coordinates
  for (int i=0; i<nBlend; i++)
  {
    const BlendAnimInfo &info = blend[i];
    if (info.matrixIndex>=lShape->GetSkeleton()->NBones())
    {
      RptF("Bad skeleton referenced in %s",cc_cast(lShape->Name()));
      continue;
    }
    // convert to matrix index
    const SubSkeletonIndexSet &ssis = shape->GetSubSkeletonIndexFromSkeletonIndex(info.matrixIndex);
    if (ssis.Size() > 0)
    {
      // animation present - blend it
      Matrix4 &mt = matrices[GetSubSkeletonIndex(ssis[0])];
      float factor = info.factor;
      // interpolate matrix between trans and identity
      // factor is quite often 1;
      if (factor>0.99)
      {
        mt = trans * mt;
      }
      else
      {
        //Matrix4 ipol = trans*factor+Matrix4(MScale,1-factor);
        //mt = ipol * mt;

        // one matrix multiplication
        mt = trans*mt*factor+mt*(1-factor);
      }
      REPLICATE_FIRST_BONE(matrices, ssis);
    }
  }

  //mb*x = (mt*(x+bc)-bc)
  //mb*x = T-bc*mt*Tbc*x
}

void AnimationRT::TransformPoint(
  Vector3 &pos, const Shape *shape,
  Matrix4Par trans,
  const BlendAnimInfo *blend, int nBlend, int index
)
{
  // note: no actual data from animation are needed
  // check which selection are we in
  if (index<0) return;

  Assert(shape->IsLoadLocked());
  if (shape->GetVertexBoneRef().Size() <= index)
  {
    //Fail("Error: Accessing undefined vertex bone references");
    return;
  }
  const AnimationRTWeight &wg = shape->GetVertexBoneRef()[index];
  if( wg.Size()<=0 ) return; // this point is not animated
  
  Vector3 sPos = pos;

  pos = VZero;
  for (int i=0; i<wg.Size(); i++)
  {
    const AnimationRTPair &pair = wg[i];
    SubSkeletonIndex matIndex = pair.GetSel(); // note: sel is matrix index
    SkeletonIndex oMatIndex = shape->GetSkeletonIndexFromSubSkeletonIndex(matIndex);
    float ww = pair.GetWeight();
    // check which selection corresponds to this matrix   
    // we need selection index
    int blendIndex = -1;
    for ( int bi = 0; bi<nBlend; bi++)
    {
      if (oMatIndex==blend[bi].matrixIndex) {blendIndex=bi;break;}
    }
    if (blendIndex<0)
    {
      // matIndex matrix not affected - factor 0
      pos += sPos*ww;
      continue;
    }

    float factor=blend[blendIndex].factor;
    // animation present - blend it
    // interpolate matrix between trans and identity
    Vector3 tPos = trans.FastTransform(sPos);
    // blend multiple matrices from wg

    pos += tPos*(factor*ww) + sPos*((1-factor)*ww);
    //LogF("  source %.3f,%.3f,%.3f",pos[0],pos[1],pos[2]);
    //LogF("    tpos %.3f,%.3f,%.3f",tPos[0],tPos[1],tPos[2]);
    //LogF("    res  %.3f,%.3f,%.3f",pos[0],pos[1],pos[2]);
  }

}

void AnimationRT::RemoveLoopFrame()
{
  if (GetLooped() && _loadCount>0)
  {
    // remove keyframe with time>1
    for( int i=0; i<_phaseTimes.Size(); i++ )
    {
      if (_phaseTimes[i]>0.999 && i>0 )
      {
        _phaseTimes.Resize(i);
        _phasesBuffer.LimitCount(i);
        break;
      }
    }
  }
}

bool AnimationRT::CheckStatic() const
{
  if (_phaseTimes.Size()<=1) return true;
  if (_phasesBuffer.IsStreamed()) return false;
  if (_loadCount<=0) return false; // if the animation is long, assume it is not static
  AnimationRTPhase *first = _phasesBuffer._firstPhase;
  for (int i=0; i<_phasesBuffer._buffer._phases.Size(); i++)
  {
    AnimationRTPhase *phase = _phasesBuffer._buffer._phases[i];
    for (int b=0; i<first->_local.Size(); i++)
    {
      if (first->_local[b]!=phase->_local[b]) return false;
    }
  }
  return true;
}

void AnimationRT::SetLooped( bool looped )
{
  if (_looped!=LoopedNotInit && (_looped==LoopedTrue)==looped) return;
  _looped = looped ? LoopedTrue : LoopedFalse;
  if (_nPhases==2 && _step.SquareSize()>Square(0.01f))
  {
    LogF("%s: Strange looped movement, 2 phases, step %g,%g,%g",cc_cast(_name.name),_step.X(),_step.Y(),_step.Z());
  }
  RemoveLoopFrame();
}

void Skeleton::PrepareProxyBones(LODShape *lodShape, Shape *shape) const
{
  // scan all proxies and set weights for them
  for (int p=0; p<shape->NProxies(); p++)
  {
    // get weights for this particular point
    ProxyObject &proxy = shape->Proxy(p);
    proxy.boneIndex = SkeletonIndexNone;
    const NamedSelection::Access &proxySel = shape->NamedSel(proxy.selection,lodShape);

    for(int i = NBones() - 1; i >= 0; i--)
    {
      SkeletonIndex oi = SetSkeletonIndex(i);
      RStringB name = GetBone(oi);
      int selIndex = lodShape->FindNamedSel(shape,name);
      // we need to check which selection is the proxy selection part of
      // TODO: avoid scanning all bone selections here, use proxy vertex binding instead
      const NamedSelection::Access boneSel = shape->NamedSel(selIndex,lodShape);
      if (selIndex >= 0 && proxySel.IsSubset(boneSel))
      {
        if (GetSkeletonIndex(proxy.boneIndex) >= 0)
        {
          // for discrete skeleton first encountered (highest index) bone is more important
          if (!_isDiscrete)
          {
            Fail("No weigting supported");
          }
        }
        else
        {
          proxy.boneIndex = oi;
        }
      }
    }

  }
}

void Skeleton::FinishProxyBones(LODShape *lodShape, Shape *shape) const
{
  // scan all proxies and reset weights
  for (int p=0; p<shape->NProxies(); p++)
  {
    // release weights for this particular point
    ProxyObject &proxy = shape->Proxy(p);
    proxy.boneIndex = SkeletonIndexNone;
  }
}

/*!
For compatibility only.
Use Skeleton::PrepareSkeleton and AnimationRT::SetLooped instead.
*/

void AnimationRT::Prepare(bool looped)
{
  SetLooped(looped);
}

int AnimationRT::GetBonesAffected() const
{
  return _phasesBuffer.NAnimatedBones();
}

int AnimationRT::GetMemoryControlled() const
{
  // what memory is allocated/freed by FullLoad/FullRelease
  return _phasesBuffer.GetMemoryControlled();
}

void AnimationRT::Find( int &nextIndex, int &prevIndex, float time ) const
{
  DoAssert(_phasesBuffer.Initialized());

  // works only with the _phaseTimes array
  int nPhases = _phaseTimes.Size();

  //find the frame by binary division
  int nIndex = 0;
  int nLeft = 0;
  int nRight = nPhases - 1;
  while(true)
  {
    if(nLeft > nRight)
    {
      //we are over
      nIndex = nLeft;
      break;
    }
    nIndex = (nLeft + nRight) >> 1;
    if(_phaseTimes[nIndex] <= time)
    {
      //higher part
      nLeft = nIndex + 1;
    }
    else
    {
      //not necessary. The overflow condition works as well, anyway this is more intuitive
      //if(nIndex == 0 || _phaseTimes[nIndex - 1] <= time)
      //break;

      //lower part
      nRight = nIndex - 1;
    }
  }

  int pIndex=nIndex-1;
  if( pIndex<0 ) pIndex=GetLooped() ? nPhases-1 : 0;
  if( nIndex>=nPhases ) nIndex=GetLooped() ? 0 : nPhases-1;
  Assert( pIndex>=0 );
  nextIndex=nIndex;
  prevIndex=pIndex;
}

int AnimationRT::FindNearest(float time) const
{
  int nextIndex = -1;
  int prevIndex = -1;
  Find(nextIndex, prevIndex, time);
  if (fabs(_phaseTimes[nextIndex] - time) < fabs(time - _phaseTimes[prevIndex]))
  {
    return nextIndex;
  }
  else
  {
    return prevIndex;
  }
}

Vector3 AnimationRT::ApplyMatricesPoint(
  AnimationContext &animContext, LODShape *lShape, int level, const Matrix4Array &matrices, int pointIndex
)
{
  ShapeUsed shape=lShape->Level(level);
  if( shape.IsNull() ) return VZero;
  Assert(shape->IsLoadLocked());
  
  // apply transformations to current animation phase
  //int npos=shape->NPos();
  if (shape->GetVertexBoneRef().Size() == 0)
  {
    Fail("Error: Accessing undefined vertex bone references");
    return VZero;
  }
  const AnimationRTWeight &wg = shape->GetVertexBoneRef()[pointIndex];
  int wsize=wg.Size();
  // note: animation is calculated before BC centering

  Vector3Val pos=shape->Pos(pointIndex);

  // following loop certainly executes at least once
  // and typically once in simple LOD levels
  Vector3 res;
  if( wsize==1 )
  {
    const AnimationRTPair &pw=wg[0];
    // optimized for wg.Size==1
    Assert( fabs(pw.GetWeight()-1)<1e-6 );
    res=matrices[GetSubSkeletonIndex(pw.GetSel())]*pos;
  }
  else if( wsize<=0 )
  {
    res=pos;
  }
  else
  {
    const AnimationRTPair &pw=wg[0];
    // do compromises between weighted transformations
    res=matrices[GetSubSkeletonIndex(pw.GetSel())]*pos*pw.GetWeight();
    for( int w=1; w<wsize; w++ )
    {
      const AnimationRTPair &pw=wg[w];
      res+=matrices[GetSubSkeletonIndex(pw.GetSel())]*pos*pw.GetWeight();
    }
  }
  return res;
}


void AnimationRT::ApplyMatricesComplex(
  AnimationContext &animContext, LODShape *lShape, int level,
  const Matrix4Array &matrices
)
{
  ShapeUsed shape=lShape->Level(level);
  if( shape.IsNull() ) return;
  Assert(shape->IsLoadLocked());

  if (shape->GetVertexBoneRef().Size() == 0)
  {
    Fail("Error: Accessing undefined vertex bone references");
    return;
  }

  // when animating for collisions/physics, there is no need to animate normals
  bool animNormals = level != lShape->FindGeometryLevel() &&
    level != lShape->FindFireGeometryLevel() &&
    level != lShape->FindViewGeometryLevel() &&
    lShape->LevelUsesVBuffer(level);

  // apply transformations to current animation phase
  int npos=shape->NVertex();
  if (!animNormals) for( int i=0; i<npos; i++ )
  {
    const AnimationRTWeight &wg = shape->GetVertexBoneRef()[i];
    int wsize=wg.Size();
    // note: animation is calculated before BC centering

    Vector3Val pos=shape->Pos(i);
    #if 0 // _KNI
    _mm_prefetch((char *)(&shape->OrigPos(i)+2),_MM_HINT_NTA);
    _mm_prefetch((char *)(&shape->OrigNorm(i)+2),_MM_HINT_NTA);
    #endif

    // following loop certainly executes at least once
    // case wsize==1 is unlikely, as this is most often handled by ApplyMatricesSimple
    Vector3 res;
    if( wsize>0 )
    {
      const AnimationRTPair &pw=wg[0];
      // do compromises between weighted transformations
      Matrix4Val mat = matrices[GetSubSkeletonIndex(pw.GetSel())];
      res.SetMultiply(mat,pos);
      float pww = pw.GetWeight();
      res *= pww;
      for( int w=1; w<wsize; w++ )
      {
        const AnimationRTPair &pw=wg[w];
        Matrix4Val mat = matrices[GetSubSkeletonIndex(pw.GetSel())];
        Vector3 add;
        add.SetMultiply(mat,pos);
        float pww = pw.GetWeight();
        add *= pww;
        res += add;
      }
    }
    else 
    { // wsize ==0 
      res = pos;
    }


    animContext.SetPos(shape, i) = res;
  }
  else for( int i=0; i<npos; i++ )
  {
    const AnimationRTWeight &wg = shape->GetVertexBoneRef()[i];
    int wsize=wg.Size();
    // note: animation is calculated before BC centering

    Vector3Val pos=shape->Pos(i);
    Vector3 norm = shape->Norm(i).Get();
    #if 0 // _KNI
    _mm_prefetch((char *)(&shape->OrigPos(i)+2),_MM_HINT_NTA);
    _mm_prefetch((char *)(&shape->OrigNorm(i)+2),_MM_HINT_NTA);
    #endif

    // following loop certainly executes at least once
    // case wsize==1 is unlikely, as this is most often handled by ApplyMatricesSimple
    Vector3 res,resNorm;
    if( wsize>0 )
    {
      const AnimationRTPair &pw=wg[0];
      // do compromises between weighted transformations
      Matrix4Val mat = matrices[GetSubSkeletonIndex(pw.GetSel())];
      res.SetMultiply(mat,pos);
      resNorm.SetMultiply(mat.Orientation(),norm);
      float pww = pw.GetWeight();
      res *= pww;
      resNorm *= pww;
      for( int w=1; w<wsize; w++ )
      {
        const AnimationRTPair &pw=wg[w];
        Matrix4Val mat = matrices[GetSubSkeletonIndex(pw.GetSel())];
        Vector3 add,addNorm;
        add.SetMultiply(mat,pos);
        addNorm.SetMultiply(mat.Orientation(),norm);
        float pww = pw.GetWeight();
        add *= pww;
        addNorm *= pww;
        res += add;
        resNorm += addNorm;
      }
    }
    else 
    { // wsize ==0 
      res = pos;
      resNorm = norm;
    }


    animContext.SetPos(shape, i) = res;
    animContext.SetNorm(shape, i).Set(resNorm);
  }
  animContext.InvalidateNormals(shape);
  animContext.InvalidateConvexComponents();
}

void AnimationRT::ApplyMatricesIdentity(AnimationContext &animContext, LODShape *lShape, int level)
{
  // nothing to do, the shape is not changing now
}

void AnimationRT::ApplyMatricesSimple(
  AnimationContext &animContext, LODShape *lShape, int level, const Matrix4Array &matrices
)
{
  ShapeUsed shape=lShape->Level(level);
  if( shape.IsNull() ) return;
  Assert(shape->IsLoadLocked());

  // when animating for collisions/physics, there is no need to animate normals
  bool animNormals = level != lShape->FindGeometryLevel() &&
    level != lShape->FindFireGeometryLevel() &&
    level != lShape->FindViewGeometryLevel() &&
    lShape->LevelUsesVBuffer(level);

  // check if any matrix significantly differs from identity
  bool identity = true;
  
  for (int m=0; m<matrices.Size(); m++)
  {
    float diff2 = matrices[m].Distance2(MIdentity);
    if (diff2>Square(1e-5f))
    {
      identity = false;
      break;
    }
  }
  if (identity)
  {
    // if all matrices are identities, we can use the original shape data
    return;
  }
  // apply transformations to current animation phasis
  int npos=shape->NVertex();
  const AnimationRTWeight *a = shape->GetVertexBoneRef().Data();
  // for non-graphical lods normals do not matter
  if (!animNormals) for( int i=0; i<npos; i++ )
  {
    if (a->Size()>0)
    {
      int pwsel=GetSubSkeletonIndex((*a)[0].GetSel());
      Vector3Val pos=shape->Pos(i);
      Matrix4Val val = matrices[pwsel];
      animContext.SetPos(shape, i) = val * pos;
    }
    else
    {
      animContext.SetPos(shape, i) = shape->Pos(i);
    }
    a++;
  }
  else for( int i=0; i<npos; i++ )
  {
    if (a->Size()>0)
    {
      int pwsel=GetSubSkeletonIndex((*a)[0].GetSel());
      Vector3Val pos=shape->Pos(i);
      Vector3 norm = shape->Norm(i).Get();
      Matrix4Val val = matrices[pwsel];
      animContext.SetPos(shape, i) = val * pos;
      animContext.SetNorm(shape, i).Set(val.Orientation() * norm);
    }
    else
    {
      animContext.SetPos(shape, i) = shape->Pos(i);
      animContext.SetNorm(shape, i) = shape->Norm(i);
    }
    a++;
  }
  animContext.InvalidateNormals(shape);
  animContext.InvalidateConvexComponents();
}


void AnimationRT::ApplyMatrices(
  AnimationContext &animContext, LODShape *lShape, int level, const Matrix4Array &matrices
)
{
  if( matrices.Size()<=0 )
  {
    // restore original shape position
    ApplyMatricesIdentity(animContext, lShape, level);
    return;
  }
  ShapeUsed shape = lShape->Level(level);
  
  if (shape->VertexBoneRefIsSimple())
  {
    ApplyMatricesSimple(animContext, lShape, level, matrices);
  }
  else
  {
    ApplyMatricesComplex(animContext, lShape, level, matrices);
  }
}

void AnimationRT::Apply
(
  AnimationContext &animContext, LODShape *lShape, int level, float time
) const
{
  if (_nPhases<=0) return; // not animated

  ShapeUsed shape=lShape->Level(level);
  if( shape.IsNull() ) return;

  MATRIX_4_ARRAY(matrix,128);
  PrepareMatrices(matrix, time, 1, shape,lShape->GetSkeleton());
  // make sure matrices are prepared for all skeleton bones
  int oldSize = matrix.Size();

  if (lShape->GetSkeleton())
  {
    int newSize = shape->GetSubSkeletonSize();
    if (oldSize<newSize)
    {
      Fail("We shouldn't reach this point");
      matrix.Resize(newSize);
      for (int i=oldSize; i<newSize; i++)
      {
        matrix[i] = MIdentity;
      }
    }
  }
  else
  {
    RptF("Error: Skeleton not available in model %s", cc_cast(lShape->GetName()));
  }

  ApplyMatrices(animContext, lShape,level,matrix);
}


/**
Note: obtaining multiple matrices one by one is inefficient, as common parents need to be evaluated multiple times.
*/
Matrix4 AnimationRT::GetMatrix(float time, SkeletonIndex si) const
{
  if (_nPhases<=0) return MIdentity; // not animated

  Matrix4 matrix = MIdentity;
  Matrix(matrix,time,si);
  return matrix;
}

static __forceinline Matrix4 operator * (Matrix4Par a, const AnimationRTMatrix &b)
{
  return a*Matrix4(b);
}

struct AnimateSetMul
{
  const Array<AnimationRTMatrix> &_phase;
  const Array<int> &_boneMap;

  AnimateSetMul(const Array<AnimationRTMatrix> &phase, const Array<int> &boneMap)
  :_phase(phase),_boneMap(boneMap)
  {}
  
  void operator () (Matrix4 &tgt, SkeletonIndex si, const Skeleton *sk, Matrix4Val parent) const
  {
    // _phase indexed via MapBone
    int index = _boneMap[GetSkeletonIndex(si)];
    if (index>=0)
    {
      tgt.SetMultiply(parent,_phase[index]);
    }
  }
};

struct AnimateBlendAnim
{
  const BlendAnims &_blend;

  AnimateBlendAnim(const BlendAnims &blend)
  :_blend(blend)
  {}
  
  void operator () (Matrix4 &tgt, SkeletonIndex si, const Skeleton *sk, Matrix4Val parent) const
  {
    Matrix4 anim;
    _blend.GetAnim(anim,si,sk);
    tgt.SetMultiply(parent,anim);
  }
};

/// temp is needed in case ssis is empty - we still need a space to perform animation
static inline Matrix4 &FirstBone(Matrix4Array &matrix, const SubSkeletonIndexSet &ssis, Matrix4Val anim, Matrix4 &temp)
{
  if (ssis.Size()<=0) return temp;
  return matrix[GetSubSkeletonIndex(ssis[0])];
  
}

static inline void ReplicateBone(Matrix4Array &matrix, const SubSkeletonIndexSet &ssis, Matrix4Val anim)
{
  // blending is the same for all matrices
  // actually we could do the replication much later, after the blending is completed
  // it is however unsure how much it would be faster
  for (int i = 1; i < ssis.Size(); i++)
  {
    matrix[GetSubSkeletonIndex(ssis[i])] = anim;
  }
}

/// functor for Blend in AnimateBone
struct BlendSet
{
  void operator() (Matrix4 &tgt, Matrix4Par src) const {tgt=src;}
};

struct AnimateBoneContext
{
  Matrix4 _temp;
  const Matrix4 &_parent;
  
  explicit AnimateBoneContext(const Matrix4 &parent)
  :_parent(parent),_temp(MIdentity)
  {
  }
  
};

template <class Animate>
struct AnimateBone
{
  Matrix4Array &_matrix;
  const Shape *_shape;
  const Animate &_animate;
  const Skeleton *_skeleton;
  
  AnimateBone(Matrix4Array &matrix, const Shape *shape, const Animate &animate, const Skeleton *skeleton)
  :_matrix(matrix),_shape(shape),_animate(animate),_skeleton(skeleton)
  {
  }
  
  AnimateBoneContext operator () (int bone, AnimateBoneContext &ctx) const
  {
    if (bone<=0) return ctx;
    SkeletonIndex si = _shape->GetSubSkeletonTreeBone(bone-1);
    const SubSkeletonIndexSet &ssis = _shape->GetSubSkeletonIndexFromSkeletonIndex(si);
    if (SkeletonIndexValid(si))
    {
      // ssis may be empty - there may be no bone, only needed by inheritance
      // caution: we must not return a reference to temp here!
      Matrix4 &anim = FirstBone(_matrix,ssis,ctx._parent,ctx._temp);
      _animate(anim,si,_skeleton,ctx._parent);
      ReplicateBone(_matrix,ssis,anim);
      return AnimateBoneContext(anim);
    }
    return ctx;
  
  }
};

const AnimationRTPhase &AnimationRT::GetFrame(int i) const
{
  Assert(i >= 0);
  if (i == 0)
    // first keyframe is stored specially
    return *_phasesBuffer._firstPhase;
  // following keyframes (if any) are stored in a buffer
  i--;
  Assert(i < _phasesBuffer._buffer._phases.Size())
  return *_phasesBuffer._buffer._phases[i];
}

Matrix4 AnimationRT::GetHeadBobCorrection(HeadBobModeEnum mode, float time, const AnimationRTLock &lock, const Skeleton *skeleton, SkeletonIndex bone) const
{
  Matrix4 corr = MIdentity;
  if ((mode != HeadBobBeginEnd) && (mode != HeadBobAverageFrame))
    // un-correctable mode
    return corr;
  if (!_headBobCorrection)
  { // no correction matrices yet, create them (un-initialized)
    _headBobCorrection = new HeadBobCorrection;
    for (int i = 0; i < lenof(_headBobCorrection->_matrices); i++)
      _headBobCorrection->_matrices[i](0, 0) = FLT_SNAN;
  }
  if (mode == HeadBobBeginEnd)
  { // begin/end correction
    Matrix4Val beginCorr = GetHBCMatrix(0, lock, skeleton, bone);
    Matrix4Val endCorr = GetHBCMatrix(1, lock, skeleton, bone);
    if (ValidHBCMatrix(beginCorr) && ValidHBCMatrix(endCorr))
      corr = beginCorr * (1.0 - time) + endCorr * time;
  }
  else if (mode == HeadBobAverageFrame)
  { // average correction
    Matrix4Val averageCorr = GetHBCMatrix(2, lock, skeleton, bone);
    if (ValidHBCMatrix(averageCorr))
      corr = averageCorr;
  }
  else
    // just to be sure
    Assert(false);
  return corr;
}

Matrix4Val AnimationRT::GetHBCMatrix(int i, const AnimationRTLock &lock, const Skeleton *skeleton, SkeletonIndex bone) const
{
  if (!ValidHBCMatrix(_headBobCorrection->_matrices[i]))
  { // given matrix hasn't been computed yet, try that now
    if ((i != 0) && ((_loadCount <= 0) || _phasesBuffer.IsStreamed()))
    { // request for a non-zero frame in unloaded or streamed animation -> return the zeroth frame (which is always defined) instead
      return GetHBCMatrix(0, lock, skeleton, bone);
    }
    BlendAnims blend;
    if (i == 0)
    { // begin matrix
      blend._list.Add(AnimBlendInfo(lock, &GetFrame(0),NULL, 0.0,1.0, 0.0, NULL, 1.0));
    }
    else if (i == 1)
    { // end matrix
      // looped animations has the last frame equivalent to the first one by definition
      int j = GetLooped() ? 0 : GetFrameCount() - 1;
      blend._list.Add(AnimBlendInfo(lock, &GetFrame(j),NULL, 0.0,1.0, 0.0, NULL, 1.0));
    }
    else if (i == 2)
    { // average matrix
      int n = GetFrameCount();
      for (int j = 0; j < n; j++)
        blend._list.Add(AnimBlendInfo(lock, &GetFrame(j),NULL, 0.0,1.0 / n, 0.0, NULL, 1.0));
    }
    // compute the corresponding correction matrix
    Matrix4 corr = MIdentity;
    blend.DoBoneAnim(corr, skeleton, bone);
    corr.Orthogonalize();
    corr = corr.InverseGeneral();
    // and store it for future usage
    _headBobCorrection->_matrices[i] = corr;
  }
  return _headBobCorrection->_matrices[i];
}

void AnimationRT::PrepareMatrices(
  Matrix4Array &matrix, float time, float factor, const Shape *shape, const Skeleton *skeleton
) const
{
  if (_nPhases<=0) return; // not animated
  if (factor<0.01f)
  {
    if( matrix.Size()==0 )
    {
      int nMatrices = shape->GetSubSkeletonSize();
      matrix.Resize(nMatrices);
      for (int i=0; i<nMatrices; i++)
      {
        matrix[i] = MZero;
      }
    }
    return;
  }

  int nMatrices = shape->GetSubSkeletonSize();
  if (nMatrices == 0)
  {
    // no bones to animate in this LOD level
    return;	
  }

  AnimationRTLock load(unconst_cast(*this));

  BlendAnims blend;
  GetBlend(blend,time,factor);
  
  PrepareMatrices(matrix,blend,shape,skeleton);
}

void AnimationRT::PrepareMatrices(Matrix4Array &matrices, const BlendAnims &blend, const Shape *shape, const Skeleton *skeleton)
{
  int nMatrices = shape->GetSubSkeletonSize();
  // initialize the target storage
  // we cannot play RTM over another animation
  DoAssert(matrices.Size()==0);
  matrices.Resize(nMatrices);
  // prevent uninitialized values
  memset(matrices.Data(),0,matrices.Size()*sizeof(Matrix4Array::DataType));

  if (blend._list.Size()<=0)
  { // no animation - singular case
    matrices.Resize(nMatrices);
    for (int i=0; i<nMatrices; i++)
    {
      // use identity rather than zero - zero makes object invisible and causes computation singularities
      matrices[i] = MIdentity;
    }
  }
  /*else if (blend._list.Size()==1 && blend._masked.Size()==0)
  { // exactly one animation, no masked layer
    const AnimBlendInfo &info0 = blend._list[0];
    AnimateBoneContext ctx(Matrix4(MScale,blend._list[0]._factor));
    shape->GetSubSkeletonTree().ProcessCtx(
      AnimateBone<AnimateSetMul>(matrices,shape,AnimateSetMul(info0._phase->_local,info0._lock->_boneMap),skeleton),
      0,ctx
    );
    // when only one animation is blending, factor should be 1
    Assert(fabs(blend._list[0]._factor-1)<0.01f);

  }*/
  else
  {
    AnimateBoneContext ctx(MIdentity);
    shape->GetSubSkeletonTree().ProcessCtx(
      AnimateBone<AnimateBlendAnim>(matrices,shape,AnimateBlendAnim(blend),skeleton),0,ctx
    );
  }
}

# define XBOX_OPTIMIZATIONS 0

/**
Traverses all parents - not very efficient for multiple bones
*/
void BlendAnims::DoBoneAnim(Matrix4 &mat, const Skeleton *sk, SkeletonIndex boneIndex) const
{
  if (_list.Size()>0)
  {
#if XBOX_OPTIMIZATIONS
		XMMATRIX xmat = ToXMMatrix(mat);

    for(SkeletonIndex bone = boneIndex; SkeletonIndexValid(bone); bone = sk->GetParent(bone))
    {
			XMMATRIX ppMatrix = GetAnim(bone);
      xmat = XMMatrixMultiply3(ppMatrix, xmat);
    }
		FromXMMatrix(mat, xmat);
#else
    for(SkeletonIndex bone = boneIndex; SkeletonIndexValid(bone); bone = sk->GetParent(bone))
    {
      Matrix4 ppMatrix;
      GetAnim(ppMatrix,bone,sk);
      mat = ppMatrix*mat;
    }
#endif
  }
  else
  {
    // if there is no animation to blend, keep identity
    mat = MIdentity;
  }
}

void BlendAnims::DoBoneAnim(Matrix4 &mat, const Skeleton *sk, Shape *shape, const AnimationRTWeight &wgt) const
{
  if (_list.Size()>0)
  {
#if XBOX_OPTIMIZATIONS
    XMMATRIX ret;
		XMMATRIX xmat = ToXMMatrix(mat);
    // first iteration of the loop is optimized to avoid need to blend in it
    {
      XMMATRIX res = xmat;
      SkeletonIndex boneIndex = shape->GetSkeletonIndexFromSubSkeletonIndex(wgt[0].GetSel());
      for(SkeletonIndex bone = boneIndex; SkeletonIndexValid(bone); bone = sk->GetParent(bone))
      {
        XMMATRIX ppMatrix = GetAnim(bone);
				res = XMMatrixMultiply3(ppMatrix, res);
      }
			ret = XMMatrixMultiply(res, XMVectorSet1(wgt[0].GetWeight()));
    }
    // now perform the blending with the rest
    for (int i=1; i<wgt.Size(); i++)
    {
			XMMATRIX res = xmat;
      SkeletonIndex boneIndex = shape->GetSkeletonIndexFromSubSkeletonIndex(wgt[i].GetSel());
      for(SkeletonIndex bone = boneIndex; SkeletonIndexValid(bone); bone = sk->GetParent(bone))
      {
				XMMATRIX ppMatrix = GetAnim(bone);
				res = XMMatrixMultiply3(ppMatrix, res);
      }
			ret = XMMatrixMultiplyAdd(res, XMVectorSet1(wgt[i].GetWeight()), ret);
    }
		FromXMMatrix(mat, ret);
#else
		Matrix4 ret;
		// first iteration of the loop is optimized to avoid need to blend in it
		{
			Matrix4 res = mat;
			SkeletonIndex boneIndex = shape->GetSkeletonIndexFromSubSkeletonIndex(wgt[0].GetSel());
			for(SkeletonIndex bone = boneIndex; SkeletonIndexValid(bone); bone = sk->GetParent(bone))
			{
				Matrix4 ppMatrix;
        GetAnim(ppMatrix,bone,sk);
				res = ppMatrix*res;
			}
			ret = res*wgt[0].GetWeight();
		}
		// now perform the blending with the rest
		for (int i=1; i<wgt.Size(); i++)
		{
			Matrix4 res = mat;
			SkeletonIndex boneIndex = shape->GetSkeletonIndexFromSubSkeletonIndex(wgt[i].GetSel());
			for(SkeletonIndex bone = boneIndex; SkeletonIndexValid(bone); bone = sk->GetParent(bone))
			{
				Matrix4 ppMatrix;
        GetAnim(ppMatrix,bone,sk);
				res = ppMatrix*res;
			}
			ret += res*wgt[i].GetWeight();
		}
		mat = ret;
#endif
  }
  else
  {
    mat = MIdentity*wgt[0].GetWeight();
  }
}

void BlendAnims::DoPointAnim(Vector3 &pos, const Skeleton *sk, const Shape *shape, const AnimationRTWeight &wgt) const
{
  if (_list.Size()>0)
  {
#if XBOX_OPTIMIZATIONS
		XMVECTOR ret;
		XMVECTOR xpos = XMLoadFloat3((const XMFLOAT3*)&pos);
		// first iteration of the loop is optimized to avoid need to blend in it
		{
			XMVECTOR res = xpos;
			SkeletonIndex boneIndex = shape->GetSkeletonIndexFromSubSkeletonIndex(wgt[0].GetSel());
			for(SkeletonIndex bone = boneIndex; SkeletonIndexValid(bone); bone = sk->GetParent(bone))
			{
				XMMATRIX ppMatrix = GetAnim(bone);
				res = XMVector3Transform(res, ppMatrix);
			}
			ret = XMVectorMultiply(res, XMVectorSet1(wgt[0].GetWeight()));
		}
		// now perform the blending with the rest
		for (int i=1; i<wgt.Size(); i++)
		{
			XMVECTOR res = xpos;
			SkeletonIndex boneIndex = shape->GetSkeletonIndexFromSubSkeletonIndex(wgt[i].GetSel());
			for(SkeletonIndex bone = boneIndex; SkeletonIndexValid(bone); bone = sk->GetParent(bone))
			{
				XMMATRIX ppMatrix = GetAnim(bone);
				res = XMVector3Transform(res, ppMatrix);
			}
			ret = XMVectorMultiplyAdd(res, XMVectorSet1(wgt[i].GetWeight()), ret);
		}
		XMStoreFloat3((XMFLOAT3*)&pos, ret);
#else
		Vector3 ret;
    // first iteration of the loop is optimized to avoid need to blend in it
    {
      Vector3 res = pos;
      SkeletonIndex boneIndex = shape->GetSkeletonIndexFromSubSkeletonIndex(wgt[0].GetSel());
      for(SkeletonIndex bone = boneIndex; SkeletonIndexValid(bone); bone = sk->GetParent(bone))
      {
        Matrix4 ppMatrix;
        GetAnim(ppMatrix,bone,sk);
        res = ppMatrix.FastTransform(res);
      }
      ret = res*wgt[0].GetWeight();
    }
    // now perform the blending with the rest
    for (int i=1; i<wgt.Size(); i++)
    {
      Vector3 res = pos;
      SkeletonIndex boneIndex = shape->GetSkeletonIndexFromSubSkeletonIndex(wgt[i].GetSel());
      for(SkeletonIndex bone = boneIndex; SkeletonIndexValid(bone); bone = sk->GetParent(bone))
      {
        Matrix4 ppMatrix;
        GetAnim(ppMatrix,bone,sk);
        res = ppMatrix.FastTransform(res);
      }
      ret += res*wgt[i].GetWeight();
    }
    pos = ret;
#endif
  }
  else
  {
    // if there is no animation, keep the point in its original position
    //pos = VZero;
  }
}

#if !XBOX_OPTIMIZATIONS

void BlendAnims::InterpolateMatrix( Matrix4 &mat, SkeletonIndex bone, const TAnimBlendInfoArray &list,float s) const
{
  {
    const AnimBlendInfo &info = list[0];
    mat.InlineSetMultiply(*info.GetBoneP(bone),(1-info._p_factor)*info._b_factor*s);
    if (info.GetBoneN(bone))
      mat.InlineAddMultiply(*info.GetBoneN(bone),info._p_factor*info._b_factor*s);
  }
  for (int i=1; i<list.Size(); i++)
  {
    const AnimBlendInfo &info = list[i];
    mat.InlineAddMultiply(*info.GetBoneP(bone),(1-info._p_factor)*info._b_factor*s);
    if (info.GetBoneN(bone))
      mat.InlineAddMultiply(*info.GetBoneN(bone),info._p_factor*info._b_factor*s);
    Assert(info._mask==list[0]._mask);
  }
}

void BlendAnims::InterpolateQuaternion( Quaternion<float> &q, Vector3 &pos,const Vector3 &pivot,SkeletonIndex bone, const TAnimBlendInfoArray &list) const
{
  const AnimationRTMatrix *rtmat1;
  const AnimationRTMatrix *rtmat2;
  Quaternion<float> q1,q2;
  Vector3 pos1,pos2;
  for(int i=0;i<list.Size();i++)
  {
    rtmat1=list[i].GetBoneP(bone);

    q1=rtmat1->GetOrientQ();
    pos1=rtmat1->Position();
    
    rtmat2=list[i].GetBoneN(bone);
    if (rtmat2)
    {
      q2=rtmat2->GetOrientQ();
      pos2=rtmat2->Position();

      q1=q1.Slerp(q2,list[i]._p_factor);
      pos1=pos1*(1-list[i]._p_factor)+pos2*list[i]._p_factor;
  }
    if (i)
    {
      q=q.Slerp(q1,list[i]._b_factor);
      pos=pos*(1-list[i]._b_factor)+pos1*list[i]._b_factor;
    }
  else
  {
      q=q1; pos=pos1;
  }
}
}

/**
This is the function responsible for animation blending

Slerp/Nlerp/matrix blending/ whatever may be used here.
*/
void BlendAnims::GetAnim( Matrix4 &mat, SkeletonIndex bone, const Skeleton *sk ) const
{
  // in case we need to find the axis for the animation, check vcs://Pgm/Drobnosti/RTAnimConfig/rtAnimConfig.cpp - SaveAxisAngleCenter
  // conversion from rotation+axis to rotation+offset is R T = A R -A
  // experimental quaternion lerp
  
  #if 0
  // following special case optimization turned out to be very rare
  // it seems the bones not used in the animations are mostly not used in the model as well
  // optimization intent here is: when bone not is affected by any of the animations in the list
  // no matter how we lerp, the result will be identity then
  // and we may want to avoid the complex blending / lerping in such case
  bool affected = false;
  for (int i=0; i<_list.Size(); i++)
  {
    const AnimBlendInfo &info = _list[i];
    if (info._lock->MapBone(bone)>=0)
    {
      affected = true;
      break;
    }
  }
  if (!affected)
  {
    mat = MIdentity;
    return;
  }
  #endif
  
  const bool alwaysmatrix=false;
  if (!sk->GetPivots().Size() || alwaysmatrix || bone == sk->GetWeaponBone())
  {
    InterpolateMatrix(mat, bone, _list,1.0f);
    // experimental matrix ortho-normalization
    const bool orthogon = false;
    if (orthogon)
    {
      mat.Orthogonalize();
    }
    // experimental root matrix ortho-normalization
    const bool orthogonRoot = true;
    if (orthogonRoot && GetSkeletonIndex(bone)==0)
    {
      // special handling of a root bone
      // this is a pelvis - we want to avoid deforming whole animation because of it
      mat.SetUpAndDirection(mat.DirectionUp(),mat.Direction());
    }
    if (_masked.Size()>0)
  {
      // TODO: optimize mask traversal or representation
      // lerp computation - lerp(x,y,s) = x*(1-s)+y*s
      const BlendAnimSelections *mask = _masked[0]._mask;
      float s = 0;
      for (int i=0; i<mask->Size(); i++)
    {
        const BlendAnimInfo &maskI = mask->Get(i);
        if (maskI.matrixIndex!=bone) continue;
        s = maskI.factor*_masked[0]._maskFactor;
        break;
    }
      if (s>0)
    {
        Matrix4 mmat;
        InterpolateMatrix(mmat, bone, _masked,s);

        // now lay-over the masked layer with lerp
        // consider: special case optimization when _masked contains only one item (factor=1 implied in such case)
        // find a mask item corresponding to bone
        // TODO: implement via optimized inline function (mad)
        mat.InlineSetMultiply(mat,1-s);
        mat += mmat;
        }
      }
    }
  else // quaternion slerp
  {
    const Vector3 &pivot=sk->GetPivots().Get(GetSkeletonIndex(bone));
    Quaternion<float> q;
    Vector3 pos;
    InterpolateQuaternion(q,pos,pivot,bone,_list);

  if (_masked.Size()>0)
  {
    // TODO: optimize mask traversal or representation
    // lerp computation - lerp(x,y,s) = x*(1-s)+y*s
    const BlendAnimSelections *mask = _masked[0]._mask;
    float s = 0;
    for (int i=0; i<mask->Size(); i++)
    {
      const BlendAnimInfo &maskI = mask->Get(i);
      if (maskI.matrixIndex!=bone) continue;
      s = maskI.factor*_masked[0]._maskFactor;
      break;
    }
    if (s>0)
    {
        Quaternion<float> mq;
        Vector3 mpos;
        InterpolateQuaternion(mq,mpos,pivot,bone,_masked);

      // now lay-over the masked layer with lerp
      // consider: special case optimization when _masked contains only one item (factor=1 implied in such case)
      // find a mask item corresponding to bone
      // TODO: implement via optimized inline function (mad)
        q=q.Slerp(mq,s);
        pos=pos*(1-s)+mpos*s;
    }
  }

    q.ToMatrix(mat);
    mat.SetPosition(Vector3(0,0,0));
    Vector3 pivot_trans=mat.FastTransform(pivot);
    pos-=pivot_trans;

#if USER_jana1
   // bool fullRagdoll=false;
    if (m_ragdollAnims.Size() > 0)
    {
      InterpolateQuaternionRagdoll(q, pos, pivot, bone, m_ragdollAnims);
      
			/*
      if (useMasked._used)
      {
        float const bFactor = _ragdoll[0]._b_factor;
        if (bFactor < maxFactor)
          useMasked._factor *= 1 - bFactor;          
        else
          useMasked._used = false;
      }
			*/
    }
#endif

    mat.SetPosition(pos);
}
}

#if USER_jana1
void BlendAnims::InterpolateQuaternionRagdoll(Quaternion<float> &q, Vector3 &pos,const Vector3 &pivot,SkeletonIndex bone, const TAnimBlendInfoArray &list) const
{
  if (list.Size() == 0)
  {
    return;
  }

  for (int i=0; i<list.Size(); ++i)
  {
    const AnimBlendInfo &info = list[i];
    Assert(info.m_ragdoll);
    if (!info.m_ragdoll)
      continue;

    float blendFactor = 1.0f;

    // add mask blend factor
    if (info._mask)
    {
      int index =info._mask->WeightIndex(bone);
      if (index > GetSkeletonIndex(SkeletonIndexNone)) 
				blendFactor *= info._mask->Get(index).factor;
    }

    // add ragdoll bone blend factor
    blendFactor *= info.GetRagdollBoneBlendFactor(bone);

    // add ragdoll global blend factor
    blendFactor *= info._b_factor;

    saturate(blendFactor, 0.0f, 1.0f);

    if (blendFactor > 0.001f)
    {
      const MatrixWithQuaternion& m = info.GetRagdollBone(bone);
      Vector3 pos1 = m._matrix.FastTransform(pivot);

      q=q.Slerp(m._quaternion,blendFactor);
      pos=pos*(1-blendFactor)+pos1*blendFactor;
    }
  }        
}

#endif
#else

/**
This is the function responsible for animation blending

Slerp/Nlerp/matrix blending/ whatever may be used here.
*/
void BlendAnims::GetAnim( Matrix4 &mat, SkeletonIndex bone ) const
{
	//To avoid LHS, do everything in VMX128. Only matrixLerp supported ATM

	XMMATRIX xmat;

	{
		const AnimBlendInfo &info = _list[0];
		const AnimationRTMatrix &bmat = info.GetBone(bone);
		XMVECTOR xfactor = XMVectorSet1(&info._factor);
		XMMATRIX xbone = bmat.ToVMXMatrix();

		xmat = XMMatrixMultiply(xbone, xfactor);
	}

	for (int i=1; i<_list.Size(); i++)
	{
		const AnimBlendInfo &info = _list[i];
		const AnimationRTMatrix &bmat = info.GetBone(bone);
		XMVECTOR xfactor = XMVectorSet1(&info._factor);
		XMMATRIX xbone = bmat.ToVMXMatrix();
		xmat = XMMatrixMultiplyAdd(xbone, xfactor, xmat);
	}

	// experimental root matrix ortho-normalization
	const bool orthogonRoot = true;
	if (orthogonRoot && GetSkeletonIndex(bone)==0)
	{
		//Matrix4K xxx;
		//xxx.SetUpAndDirection(Vector3(FloatQuad(xmat.r[1])),Vector3(FloatQuad(xmat.r[2])));

		// special handling of a root bone
		// this is a pelvis - we want to avoid deforming whole animation because of it
		xmat = XMMatrixSetUpAndDirection(xmat.r[3], xmat.r[1], xmat.r[2]);
	}

	if (_masked.Size()>0)
	{
		// TODO: optimize mask traversal or representation
		// lerp computation - lerp(x,y,s) = x*(1-s)+y*s
		const BlendAnimSelections *mask = _masked[0]._mask;
		float s = 0;
		for (int i=0; i<mask->Size(); i++)
		{
			const BlendAnimInfo &maskI = mask->Get(i);
			if (maskI.matrixIndex!=bone) continue;
			s = maskI.factor*_masked[0]._maskFactor;
			break;
		}
		if (s>0)
		{
			XMVECTOR xs = XMVectorSet1(s);

			// xx
			XMMATRIX mmat;
			{
				const AnimBlendInfo &info = _masked[0];
				const AnimationRTMatrix &bmat = info.GetBone(bone);
				XMVECTOR xfactor = XMVectorSet1(&info._factor);
				XMMATRIX xbone = bmat.ToVMXMatrix();
				mmat = XMMatrixMultiply(xbone, XMVectorMultiply(xfactor, xs));
			}

			for (int i=1; i<_masked.Size(); i++)
			{
				const AnimBlendInfo &info = _masked[i];
				const AnimationRTMatrix &bmat = info.GetBone(bone);
				XMVECTOR xfactor = XMVectorSet1(&info._factor);
				XMMATRIX xbone = bmat.ToVMXMatrix();
				mmat = XMMatrixMultiplyAdd(xbone, XMVectorMultiply(xfactor, xs), mmat);
				Assert(info._mask==mask);
			}
			// now lay-over the masked layer with lerp
			// consider: special case optimization when _masked contains only one item (factor=1 implied in such case)
			// find a mask item corresponding to bone

			xmat = XMMatrixMultiplyAdd(xmat, XMVectorSubtract(XMVectorSplatOne(), xs), mmat);
		}
	}

	FromXMMatrix(mat, xmat);
}

XMMATRIX BlendAnims::GetAnim( SkeletonIndex bone ) const
{
	//To avoid LHS, do everything in VMX128. Only matrixLerp supported ATM

	XMMATRIX xmat;

	{
		const AnimBlendInfo &info = _list[0];
		const AnimationRTMatrix &bmat = info.GetBone(bone);
		XMVECTOR xfactor = XMVectorSet1(&info._factor);
		XMMATRIX xbone = bmat.ToVMXMatrix();

		//bmat.ToMatrix(_GMat);
		//_GXMat = xbone;

		xmat = XMMatrixMultiply(xbone, xfactor);
	}

	for (int i=1; i<_list.Size(); i++)
	{
		const AnimBlendInfo &info = _list[i];
		const AnimationRTMatrix &bmat = info.GetBone(bone);
		XMVECTOR xfactor = XMVectorSet1(&info._factor);
		XMMATRIX xbone = bmat.ToVMXMatrix();
		xmat = XMMatrixMultiplyAdd(xbone, xfactor, xmat);
	}

	// experimental root matrix ortho-normalization
	const bool orthogonRoot = true;
	if (orthogonRoot && GetSkeletonIndex(bone)==0)
	{
		// special handling of a root bone
		// this is a pelvis - we want to avoid deforming whole animation because of it
		xmat = XMMatrixSetUpAndDirection(xmat.r[3], xmat.r[1], xmat.r[2]);
	}

	if (_masked.Size()>0)
	{
		// TODO: optimize mask traversal or representation
		// lerp computation - lerp(x,y,s) = x*(1-s)+y*s
		const BlendAnimSelections *mask = _masked[0]._mask;
		float s = 0;
		for (int i=0; i<mask->Size(); i++)
		{
			const BlendAnimInfo &maskI = mask->Get(i);
			if (maskI.matrixIndex!=bone) continue;
			s = maskI.factor*_masked[0]._maskFactor;
			break;
		}
		if (s>0)
		{
			XMVECTOR xs = XMVectorSet1(s);

			// xx
			XMMATRIX mmat;
			{
				const AnimBlendInfo &info = _masked[0];
				const AnimationRTMatrix &bmat = info.GetBone(bone);
				XMVECTOR xfactor = XMVectorSet1(&info._factor);
				XMMATRIX xbone = bmat.ToVMXMatrix();
				mmat = XMMatrixMultiply(xbone, XMVectorMultiply(xfactor, xs));
			}

			for (int i=1; i<_masked.Size(); i++)
			{
				const AnimBlendInfo &info = _masked[i];
				const AnimationRTMatrix &bmat = info.GetBone(bone);
				XMVECTOR xfactor = XMVectorSet1(&info._factor);
				XMMATRIX xbone = bmat.ToVMXMatrix();
				mmat = XMMatrixMultiplyAdd(xbone, XMVectorMultiply(xfactor, xs), mmat);
				Assert(info._mask==mask);
			}
			// now lay-over the masked layer with lerp
			// consider: special case optimization when _masked contains only one item (factor=1 implied in such case)
			// find a mask item corresponding to bone

			xmat = XMMatrixMultiplyAdd(xmat, XMVectorSubtract(XMVectorSplatOne(), xs), mmat);
		}
	}
	return xmat;
}

#endif

/**
@param sel when there is a mask provided, a separate slot is used
*/
void AnimationRT::GetBlend(BlendAnims &blend, float time, float factor, float maskFactor, const BlendAnimSelections *sel, const AnimationRTBuffer *buffer, const MoveInfoBase *moveInfo) const
{
  if (_nPhases<=0) return; // not animated
  AnimationRTLock load(unconst_cast(*this));

  if (!_phasesBuffer.Initialized())
  {
    RPTSTACK();
    RString failMessage = Format("_phases.Size()==0, name=%s",cc_cast(_name.name));
    Fail(cc_cast(failMessage));
    return;
  }
  if (load->_boneMap.Size()<=0)
  {
    // no bones - empty animation, ignore it
    return;
  }

  // select nearest animation phase
  int nextIndex, prevIndex;
  Find(nextIndex, prevIndex, time);
  
  // request the available phase from the buffer
  const AnimationRTPhase *prevPhase = NULL;
  const AnimationRTPhase *nextPhase = NULL;
  _phasesBuffer.Request(prevPhase, nextPhase, prevIndex, nextIndex, GetLooped(), buffer);
  Assert(prevPhase);
  Assert(nextPhase);

  // interpolate between phases
  const float epsilon = 1e-6f;
  if (prevIndex == nextIndex)
  {
    if (factor>epsilon)
    {
      if (!sel)
      {
        blend._list.Add(AnimBlendInfo(load,prevPhase,NULL,0.0f,factor,_phaseTimes[prevIndex],sel,maskFactor,moveInfo));
      }
      else
      {
        blend._masked.Add(AnimBlendInfo(load,prevPhase,NULL,0.0f,factor,_phaseTimes[prevIndex],sel,maskFactor,moveInfo));
      }
    }
  }
  else if (factor>epsilon)
  {
    float nextTime = _phaseTimes[nextIndex];
    float prevTime = _phaseTimes[prevIndex];
    if (nextTime <= time) nextTime += 1;
    if (prevTime > time) prevTime -= 1;
    Assert( nextTime>time );
    Assert( prevTime<=time );

    float interpol = nextTime>prevTime ? (time-prevTime)/(nextTime-prevTime) : 1;

    if (interpol<epsilon)
    {
      if (!sel)
      {
        blend._list.Add(AnimBlendInfo(load,prevPhase,NULL,interpol,factor,_phaseTimes[prevIndex],sel,maskFactor,moveInfo));
      }
      else
      {
        blend._masked.Add(AnimBlendInfo(load,prevPhase,NULL,interpol,factor,_phaseTimes[prevIndex],sel,maskFactor,moveInfo));
      }
    }
    else if ((1-interpol)<epsilon)
    {
      if (!sel)
      {
        blend._list.Add(AnimBlendInfo(load,nextPhase,NULL,1-interpol,factor,_phaseTimes[nextIndex],sel,maskFactor,moveInfo));
      }
      else
      {
        blend._masked.Add(AnimBlendInfo(load,nextPhase,NULL,1-interpol,factor,_phaseTimes[nextIndex],sel,maskFactor,moveInfo));
      }
    }
    else
    {
      if (!sel)
      {
        blend._list.Add(AnimBlendInfo(load,prevPhase,nextPhase,interpol,factor,time,sel,maskFactor,moveInfo));
  }
      else
      {
        blend._masked.Add(AnimBlendInfo(load,prevPhase,nextPhase,interpol,factor,time,sel,maskFactor,moveInfo));
}
    }
  }
}

void AnimationRT::Matrix(Matrix4 &mat, float time, SkeletonIndex boneIndex) const
{
	// prepare blending information
  BlendAnims blend;

  GetBlend(blend, time, 1);
  
  // now perform the blending
  blend.DoBoneAnim(mat,_name.skeleton,boneIndex);
}


BankArray<Skeleton> Skeletons;


/** Functions loads capsule into geometry level of input LODShape.
@param pLodShapeTo the LODshape where will be geometry level changed
@param vertexMapping the mapping between vertexes in pLodShapeTo and pattern CollisionVertexPattern. 
@param geomCompMapping the mapping between convex comp in pLodShapeTo and CollisionGeomCompPattern.
*/
void CollisionCapsule::ReplaceGeometryLevel(AnimationContext &animContext, const Shape *shape,
                                            const Mapping& vertexMapping, 
                                            const Mapping& geomCompMapping)
{
  if (!_initalized)
  {
    RptF("Collision capsule %s not initialized.", (const char *) _fileName);
    return;
  }

  // Move vertexes
  for (int i=0; i<vertexMapping.Size(); i++)
  {
    int iIndex = vertexMapping[i];
    if (iIndex >= 0) animContext.SetPos(shape, iIndex) = _pos[i];      
  }

  // Enable / disable components
  for (int i=0; i<_enabledGeomComp.Size(); i++)
  {         
    int iIndex = geomCompMapping[i];
    if (iIndex >= 0)
    {         
      if (_enabledGeomComp[i])
        animContext.CCEnable(iIndex);
      else
        animContext.CCDisable(iIndex);
    }
  } 
}

/** Function creates mapping between capsule's vertexes and convex components and pattern orders
CollisionVertexPattern and CollisionGeomCompPattern. 
@return true in case of success, false otherwise.
*/

bool CollisionCapsule::FindMapping(const FindArray<RStringB>& vertexPattern, const FindArray<int>& geomCompPattern)
{  
  if (_initalized)
    return true;

  _initalized = false; 

  LODShape shape(_fileName, true);

  {
    const Shape *shapeFrom = shape.GeometryLevel();
    if (!shapeFrom)
    {
      WarningMessage("No geometry level found in collision shape %s", (const char *) shape.GetName());
      return false;
    }

    //_vertexMapping.Resize(shapeFrom->NVertex());
    _pos.Realloc(vertexPattern.Size());
    _pos.Resize(vertexPattern.Size());
    //for(int i = 0; i < shapeFrom->NVertex(); i++)
    //  _vertexMapping[i] = -1;

    // make sure some initialization is there for the vertex data
    for (int i = 0; i <_pos.Size(); i++)
    {
      _pos[i] = VZero;
    }
    
    for(int i = 0; i < shapeFrom->NNamedSel(); i++)
    {
      const NamedSelection::Access &cNamedSelFrom = shapeFrom->NamedSel(i,&shape);
      if (cNamedSelFrom.Size() == 1)
      {
        cNamedSelFrom.ReportSelectionUsed(); // prevent assuming the selection was unused
        int iNamedSelTo = vertexPattern.Find(cNamedSelFrom.GetName());           

        if (iNamedSelTo == -1)
        {
          WarningMessage("Unknown selection %s in collision geometry pattern.", cNamedSelFrom.Name());                                    
        }
        else
          //_vertexMapping[cNamedSelFrom[0]] = iNamedSelTo;                     
          _pos[iNamedSelTo] =  shapeFrom->Pos(cNamedSelFrom[0]) + shape.BoundingCenter();
      }
    }


    // Enable/Disable components   
    _enabledGeomComp.Realloc(geomCompPattern.Size());
    _enabledGeomComp.Resize(geomCompPattern.Size());
    for(int i = 0; i < geomCompPattern.Size(); i++)
    {
      int componentIndex = shape.GetGeomComponents().FindNameIndex(geomCompPattern[i]);

      _enabledGeomComp[i] = (componentIndex >= 0);
    } 
  }
  _initalized = true;
  return true;
}

#ifdef _DEBUG
double CollisionCapsule::GetMemoryUsed() const
{
  return sizeof(bool) + /*_vertexMapping.GetMemoryUsed()*/ + _pos.GetMemoryUsed() + _enabledGeomComp.GetMemoryUsed(); //+ base::GetMemoryUsed();
}
#endif

CollisionCapsuleBank CollisionCapsules; ///< Array with collision capsules.
