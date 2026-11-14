#ifdef _MSC_VER
#pragma once
#endif

#ifndef _IDENTITY_HPP
#define _IDENTITY_HPP

#if _ENABLE_IDENTITIES

#include <time.h>
#include <Es/Types/lLinks.hpp>
#include <El/HierWalker/hierWalker.hpp>
#include <El/ParamArchive/serializeClass.hpp>
#include <El/Evaluator/expressImpl.hpp>

#include "AI/abilities.hpp"
#include "Network/networkObject.hpp"

class FSMScripted;
class FSMScriptedType;
class AITeamMember;
class GameVarSpace;
class AIBrain;
class ParamEntry;

FSMScripted *CreateTeamMemberFSMScripted(const FSMScriptedType *type, GameVarSpace *parentVars);

// TODO: move somewhere to AI

#define TASK_STATE_ENUM(type, prefix, XX) \
  XX(type, prefix, None) \
  XX(type, prefix, Created) \
  XX(type, prefix, Assigned) \
  XX(type, prefix, Succeeded) \
  XX(type, prefix, Failed) \
  XX(type, prefix, Canceled)

#ifndef DECL_ENUM_TASK_STATE
#define DECL_ENUM_TASK_STATE
DECL_ENUM(TaskState)
#endif
DECLARE_ENUM(TaskState, TS, TASK_STATE_ENUM);

/// Abstract task implementation, for real implementation, see class AITask
class Task : public RemoveLLinks
{
protected:
  /// parent task
  LLink<Task> _parent;
  /// list of all subtasks
  LLinkArray<Task> _subtasks;
  /// identifier used for links from diary
  int _id;
  /// name of the task
  RString _name;
  /// state of the task
  TaskState _state;
  /// result of the task
  // Remark: move out of task to avoid cyclic reference and so the memory leaks
  GameValue _result;

public:
  Task() {_id = -1; _state = TSCreated;} // used for serialization
  Task(Task *parent, RString name);

  /// access to parent task
  Task *GetParent() {return _parent;}
  /// access to parent task
  const Task *GetParent() const {return _parent;}
  /// number of subtasks
  int NSubtasks() const {return _subtasks.Size();}
  /// access to subtask
  Task *GetSubtask(int i) {return _subtasks[i];}
  /// access to subtask
  const Task *GetSubtask(int i) const {return _subtasks[i];}
  /// access to identifier
  int GetId() const {return _id;}
  /// set the identifier
  void SetId(int id) {_id = id;}
  /// access to name
  RString GetName() const {return _name;}
  /// access to the current task state
  TaskState GetState() const {return _state;}
  /// change the current task state
  void SetState(TaskState state) {_state = state;}
  /// access to the task result
  GameValue GetResult() const {return _result;}
  /// set the task result
  void SetResult(GameValuePar result) {_result = result;}
  /// check if task is completed (by its state)
  bool IsCompleted() const
  {
    switch (_state)
    {
    case TSSucceeded:
    case TSFailed:
    case TSCanceled:
      return true;
    }
    return false;
  }
  /// add a new subtask
  void AddSubtask(Task *task);

  /// return the task priority (implemented in AITask)
  virtual float GetPriority() const {return 0;}
  /// return the task description (implemented in AITask)
  virtual bool GetDescription(RString &result) const {return false;}
  /// return the task description (implemented in AITask)
  virtual bool GetDescriptionShort(RString &result) const {return false;}
  /// return the task description (implemented in AITask)
  virtual bool GetDescriptionHUD(RString &result) const {return false;}
  /// return the task destination (implemented in AITask)
  virtual bool GetDestination(Vector3 &result) const {return false;}

  virtual void ChangeOwner(const Person *originalOwner, Person *newOwner) {}

  /// serialization
  virtual LSError Serialize(ParamArchive &ar);
  /// serialization of reference
  virtual LSError SaveRef(ParamArchive &ar);

  /// serialization of reference
  static LSError LoadRefRef(ParamArchive &ar, Ref<Task> &link)
  {
    link = Task::LoadRef(ar);
    return (LSError)0;
  }
  /// serialization of reference
  static LSError SaveRefRef(ParamArchive &ar, Ref<Task> &link)
  {
    return link->SaveRef(ar);
  }
  /// serialization of reference
  static LSError LoadLLinkRef(ParamArchive &ar, LLink<Task> &link)
  {
    link = Task::LoadRef(ar);
    return (LSError)0;
  }
  /// serialization of reference
  static LSError SaveLLinkRef(ParamArchive &ar, LLink<Task> &link)
  {
    return link->SaveRef(ar);
  }

  DECL_SERIALIZE_TYPE_REF_INFO_ROOT_ONLY(Task);
};

/// Single diary record
class DiaryRecord : public RemoveLLinks
{
protected:
  /// the unique id of the record (for serialization)
  int _id;
  /// when the record was created
  tm _time;
  /// the title of the record
  RString _title;
  /// the content of the record
  RString _text;
  /// related task
  LLink<Task> _task;
  /// state of the related task
  TaskState _state;
  /// attached person (sort by)
  OLinkPermNO(AIBrain) _unit;
  /// for units which vanished, store name for conversation history
  RString _personName;
  /// attached target (for example the speaker)
  OLinkPermNO(AIBrain) _target;
  /// for targets which vanished, store name for conversation history
  RString _targetName;
  /// record is read only, even on the read-write page
  bool _readOnly;

public:
  DiaryRecord(ForSerializationOnly)
  {
    _id = -1;
    _state = TSNone;
    _readOnly = false;
  }
  DiaryRecord(const tm &time, RString title, RString text, Task *task, TaskState state, AIBrain *unit, AIBrain *target, bool readOnly);
  int GetId() const {return _id;}
  void SetId(int id) {_id = id;}
  bool IsReadOnly() const {return _readOnly;}
  /// return the record creation time in readable form
  RString FormatDate(const char *format) const;
  /// change the record title
  void SetTitle(RString title) {_title = title;}
  /// return the record title
  RString GetTitle() const {return _title;}
  /// change the record content
  void SetText(RString text) {_text = text;}
  /// return the record content
  RString GetText() const {return _text;}
  /// return the related task
  const Task *GetTask() const {return _task;}
  /// return the record creation time
  const tm &GetDate() const {return _time;}
  /// return the attached unit
  const AIBrain *GetUnit() const {return _unit;}
  /// return the name of attached unit
  RString GetPersonName() const;
  /// return the attached target
  const AIBrain *GetTarget() const {return _target;}
  /// return the name of attached target
  RString GetTargetName() const;

  static DiaryRecord *CreateObject(ParamArchive &ar) {return new DiaryRecord(SerializeConstructor);}
  LSError Serialize(ParamArchive &ar);
};

enum DiarySortOrder
{
  DSONone,
  DSODate,
  DSOTask,
  DSOUnit
};

/// page containing single subject
class DiaryPage : public RefCount
{
protected:
  /// internal name
  RString _name;
  /// user friendly name
  RString _displayName;
  /// UI picture
  RString _picture;
  /// list of shortcuts in the page selection screen
  AutoArray<int> _shortcuts;

  /// template for the index title format
  RString _indexTitle;
  /// template for the index icon format
  RString _indexIcon;
  /// template for the record title format
  RString _recordTitle;
  /// template for the record text format
  RString _recordText;
  /// true if empty diary page should be listed
  bool _showEmpty;

  /// records
  RefArray<DiaryRecord> _log;
  /// the id of next created record
  int _nextRecordId;
  /// dirty flag
  mutable bool _changed;
  /// sorting index
  mutable AutoArray<int> _indices;

public:
  DiaryPage(RString name, RString displayName, RString picture);
  DiaryPage(ForSerializationOnly) {_changed = false; _nextRecordId = 0;}

  /// load the properties from the config
  virtual void Load(const ParamEntry &cls);

  RString GetName() const {return _name;}
  RString GetDisplayName() const {return _displayName;}
  RString GetPicture() const {return _picture;}
  const AutoArray<int> &GetShortcuts() const {return _shortcuts;}

  /// manually set the dirty flag (used in CopyIdentity)
  void SetChanged() {_changed = true;}

  /// check if page needs update
  virtual bool IsChanged() const {return _changed;}
  /// check if changes of page are allowed
  virtual bool IsReadOnly() const {return false;}
  /// return default sort order
  virtual DiarySortOrder GetSortOrder() const {return DSODate;}
  /// title of the record in the diary index
  RString GetIndexTitle(const DiaryRecord *record) const;
  /// icon of the record in the diary index
  RString GetIndexIcon(const DiaryRecord *record) const;
  /// title of the record in the diary content
  RString GetRecordTitle(const DiaryRecord *record) const;
  /// text of the record in the diary content
  RString GetRecordText(const DiaryRecord *record) const;
  /// some arguments can be handled only by the special page
  virtual RString HandleSpecialArgument(RString name, const DiaryRecord *record) const;

  DiaryRecord *AddRecord(RString title, RString text, Task *task = NULL, TaskState state = TSNone, AIBrain *unit = NULL, AIBrain *target = NULL, bool readOnly = true);
  /// set a new text to the existing record
  void EditRecord(int id, RString title, RString text);
  /// delete a diary record
  void DeleteRecord(int id);
  /// find the record by its id
  DiaryRecord *FindRecord(int id) const;
  /// number of records
  int NRecords() const {return _log.Size();}
  /// access to records
  const DiaryRecord *GetRecord(int index) const {return _log[index];}

  template <class Functor>
  bool ForEachRecord(const Functor &func, const Person *person)
  {
    if (IsChanged()) Update(person);
    for (int i=0; i<_indices.Size(); i++)
    {
      int index = _indices[i];
      if (func(index, _log[index])) return true;
    }
    return false;
  }

  LSError Serialize(ParamArchive &ar);

  // enable dynamic serialization
  DECL_SERIALIZE_TYPE_INFO_ROOT(DiaryPage);
  // instances of DiaryPage are valid
  DECL_SERIALIZE_TYPE_INFO(DiaryPage, DiaryPage);

  /// simulation type of the page (class name)
  RString GetPageType() const {return GetDiaryPageTypeInfo()->_typeName;}

  /// refresh the content
  virtual void Update(const Person *person);

  bool ShowEmpty() {return _showEmpty;}
};

/// Whole diary
class Diary : public SerializeClass
{
protected:
  /// pages for subjects
  RefArray<DiaryPage> _pages;
  /// number of fixed subjects
  int _fixedSubjects;
  /// what page is currently opened in the UI
  RString _currentSubject;
  /// dirty flag
  mutable bool _changed;

public:
  Diary() {Init();}
  /// insert all fixed pages
  void Init();
  /// insert all fixed pages
  void Update() {_changed = false;}
  /// manually set the dirty flag both for the diary and all pages (used in CopyIdentity)
  void SetChanged();

  /// add a page for a new subject
  int AddSubject(RString subject, RString displayName, RString picture);

  /// check if list of pages needs update
  bool IsChanged() const {return _changed;}
  /// check if page needs update
  bool IsChanged(RString subject) const;
  /// check if changes of page are allowed
  bool IsReadOnly(RString subject) const;
  /// add a new record to the page
  DiaryRecord *AddRecord(RString subject, RString title, RString text, Task *task = NULL, TaskState state = TSNone, AIBrain *unit = NULL, AIBrain *target = NULL, bool readOnly = true);
  /// set a new text to the existing record
  void EditRecord(RString subject, int id, RString title, RString text);
  /// delete a diary record
  void DeleteRecord(RString subject, int id);

  int NSubjects() const {return _pages.Size();}
  const DiaryPage *GetSubject(int i) const {return _pages[i];}
  DiaryPage *GetSubject(int i) {return _pages[i];}

  /// What page is currently selected in the UI
  RString GetCurrentSubject() const {return _currentSubject;}
  /// Select page in the UI
  void SetCurrentSubject(RString subject) {_currentSubject = subject;}

  template <class Functor>
  bool ForEachRecord(RString subject, const Functor &func, const Person *person) const
  {
    const DiaryPage *page = FindSubject(subject);
    if (!page) return false;
    return page->ForEachRecord(func, person);
  }

  const DiaryPage *FindSubject(RString subject) const
  {
    int index = FindSubjectIndex(subject);
    if (index < 0) return NULL;
    return _pages[index];
  }
  DiaryPage *FindSubject(RString subject)
  {
    int index = FindSubjectIndex(subject);
    if (index < 0) return NULL;
    return _pages[index];
  }

  LSError Serialize(ParamArchive &ar);

protected:
  int FindSubjectIndex(RString subject) const;
};

class AIBrain;
class KBMessageInfo;
class KBMessage;

/// one item in the history
struct HistoryItem
{
  RString _topic;
  RString _sentenceId;
  Time _time;

  HistoryItem(){}
  HistoryItem(const KBMessageInfo *info, Time time);

  LSError Serialize(ParamArchive &ar);
};

TypeIsMovableZeroed(HistoryItem)

/// remember what you have said to one unit
struct ConversationWith
{
  // who was this told to
  int _objId;
  // what was told
  AutoArray<HistoryItem> _history;

  void Add(const KBMessageInfo *msg, Time time) {_history.Add(HistoryItem(msg,time));}

  LSError Serialize(ParamArchive &ar);
};

TypeContainsOLink(ConversationWith)

template <>
struct MapClassTraits<ConversationWith>: public DefMapClassTraits<ConversationWith>
{
  typedef int KeyType;
  typedef ConversationWith Type;

  static unsigned int CalculateHashValue(KeyType key);

  static int CmpKey(KeyType k1, KeyType k2) 
  {
    return (k1 != k2);
  }
  
  static KeyType GetKey(const Type &item);
};

/// Identity of the person
class Identity : public SerializeClass
{
protected:
  /// List of fixed (program handled) skills
  float _skills[NAbilityKind];
  /// Whole diary
  Diary _diary;

  /// The list of simple tasks used in the diary
  RefArray<Task> _simpleTasks;
  /// The current task (shown for example in HUD)
  LLink<Task> _currentTask;
  /// id of the next created task
  int _nextTaskId;

  MapStringToClass<ConversationWith, AutoArray<ConversationWith> > _said;

  /// User defined mark in the map and HUD
  bool _customMarkSet;
  /// User defined mark in the map and HUD
  Vector3 _customMarkPos;

public:
  Identity();

  /// Get position of the custom mark
  bool GetCustomMark(Vector3 &pos) const {pos = _customMarkPos; return _customMarkSet;}
  /// Set position of the custom mark
  void SetCustomMark(Vector3Par pos) {_customMarkPos = pos; _customMarkSet = true;}
  /// Cancel the custom mark
  void ClearCustomMark() {_customMarkSet = false;}

  int NDiarySubjects() const
  {
    return _diary.NSubjects();
  }
  const DiaryPage *GetDiarySubject(int i) const
  {
    return _diary.GetSubject(i);
  }
  DiaryPage *GetDiarySubject(int i)
  {
    return _diary.GetSubject(i);
  }
  const DiaryPage *FindDiarySubject(RString subject) const
  {
    return _diary.FindSubject(subject);
  }
  DiaryPage *FindDiarySubject(RString subject)
  {
    return _diary.FindSubject(subject);
  }
  bool IsDiaryChanged() const
  {
    return _diary.IsChanged();
  }
  bool IsDiaryChanged(RString subject) const
  {
    return _diary.IsChanged(subject);
  }
  bool IsDiaryReadOnly(RString subject) const
  {
    return _diary.IsReadOnly(subject);
  }
  void UpdateDiary()
  {
    _diary.Update();
  }
  void SetDiaryChanged()
  {
    _diary.SetChanged();
  }
  template <class Functor>
  bool ForEachDiaryRecord(RString subject, const Functor &func, const Person *person)
  {
    return _diary.ForEachRecord(subject, func, person);
  }

  int AddDiarySubject(RString subject, RString displayName, RString picture)
  {
    return _diary.AddSubject(subject, displayName, picture);
  }
  DiaryRecord *AddDiaryRecord(RString subject, RString title, RString text, Task *task = NULL, TaskState state = TSNone, AIBrain *unit = NULL, AIBrain *target = NULL, bool readOnly = true)
  {
    return _diary.AddRecord(subject, title, text, task, state, unit, target, readOnly);
  }
  void EditDiaryRecord(RString subject, int id, RString title, RString text)
  {
    return _diary.EditRecord(subject, id, title, text);
  }
  void DeleteDiaryRecord(RString subject, int id)
  {
    return _diary.DeleteRecord(subject, id);
  }

  /// What page is currently selected in the diary UI
  RString GetCurrentDiarySubject() const {return _diary.GetCurrentSubject();}
  /// Select page in the diary UI
  void SetCurrentDiarySubject(RString subject) {_diary.SetCurrentSubject(subject);}

  /// Access to the list of simple tasks
  const RefArray<Task> &GetSimpleTasks() const {return _simpleTasks;}
  /// Task creation
  int AddTask(Task *task);
  /// Task deletion
  bool DeleteTask(Task *task);
  /// Searching for task by its id
  Task *FindTask(int id);
  /// Mark task as current 
  void SetCurrentTask(Task *task) {_currentTask = task;}
  /// Return the current task
  Task *GetCurrentTask() const {return _currentTask;}

  /// remember what we have said
  void AddSaid(AIBrain *to, const KBMessageInfo *question);
  /// check if given sentence was said within a given time-frame
  bool WasSaid(AIBrain *to, RString topic, RString sentenceId, float maxAge) const;

  float GetRawAbility(AbilityKind kind) const {return _skills[kind];}
  void SetRawAbility(AbilityKind kind, float ability)
  {
    Assert(ability > 0);
    _skills[kind] = ability;
  }
  void SetAllAbilities(float ability);

  LSError Serialize(ParamArchive &ar);
};

GameValue CreateGameTask(Task *task);

#endif // _ENABLE_IDENTITIES

#endif
