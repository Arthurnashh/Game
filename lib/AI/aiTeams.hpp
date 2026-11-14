#ifdef _MSC_VER
#pragma once
#endif

#ifndef _AI_TEAMS_HPP
#define _AI_TEAMS_HPP

#include <El/Evaluator/expressTypes.hpp>

#if _ENABLE_IDENTITIES || _ENABLE_INDEPENDENT_AGENTS
typedef RString AITaskResource;
#endif

#if _ENABLE_IDENTITIES

class AITaskType;
class AITask;

/// Info about registered AITaskType (including counting of instances)
struct AITaskTypeInfo
{
  RefR<AITaskType> _type;
  int _created;

  AITaskTypeInfo() {_created = 0;}
  AITaskTypeInfo(AITaskType *type);
  LSError Serialize(ParamArchive &ar);
};
TypeIsMovableZeroed(AITaskTypeInfo);

/// Implementation of FindArrayKey<AITaskTypeInfo>
template <>
struct FindArrayKeyTraits<AITaskTypeInfo>
{
  typedef const char *KeyType;
  static bool IsEqual(const char *a, const char *b)
  {
    return !strcmpi(a, b);
  }
  static const char *GetKey(const AITaskTypeInfo &a);
};

#endif

#if _ENABLE_INDEPENDENT_AGENTS

class AITeamMember;
class AITeam;
class AIAgent;

typedef FindArrayTraitsCI AITaskResourceFindArrayTraits;

struct AITeamMemberRefTraits;

/// Reference to a class AITeamMember
class AITeamMemberRef : public RefR<AITeamMember, AITeamMember, AITeamMemberRefTraits>
{
  typedef RefR<AITeamMember, AITeamMember, AITeamMemberRefTraits> base;

public:
  /// Default constructor.
  __forceinline AITeamMemberRef() : base() {}
  /// Using this constructor you can get the existing reference from source parameter.
  AITeamMemberRef(AITeamMember *source) : base(source)
  {
  }
  /// Copy constructor.
  AITeamMemberRef(const AITeamMemberRef &sourceRef) : base(sourceRef) {}

  /// Serialization
  LSError Serialize(ParamArchive &ar);
};
TypeIsMovableZeroed(AITeamMemberRef);

class AITeamMemberLink;

/// AITeamMember - interface to a node of the new AI hierarchy - implemented in class AIAgent and class AITeam
class AITeamMember : public CountInstances<AITeamMember>
{
protected:
#if _ENABLE_IDENTITIES
  /// list of tasks
  AutoArray< RefR<AITask> > _tasks;
  /// supported types of tasks
  FindArrayKey<AITaskTypeInfo> _taskTypes;
  /// id of the next task
  int _nextTaskId;
#endif
  /// list of available resources
  FindArrayKey<AITaskResource, AITaskResourceFindArrayTraits> _resources;

  bool _fromEditor;

public:
  AITeamMember()
  {
    _fromEditor = false;
#if _ENABLE_IDENTITIES
    _nextTaskId = 0;
#endif
  }
  virtual ~AITeamMember();

  bool IsFromEditor() const {return _fromEditor;}
  void SetFromEditor(bool set) {_fromEditor = set;}

  //@{ Identification for radio protocol
  RString GetCallSignShort() const;
  RString GetCallSign() const;
  RString GetCallSignLong() const;
  template <typename Allocator>
  bool GetCallSignShortSpeech(AutoArray<RString, Allocator> &speech) const
  {
    const GameVarSpace *vars = GetVars();
    if (!vars) return false;

    GameValue value;
    if (!vars->VarGet("callSignShortSpeech", value)) return false;
    if (value.GetType() != GameArray) return false;
    const GameArrayType &array = value;
    for (int i=0; i<array.Size(); i++) speech.Add(array[i]);
    return true;
  }
  template <typename Allocator>
  bool GetCallSignSpeech(AutoArray<RString, Allocator> &speech) const
  {
    const GameVarSpace *vars = GetVars();
    if (!vars) return false;

    GameValue value;
    if (!vars->VarGet("callSignSpeech", value)) return false;
    if (value.GetType() != GameArray) return false;
    const GameArrayType &array = value;
    for (int i=0; i<array.Size(); i++) speech.Add(array[i]);
    return true;
  }
  //@}

  //@{ Reference counting
  virtual int IAddRef() = 0;
  virtual int IRelease() = 0;
  //@}

  //@{ Soft link support
  /// id type
  typedef ObjectId LinkId;
  typedef ObjectIdAutoDestroy LinkIdStorage;
  typedef SoftLinkIdTraits<LinkId, LinkIdStorage> IdTraits;

  /// Get unique ID (for unloading purposed)
  virtual LinkId GetLinkId() const = 0;

  virtual bool NeedsRestoring() const {return false;}

  virtual TrackSoftLinks<IdTraits> *ITrack() = 0;
  virtual RemoveSoftLinks<IdTraits> *GetRemoveLinks() = 0;
  //@}

  virtual void ApplyContext(GameState &state) = 0;
  virtual RString GetDebugName() const {return RString();}

  /// Direct access to inherited AITeam
  virtual AITeam *GetTeam() {return NULL;}
  /// Direct access to inherited AIAgent
  virtual AIAgent *GetAgent() {return NULL;}

  /// Basic commanding level simulation
  virtual void SimulateAI();

#if _ENABLE_IDENTITIES
  /// Registration of task type
  bool RegisterTaskType(AITaskType *taskType);
  /// Unregistration of task type
  bool UnregisterTaskType(RString name);
  /// Retrieving of task type
  const AITaskType *GetTaskType(RString name) const;

  /// Task creation
  int AddTask(AITask *task);
  /// Searching for task by its id
  AITask *FindTask(int id);

  /// Return number of tasks
  int NTasks() const {return _tasks.Size();}
  /// Return task with given index
  AITask *GetTask(int index);

  /// Return number of task types
  int NTaskTypes() const {return _taskTypes.Size();}
  /// Return task type with given index
  const AITaskType *GetTaskType(int index);
#endif

  /// Access to local variable space
  virtual GameVarSpace *GetVars() = 0;
  virtual const GameVarSpace *GetVars() const = 0;

  /// Register available resource
  int AddResource(const AITaskResource &resource) {return _resources.Add(resource);}
  /// Unregister available resource
  bool DeleteResource(const AITaskResource &resource) {return _resources.Delete(resource);}
  /// Return list of available resources
  const FindArrayKey<AITaskResource, AITaskResourceFindArrayTraits> &GetResources() const {return _resources;}

  /// Send radio message to an owned radio channel 
  void SendMsg(RadioMessage *message);

  //@{ Serialization support
  static AITeamMember *CreateObject(ParamArchive &ar);
  virtual LSError Serialize(ParamArchive &ar);
  virtual LSError SaveTeamMemberRef(ParamArchive &ar) = 0;
  static AITeamMember *LoadTeamMemberRef(ParamArchive &ar);
  static LSError SaveLinkRef(ParamArchive &ar, AITeamMemberLink &link);
  static LSError LoadLinkRef(ParamArchive &ar, AITeamMemberLink &link);
  //@}
};

struct AITeamMemberRefTraits
{
  static __forceinline int AddRef(AITeamMember *ptr) {return ptr->IAddRef();}
  static __forceinline int Release(AITeamMember *ptr) {return ptr->IRelease();}
};

LSError SaveRefT(AITeamMemberRef &value, ParamArchive &arRef);
LSError LoadRefT(AITeamMemberRef &value, ParamArchive &arRef);

/// Implementation of link to a class AITeamMember
struct AITeamMemberLinkTraits
{
  typedef AITeamMember::LinkId LinkId;
  typedef AITeamMember::LinkIdStorage LinkIdStorage;
  typedef SoftLinkIdTraits<LinkId,LinkIdStorage> IdTraits;

  typedef AITeamMember BaseType;
  typedef TrackSoftLinks<IdTraits> Tracker;

/*
  // no restoring & locking
  static AITeamMember *RestoreLink(const LinkId &id)
  {
    return AITeamMember::RestoreLink(id);
  }
  static bool RequestRestoreLink(const LinkId &id, bool noRequest=false)
  {
    return AITeamMember::RequestRestoreLink(id,noRequest);
  }
  static void LockLink(AITeamMember *obj) {obj->LockLink();}
  static void UnlockLink(AITeamMember *obj) {obj->UnlockLink();}
*/

  static LinkId GetLinkId(const AITeamMember *obj) {return obj->GetLinkId();}
  static LinkId GetNullId() {return IdTraits::GetNullId();}

  static bool NeedsRestoring(const AITeamMember *obj) {return obj->NeedsRestoring();}
  static bool IsAlwaysLocked() {return false;}

  // Support for links to non-primary interface
  static __forceinline TrackSoftLinks<IdTraits> *Track(AITeamMember *ref) {return ref->ITrack();}
  static __forceinline RemoveSoftLinks<IdTraits> *GetRemoveLinks(AITeamMember *ref) {return ref->GetRemoveLinks();}
  static AITeamMember *GetObject(TrackSoftLinks<IdTraits> *tracker);
};

/// Link to a class AITeamMember
class AITeamMemberLink : public OLinkPerm<AITeamMember, AITeamMemberLinkTraits>
{
  typedef OLinkPerm<AITeamMember, AITeamMemberLinkTraits> base;

public:
  AITeamMemberLink() {}
  AITeamMemberLink(AITeamMember *ref) : base(ref) {}
};

LSError SaveRefT(AITeamMemberLink &value, ParamArchive &arRef);
LSError LoadRefT(AITeamMemberLink &value, ParamArchive &arRef);

/// Implementation of array of references to AITeamMember
template <>
struct FindArrayKeyTraits<AITeamMemberRef>
{
  typedef AITeamMember *KeyType;
  static bool IsEqual(KeyType a, KeyType b) {return a == b;}
  static KeyType GetKey(const AITeamMemberRef &a) {return a.GetRef();}
};

/// Implementation of array of links to AITeamMember
template <>
struct FindArrayKeyTraits<AITeamMemberLink>
{
  typedef AITeamMember *KeyType;
  static bool IsEqual(KeyType a, KeyType b) {return a == b;}
  static KeyType GetKey(const AITeamMemberLink &a) {return a.GetLink();}
};

#endif

#endif

