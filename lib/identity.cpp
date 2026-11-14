#include "wpch.hpp"

#if _ENABLE_IDENTITIES

#include "identity.hpp"

#include <Es/Algorithms/qsort.hpp>

#include <El/Evaluator/express.hpp>
#include <El/XML/xml.hpp>
#include "paramArchiveExt.hpp"

#include "global.hpp"
#include "gameStateExt.hpp"

#include "AI/fsmScripted.hpp"
#include "AI/aiRadio.hpp"
#include "AI/ai.hpp"
#include "Network/network.hpp"

#include "stringtableExt.hpp"
 
/// 
unsigned int MapClassTraits<ConversationWith>::CalculateHashValue(KeyType key) {return 233 * key;}

MapClassTraits<ConversationWith>::KeyType MapClassTraits<ConversationWith>::GetKey(const Type &item) 
{
  return item._objId;
}

DEFINE_ENUM(TaskState, TS, TASK_STATE_ENUM);

Task::Task(Task *parent, RString name)
{
  _parent = parent;
  if (parent) parent->AddSubtask(this);
  _name = name;
  _id = -1;
  _state = TSCreated;
}

void Task::AddSubtask(Task *task)
{
  _subtasks.Add(task);
}

LSError Task::Serialize(ParamArchive &ar)
{
  // store the type info for dynamic creation in FSM::CreateObject
  SERIALIZE_TYPE_INFO_ROOT(Task);

  CHECK(ar.SerializeRef("parent", _parent, 1));
  CHECK(ar.SerializeRefs("subtasks", _subtasks, 1));
  CHECK(ar.Serialize("id", _id, 1, -1));
  CHECK(ar.Serialize("name", _name, 1));
  CHECK(ar.SerializeEnum("state", _state, 1));
  return LSOK;
}

LSError Task::SaveRef(ParamArchive &ar)
{
  // store the type info
  SAVE_REF_TYPE_INFO_ROOT(Task);

  return LSOK;
}

DEFINE_SERIALIZE_TYPE_REF_INFO_ROOT(Task);

//////////////////////////////////////////////////////////////////////////
// SimpleTask

/// Simple implementation of tasks (usable for diary task tracking)
class SimpleTask : public Task
{
protected:
  typedef Task base;
  OLinkPermO(Person) _owner;

  Vector3 _destination;
  Ref<EntityAI> _target;
  bool _exactTarget;
  RString _description;
  RString _descriptionShort;
  RString _descriptionHUD;

  bool _destinationValid;
  bool _descriptionValid;

public:
  SimpleTask(ForSerializationOnly) {} // used for serialization
  SimpleTask(Person *owner, Task *parent, RString name);

  virtual bool GetDestination(Vector3 &result) const;
  virtual bool GetDescription(RString &result) const;
  virtual bool GetDescriptionShort(RString &result) const;
  virtual bool GetDescriptionHUD(RString &result) const;
  virtual void ChangeOwner(const Person *originalOwner, Person *newOwner);

  /// set the task destination
  void SetDestination(Vector3Par destination);
  /// set the task target
  void SetTarget(EntityAI *target, bool exact);
  /// set the task descriptions
  void SetDescription(RString description, RString descriptionShort, RString descriptionHUD);

  LSError Serialize(ParamArchive &ar);
  static Task *LoadRef(ParamArchive &ar);
  virtual LSError SaveRef(ParamArchive &ar);

  DECL_SERIALIZE_TYPE_INFO(SimpleTask, Task);
};

SimpleTask::SimpleTask(Person *owner, Task *parent, RString name)
: Task(parent, name), _owner(owner)
{
  _destinationValid = false;
  _descriptionValid = false;
  _destination = VZero;
  _exactTarget = false;
  _target = NULL;
}

bool SimpleTask::GetDestination(Vector3 &result) const
{
  if (_target)
  {
    if(_exactTarget) result = _target->FutureVisualState().Position();
    else 
    {
      if(!_owner->Brain()) return false;

      Target *target = _owner->Brain()->FindTarget(_target);
      if (target) result = target->GetPosition();
      else return false;
    }
  }
  else result = _destination;

  return _destinationValid;
}

bool SimpleTask::GetDescription(RString &result) const
{
  result = _description;
  return _descriptionValid;
}

bool SimpleTask::GetDescriptionShort(RString &result) const
{
  result = _descriptionShort;
  return _descriptionValid;
}

bool SimpleTask::GetDescriptionHUD(RString &result) const
{
  result = _descriptionHUD;
  return _descriptionValid;
}

void SimpleTask::ChangeOwner(const Person *originalOwner, Person *newOwner)
{
  if (_owner == originalOwner) _owner = newOwner;
}

void SimpleTask::SetDestination(Vector3Par destination)
{
  _destination = destination;
  _target = NULL;
  _destinationValid = true;
}

void SimpleTask::SetTarget(EntityAI* target, bool exact)
{
  _target = target;
  _exactTarget = exact;
  _destinationValid = true;
}

void SimpleTask::SetDescription(RString description, RString descriptionShort, RString descriptionHUD)
{
  _description = description;
  _descriptionShort = descriptionShort;
  _descriptionHUD = descriptionHUD;
  _descriptionValid = true;
}

LSError SimpleTask::Serialize(ParamArchive &ar)
{
  CHECK(Task::Serialize(ar))
  CHECK(ar.SerializeRef("owner", _owner, 1))
  CHECK(::Serialize(ar, "destination", _destination, 1))
  CHECK(ar.SerializeRef("target", _target, 1))
  CHECK(ar.Serialize("exactTarget", _exactTarget, 1))
  CHECK(ar.Serialize("description", _description, 1))
  CHECK(ar.Serialize("descriptionShort", _descriptionShort, 1))
  CHECK(ar.Serialize("descriptionHUD", _descriptionHUD, 1))
  CHECK(ar.Serialize("destinationValid", _destinationValid, 1))
  CHECK(ar.Serialize("descriptionValid", _descriptionValid, 1))
  return LSOK;
}

Task *SimpleTask::LoadRef(ParamArchive &ar)
{
  // cannot use SerializeRef inside LoadRef - pass == PassFirst
  Object *obj = Person::LoadRef(ar);
  Person *owner = dyn_cast<Person>(obj);
  if (!owner) return NULL;

  int taskId;
  if (ar.Serialize("taskId", taskId, 1, -1) != LSOK) return NULL;
  return owner->GetIdentity().FindTask(taskId);
}

LSError SimpleTask::SaveRef(ParamArchive &ar)
{
  CHECK(base::SaveRef(ar))
  // cannot use SerializeRef inside LoadRef - pass == PassFirst
  if (_owner)
  {
    CHECK(_owner->SaveRef(ar))
  }
  else
  {
    // save the NULL object
    ObjectId id;
    CHECK(id.Serialize(ar, "oid", 1))
  }
  CHECK(ar.Serialize("taskId", _id, 1, -1))
  return LSOK;
}

DEFINE_SERIALIZE_TYPE_REF_INFO(SimpleTask, Task);

//////////////////////////////////////////////////////////////////////////
// DiaryRecord

static RString GetPersonName(AIBrain *unit)
{
  if (!unit) return RString();
  Person *person = unit->GetPerson();
  if (!person) return RString();
  // for soldiers, add a rank as well
  if (unit->GetSide() == TCivilian) return person->GetPersonName();
  return Format("%s. %s", cc_cast(LocalizeString(IDS_SHORT_PRIVATE + person->GetRank())), cc_cast(person->GetPersonName()));
}

DiaryRecord::DiaryRecord(const tm &time, RString title, RString text, Task *task, TaskState state, AIBrain *unit, AIBrain *target, bool readOnly)
{
  _id = -1;
  _time = time; _title = title; _text = text;
  _task = task; _state = state;
  _unit = unit; _target = target;
  _readOnly = readOnly;
  _personName = ::GetPersonName(unit);
  _targetName = ::GetPersonName(target);
}

RString DiaryRecord::GetPersonName() const
{
  // check the unit
  RString personName = ::GetPersonName(_unit);
  if (personName.GetLength() > 0) return personName;
  // use the stored name
  if (_personName.GetLength() > 0) return _personName;
  // fall back
  return LocalizeString(IDS_NOBODY);
}

RString DiaryRecord::GetTargetName() const
{
  // check the unit
  RString personName = ::GetPersonName(_target);
  if (personName.GetLength() > 0) return personName;
  // use the stored name
  if (_targetName.GetLength() > 0) return _targetName;
  // fall back
  return LocalizeString(IDS_NOBODY);
}

RString DiaryRecord::FormatDate(const char *format) const
{
  char buffer[256];
  Clock::FormatDate(format, buffer, _time);
  return buffer;
}

LSError DiaryRecord::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("id", _id, 1))
  CHECK(ar.Serialize("title", _title, 1, RString()))
  CHECK(ar.Serialize("text", _text, 1))
  CHECK(ar.SerializeRef("Task", _task, 1))
  CHECK(ar.SerializeEnum("state", _state, 1))
  CHECK(ar.SerializeRef("Unit", _unit, 1))
  CHECK(ar.Serialize("personName", _personName, 1))
  CHECK(ar.SerializeRef("Target", _target, 1))
  CHECK(ar.Serialize("targetName", _targetName, 1))

  // time and date
  CHECK(ar.Serialize("year", _time.tm_year, 1))
  CHECK(ar.Serialize("mon", _time.tm_mon, 1))
  CHECK(ar.Serialize("day", _time.tm_mday, 1))
  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
  {
    _time.tm_hour = 12; // avoid day shift
    _time.tm_min = 0;
    _time.tm_sec = 0;
    mktime(&_time);
  }
  CHECK(ar.Serialize("hour", _time.tm_hour, 1))
  CHECK(ar.Serialize("min", _time.tm_min, 1))
  CHECK(ar.Serialize("sec", _time.tm_sec, 1))

  CHECK(ar.Serialize("readOnly", _readOnly, 1, false))

  return LSOK;
}


//////////////////////////////////////////////////////////////////////////
// DiaryPage

DiaryPage::DiaryPage(RString name, RString displayName, RString picture)
{
  _changed = false; _nextRecordId = 0;
  _name = name; _displayName = displayName; _picture = picture;
  _indexTitle = "%TITLE";
  _recordTitle = "%DATE, %TIME";
  _recordText = "%TEXT";
  _showEmpty = true;
}

void DiaryPage::Load(const ParamEntry &cls)
{
  _name = cls.GetName();
  _displayName = cls >> "displayName";
  _picture = cls >> "picture";
  ParamEntryVal shortcuts = cls >> "shortcuts";
  int n = shortcuts.GetSize();
  _shortcuts.Realloc(n);
  _shortcuts.Resize(n);
  for (int i=0; i<n; i++) _shortcuts[i] = shortcuts[i];

  _indexTitle = cls >> "indexTitle";
  _indexIcon = cls >> "indexIcon";
  _recordTitle = cls >> "recordTitle";
  _recordText = cls >> "text";
  _showEmpty = cls >> "showEmpty";
}

static int CmpRecordsByDate(const int *i0, const int *i1, const RefArray<DiaryRecord> *log)
{
  const tm &t0 = (*log)[*i0]->GetDate();
  const tm &t1 = (*log)[*i1]->GetDate();

  int diff = t0.tm_year - t1.tm_year;
  if (diff != 0) return diff;
  diff = t0.tm_yday - t1.tm_yday;
  if (diff != 0) return diff;
  diff = t0.tm_hour - t1.tm_hour;
  if (diff != 0) return diff;
  diff = t0.tm_min - t1.tm_min;
  if (diff != 0) return diff;
  diff = t0.tm_sec - t1.tm_sec;
  if (diff != 0) return diff;

  return *i0 - *i1;
}

/// newest records first
static int CmpRecordsByDateInv(const int *i0, const int *i1, const RefArray<DiaryRecord> *log)
{
  return -CmpRecordsByDate(i0, i1, log);
}

TypeIsSimple(const Task *)

static int CmpRecordsByTask(const int *i0, const int *i1, const RefArray<DiaryRecord> *log)
{
  const Task *t0 = (*log)[*i0]->GetTask();
  const Task *t1 = (*log)[*i1]->GetTask();

  if (t0 == t1) return CmpRecordsByDate(i0, i1, log);

  // sort tasks lexicographically
  AUTO_STATIC_ARRAY(const Task *, tasks0, 16);
  for (const Task *t=t0; t; t=t->GetParent())
    tasks0.Add(t);
  AUTO_STATIC_ARRAY(const Task *, tasks1, 16);
  for (const Task *t=t1; t; t=t->GetParent())
    tasks1.Add(t);

  int ii0 = tasks0.Size() - 1;
  int ii1 = tasks1.Size() - 1;
  while (ii0 >= 0 && ii1 >= 0)
  {
    const Task *t0 = tasks0[ii0];
    const Task *t1 = tasks1[ii1];

    int diff = t1->GetId() - t0->GetId();
    if (diff != 0) return diff;

    ii0--; ii1--;
  }
  if (ii0 >= 0) return 1;
  if (ii1 >= 0) return -1;

  Fail("Unaccessible");
  return 0;
}

static OIDInt CmpRecordsByUnit(const int *i0, const int *i1, const RefArray<DiaryRecord> *log)
{
  const AIBrain *u0 = (*log)[*i0]->GetUnit();
  const AIBrain *u1 = (*log)[*i1]->GetUnit();

  if (u0 == u1) return CmpRecordsByDate(i0, i1, log);
  
  RString name0, name1;
  OIDInt id0 = -1, id1 = -1;
  if (u0 && u0->GetPerson())
  {
    name0 = u0->GetPerson()->GetPersonName();
    id0 = u0->GetPerson()->GetObjectId().Encode();
  }
  else
  {
    name0 = (*log)[*i0]->GetPersonName();
    id0 = 0;
  }
  if (u1 && u1->GetPerson())
  {
    name1 = u1->GetPerson()->GetPersonName();
    id1 = u1->GetPerson()->GetObjectId().Encode();
  }
  else
  {
    name1 = (*log)[*i1]->GetPersonName();
    id1 = 0;
  }
  int val = stricmp(name0, name1);
  if (val != 0) return val;
  return id0 - id1;
}

void DiaryPage::Update(const Person *person)
{
  Assert(_changed);
  _indices.Resize(0);

  DiarySortOrder order = GetSortOrder();

  int n = _log.Size();
  for (int i=0; i<n; i++)
  {
    const Task *task = _log[i]->GetTask();
    if (order == DSOTask && !task) continue;
    _indices.Add(i);
  }
  _indices.Compact();

  switch (order)
  {
  case DSONone:
    break;
  case DSODate:
    QSort(_indices.Data(), _indices.Size(), &_log, CmpRecordsByDateInv);
    break;
  case DSOTask:
    QSort(_indices.Data(), _indices.Size(), &_log, CmpRecordsByTask);
    break;
  case DSOUnit:
    QSort(_indices.Data(), _indices.Size(), &_log, CmpRecordsByUnit);
    break;
  }

  _changed = false;
}

/// functor will format the diary record to the text
class FormatDiaryFunc
{
protected:
  const DiaryPage *_page;
  const DiaryRecord *_record;

public:
  FormatDiaryFunc(const DiaryPage *page, const DiaryRecord *record) : _page(page), _record(record) {}
  RString operator ()(const char *&ptr);
};

RString FormatDiaryFunc::operator ()(const char *&ptr)
{
  bool localize = false;
  if (*ptr == '$')
  {
    // reference to stringtable
    localize = true;
    ptr++;
  }

  // read the parameter name
  const char *begin = ptr;
  while (*ptr && __iscsym(*ptr)) ptr++;
  RString name(begin, ptr - begin);

  // localize
  if (localize)
  {
    return LocalizeString(name);
  }

  // return the parameter value
  if (stricmp(name, "DATE") == 0) return _record->FormatDate(LocalizeString(IDS_DATE_FORMAT));
  if (stricmp(name, "TIME") == 0) return _record->FormatDate(LocalizeString(IDS_TIME_FORMAT));
  if (stricmp(name, "TEXT") == 0) return _record->GetText();
  if (stricmp(name, "TITLE") == 0) return _record->GetTitle();
  if (stricmp(name, "RECORD_REF") == 0) // useful for the log reference
  {
    return Format("Record%d", _record->GetId());
  }
  if (stricmp(name, "TASK_DESCRIPTION_SHORT") == 0)
  {
    const Task *task = _record->GetTask();
    if (!task) return LocalizeString(IDS_NOCOMMAND);
    RString text;
    if (task->GetDescriptionShort(text)) return text;
    return task->GetName();
  }
  if (stricmp(name, "TASK_DESCRIPTION") == 0)
  {
    RString text;
    const Task *task = _record->GetTask();
    if (task && task->GetDescription(text)) return text;
    return RString();
  }
  if (stricmp(name, "TASK_REF") == 0) // useful for the log reference
  {
    const Task *task = _record->GetTask();
    if (task) return Format("Task%d", task->GetId());
    return RString();
  }
  if (stricmp(name, "UNIT_NAME") == 0)
  {
    return _record->GetPersonName();
  }
  if (stricmp(name, "UNIT_REF") == 0) // useful for the log reference
  {
    const AIBrain *unit = _record->GetUnit();
    Person *person = unit ? unit->GetPerson() : NULL;
#if _HELISIM_LARGE_OBJECTID
    if (person) return Format("Unit%I64d", person->GetObjectId().Encode());
#else
    if (person) return Format("Unit%d", person->GetObjectId().Encode());
#endif
    return RString();
  }
  if (stricmp(name, "UNIT_VEH_NAME") == 0)
  {
    const AIBrain *brain = _record->GetUnit();
    Transport *veh = brain ? brain->GetVehicleIn() : NULL;
    if (!veh) return RString();
    return veh->GetDisplayName();
  }
  if (stricmp(name, "UNIT_VEH_POSITION") == 0)
  {
    const AIBrain *brain = _record->GetUnit();
    Transport *veh = brain ? brain->GetVehicleIn() : NULL;
    if (!veh) return RString();

    const TransportType *type = veh->Type();
    if (veh->PilotUnit() == brain)
    {
      // driver / pilot
      if (type->HasTurret() || type->GetMaxManCargo() > 0) // some other unit than driver possible
      {
        if (type->IsKindOf(GWorld->Preloaded(VTypeAir)))
          return LocalizeString(IDS_POSITION_PILOT);
        else
          return LocalizeString(IDS_POSITION_DRIVER);
      }
      else
      {
        // only driver is possible
        return RString();
      }
    }
    else
    {
      // gunner / observer
      TurretContext context;
      if (veh->FindTurret(brain->GetPerson(), context) && context._turretType)
        return context._turretType->_gunnerName;
      // cargo
      return RString();
    }
  }
  if (stricmp(name, "TARGET_NAME") == 0)
  {
    return _record->GetTargetName();
  }
  if (stricmp(name, "PLAYER_ID") == 0)
  {
    return _record->GetText(); // player id is encoded in the text
  }
  if (stricmp(name, "PLAYER_NAME") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return identity->GetName();
  }
  if (stricmp(name, "PLAYER_FULLNAME") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return identity->fullname;
  }
  if (stricmp(name, "PLAYER_EMAIL") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return identity->email;
  }
  if (stricmp(name, "PLAYER_ICQ") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return identity->icq;
  }
  if (stricmp(name, "PLAYER_REMARK") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return identity->remark;
  }
  if (stricmp(name, "PLAYER_PING_MIN") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return Format("% 6d", identity->_minPing);
  }
  if (stricmp(name, "PLAYER_PING_AVG") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return Format("% 6d", identity->_avgPing);
  }
  if (stricmp(name, "PLAYER_PING_MAX") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return Format("% 6d", identity->_maxPing);
  }
  if (stricmp(name, "PLAYER_BANDWIDTH_MIN") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return Format("% 6d", identity->_minBandwidth);
  }
  if (stricmp(name, "PLAYER_BANDWIDTH_AVG") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return Format("% 6d", identity->_avgBandwidth);
  }
  if (stricmp(name, "PLAYER_BANDWIDTH_MAX") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return Format("% 6d", identity->_maxBandwidth);
  }
  if (stricmp(name, "PLAYER_DESYNC") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();
    return Format("% 6d", identity->_desync);
  }
  if (stricmp(name, "LINK_SET_CURRENT_TASK") == 0)
  {
    const Task *task = _record->GetTask();
    if (!task) return RString();
    // do not show completed task
    if (task->IsCompleted()) return RString();

    const AIBrain *unit = _record->GetUnit();
    if (!unit) return RString();
    Person *person = unit->GetPerson();
    if (!person) return RString();

    Task *currentTask = person->GetIdentity().GetCurrentTask();
    if (task == currentTask) return RString();

    return Format("[<currentTask id=\"%d\">%s</currentTask>]", task->GetId(), cc_cast(LocalizeString(IDS_LOG_CURRENT_TASK)));
  }
  if (stricmp(name, "LINK_UNIT_GEAR") == 0)
  {
    const AIBrain *unit = _record->GetUnit();
    Person *person = unit ? unit->GetPerson() : NULL;
    if (!person) return RString();
    Person *player = GWorld->PlayerOn();
    if (!player) return RString();

    if (person != player) // Only own gear accessible
    {
        return RString();
    }
#if _HELISIM_LARGE_OBJECTID
    return Format("[<gear unit=\"%I64d\">%s</gear>]", person->GetObjectId().Encode(), cc_cast(LocalizeString(IDS_LOG_GEAR)));
#else
    return Format("[<gear unit=\"%d\">%s</gear>]", person->GetObjectId().Encode(), cc_cast(LocalizeString(IDS_LOG_GEAR)));
#endif
  }
  if (stricmp(name, "LINK_UNIT_CONVERSATION") == 0)
  {
    const AIBrain *unit = _record->GetUnit();
    Person *person = unit ? unit->GetPerson() : NULL;
    if (!person) return RString();
    if (person == GWorld->PlayerOn()) return RString(); // no conversation with himself

    // TODO: Localize
#if _HELISIM_LARGE_OBJECTID
    return Format("[<log subject=\"Conversation\" record=\"Unit%I64d\">%s</log>]", person->GetObjectId().Encode(), "Conversation");
#else
    return Format("[<log subject=\"Conversation\" record=\"Unit%d\">%s</log>]", person->GetObjectId().Encode(), cc_cast(LocalizeString(IDS_LOGSUBJECT_CONVERSATION)));
#endif
  }
  if (stricmp(name, "LINK_PLAYER_KICK") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    // check if we can do it
    if (id == GetNetworkManager().GetPlayer()) return RString();
    if (!GetNetworkManager().IsServer() && !GetNetworkManager().IsGameMaster()) return RString();

    return Format("[<kick id=\"%d\">%s</kick>]", id, cc_cast(LocalizeString(IDS_LOG_KICKOFF)));
  }
  if (stricmp(name, "LINK_PLAYER_BAN") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    // check if we can do it
    if (id == GetNetworkManager().GetPlayer()) return RString();
    if (!GetNetworkManager().IsServer()) return RString();

    return Format("[<ban id=\"%d\">%s</ban>]", id, cc_cast(LocalizeString(IDS_LOG_BAN)));
  }
  if (stricmp(name, "LINK_PLAYER_MUTE") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    // check if we can do it
    if (id == GetNetworkManager().GetPlayer()) return RString();

    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return RString();

    RString action;
    if (GetNetworkManager().IsPlayerInMuteList(identity->xuid)) action = LocalizeString(IDS_LOG_CANCEL_MUTE);
    else action = LocalizeString(IDS_LOG_MUTE);

    return Format("[<mute id=\"%d\">%s</mute>]", id, cc_cast(action));
  }
  
  // some arguments can be handled only by the special page
  return _page->HandleSpecialArgument(name, _record);
}

/// functor will format the diary icon name
class FormatDiaryIconFunc
{
protected:
  const DiaryPage *_page;
  const DiaryRecord *_record;

public:
  FormatDiaryIconFunc(const DiaryPage *page, const DiaryRecord *record) : _page(page), _record(record) {}
  RString operator ()(const char *&ptr);
};

static int BriefingPlayerState(const PlayerIdentity &identity)
{
  switch (identity.clientState)
  {
  case NCSNone:
  case NCSCreated:
  case NCSConnected:
  case NCSLoggedIn:
  case NCSMissionSelected:
  case NCSMissionAsked:
  case NCSPlayerAssigned:
  case NCSMissionReceived:
  case NCSGameFinished:
  case NCSGameLoaded:
  default:
    return 0;
  case NCSPlaying:
    return 2;
  }
}

RString FormatDiaryIconFunc::operator ()(const char *&ptr)
{
  // read the parameter name
  const char *begin = ptr;
  while (*ptr && __iscsym(*ptr)) ptr++;
  RString name(begin, ptr - begin);

  // return the parameter value
  if (stricmp(name, "ICON_UNIT_TYPE") == 0)
  {
    const AIBrain *unit = _record->GetUnit();
    Person *person = unit ? unit->GetPerson() : NULL;
    if (!person) return RString();

    // diary owner passed as target
    const AIBrain *owner = _record->GetTarget();
    if (!owner) return RString();

    bool playable = false;
    if (GWorld->GetMode() != GModeNetware)
    {
      playable = person->QIsManual();
    }

    if (playable) return "unitPlayable";
    else return "unitNone";
  }
  if (stricmp(name, "ICON_TASK_STATE") == 0)
  {
    const Task *task = _record->GetTask();
    if (task)
    {
      switch (task->GetState())
      {
      case TSNone:
        return "taskNone";
      case TSCreated:
        return "taskCreated";
      case TSAssigned:
        return "taskAssigned";
      case TSSucceeded:
        return "taskSucceeded";
      case TSFailed:
        return "taskFailed";
      case TSCanceled:
        return "taskCanceled";
      }
    }
    return "taskNone";
  }

  if (stricmp(name, "ICON_PLAYERS_STATE") == 0)
  {
    int id = atoi(_record->GetText()); // player id is encoded in the text
    const PlayerIdentity *identity = GetNetworkManager().FindIdentity(id);
    if (!identity) return "playerUnknown";

    int state = BriefingPlayerState(*identity);
    if(state == 0) return "playerConnecting";
    if(state == 1) return "playerBriefUnknown";
    return "playerUnknown";
  }

  return RString();
}

RString DiaryPage::GetIndexTitle(const DiaryRecord *record) const
{
  // switch (GetSortOrder()) ...
  // now sorted always by date
  FormatDiaryFunc func(this, record);
  return _indexTitle.ParseFormat(func);
}

RString DiaryPage::GetIndexIcon(const DiaryRecord *record) const
{
  FormatDiaryIconFunc func(this, record);
  return _indexIcon.ParseFormat(func);
}

RString DiaryPage::GetRecordTitle(const DiaryRecord *record) const
{
  // switch (GetSortOrder()) ...
  // now sorted always by date
  FormatDiaryFunc func(this, record);
  return _recordTitle.ParseFormat(func);
}

RString DiaryPage::GetRecordText(const DiaryRecord *record) const
{
  FormatDiaryFunc func(this, record);
  return _recordText.ParseFormat(func);
}

RString DiaryPage::HandleSpecialArgument(RString name, const DiaryRecord *record) const
{
  return RString();
}

DiaryRecord *DiaryPage::AddRecord(RString title, RString text, Task *task, TaskState state, AIBrain *unit, AIBrain *target, bool readOnly)
{
  struct tm time = {0, 0, 0, 0, 0, 0};
  Glob.clockWeather.GetDate(time);
  DiaryRecord *record = new DiaryRecord(time, title, text, task, state, unit, target, readOnly);
  record->SetId(_nextRecordId++);
  _log.Add(record);
  _changed = true;
  if (task && state != TSNone) task->SetState(state);
  return record;
}

void DiaryPage::EditRecord(int id, RString title, RString text)
{
  DiaryRecord *record = FindRecord(id);
  if (record)
  {
    record->SetTitle(title);
    record->SetText(text);
    _changed = true;
  }
}

void DiaryPage::DeleteRecord(int id)
{
  for (int i=0; i<_log.Size(); i++)
  {
    if (_log[i]->GetId() == id)
    {
      _log.Delete(i);
      _changed = true;
      return;
    }
  }
}

DiaryRecord *DiaryPage::FindRecord(int id) const
{
  for (int i=0; i<_log.Size(); i++)
  {
    if (_log[i]->GetId() == id) return _log[i];
  }
  return NULL;
}

LSError DiaryPage::Serialize(ParamArchive &ar)
{
  SERIALIZE_TYPE_INFO_ROOT(DiaryPage);

  CHECK(ar.Serialize("name", _name, 1))
  ConstParamEntryPtr entry = (Pars >> "CfgDiary" >> "FixedPages").FindEntry(_name);
  if (entry)
  {
    // for fixed entries, load the parameters from the config
    if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassFirst)
    {
      Load(*entry);
    }
  }
  else
  {
    // for non-fixed entries, serialize the parameters
    CHECK(ar.Serialize("displayName", _displayName, 1, RString()))
    CHECK(ar.Serialize("picture", _picture, 1, RString()))
    CHECK(ar.SerializeArray("shortcuts", _shortcuts, 1))

    CHECK(ar.Serialize("indexTitle", _indexTitle, 1, "%TITLE"))
    CHECK(ar.Serialize("indexIcon", _indexIcon, 1, RString()))
    CHECK(ar.Serialize("recordTitle", _recordTitle, 1, "%DATE, %TIME"))
    CHECK(ar.Serialize("recordText", _recordText, 1, "%TEXT"))
    CHECK(ar.Serialize("showEmpty", _showEmpty, 1, true))
  }
  CHECK(ar.Serialize("Log", _log, 1))
  CHECK(ar.Serialize("nextRecordId", _nextRecordId, 1, 0))

  if (ar.IsLoading()) _changed = true; // rebuild indices

  return LSOK;
}

DEFINE_SERIALIZE_TYPE_INFO_ROOT(DiaryPage);
DEFINE_SERIALIZE_TYPE_INFO(DiaryPage, DiaryPage);

class DiaryPageDiary : public DiaryPage
{
public:
  DiaryPageDiary(ForSerializationOnly dummy = SerializeConstructor); // constructor usable both for creation and serialization
  DECL_SERIALIZE_TYPE_INFO(DiaryPageDiary, DiaryPage);
};

DiaryPageDiary::DiaryPageDiary(ForSerializationOnly dummy)
: DiaryPage(dummy)
{
}

DEFINE_SERIALIZE_TYPE_INFO(DiaryPageDiary, DiaryPage);

class DiaryPageTasks : public DiaryPage
{
public:
  DiaryPageTasks(ForSerializationOnly dummy = SerializeConstructor); // constructor usable both for creation and serialization

  virtual bool IsChanged() const {return true;} // update always
  virtual bool IsReadOnly() const {return true;}
  virtual DiarySortOrder GetSortOrder() const {return DSOTask;}

  DECL_SERIALIZE_TYPE_INFO(DiaryPageTasks, DiaryPage);

protected:
  virtual void Update(const Person *person);
};

DiaryPageTasks::DiaryPageTasks(ForSerializationOnly dummy)
: DiaryPage(dummy)
{
}

void DiaryPageTasks::Update(const Person *person)
{
  _log.Resize(0);
  _indices.Resize(0);
  _nextRecordId = 0;
  if (!person) return;
  AIBrain *brain = person->Brain();
  if (!brain) return;

  RefArray<Task> tasks;

#if _ENABLE_INDEPENDENT_AGENTS
  AITeamMember *member = brain->GetTeamMember();
  if (member)
  {
    // collect AI tasks
    for (int i=0; i<member->NTasks(); i++)
    {
      Task *task = member->GetTask(i);
      while (task)
      {
        tasks.AddUnique(task);
        task = task->GetParent();
      }
    }
  }
#endif

  const Identity &identity = person->GetIdentity();

  // collect simple tasks
  const RefArray<Task> &simpleTasks = identity.GetSimpleTasks();
  for (int i=0; i<simpleTasks.Size(); i++)
  {
    Task *task = simpleTasks[i];
    while (task)
    {
      tasks.AddUnique(task);
      task = task->GetParent();
    }
  }

  // add records
  for (int i=0; i<tasks.Size(); i++) AddRecord(RString(), RString(), tasks[i], TSNone, brain, NULL, true);

  // create the index
  _changed = true;
  DiaryPage::Update(person);
}

DEFINE_SERIALIZE_TYPE_INFO(DiaryPageTasks, DiaryPage);

class DiaryPageConversation : public DiaryPage
{
public:
  DiaryPageConversation(ForSerializationOnly dummy = SerializeConstructor); // constructor usable both for creation and serialization

  virtual bool IsReadOnly() const {return true;}
  virtual DiarySortOrder GetSortOrder() const {return DSOUnit;}

  DECL_SERIALIZE_TYPE_INFO(DiaryPageConversation, DiaryPage);
};

DiaryPageConversation::DiaryPageConversation(ForSerializationOnly dummy)
: DiaryPage(dummy)
{
}

DEFINE_SERIALIZE_TYPE_INFO(DiaryPageConversation, DiaryPage);

static RString GetSkillText(float skill)
{
  if (skill <= 0.25)
    return LocalizeString(IDS_SKILL_NOVICE);
  else if (skill <= 0.45)
    return LocalizeString(IDS_SKILL_ROOKIE);
  else if (skill <= 0.65)
    return LocalizeString(IDS_SKILL_RECRUIT);
  else if (skill <= 0.85)
    return LocalizeString(IDS_SKILL_VETERAN);
  else
    return LocalizeString(IDS_SKILL_EXPERT);
}

/// functor will format the skill description to the text
class FormatSkillFunc
{
protected:
  const Identity &_identity;
  int _abilityIndex;

public:
  FormatSkillFunc(const Identity &identity, int abilityIndex) : _identity(identity), _abilityIndex(abilityIndex) {}
  RString operator ()(const char *&ptr);
};

RString FormatSkillFunc::operator ()(const char *&ptr)
{
  bool localize = false;
  if (*ptr == '$')
  {
    // reference to stringtable
    localize = true;
    ptr++;
  }

  // read the parameter name
  const char *begin = ptr;
  while (*ptr && __iscsym(*ptr)) ptr++;
  RString name(begin, ptr - begin);

  // localize
  if (localize)
  {
    return LocalizeString(name);
  }

  // return the parameter value
  if (stricmp(name, "SKILL_NAME") == 0)
  {
    // TODO: display names
    return FindEnumName((AbilityKind)_abilityIndex);
  }
  if (stricmp(name, "SKILL_LEVEL") == 0)
  {
    float value = _identity.GetRawAbility((AbilityKind)_abilityIndex);
    return GetSkillText(value);
  }

  return RString();
}

class DiaryPagePlayers : public DiaryPage
{
protected:
  /// template for the squad format
  RString _squad;

public:
  DiaryPagePlayers(ForSerializationOnly dummy = SerializeConstructor); // constructor usable both for creation and serialization
  virtual void Load(const ParamEntry &cls);

  virtual bool IsChanged() const {return true;} // update always
  virtual bool IsReadOnly() const {return true;}
  virtual DiarySortOrder GetSortOrder() const {return DSONone;}

  virtual RString HandleSpecialArgument(RString name, const DiaryRecord *record) const;

  DECL_SERIALIZE_TYPE_INFO(DiaryPagePlayers, DiaryPage);

protected:
  virtual void Update(const Person *person);
};

DiaryPagePlayers::DiaryPagePlayers(ForSerializationOnly dummy)
: DiaryPage(dummy)
{
}

void DiaryPagePlayers::Load(const ParamEntry &cls)
{
  DiaryPage::Load(cls);

  _squad = cls >> "squad";
}

RString DiaryPagePlayers::HandleSpecialArgument(RString name, const DiaryRecord *record) const
{
  return RString();
}

void DiaryPagePlayers::Update(const Person *person)
{
  _log.Resize(0);
  _indices.Resize(0);
  _nextRecordId = 0;

  const AutoArray<PlayerIdentity> *identities = GetNetworkManager().GetIdentities();
  if (!identities) return;
  for (int i=0; i<identities->Size(); i++)
  {
    const PlayerIdentity &identity = (*identities)[i];
    AddRecord(RString(), Format("%d", identity.dpnid));
  }

  // create the index
  _changed = true;
  DiaryPage::Update(person);
}

DEFINE_SERIALIZE_TYPE_INFO(DiaryPagePlayers, DiaryPage);


//////////////////////////////////////////////////////////////////////////
// Diary

void Diary::Init()
{
  _pages.Resize(0);

  ParamEntryVal config = Pars >> "CfgDiary" >> "FixedPages";
  for (int i=0; i<config.GetEntryCount(); i++)
  {
    ParamEntryVal entry = config.GetEntry(i);
    if (entry.IsClass())
    {
      RString type = entry >> "type";
      DiaryPage *page = DiaryPage::_typeInfoRoot->CreateByName(type);
      if (page)
      {
        _pages.Add(page);
        page->Load(entry);
      }
    }
  }
  
  _fixedSubjects = _pages.Size();
  _changed = true;
}

void Diary::SetChanged()
{
  _changed = true;
  for (int i=0; i<_pages.Size(); i++)
  {
    _pages[i]->SetChanged();
  }
}

int Diary::AddSubject(RString subject, RString displayName, RString picture)
{
  int index = FindSubjectIndex(subject);
  if (index >= 0) return index;

  index = _pages.Add(new DiaryPage(subject, displayName, picture));
  _changed = true;
  return index;
}

bool Diary::IsChanged(RString subject) const
{
  const DiaryPage *page = FindSubject(subject);
  if (!page) return false;
  return page->IsChanged();
}

bool Diary::IsReadOnly(RString subject) const
{
  const DiaryPage *page = FindSubject(subject);
  if (!page) return false;
  return page->IsReadOnly();
}

DiaryRecord *Diary::AddRecord(RString subject, RString title, RString text, Task *task, TaskState state, AIBrain *unit, AIBrain *target, bool readOnly)
{
  DiaryPage *page = FindSubject(subject);
  if (!page) return NULL;
  return page->AddRecord(title, text, task, state, unit, target, readOnly);
}

void Diary::EditRecord(RString subject, int id, RString title, RString text)
{
  DiaryPage *page = FindSubject(subject);
  if (page) page->EditRecord(id, title, text);
}

void Diary::DeleteRecord(RString subject, int id)
{
  DiaryPage *page = FindSubject(subject);
  if (page) page->DeleteRecord(id);
}

int Diary::FindSubjectIndex(RString subject) const
{
  for (int i=0; i<_pages.Size(); i++)
  {
    if (stricmp(subject, _pages[i]->GetName()) == 0)
      return i;
  }
  return -1;
}

LSError Diary::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("Pages", _pages, 1))
  CHECK(ar.Serialize("fixedSubjects", _fixedSubjects, 1))
  CHECK(ar.Serialize("currentSubject", _currentSubject, 1))

  if (ar.IsLoading()) _changed = true; // force update
  return LSOK;
}

//////////////////////////////////////////////////////////////////////////
// Conversation history

HistoryItem::HistoryItem(const KBMessageInfo *info, Time time)
: _time(time)
{
  _topic = info->GetTopic();
  _sentenceId = info->GetMessage()->GetType()->GetName();
}

LSError HistoryItem::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("topic", _topic, 1))
  CHECK(ar.Serialize("sentenceId", _sentenceId, 1))
  CHECK(::Serialize(ar, "time", _time, 1))
  return LSOK;
}

LSError ConversationWith::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("History", _history, 1))
  CHECK(ar.Serialize("oid", _objId, 1))
  return LSOK;
}

//////////////////////////////////////////////////////////////////////////
// Identity

Identity::Identity()
{
  _nextTaskId = 0;
  for (int i=0; i<NAbilityKind; i++) _skills[i] = 1.0f;
  _customMarkSet = false;
  _customMarkPos = VZero;
}

void Identity::SetAllAbilities(float ability)
{
  Assert(ability > 0);
  for (int i=0; i<NAbilityKind; i++) _skills[i] = ability;
}

LSError Identity::Serialize(ParamArchive &ar)
{
  if (!IS_UNIT_STATUS_BRANCH(ar.GetArVersion()))
  {
    // for SaveStatus / LoadStatus avoid tasks, diary, conversation history and custom mark
    
    // tasks
    CHECK(ar.Serialize("Tasks", _simpleTasks, 1))
    CHECK(ar.Serialize("nextTaskId", _nextTaskId, 1))
    CHECK(ar.SerializeRef("CurrentTask", _currentTask, 1))

    // diary
    CHECK(ar.Serialize("Diary", _diary, 1))

    // conversation history
    CHECK(ar.Serialize("Said", _said, 1))

    // custom mark
    CHECK(ar.Serialize("customMarkSet", _customMarkSet, 1, false))
    CHECK(::Serialize(ar, "customMarkPos", _customMarkPos, 1, VZero))
  }

  // skills
  for (int i=0; i<NAbilityKind; i++)
  {
    RString name = RString("skill") + FindEnumName((AbilityKind)i);
    float defVal = i>0 ? _skills[i-1] : 1.0f;
    CHECK(ar.Serialize(name, _skills[i], 1, defVal));
  }

  return LSOK;
}

int Identity::AddTask(Task *task)
{
  task->SetId(_nextTaskId++);
  return _simpleTasks.Add(task);
}

bool Identity::DeleteTask(Task *task)
{
  return _simpleTasks.Delete(task);
}

Task *Identity::FindTask(int id)
{
  for (int i=0; i<_simpleTasks.Size(); i++)
  {
    if (_simpleTasks[i]->GetId() == id) return _simpleTasks[i];
  }
  return NULL;
}

void Identity::AddSaid( AIBrain *to, const KBMessageInfo *question )
{
  if (!to || !to->GetPerson()) return; // no add for "invalid" AIBrain
  OIDInt objId = to->GetPerson()->GetObjectId().Encode();
  // access table for given sender
  ConversationWith &history = _said.Set(MapClassTraits<ConversationWith>::KeyType(objId));
  if (_said.IsNull(history))
  {
    ConversationWith newHistory;
    newHistory._objId = objId;
    newHistory.Add(question,Glob.time);
    _said.Add(newHistory);
  }
  else
  {
    history.Add(question,Glob.time);
  }
}

/**
@param topic what topic are we checking for, empty means any
@param sentenceId id of the sentence, empty means we are checking for any conversation
*/
bool Identity::WasSaid(AIBrain *to, RString topic, RString sentenceId, float maxAge) const
{
  if (!to || !to->GetPerson()) return false; // no messages for "invalid" AIBrain
  OIDInt objId = to->GetPerson()->GetObjectId().Encode();
  // get a history associated to a given target
  const ConversationWith &history = (_said.Get(MapClassTraits<ConversationWith>::KeyType(objId)));
  if (!_said.IsNull(history))
  {
    for (int i=0; i<history._history.Size(); i++)
    {
      const HistoryItem &item = history._history[i];
      // if the item is too old, ignore it
      if (item._time<Glob.time-maxAge) continue;
      // check if it is the message we are interested about
      if (topic.GetLength()>0 && strcmpi(item._topic, topic)) continue;
      if (sentenceId.GetLength()>0 && strcmpi(item._sentenceId, sentenceId)) continue;
      // if it is, we are done
      return true;
    }
  }

  // no such message was found
  return false;
}

//////////////////////////////////////////////////////////////////////////
//
// Scripting functions interface
//

// Types registration
#define TYPES_IDENTITY(XX, Category) \
  XX("TASK", GameTask, CreateGameDataTask, "Task", "Task", "Task.", Category, "GameTask", "Lcom/bistudio/JNIScripting/RVEngine$GameTask;") \
  XX("DIARY_RECORD", GameDiaryRecord, CreateGameDataDiaryRecord, "Diary record", "Diary record", "Diary record.", Category, "GameDiaryRecord", "Lcom/bistudio/JNIScripting/RVEngine$GameDiaryRecord;")

TYPES_IDENTITY(DECLARE_TYPE, "Identity")

typedef LLink<Task> GameTaskType;

#include <Es/Memory/normalNew.hpp>

class GameDataTask : public GameData, public CountInstances<GameDataTask>
{
  typedef GameData base;

  GameTaskType _value;

public:
  GameDataTask() {}
  GameDataTask(const GameTaskType &value) : _value(value) {}
  ~GameDataTask() {}

  const GameType &GetType() const {return GameTask;}
  const GameTaskType &GetTask() const {return _value;}

  RString GetText() const;
  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "task";}
  GameData *Clone() const {return new GameDataTask(*this);}

  jobject ToJObject(JNIEnv *env, GameValuePar from) const;

  LSError Serialize(ParamArchive &ar);

  USE_FAST_ALLOCATOR_EXPRESS
};

class GameDataDiaryRecord : public GameData
{
  typedef GameData base;

  OLinkPermO(Person) _owner;  // diary owner (needed for serialization)
  RString _subject;           // page where record is (needed for serialization)
  LLink<DiaryRecord> _record; // the record itself

public:
  GameDataDiaryRecord() {}
  GameDataDiaryRecord(Person *owner, RString subject, DiaryRecord *record)
    : _owner(owner), _subject(subject), _record(record) {}
  ~GameDataDiaryRecord() {}

  const GameType &GetType() const {return GameDiaryRecord;}
  DiaryRecord *GetDiaryRecord() const {return _record;}

  RString GetText() const;
  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "diary record";}
  GameData *Clone() const {return new GameDataDiaryRecord(*this);}

  jobject ToJObject(JNIEnv *env, GameValuePar from) const;

  LSError Serialize(ParamArchive &ar);

  USE_FAST_ALLOCATOR_EXPRESS
};

#include <Es/Memory/debugNew.hpp>

GameData *CreateGameDataTask(ParamArchive *ar) {return new GameDataTask();}
GameData *CreateGameDataDiaryRecord(ParamArchive *ar) {return new GameDataDiaryRecord();}

GameValue CreateGameTask(Task *task) {return GameValue(new GameDataTask(task));}
GameValue CreateGameDiaryRecord()
{
  return GameValue(new GameDataDiaryRecord());
}
GameValue CreateGameDiaryRecord(Person *owner, RString subject, DiaryRecord *record)
{
  return GameValue(new GameDataDiaryRecord(owner, subject, record));
}

TYPES_IDENTITY(DEFINE_TYPE, "Identity")

template<> CachedJavaClass ConvertTypes<&GTypeGameTask>::_cache("com/bistudio/JNIScripting/RVEngine$GameTask");
template<> CachedJavaClass ConvertTypes<&GTypeGameDiaryRecord>::_cache("com/bistudio/JNIScripting/RVEngine$GameDiaryRecord");

//////////////////////////////////////////////////////////////////////////
//
// Scripting functions implementation
//

DEFINE_FAST_ALLOCATOR_EXPRESS(GameDataTask)

RString GameDataTask::GetText() const
{
  if (!_value) return "No task";
  return Format("Task %s (id %d)", (const char *)_value->GetName(), _value->GetId());
}

bool GameDataTask::IsEqualTo(const GameData *data) const
{
  const GameTaskType &val1 = GetTask();
  const GameTaskType &val2 = static_cast<const GameDataTask *>(data)->GetTask();

  return val1 == val2;
}

jobject GameDataTask::ToJObject(JNIEnv *env, GameValuePar from) const
{
  return ConvertTypes<&GTypeGameTask>::FromGameValue(env, from);
}

LSError GameDataTask::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar));
  CHECK(ar.SerializeRef("Value", _value, 1));
  return LSOK;
}

DEFINE_FAST_ALLOCATOR_EXPRESS(GameDataDiaryRecord)

RString GameDataDiaryRecord::GetText() const
{
  if (!_record) return "No diary record";
  return _record->GetText();
}

bool GameDataDiaryRecord::IsEqualTo(const GameData *data) const
{
  DiaryRecord *val1 = GetDiaryRecord();
  DiaryRecord *val2 = static_cast<const GameDataDiaryRecord *>(data)->GetDiaryRecord();

  return val1 == val2;
}

jobject GameDataDiaryRecord::ToJObject(JNIEnv *env, GameValuePar from) const
{
  return ConvertTypes<&GTypeGameDiaryRecord>::FromGameValue(env, from);
}

LSError GameDataDiaryRecord::Serialize(ParamArchive &ar)
{
  CHECK(base::Serialize(ar));

  CHECK(ar.SerializeRef("Owner", _owner, 1));
  CHECK(ar.Serialize("subject", _subject, 1));
  // _record serialization
  if (ar.IsSaving())
  {
    int id = _record ? _record->GetId() : -1;
    CHECK(ar.Serialize("id", id, 1));
  }
  else if (ar.GetPass() == ParamArchive::PassSecond)
  {
    // we need to load integer
    ar.FirstPass();
    int id;
    LSError err = ar.Serialize("id", id, 1);
    ar.SecondPass();
    if (err != LSOK) return err;
    _record = NULL;
    if (id >= 0 && _owner)
    {
      const DiaryPage *page = _owner->GetIdentity().FindDiarySubject(_subject);
      if (page) _record = page->FindRecord(id);
    }
  }

  return LSOK;
}

Object *GetObject(GameValuePar oper);
GameTextType GetTextValue(GameValuePar oper);
bool GetPos(const GameState *state, Vector3 &ret, GameValuePar oper);
Task *GetTask(GameValuePar oper)
{
  if (oper.GetType() == GameTask)
    return static_cast<GameDataTask *>(oper.GetData())->GetTask();
  return NULL;
}
DiaryRecord *GetDiaryRecord(GameValuePar oper)
{
  if (oper.GetType() == GameDiaryRecord)
    return static_cast<GameDataDiaryRecord *>(oper.GetData())->GetDiaryRecord();
  return NULL;
}

#if _ENABLE_INDEPENDENT_AGENTS

AITeamMember *GetTeamMember(GameValuePar oper);

#include <Es/Memory/normalNew.hpp>

class AITeamMemberFSMScripted : public FSMScriptedTyped<AITeamMemberContext>
{
  typedef FSMScriptedTyped<AITeamMemberContext> base;

protected:
  DECL_SERIALIZE_TYPE_INFO(AITeamMemberFSMScripted, FSM)

public:
  AITeamMemberFSMScripted(const FSMScriptedType *type, GameDataNamespace *globals, GameVarSpace *parentVars) : base(type, globals, parentVars) {}
  AITeamMemberFSMScripted(ForSerializationOnly dummy) : base(dummy) {}

  virtual void ApplyContext(GameState &state, void *context);

  USE_FAST_ALLOCATOR
};

#include <Es/Memory/debugNew.hpp>

DEFINE_FAST_ALLOCATOR(AITeamMemberFSMScripted)
DEFINE_SERIALIZE_TYPE_INFO(AITeamMemberFSMScripted, FSM)

void AITeamMemberFSMScripted::ApplyContext(GameState &state, void *context)
{
  AITeamMemberContext *ctx = reinterpret_cast<AITeamMemberContext *>(context);
  if (ctx->_teamMember) ctx->_teamMember->ApplyContext(state);
}

FSMScripted *CreateTeamMemberFSMScripted(const FSMScriptedType *type, GameVarSpace *parentVars)
{
  return new AITeamMemberFSMScripted(type, GWorld->GetMissionNamespace(), parentVars); // mission namespace
}

#endif

GameValue TeamMemberTaskRegister(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
#if _ENABLE_INDEPENDENT_AGENTS
  // left operand
  AITeamMember *teamMember = GetTeamMember(oper1);
  if (!teamMember) return false;

  AITaskTypeName name = oper2;
  Ref<AITaskType> taskType = AITaskTypes.New(name);
  return teamMember->RegisterTaskType(taskType);
#else
  return false;
#endif
/*
  // right operand 
  // size
  const GameArrayType &array = oper2;
  RString descriptionExp;
  RString destinationExp;
  switch (array.Size())
  {
  case 6:
    if (array[5].GetType() != GameString)
    {
      state->TypeError(GameString, array[5].GetType());
      return false;
    }
    destinationExp = array[5];
  case 5:
    if (array[4].GetType() != GameString)
    {
      state->TypeError(GameString, array[4].GetType());
      return false;
    }
    descriptionExp = array[4];
    // continue
  case 4:
    if (array[0].GetType() != GameString)
    {
      state->TypeError(GameString, array[0].GetType());
      return false;
    }
    if (array[1].GetType() != GameString)
    {
      state->TypeError(GameString, array[1].GetType());
      return false;
    }
    if (array[2].GetType() != GameString)
    {
      state->TypeError(GameString, array[2].GetType());
      return false;
    }
    if (array[3].GetType() != GameArray)
    {
      state->TypeError(GameArray, array[3].GetType());
      return false;
    }
    {
      const GameArrayType &resources = array[3];
      for (int i=0; i<resources.Size(); i++)
      {
        if (resources[i].GetType() != GameString)
        {
          state->TypeError(GameString, resources[i].GetType());
          return false;
        }
      }
    }
    break;
  default:
    state->SetError(EvalDim, array.Size(), 6);
    return false;
  }

  RString FindScript(RString name);

  RString name = array[0];
  RString fsmFilename = FindScript(array[1]);
  if (fsmFilename.GetLength() == 0) return false;
  const GameArrayType &resources = array[3];
  RString conditionFilename = array[2];
  if (conditionFilename.GetLength() > 0)
  {
    // no script is enabled, non-existing script not
    conditionFilename = FindScript(conditionFilename); // condition script
    if (conditionFilename.GetLength() == 0) return false;
  }

  AITaskTypeName info(name, FSMScriptedTypes.New(fsmFilename), conditionFilename, descriptionExp, destinationExp);
  for (int i=0; i<resources.Size(); i++)
  {
    info._resources.Add(resources[i]);
  }
  info._resources.Compact();
  AITaskType *taskType = AITaskTypes.New(info);

  return teamMember->RegisterTaskType(taskType);
*/
};

GameValue TeamMemberTaskUnregister(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
#if _ENABLE_INDEPENDENT_AGENTS
  // left operand
  AITeamMember *teamMember = GetTeamMember(oper1);
  if (!teamMember) return false;

  // right operand 
  RString name = oper2;
  return teamMember->UnregisterTaskType(name);
#else
  return false;
#endif
}

GameValue TeamMemberTaskCreate(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
#if _ENABLE_INDEPENDENT_AGENTS
  // left operand
  AITeamMember *teamMember = GetTeamMember(oper1);
  if (!teamMember) return CreateGameTask(NULL);

  // right operand 
  // size
  const GameArrayType &array = oper2;
  int n = array.Size();
  if (n == 0 || n % 2 != 0)
  {
    state->SetError(EvalDim, array.Size(), 2);
    return CreateGameTask(NULL);
  }

  // [name] or [name, parentTask]
  if (array[0].GetType() != GameArray)
  {
    state->TypeError(GameArray, array[0].GetType());
    return CreateGameTask(NULL);
  }
  const GameArrayType &subarray = array[0];
  Task *parent = NULL;
  RString name;
  switch (subarray.Size())
  {
  case 2:
    if (subarray[1].GetType() != GameTask)
    {
      state->TypeError(GameTask, subarray[1].GetType());
      return CreateGameTask(NULL);
    }
    parent = GetTask(subarray[1]);
    // continue with the next argument
  case 1:
    if (subarray[0].GetType() != GameString)
    {
      state->TypeError(GameString, subarray[0].GetType());
      return CreateGameTask(NULL);
    }
    name = subarray[0];
    break;
  default:
    state->SetError(EvalDim, subarray.Size(), 2);
    return CreateGameTask(NULL);
  }

  // priority
  if (array[1].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[1].GetType());
    return CreateGameTask(NULL);
  }
  float priority = array[1];

  // arguments
  for (int i=2; i<n; i+=2)
  {
    if (array[i].GetType() != GameString)
    {
      state->TypeError(GameString, array[i].GetType());
      return CreateGameTask(NULL);
    }
  }

  const AITaskType *taskType = teamMember->GetTaskType(name);
  if (!taskType) return CreateGameTask(NULL);

  // Task creation
  FSMScripted *fsm = new AITeamMemberFSMScripted(taskType->GetFSMType(), GWorld->GetMissionNamespace(), teamMember->GetVars()); // mission namespace
  AITask *task = new AITask(teamMember, teamMember, parent, taskType, fsm, priority);
  teamMember->AddTask(task);
  
  GameValue value = CreateGameTask(task);

  // parameters
  for (int i=2; i<n; i+=2)
    fsm->SetParam(array[i], array[i + 1]);

  return value;
#else
  return CreateGameTask(NULL);
#endif
}

#if _ENABLE_INDEPENDENT_AGENTS
AIBrain *GetSpeaker(AITeamMember *member);
#endif

GameValue TeamMemberTaskSend(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
#if _ENABLE_INDEPENDENT_AGENTS
  // left operand
  AITeamMember *sender = GetTeamMember(oper1);
  if (!sender || !GetSpeaker(sender)) return CreateGameTask(NULL);

  // right operand 
  // size
  const GameArrayType &array = oper2;
  int n = array.Size();
  if (n < 3 || n % 2 != 1)
  {
    state->SetError(EvalDim, array.Size(), 3);
    return CreateGameTask(NULL);
  }

  // receiver
  if (array[0].GetType() != GameTeamMember)
  {
    state->TypeError(GameTeamMember, array[0].GetType());
    return CreateGameTask(NULL);
  }
  AITeamMember *receiver = GetTeamMember(array[0]);
  if (!receiver) return CreateGameTask(NULL);

  // [name] or [name, parentTask]
  if (array[1].GetType() != GameArray)
  {
    state->TypeError(GameArray, array[1].GetType());
    return CreateGameTask(NULL);
  }
  const GameArrayType &subarray = array[1];
  Task *parent = NULL;
  RString name;
  switch (subarray.Size())
  {
  case 2:
    if (subarray[1].GetType() != GameTask)
    {
      state->TypeError(GameTask, subarray[1].GetType());
      return CreateGameTask(NULL);
    }
    parent = GetTask(subarray[1]);
    // continue with the next argument
  case 1:
    if (subarray[0].GetType() != GameString)
    {
      state->TypeError(GameString, subarray[0].GetType());
      return CreateGameTask(NULL);
    }
    name = subarray[0];
    break;
  default:
    state->SetError(EvalDim, subarray.Size(), 2);
    return CreateGameTask(NULL);
  }

  // priority
  if (array[2].GetType() != GameScalar)
  {
    state->TypeError(GameScalar, array[2].GetType());
    return CreateGameTask(NULL);
  }
  float priority = array[2];

  // arguments
  for (int i=3; i<n; i+=2)
  {
    if (array[i].GetType() != GameString)
    {
      state->TypeError(GameString, array[i].GetType());
      return CreateGameTask(NULL);
    }
  }

  const AITaskType *taskType = receiver->GetTaskType(name);
  if (!taskType) return CreateGameTask(NULL);

  // Task creation
  FSMScripted *fsm = new AITeamMemberFSMScripted(taskType->GetFSMType(), GWorld->GetMissionNamespace(), receiver->GetVars()); // mission namespace
  // parameters
  for (int i=3; i<n; i+=2)
    fsm->SetParam(array[i], array[i + 1]);
  // task
  Ref<AITask> task = new AITask(receiver, sender, parent, taskType, fsm, priority);
  // Task sending
  if (sender == receiver || GetSpeaker(sender)->GetTeamMember() == receiver)
  {
    // silent processing
    receiver->AddTask(task);
  }
  else
  {
    GameValue value = state->CreateGameValue(GameArray);
    GameArrayType &params = value;
    params.Realloc(n - 3);
    params.Resize(n - 3);
    for (int i=3,j=0; i<n; i+=2,j++)
      params[j] = array[i + 1];

    RadioMessage *message = new RadioMessageTask(task, value);
    sender->SendMsg(message);
  }

  GameValue value = CreateGameTask(task);
  return value;
#else
  return CreateGameTask(NULL);
#endif
}

GameValue DiarySubjectCreate(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Object *obj = GetObject(oper1);
  Person *person = dyn_cast<Person>(obj);
  if (!person) return -1.0f;

  const GameArrayType &array = oper2;
  RString picture;
  switch (array.Size())
  {
  case 3:
    if (array[2].GetType() != GameString)
    {
      state->TypeError(GameString, array[2].GetType());
      return -1.0f;
    }
    picture = array[2];
    // continue
  case 2:
    if (array[1].GetType() != GameString)
    {
      state->TypeError(GameString, array[1].GetType());
      return -1.0f;
    }
    if (array[0].GetType() != GameString)
    {
      state->TypeError(GameString, array[0].GetType());
      return -1.0f;
    }
    break;
  default:
    state->SetError(EvalDim, array.Size(), 3);
    return -1.0f;
  }

  float index = person->GetIdentity().AddDiarySubject(array[0], array[1], picture);
  return index;
}

GameValue DiarySubjectSelect(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Object *obj = GetObject(oper1);
  Person *person = dyn_cast<Person>(obj);
  if (person) person->GetIdentity().SetCurrentDiarySubject(oper2);
  return NOTHING;
}

GameValue DiaryHasSubject(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Object *obj = GetObject(oper1);
  Person *person = dyn_cast<Person>(obj);
  if (!person) return false;

  DiaryPage *page = person->GetIdentity().FindDiarySubject(oper2);
  return page != NULL;
}

GameValue DiaryRecordCreate(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Object *obj = GetObject(oper1);
  Person *person = dyn_cast<Person>(obj);
  if (!person) return CreateGameDiaryRecord();

  const GameArrayType &array = oper2;
  RString subject;
  RString title;
  RString text;
  Task *task = NULL;
  TaskState taskState = TSNone;
  switch (array.Size())
  {
  case 4:
    if (array[3].GetType() != GameString)
    {
      state->TypeError(GameString, array[3].GetType());
      return CreateGameDiaryRecord();
    }
    {
      RString stateName = array[3];
      taskState = GetEnumValue<TaskState>((const char *)stateName);
      if (taskState < 0)
      {
        RptF("Unknown task state: %s", (const char *)stateName);
        taskState = TSNone;
      }
    }
    // continue with next argument
  case 3:
    if (array[2].GetType() != GameTask)
    {
      state->TypeError(GameTask, array[2].GetType());
      return CreateGameDiaryRecord();
    }
    task = GetTask(array[2]);
    // continue with next argument
  case 2:
    if (array[0].GetType() != GameString)
    {
      state->TypeError(GameString, array[0].GetType());
      return CreateGameDiaryRecord();
    }
    subject = array[0];
    if (array[1].GetType() == GameString)
    {
      text = array[1];
    }
    else if (array[1].GetType() == GameArray)
    {
      const GameArrayType &subarray = array[1];
      if (subarray.Size() != 2)
      {
        state->SetError(EvalDim, subarray.Size(), 2);
        return CreateGameDiaryRecord();
      }
      if (subarray[0].GetType() != GameString)
      {
        state->TypeError(GameString, subarray[0].GetType());
        return CreateGameDiaryRecord();
      }
      if (subarray[1].GetType() != GameString)
      {
        state->TypeError(GameString, subarray[1].GetType());
        return CreateGameDiaryRecord();
      }
      title = subarray[0];
      text = subarray[1];
    }
    else
    {
      state->TypeError(GameArray, array[1].GetType());
      return CreateGameDiaryRecord();
    }
    break;
  default:
    state->SetError(EvalDim, array.Size(), 4);
    return CreateGameDiaryRecord();
  }
  DiaryRecord *record = person->GetIdentity().AddDiaryRecord(subject, title, text, task, taskState, NULL, NULL, true);
  return CreateGameDiaryRecord(person, subject, record);
}

GameValue DiaryCreateLink(const GameState *state, GameValuePar oper1)
{
  const GameArrayType &array = oper1;
  if (array.Size() != 3)
  {
    state->SetError(EvalDim, array.Size(), 3);
    return RString();
  }

  if (array[0].GetType() != GameString)
  {
    state->TypeError(GameString, array[0].GetType());
    return RString();
  }
  RString subject = array[0];

  RString recordId;
  if (array[1].GetType() == GameObject)
  {
    Person *person = dyn_cast<Person>(GetObject(array[1]));
    if (!person) return RString();

#if _HELISIM_LARGE_OBJECTID
    recordId = Format("Unit%I64d", person->GetObjectId().Encode());
#else
    recordId = Format("Unit%d", person->GetObjectId().Encode());
#endif
  }
  else if (array[1].GetType() == GameTask)
  {
    Task *task = GetTask(array[1]);
    if (!task) return RString();
    recordId = Format("Task%d", task->GetId());
  }
  else if (array[1].GetType() == GameDiaryRecord)
  {
    DiaryRecord *record = GetDiaryRecord(array[1]);
    if (!record) return RString();
    recordId = Format("Record%d", record->GetId());
  }
  else
  {
    state->TypeError(GameDiaryRecord, array[1].GetType());
    return RString();
  }

  if (array[2].GetType() != GameString)
  {
    state->TypeError(GameString, array[2].GetType());
    return RString();
  }
  RString text = array[2];

  return Format("<log subject=\"%s\" record=\"%s\">%s</log>", cc_cast(subject), cc_cast(recordId), cc_cast(text));
}

/// XML parser used to find info in the diary link
class DiaryLinkParser : public SAXParser
{
public:
  RString _subject;
  RString _recordId;

public:
  void OnStartElement(RString name, XMLAttributes &attributes);
};

void DiaryLinkParser::OnStartElement(RString name, XMLAttributes &attributes)
{
  if (strcmp(name, "log") == 0)
  {
    const XMLAttribute *attr = attributes.Find("subject");
    if (attr) _subject = attr->value;
    attr = attributes.Find("record");
    if (attr) _recordId = attr->value;
  }
}

GameValue DiaryProcessLink(const GameState *state, GameValuePar oper1)
{
  // convert from XML to HTML link
  RString link = oper1;
  QIStrStream in(cc_cast(link), link.GetLength());

  DiaryLinkParser parser;
  parser.Parse(in);

  RString htmlLink;
  if (parser._subject.GetLength() == 0) return NOTHING;

  if (parser._recordId.GetLength() > 0)
    htmlLink = Format("%s:%s", cc_cast(parser._subject), cc_cast(parser._recordId));
  else
    htmlLink = parser._subject;

  // process the link
  void ProcessDiaryLink(RString link);
  ProcessDiaryLink(htmlLink);
  return NOTHING;
}

GameValue TaskGetState(const GameState *state, GameValuePar oper1)
{
  Task *task = GetTask(oper1);
  if (!task) return RString();
  TaskState taskState = task->GetState();
  return FindEnumName(taskState);
}

GameValue TaskIsCompleted(const GameState *state, GameValuePar oper1)
{
  Task *task = GetTask(oper1);
  if (!task) return true;
  return task->IsCompleted();
};

static void SubTaskSetState(Task *task, TaskState taskState)
{
  if (task)
  {
    task->SetState(taskState);
    for (int i=0; i<task->NSubtasks(); i++)
      SubTaskSetState(task->GetSubtask(i),taskState);
  }
}

GameValue TaskSetState(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Task *task = GetTask(oper1);
  if (!task) return GameValue();
  RString stateName = oper2;
  TaskState taskState = GetEnumValue<TaskState>((const char *)stateName);
  if (taskState != INT_MIN)
  {
    SubTaskSetState(task,taskState);
  }
  return GameValue();
};

GameValue TaskGetParent(const GameState *state, GameValuePar oper1)
{
  Task *task = GetTask(oper1);
  if (!task) return NOTHING;

  Task *parent = task->GetParent();
  if (parent)
    return CreateGameTask(parent);

  return NOTHING;
};

GameValue TaskGetChildren(const GameState *state, GameValuePar oper1)
{
  Task *parent = GetTask(oper1);
  if (!parent) return NOTHING;

  GameValue value = state->CreateGameValue(GameArray);
  GameArrayType &array = value;

  for (int i=0; i<parent->NSubtasks(); i++)
      array.Add(CreateGameTask(parent->GetSubtask(i)));
    
  return value;
};

GameValue TaskGetResult(const GameState *state, GameValuePar oper1)
{
  GameValue value = state->CreateGameValue(GameArray);

  Task *task = GetTask(oper1);
  if (!task) return value;

  GameArrayType &array = value;
  array.Realloc(2);
  array.Resize(2);

  TaskState taskState = task->GetState();
  array[0] = FindEnumName(taskState);
  array[1] = task->GetResult();
  return value;
};

GameValue TaskSetResult(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Task *task = GetTask(oper1);
  if (!task) return GameValue();

  const GameArrayType &array = oper2;
  if (array.Size() != 2)
  {
    state->SetError(EvalDim, array.Size(), 2);
    return GameValue();
  }
  if (array[0].GetType() != GameString)
  {
    state->TypeError(GameString, array[0].GetType());
    return GameValue();
  }

  RString stateName = array[0];
  TaskState taskState = GetEnumValue<TaskState>((const char *)stateName);
  if (taskState != INT_MIN)
  {
    // valid state
    task->SetState(taskState);
    task->SetResult(array[1]);
  }
  return GameValue();
};

GameValue TaskSendResult(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
#if _ENABLE_INDEPENDENT_AGENTS
  AITask *task = dynamic_cast<AITask *>(GetTask(oper1));
  if (!task) return GameValue();

  AITeamMember *sender = task->GetOwner();
  if (!sender || !GetSpeaker(sender)) return GameValue();

  AITeamMember *receiver = task->GetSender();
  if (!receiver) return GameValue();

  const GameArrayType &array = oper2;
  if (array.Size() != 3)
  {
    state->SetError(EvalDim, array.Size(), 2);
    return GameValue();
  }
  if (array[0].GetType() != GameString)
  {
    state->TypeError(GameString, array[0].GetType());
    return GameValue();
  }
  if (array[2].GetType() != GameString)
  {
    state->TypeError(GameString, array[2].GetType());
    return GameValue();
  }

  RString stateName = array[0];
  TaskState taskState = GetEnumValue<TaskState>((const char *)stateName);
  if (taskState != INT_MIN)
  {
    // valid state
    if (sender == receiver || GetSpeaker(sender)->GetTeamMember() == receiver)
    {
      // silent processing
      task->SetState(taskState);
      task->SetResult(array[1]);
    }
    else
    {
      RadioMessage *message = new RadioMessageTaskResult(task, taskState, array[1], array[2]);
      sender->SendMsg(message);
    }
  }
  return GameValue();
#else
  return GameValue();
#endif
};

GameValue TaskGetType(const GameState *state, GameValuePar oper1)
{
  Task *task = GetTask(oper1);
  if (!task) return RString();
  return task->GetName();
}

GameValue TaskGetDestination(const GameState *state, GameValuePar oper1)
{
  Task *task = GetTask(oper1);
  if (!task) return NOTHING;

  Vector3 destination;
  if (task->GetDestination(destination))
  {
    GameValue value = state->CreateGameValue(GameArray);
    GameArrayType &array = value;
    array.Add(destination[0]);
    array.Add(destination[2]);
    array.Add(0.0f);
    return array;
  }
 
  return NOTHING;
}

GameValue TaskGetDescription(const GameState *state, GameValuePar oper1)
{
  Task *task = GetTask(oper1);
  if (!task) return NOTHING;

  GameValue value = state->CreateGameValue(GameArray);
  GameArrayType &array = value;
  array.Resize(0);

  RString descripction;
  RString descripctionShort;
  RString descripctionHUD;
  if (task->GetDescription(descripction)) array.Add(descripction);
  else  array.Add("");
  if (task->GetDescriptionShort(descripctionShort)) array.Add(descripctionShort);
  else  array.Add("");
  if (task->GetDescriptionHUD(descripctionHUD)) array.Add(descripctionHUD);
  else  array.Add("");

  return array;
}

GameValue TaskGetPriority(const GameState *state, GameValuePar oper1)
{
  Task *task = GetTask(oper1);
  if (!task) return 0.0f;
  return task->GetPriority();
}

GameValue TeamMemberGetCurrentTasks(const GameState *state, GameValuePar oper1)
{
  GameValue value = state->CreateGameValue(GameArray);
  GameArrayType &array = value;

#if _ENABLE_INDEPENDENT_AGENTS
  AITeamMember *teamMember = GetTeamMember(oper1);
  if (teamMember)
  {
    int n = teamMember->NTasks();
    for (int i=0; i<n; i++)
    {
      Task *task = teamMember->GetTask(i);
      if (task && !task->IsCompleted()) array.Add(CreateGameTask(task));
    }
    array.Compact();
  }
#else
#endif

  return array;
}

GameValue TeamMemberGetRegisteredTasks(const GameState *state, GameValuePar oper1)
{
  GameValue value = state->CreateGameValue(GameArray);
  GameArrayType &array = value;

#if _ENABLE_INDEPENDENT_AGENTS
  AITeamMember *teamMember = GetTeamMember(oper1);
  if (teamMember)
  {
    int n = teamMember->NTaskTypes();
    for (int i=0; i<n; i++)
    {
      const AITaskType *taskType = teamMember->GetTaskType(i);
      if (taskType) array.Add(taskType->GetTypeName());
    }
    array.Compact();
  }
#else
#endif

  return array;
}

GameValue TaskNull(const GameState *state)
{
  return CreateGameTask(NULL);
}

GameValue TaskIsNull(const GameState *state, GameValuePar oper1)
{
  if (oper1.GetType() == GameTask)
  {
    Task *task = static_cast<GameDataTask *>(oper1.GetData())->GetTask();
    return !task;
  }
  else return true;
}

GameValue TaskCmpE( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  Task *task1 = GetTask(oper1);
  Task *task2 = GetTask(oper2);
  if( !task1 || !task2 ) return false;
  return task1==task2;
}

GameValue TaskCmpNE( const GameState *state, GameValuePar oper1, GameValuePar oper2 )
{
  Task *task1 = GetTask(oper1);
  Task *task2 = GetTask(oper2);
  if( !task1 || !task2 ) return true;
  return task1!=task2;
}

GameValue TaskGetVariable(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  AITask *task = dynamic_cast<AITask *>(GetTask(oper1));
  if (!task) return NOTHING;

  FSM *fsm = task->GetFSM();
  if (!fsm) return NOTHING;
  
  RString name = oper2;
  GameVarSpace *vars = fsm->GetVars();
  if (vars)
  {
    name.Lower();
    GameValue var;
    if (vars->VarGet(name, var)) return var;
  }

  return NOTHING;
}

GameValue TaskSetVariable(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  AITask *task = dynamic_cast<AITask *>(GetTask(oper1));
  if (!task) return NOTHING;

  FSMScripted *fsm = dynamic_cast<FSMScripted *>(task->GetFSM());
  if (!fsm) return NOTHING;
  
  const GameArrayType &array = oper2;
  if (array.Size() != 2)
  {
    state->SetError(EvalDim, array.Size(), 2);
    return NOTHING;
  }
  if (array[0].GetType() != GameString)
  {
    state->TypeError(GameString, array[0].GetType());
    return NOTHING;
  }

  fsm->SetParam(array[0], array[1]);

  return NOTHING;
}

#if _ENABLE_CHEATS

GameValue Diag_TaskDebugFSM(const GameState *state, GameValuePar oper1)
{
#if DEBUG_FSM
  AITask *task = dynamic_cast<AITask *>(GetTask(oper1));
  if (task)
  {
    FSM *fsm = task->GetFSM();
    if (fsm) fsm->Debug();
  }
#endif
  return NOTHING;
}

#endif

GameValue ObjSimpleTaskCreate(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Object *obj = GetObject(oper1);
  Person *person = dyn_cast<Person>(obj);
  if (!person) return CreateGameTask(NULL);

  const GameArrayType &array = oper2;
  Task *parent = NULL;
  RString name;
  switch (array.Size())
  {
  case 2:
    if (array[1].GetType() != GameTask)
    {
      state->TypeError(GameTask, array[1].GetType());
      return CreateGameTask(NULL);
    }
    parent = GetTask(array[1]);
    // continue with the next argument
  case 1:
    if (array[0].GetType() != GameString)
    {
      state->TypeError(GameString, array[0].GetType());
      return CreateGameTask(NULL);
    }
    name = array[0];
    break;
  default:
    state->SetError(EvalDim, array.Size(), 2);
    return CreateGameTask(NULL);
  }

  Ref<SimpleTask> task = new SimpleTask(person, parent, name);
  person->GetIdentity().AddTask(task);

  return CreateGameTask(task);
}

GameValue ObjSimpleTaskRemove(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Object *obj = GetObject(oper1);
  Person *person = dyn_cast<Person>(obj);
  if (!person) return NOTHING;

  Task *task = GetTask(oper2);
  if (task) person->GetIdentity().DeleteTask(task);

  return NOTHING;
}

GameValue ObjSetCurrentTask(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  Object *obj = GetObject(oper1);
  Person *person = dyn_cast<Person>(obj);
  if (!person) return NOTHING;

  Task *task = GetTask(oper2);
  Task *current = person->GetIdentity().GetCurrentTask();
  if (task){ 
    if(current && current->GetState() == TSAssigned ) current->SetState(TSCreated);
    person->GetIdentity().SetCurrentTask(task);
    task->SetState(TSAssigned);
  }

  return NOTHING;
}

GameValue SimpleTaskSetDestination(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  SimpleTask *task = dynamic_cast<SimpleTask *>(GetTask(oper1));
  Vector3 destination;
  bool ok = GetPos(state, destination, oper2);
  if (task && ok) task->SetDestination(destination);
  return NOTHING;
}

GameValue SimpleTaskSetTarget(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  SimpleTask *task = dynamic_cast<SimpleTask *>(GetTask(oper1));

  EntityAI *entity = NULL;
  bool exact = true;
  const GameArrayType &array = oper2;

  if (array.Size() >= 1 && array[0].GetType() == GameObject)
  {
    entity = dyn_cast<EntityAI>(GetObject(array[0]));
  }
  else return NOTHING;

  if (array.Size() >= 2 && array[1].GetType() == GameBool)
  {
    exact = (array[1]);
  }


  if (task && entity) task->SetTarget(entity, exact);
  return NOTHING;
}

GameValue SimpleTaskSetDescription(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  SimpleTask *task = dynamic_cast<SimpleTask *>(GetTask(oper1));
  const GameArrayType &array = oper2;
  if (array.Size() != 3)
  {
    state->SetError(EvalDim, array.Size(), 2);
    return NOTHING;
  }
  if (array[0].GetType() != GameString)
  {
    state->TypeError(GameString, array[0].GetType());
    return NOTHING;
  }
  if (array[1].GetType() != GameString)
  {
    state->TypeError(GameString, array[1].GetType());
    return NOTHING;
  }
  if (array[2].GetType() != GameString)
  {
    state->TypeError(GameString, array[2].GetType());
    return NOTHING;
  }

  if (task) task->SetDescription(array[0], array[1], array[2]);
  return NOTHING;
}

GameValue ObjGetSimpleTasks(const GameState *state, GameValuePar oper1)
{
  GameValue value = state->CreateGameValue(GameArray);
  GameArrayType &array = value;

  Object *obj = GetObject(oper1);
  Person *person = dyn_cast<Person>(obj);
  if (person)
  {
    const RefArray<Task> &tasks = person->GetIdentity().GetSimpleTasks();
    for (int i=0; i<tasks.Size(); i++)
    {
      Task *task = tasks[i];
      if (task) array.Add(CreateGameTask(task));
    }
    array.Compact();
  }

  return value;
}

GameValue ObjGetCurrentTask(const GameState *state, GameValuePar oper1)
{
  Object *obj = GetObject(oper1);
  Person *person = dyn_cast<Person>(obj);
  if (!person) return CreateGameTask(NULL);
  return CreateGameTask(person->GetIdentity().GetCurrentTask());
}

#if DOCUMENT_COMREF
static ComRefType ExtComRefType[] =
{
  TYPES_IDENTITY(COMREF_TYPE, "Identity")
};
#endif

// Nulars registrations
#define NULARS_IDENTITY(XX, Category) \
  XX(GameTask, "taskNull", TaskNull, "A non-existing task. This value is not equal to anything, including itself.", "taskNull == taskNull", "false", "5153", "", Category) \

NULARS_IDENTITY(JNI_NULAR, "Identity")

static const GameNular ExtNular[]=
{
  NULARS_IDENTITY(REGISTER_NULAR, "Identity")
};

#if DOCUMENT_COMREF
static ComRefFunc ExtComRefFunc0[] =
{
  NULARS_IDENTITY(COMREF_NULAR, "Identity")
};
#endif

// Functions registrations
#define FUNCTIONS_IDENTITY(XX, Category) \
  XX(GameString, "taskState", TaskGetState, GameTask, "task", "Return the state of the given task.", "", "", "2.89", "", Category) \
  XX(GameArray, "taskResult", TaskGetResult, GameTask, "task", "Return the result of the given task.", "", "", "2.92", "", Category) \
  XX(GameBool, "taskCompleted", TaskIsCompleted, GameTask, "task", "Return if task is completed. (state Succeeded, Failed or Canceled)", "", "", "2.89", "", Category) \
  XX(GameTask, "taskParent", TaskGetParent, GameTask, "task", "Return the parent task of the specified task.", "", "", "2.92", "", Category) \
  XX(GameArray, "taskChildren", TaskGetChildren, GameTask, "task", "Return the child tasks of the specified task.", "", "", "2.92", "", Category) \
  XX(GameString, "type", TaskGetType, GameTask, "task", "Return the type of the task.", "", "", "2.91", "", Category) \
  XX(GameArray, "taskDestination", TaskGetDestination, GameTask, "task", "Returns the position of the task (as specified by destination parameter in config).", "", "", "2.92", "", Category) \
  XX(GameArray, "taskDescription", TaskGetDescription, GameTask, "task", "Returns the descripction of the task.", "", "", "5501", "", Category) \
  XX(GameScalar, "priority", TaskGetPriority, GameTask, "task", "Return the priority of the task.", "", "", "2.91", "", Category) \
  XX(GameArray, "currentTasks", TeamMemberGetCurrentTasks, GameTeamMember, "teamMember", "List all uncompleted tasks.", "", "", "2.91", "", Category) \
  XX(GameArray, "registeredTasks", TeamMemberGetRegisteredTasks, GameTeamMember, "teamMember", "List all registered task types.", "", "", "2.91", "", Category) \
  XX(GameBool, "isNull", TaskIsNull, GameTask, "task", "Checks whether the value is equal to taskNull.\r\nNote: a==TaskNull does not work, because taskNull is not equal to anything, even to itself.", "isNull taskNull", "true", "5160", "", Category) \
  XX(GameString, "createDiaryLink", DiaryCreateLink, GameArray, "[subject, object, text]", "Create a link to the section of diary given by subject. Record is selected based on given object (diary record, task or unit).", "_link = createDiarySubject [\"Group\", player, \"Player\"]", "", "5500", "", Category) \
  XX(GameNothing, "processDiaryLink", DiaryProcessLink, GameString, "link", "Open the diary screen on the record specified by link.", "", "", "5500", "", Category) \
  XX(GameArray, "simpleTasks", ObjGetSimpleTasks, GameObject, "person", "Return all simple tasks assigned to given person.", "", "", "5501", "", Category) \
  XX(GameTask, "currentTask", ObjGetCurrentTask, GameObject, "person", "Return current task of given person.", "", "", "5501", "", Category) \

#define FUNCTIONS_IDENTITY_DEBUG(XX, Category) \
  XX(GameNothing, "diag_debugFSM", Diag_TaskDebugFSM, GameTask, "task", "The FSM debugger is opened for the FSM attached to this task.", "", "", "5153", "", Category) \

FUNCTIONS_IDENTITY(JNI_FUNCTION, "Identity")
#if _ENABLE_CHEATS
  FUNCTIONS_IDENTITY_DEBUG(JNI_FUNCTION, "Identity Debug")
#endif

static const GameFunction ExtUnary[]=
{
  FUNCTIONS_IDENTITY(REGISTER_FUNCTION, "Identity")
#if _ENABLE_CHEATS
  FUNCTIONS_IDENTITY_DEBUG(REGISTER_FUNCTION, "Identity Debug")
#endif
};

#if DOCUMENT_COMREF
static ComRefFunc ExtComRefFunc1[] =
{
  FUNCTIONS_IDENTITY(COMREF_FUNCTION, "Identity")
#if !ARMA_RELEASE_DOCUMENTATION
  FUNCTIONS_IDENTITY_DEBUG(COMREF_FUNCTION, "Identity Debug")
#endif
};
#endif

// Operators registration
#define OPERATORS_IDENTITY(XX, Category) \
  XX(GameBool, "registerTask", function, TeamMemberTaskRegister, GameTeamMember, GameString, "teamMember", "entry name", "Register a new task type. Parameters are defined in the given config class (subclass of CfgTasks).", "", "", "2.90", "", Category) \
  XX(GameBool, "unregisterTask", function, TeamMemberTaskUnregister, GameTeamMember, GameString, "teamMember", "name", "Unregister a task type.", "", "", "2.90", "", Category) \
  XX(GameTask, "createTask", function, TeamMemberTaskCreate, GameTeamMember, GameArray, "teamMember", "[[type] or [type, parentTask], priority, name1, value1, name2, value2, ...]", "Create a new AI task (subtask of parentTask). Type is name of registered task type.", "", "", "2.62", "", Category) \
  XX(GameTask, "sendTask", function, TeamMemberTaskSend, GameTeamMember, GameArray, "sender", "[receiver, [type] or [type, parentTask], priority, name1, value1, name2, value2, ...]", "Create a new AI task (subtask of parentTask). Type is name of registered task type.", "", "", "2.92", "", Category) \
  XX(GameScalar, "createDiarySubject", function, DiarySubjectCreate, GameObject, GameArray, "person", "[subject, display name] or [subject, display name, picture]", "Create a new subject page in a log.", "_index = player createDiarySubject [\"myPage\", \"My page\"]", "", "2.92", "", Category) \
  XX(GameDiaryRecord, "createDiaryRecord", function, DiaryRecordCreate, GameObject, GameArray, "person", "[subject, text(, task(, state))] or [subject, [title, text](, task(, state))]", "Create a new record in a log.", "_record = player createDiaryRecord [\"diary\", \"Kill all enemies.\"]", "", "2.53", "", Category) \
  XX(GameNothing, "setTaskState", function, TaskSetState, GameTask, GameString, "task", "state", "Set a new state of the task.", "", "", "2.89", "", Category) \
  XX(GameNothing, "setTaskResult", function, TaskSetResult, GameTask, GameArray, "task", "[state, result]", "Set a result of the task.", "", "", "2.92", "", Category) \
  XX(GameNothing, "sendTaskResult", function, TaskSendResult, GameTask, GameArray, "task", "[state, result, sentence]", "Send a result of the task to the task sender.", "", "", "2.92", "", Category) \
  XX(GameBool, "==", function, TaskCmpE, GameTask, GameTask, "task1", "task2", "Checks whether two tasks are equal.", "", "", "2.92", "", Category) \
  XX(GameBool, "!=", function, TaskCmpNE, GameTask, GameTask, "task1", "task2", "Checks whether two tasks are different.", "", "", "2.92", "", Category) \
  XX(GameVoid, "getVariable", function, TaskGetVariable, GameTask, GameString, "task", "name", "Return the value of variable in the variable space of given task.", "", "", "2.92", "", Category) \
  XX(GameNothing, "setVariable", function, TaskSetVariable, GameTask, GameArray, "task", "[name, value]", "Set variable to given value in the variable space of given task.", "", "", "2.92", "", Category) \
  XX(GameNothing, "selectDiarySubject", function, DiarySubjectSelect, GameObject, GameString, "person", "subject", "Select the subject page in a log.", "", "", "5500", "", Category) \
  XX(GameTask, "createSimpleTask", function, ObjSimpleTaskCreate, GameObject, GameArray, "person", "[name] or [name, parentTask]", "Create a new simple task (subtask of parentTask).", "", "", "5500", "", Category) \
  XX(GameNothing, "setSimpleTaskDestination", function, SimpleTaskSetDestination, GameTask, GameArray, "task", "position", "Attach a destination to the simple task. Overrides setSimpleTaskTarget.", "", "", "5500", "", Category) \
  XX(GameNothing, "setSimpleTaskTarget", function, SimpleTaskSetTarget, GameTask, GameArray, "task", "[target,precise position]", "Attach a target to the simple task. Overrides setSimpleTaskDestination.", "task setSimpleTaskTarget [vehicle, true]", "", "5500", "", Category) \
  XX(GameNothing, "setSimpleTaskDescription", function, SimpleTaskSetDescription, GameTask, GameArray, "task", "[description, descriptionShort, descriptionHUD]", "Attach descriptions to the simple task.", "", "", "5500", "", Category) \
  XX(GameNothing, "setCurrentTask", function, ObjSetCurrentTask, GameObject, GameTask, "person", "task", "Set the task as a current task of the person.", "", "", "5501", "", Category) \
  XX(GameBool, "diarySubjectExists", function, DiaryHasSubject, GameObject, GameString, "person", "name", "Checks whether given subject is present in the diary of given person.", "", "", "5501", "", Category) \
  XX(GameNothing, "removeSimpleTask", function, ObjSimpleTaskRemove, GameObject, GameTask, "person", "task", "Remove a simple task from the list of simple tasks.", "", "", "5501", "", Category) \

OPERATORS_IDENTITY(JNI_OPERATOR, "Identity")

static const GameOperator ExtBinary[]=
{
  OPERATORS_IDENTITY(REGISTER_OPERATOR, "Identity")
};

#if DOCUMENT_COMREF
static ComRefFunc ExtComRefFunc2[] =
{
  OPERATORS_IDENTITY(COMREF_OPERATOR, "Identity")
};
#endif

#include <El/Modules/modules.hpp>

INIT_MODULE(Identity, 2)
{
  GameState &state = GGameState; // helper to make macro works
  TYPES_IDENTITY(REGISTER_TYPE, "Identity")

  for (int i=0; i<lenof(ExtNular); i++)
  {
    GGameState.NewNularOp(ExtNular[i]);
  }
  for (int i=0; i<lenof(ExtUnary); i++)
  {
    GGameState.NewFunction(ExtUnary[i]);
  }
  for (int i=0; i<lenof(ExtBinary); i++)
  {
    GGameState.NewOperator(ExtBinary[i]);
  }

#if DOCUMENT_COMREF
  for (int i=0; i<lenof(ExtComRefType); i++)
  {
    GGameState.AddComRefType(ExtComRefType[i]);
  }
  for (int i=0; i<lenof(ExtComRefFunc0); i++)
  {
    GGameState.AddComRefFunction(ExtComRefFunc0[i]);
  }
  for (int i=0; i<lenof(ExtComRefFunc1); i++)
  {
    GGameState.AddComRefFunction(ExtComRefFunc1[i]);
  }
  for (int i=0; i<lenof(ExtComRefFunc2); i++)
  {
    GGameState.AddComRefFunction(ExtComRefFunc2[i]);
  }
#endif
}

#endif // _ENABLE_IDENTITIES
