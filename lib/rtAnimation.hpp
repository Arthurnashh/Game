#ifdef _MSC_VER
#pragma once
#endif

#ifndef _RT_ANIMATION_HPP
#define _RT_ANIMATION_HPP

#include "Shape/shape.hpp"
#include <El/ParamFile/paramFileDecl.hpp>

/// matrix index and weight pair for skeleton skinning
struct AnimationRTPair
{
  //int sel;
  //float weight;
  unsigned char _sel; // matrix index 
  unsigned char _weight; // matrix used with given weigth
  #define WeightScale 255.0f
  #define InvWeightScale (1.0f/255)

  static inline int ConvertWeight(float w)
  {
    int wi = toInt(w*WeightScale);
    saturate(wi,0,255);
    return wi;
  }

  //!{ Converting between SubSkeletonIndex and Sel
  static unsigned char SubSkeletonIndexToSel(SubSkeletonIndex s)
  {
    int i = GetSubSkeletonIndex(s);
    return (i < 0) ? 255 : i;
  }
  static SubSkeletonIndex SelToSubSkeletonIndex(unsigned char sel)
  {
    SubSkeletonIndex si;
    if (sel == 255)
    {
      SetSubSkeletonIndex(si, -1);
    }
    else
    {
      SetSubSkeletonIndex(si, (int)sel);
    }
    return si;
  }
  //!}

  AnimationRTPair(){}
  AnimationRTPair( SubSkeletonIndex s, float w ):_sel(SubSkeletonIndexToSel(s)),_weight(ConvertWeight(w))
  {
    if ((GetSubSkeletonIndex(s) < -1) || (GetSubSkeletonIndex(s) >= 255))
    {
      RptF("SubSkeletonIndex: %d", GetSubSkeletonIndex(s));
      Fail("Error: SubSkeletonIndex out of bounds");
    }
    Assert(w>=0 && w<=1.0f);
  }
  AnimationRTPair( SubSkeletonIndex s, unsigned char w=255 ):_sel(SubSkeletonIndexToSel(s)),_weight(w)
  {
    if ((GetSubSkeletonIndex(s) < -1) || (GetSubSkeletonIndex(s) >= 255))
    {
      RptF("SubSkeletonIndex: %d", GetSubSkeletonIndex(s));
      Fail("Error: SubSkeletonIndex out of bounds");
    }
    Assert(w>=0 && w<=255);
  }
  SubSkeletonIndex GetSel() const {return SelToSubSkeletonIndex(_sel);}
  // get weight, float result (0..1)
  float GetWeight() const {return _weight*InvWeightScale;}
  /// get weight, 255 corresponds to 1.0
  unsigned char GetWeight8b() const {return _weight;}
  void SetWeight( float w )
  {
    Assert(w>=0 && w<=1.0f);
    _weight=ConvertWeight(w);
  }
  void SetWeight8b( unsigned char w )
  {
    Assert(w>=0 && w<=255);
    _weight=w;
  }
};

TypeIsSimple(AnimationRTPair);

#include <Es/Containers/smallArray.hpp>

//class AnimationRTWeight: public AutoArray<AnimationRTPair>
// max. 4 matrices per vertex - much less memory used

const int ARTWMaxSize = sizeof(AnimationRTPair)*4+sizeof(int);

class AnimationRTWeight: public VerySmallArray<AnimationRTPair,ARTWMaxSize>
{
  public:
  void Normalize();
  //bool GetSimple() const {return _simple;}
  bool BoneRefMatches(const AnimationRTWeight &artw)
  {
    if (Size() != artw.Size()) return false;
    for (int i = 0; i < Size(); i++)
    {
      const AnimationRTPair &a = Get(i);
      const AnimationRTPair &b = artw.Get(i);
      if (a.GetSel() != b.GetSel())
      {
        return false;
      }
    }
    return true;
  }
};

TypeIsMovable(AnimationRTWeight);
typedef InitVal<int,-1> InitInt;
TypeIsMovable(InitInt);

class Skeleton;

#include <El/Math/mathStore.hpp>

typedef Matrix4Quat16b AnimationRTMatrix;
extern const AnimationRTMatrix AnimationRTMatrixIdentity;


/// single animation keyframe
/**
Contains transformation matrices for all bones
*/
class AnimationRTPhase: public RefCount
{
  friend class AnimationRTPhases;
  friend class AnimationRT;
  /// matrix for each bone (hierarchical, local space)
  AutoArray<AnimationRTMatrix> _local;

public:
  AnimationRTPhase();
  ~AnimationRTPhase();

  // Load returns animation time
  float Load( QIStream &in, Skeleton *skelet, int nSel, const Array<int> &boneMap, bool reversed=false );
  static void Skip( QIStream &in, int nSel);
  __forceinline const AnimationRTMatrix &GetBone(int bone) const {return _local[bone];}

  // skeleton included in f context
  void Reverse();
  void SerializeBin(SerializeBinStream &f,const Skeleton *sk,const Array<int> *boneMap);
  void ConvertLocalWithPivots(const Skeleton *sk,const Array<int> *boneMap);
};

//TypeIsMovableZeroed(AnimationRTPhase);

struct WeightInfoName
{
  LLink<LODShape> shape;
  Link<Skeleton> skeleton;
  WeightInfoName(){shape=NULL,skeleton=NULL;}
};

/// SkeletonSource name used for BankArray
struct SkeletonSourceName
{
  /// used only during loading - not used for name storage
  InitPtr<const ParamEntry> _class;
  
  /// used for item identification
  const char *_name;
  
  SkeletonSourceName(const char *name)
  :_name(name)
  {
  }
  SkeletonSourceName(const RStringB &name)
  :_name(name)
  {
  }
  
  SkeletonSourceName(ParamEntryPar cls, const char *name);
};


//! Source of the skeleton
class SkeletonSource : public RefCount
{
private:
  //! Name of the skeleton source
  RStringB _name;
  //! Flag to determine the skeleton has got only the one-bone weights (_isDiscrete == true)
  bool _isDiscrete;
  //! Structure that stores the hierarchy of the skeleton
  FindArrayKey<SBoneRelation> _structure;
  /// Filename of the model containing pivot points - read from model.cfg, embedded in p3d; old method, ignored; just for compatibility
  RStringB _pivotsNameObsolote;
  /// Filename of the model containing pivot points (needed for animation blending)
  RStringB _pivotsName;
  /// tree description suitable for a fast traversal
  BoneTree _tree;
  /// for each bone in _tree excluding the root we store the SkeletonIndex
  /** note: this may be removed after all models are re-binarized to be ordered by hierarchy */
  AutoArray<SkeletonIndex> _treeSI;
  /// parent for each bone - allowing leaf-root direction traversal
  AutoArray<SkeletonIndex> _parent;

  // we need to know weapon bone because it need to be interpolated by non-quaternion transformation
  RString _weaponName;
  /// loads CfgSkeletonParameters from config file
  void LoadConfig();

protected:
  //! Loading of the skeleton source from the CfgSkeletons entry
  void Load(ParamEntryPar cfg);
  
  /// order bones so that order conforms to BoneTree (this also means parents are before children)
  void OrderHierarchy();
  /// build a hierarchy, but do not change ordering, create a mapping instead
  void BuildHierarchy();
  /// rebuild _tree member, no changes to the bone ordering should be needed
  void RescanHierarchy();
  /// rebuild _parent member
  void RebuildParents();

public:
  //! Empty constructor
  SkeletonSource() {};
  //! Custom initialization
  void Init(bool isDiscrete, const FindArrayKey<SBoneRelation> &structure)
  {
    _isDiscrete = isDiscrete;
    _structure = structure;
    OrderHierarchy();
  }
  //! Constructor to load the entry named "name" from the CfgSkeletons
  SkeletonSource(const SkeletonSourceName &name);
  //! Constructor to load during the serialization
  SkeletonSource(RStringB name, SerializeBinStream &f);
  //! Serialization
  bool SerializeBin(SerializeBinStream &f);
  //! skip the space in the serialization stream without loading
  void SkipBin(SerializeBinStream &f);
  //! Returns the name of the skeleton source
  const RStringB &GetName() const {return _name;}
  //! Determines whether structure is discrete or not
  bool IsDiscrete() const {return _isDiscrete;}
  //! Returns structure of the skeleton source
  const FindArrayKey<SBoneRelation> &Structure() const {return _structure;}
  /// find index of given bone in the skeleton description
  int FindBone(const RStringB &name) const;
  /// total bone count
  int NBones() const {return _structure.Size();}
  /// get bone name
  const SBoneRelation &GetBoneInfo(int i) const {return _structure[i];}

  /// get information about a parent
  SkeletonIndex GetParent(SkeletonIndex i) const {return _parent[GetSkeletonIndex(i)];}

  const RStringB &GetPivotsName() const {return _pivotsName;}

  /// access to hierarchical description
  const BoneTree &GetTree() const {return _tree;}

  const RString &GetWeaponName() const {return _weaponName;}
};



template<>
struct BankTraits<SkeletonSource>: public DefBankTraits<SkeletonSource>
{
  typedef SkeletonSourceName NameType;
  static int CompareNames( const NameType &n1, const NameType &n2 )
  {
    return strcmp(n1._name,n2._name);
  }
};

//! Bank of all skeletons
class SkeletonBank : public BankArray<SkeletonSource>
{
public:
  SkeletonSource *NewEmbedded(const RStringB &name, SerializeBinStream &f);
};

extern SkeletonBank SkeletonSources;

/// skeleton used for animations (both motion captured and code driven)

class Skeleton: public RefCountWithLinks
{
  RStringB _name;
  // TODO: replace _matrixNames by accessing _source->_structure
  FindArray<RStringB> _matrixNames; // matrix names should be shared by all animations

  //! Flag to determine we use just one-bone skinning
  bool _isDiscrete;

  /// sometimes we do not want to report missing bones - e.g. in RTM binarization
  bool _reportMissingBones;
  
  
  /// description of the skeleton
  Ref<SkeletonSource> _source;

  FindArray<RStringB> _collisionVertexPattern; ///< Pattern order of vertexes in man's geometry level.
  FindArray<int> _collisionGeomCompPattern;  ///<Pattern order of convex component in  man's geometry level.
  /// pivot points are loaded from a p3d shape
  AutoArray<Vector3> _pivots;

  // we need to know weapon bone because it need to be interpolated by non-quaternion transformation
  SkeletonIndex _weapBone; 
  
public:
  Skeleton();
  Skeleton(RStringB name);
  ~Skeleton();
  const RStringB &GetName() const {return _name;}

  SkeletonIndex NewBone(RStringB name);
  SkeletonIndex FindBone(RStringB name) const;
  int NBones() const {return _matrixNames.Size();}
  RStringB GetBone(SkeletonIndex i) const;
  /**
  @return if SkeletonIndexNone, parent is the root
  */
  SkeletonIndex GetParent(SkeletonIndex i) const {return _source->GetParent(i);}
  /// init skeleton hierarchy from the config
  void LoadFromSkeletonSource(Ref<SkeletonSource> skeletonSource);
  /// some skeletons are used only to animated proxies / selections
  void PrepareProxyBones(LODShape *lodShape, Shape *shape) const;
  /// some skeletons are used only to animated proxies / selections
  void FinishProxyBones(LODShape *lodShape, Shape *shape) const;
  
  /// init skeleton collision patterns from the config
  void LoadCollPatterns(ParamEntryPar cfg);

  const FindArray<RStringB>& GetCollVertexPattern() const {return _collisionVertexPattern;};
  const FindArray<int>& GetCollGeomCompPattern() const {return _collisionGeomCompPattern;};

  const BoneTree &GetTree() const {return _source->GetTree();}
  const Array<Vector3> &GetPivots() const {return _pivots;}

  bool GetReportMissingBones() const {return _reportMissingBones;}
  void SetReportMissingBones(bool val) {_reportMissingBones = val;}

  // we need to know weapon bone because it need to be interpolated by non-quaternion transformation
  SkeletonIndex GetWeaponBone() const {return _weapBone;}
};

template<>
struct BankTraits<Skeleton>: public DefBankTraits<Skeleton>
{
  typedef const RStringB &NameType;
  static int CompareNames( NameType n1, NameType n2 )
  {
    return n1!=n2;
  }
};

extern BankArray<Skeleton> Skeletons;

//! Information about neighbors in skinned shadow volume
struct VertexNeighborInfo
{
  VertexIndex _posA;
  AnimationRTWeight _rtwA;
  VertexIndex _posB;
  AnimationRTWeight _rtwB;
};
TypeIsMovable(VertexNeighborInfo);

#include "engine.hpp"

#define MATRIX_4_ARRAY(name,size) AUTO_STATIC_ARRAY(Matrix4,name,size)

#include <Es/Memory/normalNew.hpp>
#include <Es/Memory/fastAlloc.hpp>

struct BlendAnimInfo
{
  //RStringB name;
  SkeletonIndex matrixIndex;
  float factor;
};
TypeIsSimple(BlendAnimInfo)

typedef MemAllocSA BlendAnimSelectionsStorage;

class BlendAnimSelections: public StaticArrayAuto<BlendAnimInfo>
{
public:
  BlendAnimSelections();
  void Load( Skeleton *skelet, ParamEntryPar cfg );
  void AddOther( const BlendAnimSelections &src, float factor );
};



typedef AutoArray<int> Mapping; ///< int array used for vertex index mapping in collisionCapsules 

/** The class represents collision capsule. It also provides functionality that replaces collision geometry
in Man by this capsule. The pattern order of veretexes and GeomComponents is defined by global 
variables CollisionVertexPattern and CollisionGeomCompPattern.
*/
class CollisionCapsule : public RefCountWithLinks
{
protected:
  bool _initalized;                    ///< true if mapping is already established
      
  AutoArray<bool> _enabledGeomComp;         /**< order of the array is according pattern geom comp order CollisionGeomCompPattern. 
                                      The item value is true if component is present in capsule.*/
  AutoArray<Vector3> _pos;            ///< vertex positions ordered according to CollisionVertexPattern.
  RStringB _fileName;                 ///< filename of p3d with capsule 

public:    
  CollisionCapsule(const RStringB & name)
  : _fileName(name), _initalized(false) {};  

  /** Function replaces properties in animContext by Capsule.*/
  void ReplaceGeometryLevel(AnimationContext &animContext, const Shape *shape,
      const Mapping& vertexMapping,      // mapping from pShapeTo vertexes order to pattern vertex order
      const Mapping& geomCompMapping);   // mapping from pShapeTo geom comp order to pattern order        

#ifdef _DEBUG
  /** Functions returns memory pointed by this object. */
  double GetMemoryUsed() const;
#endif

   /** Functions find mapping between capsule and pattern. */
  bool FindMapping(const FindArray<RStringB>& vertexPattern, const FindArray<int>& geomCompPattern);

  const RStringB &GetName() const {return _fileName;};
};

template <> 
struct BankTraits<CollisionCapsule>: public DefLinkBankTraits<CollisionCapsule>
{
  // default name is character
  typedef const RStringB &NameType;
  // default name comparison
  static int CompareNames( RStringB n1, RStringB n2 )
  {
    return n1 != n2;
  }  
};


typedef BankArray<CollisionCapsule> CollisionCapsuleBank;  
extern CollisionCapsuleBank CollisionCapsules; ///< Array with all collision capsules

// identifies of a single AnimationRT
struct AnimationRTName
{
  /// rtm name is important
  RStringB name;
  /// animation is bound to a given skeleton
  Ref<Skeleton> skeleton;
  /// model is reversed
  bool reversed;
  /// streaming enabled for this animation
  bool streamingEnabled;
};

struct BlendAnims;


#include <Es/Containers/listBidir.hpp>

/**
The _frameId mechanism is unreliable, as updates are not atomic.
In worst case the same texture can be inserted for each thread
Therefore _processedId exists to filter this once more while processing.
TODO: common LoadRequested class can be introduced, used as a base for both AnimationRT and Texture
*/
ALIGN_SLIST class AnimationRTLoadRequested : public SLIST_ENTRY
{
  /// non-zero when we are currently in the list
  mutable LONG _inList;

public:
  AnimationRTLoadRequested()
  {
    _inList = 0;
  }

  /// check if we were already added into the list in this frame
  /* caution: not atomic */
  bool AddIntoSList() const
  {
    LONG was = InterlockedExchange(&_inList,1);
    //LONG was = InterlockedCompareExchange(&_inList,1,0);
    // if was not there yet, report
    return was==0;
  }

  /// check if we were already extracted from the list in this frame
  bool RemovedFromSList() const
  {
    LONG was = InterlockedExchange(&_inList,0);
    //LONG was = InterlockedCompareExchange(&_inList,0,1);
    // return true (already removed) if value was zero (not in list)
    return was==0;
  }
  void RemoveFromSList()  const {_inList = 0;}
};

#define DIAG_EXTERNAL_BUFFER 0

/// cyclic buffer of animation phases
struct AnimationRTBuffer
{
  /// cyclic buffer of animation phases
  RefArray<AnimationRTPhase> _phases; 
  /// index of the lowest phase stored in _phases
  int _phasesStart;
  /// number of the first phase stored in _phases
  int _phasesOffset;

#if DIAG_EXTERNAL_BUFFER
  /// name of animation the buffer was initialized from
  RString _animName;
  /// how many times the animation was loaded when buffer was initialized
  int _animLoadCount;
#endif

  AnimationRTBuffer()
  {
    _phasesStart = 0; // beginning of _phases
    _phasesOffset = 1; // frame #0 stored in _firstPhase
#if DIAG_EXTERNAL_BUFFER
    _animLoadCount = 0;
#endif
  }
  void Init()
  {
    _phases.Clear();
    _phasesStart = 0; // beginning of _phases
    _phasesOffset = 1; // frame #0 stored in _firstPhase
#if DIAG_EXTERNAL_BUFFER
    _animLoadCount = 0;
#endif
  }
};

/// encapsulate AnimationRTBuffer using Copy On Write
/** reduces copying in the visual states */
struct AnimationRTBufferCopyOnWrite
{
  class AnimationRTBufferWithRef: public AnimationRTBuffer, public RefCount
  {

  };

  Ref<AnimationRTBufferWithRef> _buffer;

  public:

  AnimationRTBufferCopyOnWrite()
  {
    _buffer = new AnimationRTBufferWithRef;
  }
  operator const AnimationRTBuffer &() const {return *_buffer;}
  operator AnimationRTBuffer &()
  {
    _buffer = new AnimationRTBufferWithRef(*_buffer);
    return *_buffer;
  }

  void Init()
  {
    _buffer->Init();
  }

};

class QIFStreamB;

/// container for the animation phases (with support for streaming)
class AnimationRTPhases
{
  friend class AnimationRT;
  
protected:
  /// max. size of the _phases buffer in bytes when loading from the binarized file
  static const int _bufferSize;
  /// positions of phases in the binarized file - _streamOffsets[0] is the offset of the second frame in the animation
  mutable AutoArray<int> _streamOffsets; // updated in LoadNeededPhases, even when _buffer is const (protected)
  /// the first animation phase (time = 0.0f)
  Ref<AnimationRTPhase> _firstPhase;
  /// cyclic buffer of animation phases
  AnimationRTBuffer _buffer;
  /// the total number of phases the structure is covering
  int _phasesCount;
  /// true when _phases contain only subset of all phases
  bool _streamed;
  /// true when _firstPhase is loaded
  bool _firstValid;

public:
  AnimationRTPhases()
  {
    _phasesCount = 0;
    _streamed = false;
    _firstValid = false;
  }
  /// empty the container to uninitialized state
  void Init();
  /// set the animation to all identities (fall back when load failed)
  void InitIdentity(Skeleton *skelet);
  /// limit the count of frames (for example because of looping)
  void LimitCount(int count);
  /// check if the container is initialized
  bool Initialized() const {return _firstValid;}
  /// streamed animation cannot be saved
  bool CanBeSaved() const {return !_streamed;}
  /// animation is streamed - requires regular call of LoadNeededPhases
  bool IsStreamed() const {return _streamed;}
  /// number of bones affected by the animation
  int NAnimatedBones() const {return Initialized() ? _firstPhase->_local.Size() : 0;}
  /// drop memory allocated to phases
  void Release();
  /// memory we can drop on demand
  int GetMemoryControlled() const;

  double GetMemoryUsed() const;

  /// serialization (save) - binarized format
  void SaveBin(SerializeBinStream &f);
  /// serialization (the first load) - binarized format
  void LoadHeaderBin(SerializeBinStream &f,const Skeleton *sk,const Array<int> *boneMap);
  /// serialization - binarized format
  void LoadPhasesBin(SerializeBinStream &f, int count, bool streamingEnabled,const Skeleton *sk,const Array<int> *boneMap);
  /// preload - binarized format
  static bool Preload(QIStream &in, FileRequestPriority priority, bool fullLoad);
  /// streaming - binarized format - try to load animation phases around phase
  void LoadNeededPhases(SerializeBinStream &f, AnimationRTBuffer &buffer, int phase, int phasePastNeeded, const Skeleton *sk,const Array<int> *boneMap) const;

  /// validate data sanity (debugging)
  bool CheckBufferValid(const AnimationRTBuffer &buffer) const;

  /// loading - source format
  void LoadPhases(
    QIStream &in, Skeleton *skelet, int nSel, int startAnim, int endAnim,
    AutoArray<int> &boneMap, bool reversed, AutoArray<float> &times);

  /// request the available phases
  void Request(
    const AnimationRTPhase* &prevPhase, const AnimationRTPhase* &nextPhase,
    int &prevIndex, int &nextIndex, bool looped, const AnimationRTBuffer *buffer) const;

#if DIAG_EXTERNAL_BUFFER
  void SetName(RString name) {_buffer._animName = name;}
#endif
};

// head bob modes
enum HeadBobModeEnum
{
  HeadBobUndef = 0,
  HeadBobRaw,
  HeadBobBeginEnd,
  HeadBobAverageFrame,
  HeadBobStraightHorizon,
  HeadBobCutScene,
};

// matrices for head bob corrections
struct HeadBobCorrection
{
  // begin, end, and average matrices
  Matrix4 _matrices[3];
};

//! Real Time skeletal animation
class AnimationRT: public RefCountWithLinks, public TLinkBidirD, public AnimationRTLoadRequested //, private NoCopy
{
  /// mapping skeleton index to animation index
  AutoArray<int> _boneMap;
  
  /// animation keyframes (cyclic buffer)
  AnimationRTPhases _phasesBuffer;
  /// time of each keyframe (0..1)
  AutoArray<float> _phaseTimes;
  /// stream used for streaming (to avoid opening it again and again)
  SRef<QIFStreamB> _stream;
  
  /// movement per single animation cycle
  Vector3 _step; 
  /// which bones are used in this animation
  AutoArray<RStringB> _selections;
  /// animation name (usually file name)
  AnimationRTName _name;
  
  enum {LoopedNotInit=-1, LoopedFalse, LoopedTrue} _looped; //!< looping flag
  bool _reversed; //! animation should be reversed during loading
  /// what file version (0 = non-streamed, 1 = streamed, 2 = binarized streamed)
  char _version;
  #ifdef _M_PPC
  /// binarized file endian - needed for streamed loading
  QFileEndian _endianState;
  #endif
  /// compressed arrays are using LZO compression
  bool _lzoCompression;

  int _preloadCount; //!< animation should be always loaded
  mutable int _loadCount; //!< load "reference" counting
  int _nPhases; //!< number of keyframes in full animation

  /// eventual correction for head bob (with lazy initialization, see GetHeadBobCorrection())
  mutable SRef<HeadBobCorrection> _headBobCorrection;
  
  protected:
  void Find( int &prevIndex, int &nextIndex, float time ) const;
  int FindNearest(float time) const;
  Matrix4Val GetHBCMatrix(int i, const class AnimationRTLock &lock, const Skeleton *skeleton, SkeletonIndex bone) const;
  static bool ValidHBCMatrix(Matrix4Par mat) {return _finite(mat(0, 0)) ? true : false;}
  static void ApplyMatricesComplex(
    AnimationContext &animContext, LODShape *lShape, int level, const Matrix4Array &matrices
  );
  static void ApplyMatricesSimple(
    AnimationContext &animContext, LODShape *lShape, int level, const Matrix4Array &matrices
  );
  static void ApplyMatricesIdentity(
    AnimationContext &animContext, LODShape *lShape, int level
  );

  public:
  AnimationRT();
  AnimationRT(const AnimationRTName &name);
  ~AnimationRT();

  //! preload attribute is used to mark persistent animation
  //! same AnimationRT may be shared between several states with different
  //! preload attribute, it is therefore implemented as counter here
  void AddPreloadCount();
  void ReleasePreloadCount();
  /// load the animation from a file
  void Load(QIStream &in, Skeleton *skelet,const char *name, bool reversed);
  /// check how well does the animation match the skeleton
  static int Match(QIStream &in, Skeleton *skelet,const char *name);
  //! load full data (usually after AddLoadCount is called)
  bool FullLoad(Skeleton *skelet, const char *file, bool sync);
  //! release full data - keep only minimal representation
  void FullRelease();
  
  static bool Preload(FileRequestPriority priority, const char *name, bool fullLoad);

  /// animation streaming - load phases around the given time
  void LoadNeededPhases(AnimationRTBufferCopyOnWrite &buffer, float time, float timePastNeeded, const Skeleton *sk) const;

  bool CheckValidState(const AnimationRTBuffer &buffer) const {return _phasesBuffer.CheckBufferValid(buffer);}
  //! increase load count - make animation loaded
  int AddLoadCount(bool sync=false) const;
  //! release load count - let animation be freed
  void ReleaseLoadCount() const;

  double GetMemoryUsed() const;

  void LoadPhases(
    QIStream &in, Skeleton *skelet, int nSel, int startAnim, int endAnim
  );
  void Load(
    Skeleton *skelet,
    const char *file, bool reversed=true
  );
  void Reverse();
  
  // skeleton included in f context
  void SerializeBin(Skeleton *skelet, SerializeBinStream &f);

  /// binary loading
  bool LoadOptimized(Skeleton *skelet, const char *name);
  bool LoadOptimized(Skeleton *skelet, QIStream &in);
  
  /// binary saving
  void SaveOptimized(Skeleton *skelet, const char *name);

  static void TransformPoint(
    Vector3 &pos,
    const Shape *shape, Matrix4Par trans,
    const BlendAnimInfo *blend, int nBlend, int index
  );
  static void CombineTransform(
    LODShape *lShape, int level,
    Matrix4Array &matrices, Matrix4Par trans,
    const BlendAnimInfo *blend, int nBlend
  );
  // Returns the transformed matrix specified
  Matrix4 GetMatrix(float time, SkeletonIndex si) const;
  /// get matrices based on time and factor
  void PrepareMatrices(
    Matrix4Array &matrices, float time, float factor, const Shape *shape, const Skeleton *skeleton
  ) const;

  static void PrepareMatrices(Matrix4Array &matrices, const BlendAnims &blend, const Shape *shape, const Skeleton *skeleton);
  static void ApplyMatrices(AnimationContext &animContext, LODShape *lShape, int level, const Matrix4Array &matrices);
  static Vector3 ApplyMatricesPoint(
    AnimationContext &animContext, LODShape *lShape, int level,
    const Matrix4Array &matrices, int pointIndex
  );
  
  /// find which matrix corresponds to a given bone
  int MapBone(SkeletonIndex bone) const {return _boneMap[GetSkeletonIndex(bone)];}

  // mesh animation
  void Apply(AnimationContext &animContext, LODShape *lShape, int level, float time) const;
  /// get matrix
  void Matrix(Matrix4 &mat, float time, SkeletonIndex boneIndex) const;

  void GetBlend(
    BlendAnims &blend, float time, float factor=1.0f, float maskFactor=1,
    const BlendAnimSelections *sel=NULL, const AnimationRTBuffer *buffer=NULL,
    const class MoveInfoBase *moveInfo=NULL) const;
  const AnimationRTName &GetName() const {return _name;}
  const char *Name() const {return _name.name;}
  float GetStepLength() const {return _step.Z();}
  float GetStepLengthX() const {return _step.X();}
  Vector3Val GetStep() const {return _step;}
  // obsolete, see GetFrameCount()
  int GetOriginalKeyframeCount() const {return _nPhases;}
  int GetBonesAffected() const;
  int GetMemoryControlled() const;

  const Skeleton *GetSkeleton() const {return _name.skeleton;}
  
  //! PrepareSkeleton and SetLooped - obsolete
  void Prepare(bool looped);
  //! set looping - may be called before or after loading
  void RemoveLoopFrame();

  /// check if all animation keyframes are identical
  bool CheckStatic() const;

  void SetLooped( bool looped );
  bool GetLooped() const {return (_looped==LoopedTrue);};
  bool LoopNotInitialized() const {return (_looped==LoopedNotInit);};

  // get correction for the given head bob mode, note that this method eventually lazy initialize corresponding matrices stored in _headBobCorrection
  Matrix4 GetHeadBobCorrection(HeadBobModeEnum mode, float animTime, const class AnimationRTLock &lock, const Skeleton *skeleton, SkeletonIndex boneIndex) const;

  //@{ access to key-frames
  // number of frames in the animation
  int GetFrameCount() const {return 1 + _phasesBuffer._buffer._phases.Size();}
  // get i-th frame of the animation
  const AnimationRTPhase &GetFrame(int i) const;
  //@}

  USE_FAST_ALLOCATOR
};

#define ANIMATION_RT_SLIST 1

#if ANIMATION_RT_SLIST
void PerformAnimationRTLoading();
#endif

/// make sure an individual AnimationRT is loaded
class AnimationRTLock
{
#if ANIMATION_RT_SLIST
protected:
  /// the locked animation
  AnimationRT *_anim;

public:
  AnimationRTLock() : _anim(NULL) {}

  explicit AnimationRTLock(AnimationRT &lock);
  ~AnimationRTLock() {}
  AnimationRTLock(const AnimationRTLock &src) : _anim(src._anim) {}
  void operator = (const AnimationRTLock &src) {_anim = src._anim;}

  const AnimationRT *operator ->() const {return _anim;}

#else
  protected:
  /// animation which was locked
  const AnimationRT *_lock;
  /// lock may have failed, store the animation for which it was attempted so that we can access it
  const AnimationRT *_anim;

  public:
  AnimationRTLock():_lock(NULL),_anim(NULL){}

  explicit AnimationRTLock( const AnimationRT &lock )
  :_anim(&lock)
  {
    if (lock.AddLoadCount()>0)
    {
      _lock=&lock;
    }
    else
    {
      _lock = NULL;
    }
  }
  ~AnimationRTLock()
  {
    if (_lock) _lock->ReleaseLoadCount();
  }
  AnimationRTLock( const AnimationRTLock &src ):_lock(src._lock),_anim(src._anim) {if (_lock) _lock->AddLoadCount();}
  void operator = ( const AnimationRTLock &src )
  {
    if (src._lock) src._lock->AddLoadCount();
    if (_lock) _lock->ReleaseLoadCount();
    _lock = src._lock;
    _anim = src._anim;
  }
  
  const AnimationRT *operator ->() const {return _anim;}
#endif
};


/// information about one AnimationRTPhase being blended 
struct AnimBlendInfo
{
  /// make sure given animation is loaded, provides access to the animation as well
  AnimationRTLock _lock;
  /// animation data - current keyframe
  const AnimationRTPhase *_phase_p;
  const AnimationRTPhase *_phase_n;
  /// blending factor between _phase_p and _phase_n
  float _p_factor;
  /// blending factor used when blending the result against other animations
  float _b_factor;
  /// optional reference to a blending mask
  /** when provided, lerp to all previous animations is done based on this mask */
  const BlendAnimSelections *_mask;
  /// _mask is multiplied by _maskFactor
  float _maskFactor;
  /// time of the current keyframe in the animation
  float _timeInAnimation;
  /// optional reference to a corresponding MoveInfoMan (needed for head bob handling)
  const MoveInfoBase *_moveInfo;

#if USER_jana1
  /// ragdoll for anim blending (if null, then no ragdoll is active)
  void* m_ragdoll;
  /// inverse model to world transform - needed for ragdoll
  Matrix4 m_inverseModelToWorld;

	AnimBlendInfo(void* ragdoll, const Matrix4& inverseModelToWorld, float factor, const BlendAnimSelections *mask)
    :_phase_p(0),_phase_n(0),_p_factor(0),_b_factor(factor),_timeInAnimation(0),
    _mask(mask), _moveInfo(0), m_ragdoll(ragdoll), m_inverseModelToWorld(inverseModelToWorld)
  {}
#endif

	AnimBlendInfo(){}
  AnimBlendInfo(const AnimationRTLock &lock, const AnimationRTPhase *phase_p, const AnimationRTPhase *phase_n, float p_factor, float b_factor, float timeInAnimation,
    const BlendAnimSelections *mask, float maskFactor, const MoveInfoBase *moveInfo=NULL)
  :_lock(lock),_phase_p(phase_p),_phase_n(phase_n),_p_factor(p_factor),_b_factor(b_factor),_timeInAnimation(timeInAnimation),
    _mask(mask),_maskFactor(maskFactor),_moveInfo(moveInfo)
  {
    Assert(_phase_p);
    Assert(p_factor>=0 && p_factor<=1);
    Assert(b_factor>=0 && b_factor<=1);
    Assert(timeInAnimation>=0 && timeInAnimation<=1);
  }

  __forceinline const AnimationRTMatrix *GetBoneP(SkeletonIndex bone) const
  {
    int index = _lock->MapBone(bone);
    if (index<0) return &AnimationRTMatrixIdentity;
    return &_phase_p->GetBone(index);
  };
  __forceinline const AnimationRTMatrix *GetBoneN(SkeletonIndex bone) const
  {
    if (!_phase_n)
      return NULL;
    int index = _lock->MapBone(bone);
    if (index<0) return &AnimationRTMatrixIdentity;
    return &_phase_n->GetBone(index);
};
};

TypeIsMovableZeroed(AnimBlendInfo)

/// blending information for blending several AnimationRTPhase together
/**
May include animation blending as well as blending between keyframes
*/

struct BlendAnims
{
  typedef AutoArray<AnimBlendInfo, MemAllocLocal<AnimBlendInfo,8> > TAnimBlendInfoArray;
  /// list of animations to blend together
  TAnimBlendInfoArray _list;
  /// list of animations to blend over (masked)
  /**
  Currently only one layer is supported, the mask needs to be shared by all items.
  The items are blended together and lerped over _list, respecting mask
  
  TODO: more flexible store structure, allowing for multiple layers
  */
  TAnimBlendInfoArray _masked;

#if USER_jana1
	/// list of ragdoll animations to blend
	TAnimBlendInfoArray	m_ragdollAnims;

	void InterpolateMatrixRagdoll( Matrix4 &mat, SkeletonIndex bone, const TAnimBlendInfoArray &list) const;
  void InterpolateQuaternionRagdoll( Quaternion<float> &q, Vector3 &pos,const Vector3 &pivot,SkeletonIndex bone, const TAnimBlendInfoArray &list) const;
#endif

  void InterpolateMatrix( Matrix4 &mat, SkeletonIndex bone, const TAnimBlendInfoArray &list,float s ) const;
  void InterpolateQuaternion( Quaternion<float> &mat, Vector3 &pos,const Vector3 &pivot,SkeletonIndex bone,const TAnimBlendInfoArray &list) const;

  /// get a local animation matrix (blended)
  void GetAnim(Matrix4 &mat, SkeletonIndex bone, const Skeleton *sk) const;
  /// animate a matrix by a blended animation
  void DoBoneAnim(Matrix4 &mat, const Skeleton *sk, SkeletonIndex boneIndex) const;

  /// animate a matrix by a blended animation based on a blending info
  void DoBoneAnim(Matrix4 &mat, const Skeleton *sk, Shape *shape, const AnimationRTWeight &wgt) const;

  /// animate a point by a blended animation based on a blending info
  void DoPointAnim(Vector3 &pos, const Skeleton *sk, const Shape *shape, const AnimationRTWeight &wgt) const;
};


#include <Es/Memory/debugNew.hpp>

#endif
