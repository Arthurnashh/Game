#include "../wpch.hpp"
#include "kbExt.hpp"

#if _ENABLE_CONVERSATION

#include <Es/Algorithms/qsort.hpp>
#include <El/ParamFile/paramFile.hpp>
#include <El/Evaluator/express.hpp>
#include "../object.hpp"
#include "../landscape.hpp"
#include "../gameStateExt.hpp"

//////////////////////////////////////////////////////////////////////////
//
// Interface (hidden classes)
//

struct AmountValue
{
  RString _value;
  RString _text;
  AutoArray<RString> _speech;
};
TypeIsMovable(AmountValue)

class KBParameterAmount : public KBParameter
{
protected:
  AutoArray<AmountValue> _values;

public:
  KBParameterAmount(RString name, ParamEntryVal argument);
  virtual bool CreateArgument(KBArgument &result, GameValuePar value) const;
  virtual bool CreateArgument(KBArgument &result, RString value) const;
};

struct SideValue
{
  RString _text;
  AutoArray<RString> _speech;
};

class KBParameterSide : public KBParameter
{
protected:
  SideValue _values[TSideUnknown];

public:
  KBParameterSide(RString name, ParamEntryVal argument);
  virtual bool CreateArgument(KBArgument &result, GameValuePar value) const;
  virtual bool CreateArgument(KBArgument &result, RString value) const;
};

class KBParameterEntity : public KBParameter
{
protected:
  RString _textQueriedPlural;
  AutoArray<RString> _speechQueriedPlural;
  RString _textQueriedSingular;
  AutoArray<RString> _speechQueriedSingular;
  /// variant of speech preferred for entity dubbing
  RString _variant;

public:
  KBParameterEntity(RString name, ParamEntryVal argument);
  virtual bool CreateArgument(KBArgument &result, GameValuePar value) const;
  virtual bool CreateArgument(KBArgument &result, RString value) const;
};

struct PlacePhrase
{
  RString _value;
  RString _text;
  AutoArray<RString> _speech;
};
TypeIsMovable(PlacePhrase)

struct PlaceDirection
{
  float _azimuth; // in radians
  RString _text;
  AutoArray<RString> _speech;
};
TypeIsMovable(PlaceDirection)

class KBParameterPlace : public KBParameter
{
protected:
  AutoArray<PlacePhrase> _phrases; // phrases used for different distances from location
  AutoArray<PlacePhrase> _phrasesSpecial; // phrases used for pronounces "there", "here"
  AutoArray<PlaceDirection> _directions;
  RString _textThis;
  AutoArray<RString> _speechThis;
  RString _textQueried;
  AutoArray<RString> _speechQueried;
  /// variant of speech preferred for location dubbing
  RString _locationVariant;

public:
  KBParameterPlace(RString name, ParamEntryVal argument);
  virtual void GetValues(AutoArray<RString> &values) const;
  virtual bool CreateArgument(KBArgument &result, GameValuePar value) const;
  virtual bool CreateArgument(KBArgument &result, RString value) const;
};

struct TimeValue
{
  RString _value;
  RString _text;
  AutoArray<RString> _speech;
};
TypeIsMovable(TimeValue)

class KBParameterTime : public KBParameter
{
protected:
  AutoArray<TimeValue> _values;

public:
  KBParameterTime(RString name, ParamEntryVal argument);
  virtual bool CreateArgument(KBArgument &result, GameValuePar value) const;
  virtual bool CreateArgument(KBArgument &result, RString value) const;
};

class KBMessageTemplateExt : public KBMessageTemplate
{
public:
  KBMessageTemplateExt() {}
  KBMessageTemplateExt(const KBMessageTemplate &src, RString text, GameValuePar speech)
    : KBMessageTemplate(src, text, speech) {}
  virtual KBParameter *CreateParameter(ParamEntryVal cls, ParamEntryVal arguments);
};

//////////////////////////////////////////////////////////////////////////
//
// Implementation
//

KBParameterAmount::KBParameterAmount(RString name, ParamEntryVal argument)
: KBParameter(name)
{
  for (int i=0; i<argument.GetEntryCount(); i++)
  {
    ParamEntryVal cls = argument.GetEntry(i);
    if (!cls.IsClass()) continue;
    AmountValue &value = _values.Append();
    value._value = cls.GetName();
    value._text = cls >> "text";
    LoadSpeech(value._speech, cls >> "speech");
  }
  _values.Compact();
}

bool KBParameterAmount::CreateArgument(KBArgument &result, GameValuePar value) const
{
  result._name = _name;
  if (value.GetType() != GameScalar) return false;
  float amount = value;

  // find the first matching
  for (int i=0; i<_values.Size(); i++)
  {
    const AmountValue &val = _values[i];
    if (val._value[0] == 'N')
    {
      // always valid
      result._value = value;
      result._text = val._text;
      result._speech = val._speech;
      return true;
    }
    else
    {
      float limit = atof(val._value);
      if (amount <= limit)
      {
        result._value = value;
        result._text = val._text;
        result._speech = val._speech;
        return true;
      }
    }
  }

  return false;
}

bool KBParameterAmount::CreateArgument(KBArgument &result, RString value) const
{
  result._name = _name;
  for (int i=0; i<_values.Size(); i++)
  {
    const AmountValue &val = _values[i];
    if (stricmp(val._value, value) == 0)
    {
      result._value = GameValue(value);
      result._text = val._text;
      result._speech = val._speech;
      return true;
    }
  }
  return false;
}

KBParameterSide::KBParameterSide(RString name, ParamEntryVal argument)
: KBParameter(name)
{
  for (int i=0; i<argument.GetEntryCount(); i++)
  {
    ParamEntryVal cls = argument.GetEntry(i);
    if (!cls.IsClass()) continue;
    TargetSide side = GetEnumValue<TargetSide>(cc_cast(cls.GetName()));
    if (side >= 0 && side <TSideUnknown)
    {
      _values[side]._text = cls >> "text";
      LoadSpeech(_values[side]._speech, cls >> "speech");
    }
  }
}

bool KBParameterSide::CreateArgument(KBArgument &result, GameValuePar value) const
{
  result._name = _name;

  if (value.GetType() != GameSide) return false; // "$" value ignored here
  TargetSide side = GetSide(value);
  if (side < 0 || side >= TSideUnknown) return false;

  result._value = value;
  result._text = _values[side]._text;
  result._speech = _values[side]._speech;
  return true;
}

bool KBParameterSide::CreateArgument(KBArgument &result, RString value) const
{
  result._name = _name;

  TargetSide side = GetEnumValue<TargetSide>(cc_cast(value));
  if (side < 0 || side >= TSideUnknown) return false;

  result._value = GameValueExt(side);
  result._text = _values[side]._text;
  result._speech = _values[side]._speech;
  return true;
}

KBParameterEntity::KBParameterEntity(RString name, ParamEntryVal argument)
: KBParameter(name)
{
  _textQueriedPlural = argument >> "textQueriedPlural";
  LoadSpeech(_speechQueriedPlural, argument >> "speechQueriedPlural");
  _textQueriedSingular = argument >> "textQueriedSingular";
  LoadSpeech(_speechQueriedSingular, argument >> "speechQueriedSingular");
  _variant = argument >> "variant";
}

bool KBParameterEntity::CreateArgument(KBArgument &result, GameValuePar value) const
{
  RString type;
  if (value.GetType() == GameString) type = value;
  return CreateArgument(result, type);
}

bool KBParameterEntity::CreateArgument(KBArgument &result, RString value) const
{
  result._name = _name;

  if (value.GetLength() == 0) return false;

  result._value = GameValue(value);
  bool plural = value[0] == '*';
  RString name = plural ? RString(cc_cast(value) + 1) : value;

  if (strcmp(name, "$") == 0)
  {
    if (plural)
    {
      result._text = _textQueriedPlural;
      result._speech = _speechQueriedPlural;
    }
    else
    {
      result._text = _textQueriedSingular;
      result._speech = _speechQueriedSingular;
    }
    return true;
  }
  else
  {
    Ref<EntityType> type = VehicleTypes.New(name);
    if (!type) return false;
    EntityAIType *aiType = dynamic_cast<EntityAIType *>(type.GetRef());
    if (!aiType) return false;

    const EntitySpeechVariant *variant = aiType->GetSpeechVariant(_variant);
    if (plural)
    {
      result._text = aiType->GetTextPlurar();
      result._speech = variant ? variant->_speechPlural : aiType->GetSpeechPlurar();
    }
    else
    {
      result._text = aiType->GetTextSingular();
      result._speech = variant ? variant->_speechSingular : aiType->GetSpeechSingular();
    }
  }
  return true;
}

KBParameterPlace::KBParameterPlace(RString name, ParamEntryVal argument)
: KBParameter(name)
{
  ParamEntryVal phrases = argument >> "Phrases";
  for (int i=0; i<phrases.GetEntryCount(); i++)
  {
    ParamEntryVal cls = phrases.GetEntry(i);
    if (!cls.IsClass()) continue;
    PlacePhrase &phrase = _phrases.Append();
    phrase._value = cls.GetName();
    phrase._text = cls >> "text";
    LoadSpeech(phrase._speech, cls >> "speech");
  }
  _phrases.Compact();

  phrases = argument >> "PhrasesSpecial";
  for (int i=0; i<phrases.GetEntryCount(); i++)
  {
    ParamEntryVal cls = phrases.GetEntry(i);
    if (!cls.IsClass()) continue;
    PlacePhrase &phrase = _phrasesSpecial.Append();
    phrase._value = cls.GetName();
    phrase._text = cls >> "text";
    LoadSpeech(phrase._speech, cls >> "speech");
  }
  _phrasesSpecial.Compact();

  ParamEntryVal directions = argument >> "Directions";
  for (int i=0; i<directions.GetEntryCount(); i++)
  {
    ParamEntryVal cls = directions.GetEntry(i);
    if (!cls.IsClass()) continue;
    PlaceDirection &direction = _directions.Append();
    direction._azimuth = atof(cls.GetName()) * (H_PI / 180.0f);
    direction._text = cls >> "text";
    LoadSpeech(direction._speech, cls >> "speech");
  }
  _directions.Compact();

  _textThis = argument >> "textThis";
  LoadSpeech(_speechThis, argument >> "speechThis");
  _textQueried = argument >> "textQueried";
  LoadSpeech(_speechQueried, argument >> "speechQueried");

  _locationVariant = argument >> "variant";
}

#if USE_LANDSCAPE_LOCATIONS
// select all locations
static bool LocationAll(int x, int y, int size)
{
  return true;
}

struct LocationCollectNames
{
  RefArray<Location> _locations;

  LocationCollectNames()
  {
  }

  bool operator () (const Ref<LocationsItem> &item)
  {
    for (int i=0; i<item->_locations.Size(); i++)
    {
      Location *location = item->_locations[i];
      if (location->_speech.Size() > 0)
        _locations.Add(location);
    }
    return false; // continue with enumeration
  }
};

struct LocationFindName
{
  RString _name;
  Ref<Location> _location;

  LocationFindName(RString name)
  {
    _name = name;
  }

  bool operator () (const Ref<LocationsItem> &item)
  {
    for (int i=0; i<item->_locations.Size(); i++)
    {
      Location *location = item->_locations[i];
      if (location->_speech.Size() > 0 && location->_text == _name)
      {
        _location = location;
        return true; // found
      }
    }
    return false; // continue with enumeration
  }
};

int CmpLocationsByImportance(const Ref<Location> *location1, const Ref<Location> *location2);

#endif

void KBParameterPlace::GetValues(AutoArray<RString> &values) const
{
#if USE_LANDSCAPE_LOCATIONS
  // collect all names
  LocationCollectNames func;
  GLandscape->GetLocations().ForEachInRegion(LocationAll, func);
  int n = func._locations.Size();
  // sort by importance
  QSort(func._locations.Data(), n, CmpLocationsByImportance);
  // copy to output
  values.Realloc(n);
  values.Resize(n);
  for (int i=0; i<n; i++) values[i] = func._locations[i]->_text;
#endif
}

bool KBParameterPlace::CreateArgument(KBArgument &result, GameValuePar value) const
{
  result._name = _name;
#if USE_LANDSCAPE_LOCATIONS
  // decode the value
  if (value.GetType() != GameArray) return false;
  const GameArrayType &array = value;
  if (array.Size() != 3) return false;
  if (array[0].GetType() != GameArray) return false;
  if (array[1].GetType() != GameScalar) return false;
  if (array[2].GetType() != GameString) return false;
  const GameArrayType &subarray = array[0];
  if (subarray.Size() < 2) return false;
  if (subarray[0].GetType() != GameScalar) return false;
  if (subarray[1].GetType() != GameScalar) return false;

  RString type = array[2];

  // results in raw form
  Vector3 dir;
  RString locationText;
  AutoArray<RString> locationSpeech;
  const AutoArray<PlacePhrase> *phrases = NULL;
  float radius = array[1];

  if (strcmp(type, ".") == 0)
  {
    // relative position
    dir[0] = subarray[0];
    dir[1] = 0;
    dir[2] = subarray[1];
    locationText = _textThis;
    locationSpeech = _speechThis;
    phrases = &_phrasesSpecial;
  }
  else if (strcmp(type, "$") == 0)
  {
    // relative position
    dir[0] = subarray[0];
    dir[1] = 0;
    dir[2] = subarray[1];
    locationText = _textQueried;
    locationSpeech = _speechQueried;
    phrases = &_phrasesSpecial;
  }
  else
  {
    // absolute position
    Vector3 position;
    position[0] = subarray[0];
    position[1] = 0;
    position[2] = subarray[1];

    // find nearest named place
    Location *location = GLandscape->GetLocations().FindNearestWithDubbing(position, FLT_MAX);
    if (!location) return false;

    // results
    dir = location->_position - position;
    locationText = location->_text;
    const LocationSpeechVariant *variant = location->GetSpeechVariant(_locationVariant);
    locationSpeech = variant ? variant->_speech : location->_speech;
    phrases = &_phrases;
  }

  // distance and direction
  float distance = dir.SizeXZ();
  bool sameDirection = distance >= radius;

  // find the first matching phrase
  for (int i=0; i<phrases->Size(); i++)
  {
    const PlacePhrase &phrase = phrases->Get(i);
    RString name = phrase._value;
    bool directionRequired = false;
    if (name[0] == 'D')
    {
      if (!sameDirection) continue;
      directionRequired = true;
      name = name.Substring(1, INT_MAX);
    }
    if (name[0] != 'N')
    {
      // limit is given
      float limit = atof(name);
      if (distance > limit) continue;
    }
    // phrase found

    // fill subarguments
    KBArguments args;
    args.Set("PLACE", GameValue(), locationText, locationSpeech);
    if (directionRequired)
    {
      float azimuth = atan2(dir.X(), dir.Z());
      // search for the direction description
      float minDiff = 2.0f * H_PI;
      int best = -1;
      for (int j=0; j<_directions.Size(); j++)
      {
        float diff = fabs(AngleDifference(azimuth, _directions[j]._azimuth));
        if (diff < minDiff)
        {
          best = j;
          minDiff = diff;
        }
      }
      if (best >= 0)
        args.Set("DIRECTION", GameValue(), _directions[best]._text, _directions[best]._speech);
    }

    // create an argument
    result._value = value;
    result._text = FormatPhrase(phrase._text, args);
    FormatSpeech(result._speech, phrase._speech, args);
    return true;
  }
  
#endif
  return false;
}

bool KBParameterPlace::CreateArgument(KBArgument &result, RString value) const
{
  result._name = _name;
#if USE_LANDSCAPE_LOCATIONS
  LocationFindName func(value);
  if (!GLandscape->GetLocations().ForEachInRegion(LocationAll, func)) return false;
  Assert(func._location);

  result._value = GGameState.CreateGameValue(GameArray);
  GameArrayType &array = result._value;
  array.Realloc(2);
  array.Resize(2);
  array[0] = func._location->_position.X();
  array[1] = func._location->_position.Z();
  
  result._text = value;
  // result._speech

  return true;
#else
  return false;
#endif
}

KBParameterTime::KBParameterTime(RString name, ParamEntryVal argument)
: KBParameter(name)
{
  for (int i=0; i<argument.GetEntryCount(); i++)
  {
    ParamEntryVal cls = argument.GetEntry(i);
    if (!cls.IsClass()) continue;
    TimeValue &value = _values.Append();
    value._value = cls.GetName();
    value._text = cls >> "text";
    LoadSpeech(value._speech, cls >> "speech");
  }
  _values.Compact();
}

bool KBParameterTime::CreateArgument(KBArgument &result, GameValuePar value) const
{
  result._name = _name;
  if (value.GetType() != GameArray) return false;  // "$" value ignored here
  const GameArrayType &array = value;
  if (array.Size() != 2) return false;
  
  float timeAbs = array[1];
  float timeDays = timeAbs * (1.0f / (24.0f * 60.0f * 60.0f));
  int day = toIntCeil(timeDays - Glob.clock.GetTimeOfDay());

  // find the first matching
  for (int i=0; i<_values.Size(); i++)
  {
    const TimeValue &time = _values[i];
    if (time._value[0] == 'N')
    {
      // always valid
      result._value = value;
      result._text = time._text;
      result._speech = time._speech;
      return true;
    }
    else if (time._value[0] == 'D')
    {
      int limit = atoi(cc_cast(time._value) + 1);
      if (day <= limit)
      {
        result._value = value;
        result._text = time._text;
        result._speech = time._speech;
        return true;
      }
    }
    else
    {
      float limit = atof(time._value);
      if (timeAbs <= limit)
      {
        result._value = value;
        result._text = time._text;
        result._speech = time._speech;
        return true;
      }
    }
  }

  return false;
}

bool KBParameterTime::CreateArgument(KBArgument &result, RString value) const
{
  result._name = _name;
  for (int i=0; i<_values.Size(); i++)
  {
    const TimeValue &time = _values[i];
    if (stricmp(time._value, value) == 0)
    {
      result._value = GameValue(value);
      result._text = time._text;
      result._speech = time._speech;
      return true;
    }
  }
  return false;
}

KBParameter *KBMessageTemplateExt::CreateParameter(ParamEntryVal cls, ParamEntryVal arguments)
{
  RString type = cls >> "type";
  if (stricmp(type, "amount") == 0)
  {
    // config definition expected
    RString config = cls >> "config";
    return new KBParameterAmount(cls.GetName(), arguments >> config);
  }
  if (stricmp(type, "side") == 0)
  {
    // config definition expected
    RString config = cls >> "config";
    return new KBParameterSide(cls.GetName(), arguments >> config);
  }
  if (stricmp(type, "entity") == 0)
  {
    // config definition expected
    RString config = cls >> "config";
    return new KBParameterEntity(cls.GetName(), arguments >> config);
  }
  if (stricmp(type, "place") == 0)
  {
    // config definition expected
    RString config = cls >> "config";
    return new KBParameterPlace(cls.GetName(), arguments >> config);
  }
  if (stricmp(type, "time") == 0)
  {
    // config definition expected
    RString config = cls >> "config";
    return new KBParameterTime(cls.GetName(), arguments >> config);
  }
  return KBMessageTemplate::CreateParameter(cls, arguments);
}

KBMessageTemplate *KBTopicExt::CreateTemplate() const
{
  return new KBMessageTemplateExt();
}

KBMessageTemplate *KBTopicExt::CreateTemplate(const KBMessageTemplate &src, RString text, GameValuePar speech) const
{
  return new KBMessageTemplateExt(src, text, speech);
}

DEFINE_SERIALIZE_TYPE_INFO(KBTopicExt, KBTopic);

//////////////////////////////////////////////////////////////////////////
//
// Scripting functions
//

Object *GetObject(GameValuePar oper);

// KBTargetsQuery

struct TimeInterval
{
  float timeMin;
  float timeMax;
};

struct Area
{
  Vector3 center;
  float radius;
};

static const float ThresholdCandidate = 0;

static const float NearRadius = 200.0f;

static const float MaxTypeDistance = 10.0f;

static const float MetricSide = 10000.0f;
static const float MetricType = 1000.0f;
static const float MetricPosition = 1.0f;
static const float MetricTime = 0.1f;

// Query

template <typename Type>
struct TargetListQueryItem
{
  Type value;
  bool valid;

  TargetListQueryItem() {valid = false;}
  void Init(GameValuePar arg); // implemented in specializations
};

template <>
void TargetListQueryItem<TargetSide>::Init(GameValuePar arg)
{
  if (arg.GetType() == GameSide)
  {
    value = GetSide(arg);
    valid = value >= 0 && value < TSideUnknown;
  }
}

template <>
void TargetListQueryItem< Ref<const EntityType> >::Init(GameValuePar arg)
{
  if (arg.GetType() == GameString)
  {
    RString name = arg;
    value = VehicleTypes.New(name);
    valid = value != NULL;
  }
}

template <>
void TargetListQueryItem<Area>::Init(GameValuePar arg)
{
  if (arg.GetType() == GameArray)
  {
    const GameArrayType &array = arg;
    if (array.Size() >= 2 && array[0].GetType() == GameScalar && array[1].GetType() == GameScalar)
    {
      value.center[0] = array[0];
      value.center[1] = 0;
      value.center[2] = array[1];
      value.radius = NearRadius;
      valid = true;
    }
  }
}

template <>
void TargetListQueryItem<TimeInterval>::Init(GameValuePar arg)
{
  if (arg.GetType() == GameScalar)
  {
    value.timeMin = 0;
    value.timeMax = arg;
    valid = value.timeMax > 0;
  }
}

struct TargetListQuery
{
  TargetListQueryItem<TargetSide> side;
  TargetListQueryItem< Ref<const EntityType> > type;
  TargetListQueryItem<Area> area;
  TargetListQueryItem<TimeInterval> interval;
};

// Candidate

template <typename Type, typename QueryItemType = Type>
struct TargetCandidateItem
{
  Type value;
  float coef;

  TargetCandidateItem() {coef = 1.0f;}
  void CalculateMatching(const TargetListQueryItem<QueryItemType> &wanted); // implemented in specializations
};

template<>
void TargetCandidateItem<TargetSide>::CalculateMatching(const TargetListQueryItem<TargetSide> &wanted)
{
  if (wanted.valid)
    coef = (value == wanted.value) ? 1.0f : 0.01f;
}

template<>
void TargetCandidateItem< Ref<const EntityAIType>, Ref<const EntityType> >::CalculateMatching(const TargetListQueryItem< Ref<const EntityType> > &wanted)
{
  if (wanted.valid)
  {
    coef = 1.0f;
    const EntityType *wantedType = wanted.value;
    while (wantedType)
    {
      if (value->IsKindOf(wantedType)) return;
      wantedType = wantedType->_parentType;
      coef *= 0.5f;
    }
    coef = 0;
  }
}

template<>
void TargetCandidateItem<Vector3, Area>::CalculateMatching(const TargetListQueryItem<Area> &wanted)
{
  if (wanted.valid)
  {
    float dist = value.DistanceXZ(wanted.value.center);
    if (dist < wanted.value.radius) coef = 1.0f;
    else coef = wanted.value.radius / dist;
  }
}

template<>
void TargetCandidateItem<float, TimeInterval>::CalculateMatching(const TargetListQueryItem<TimeInterval> &wanted)
{
  if (wanted.valid)
  {
    if (value < wanted.value.timeMin) coef = value / wanted.value.timeMin;
    else if (value > wanted.value.timeMax) coef = wanted.value.timeMax / value;
    else coef = 1.0f;
  }
}

float GetDistance(TargetSide side1, TargetSide side2)
{
  return side1 == side2 ? 0.0f : 1.0f;
}

enum GetDistanceHelper {EntityTypePtr};
float GetDistance(const EntityType *type1, const EntityType *type2, GetDistanceHelper val=EntityTypePtr)
{
  if (type1 == type2) return 0;
  if (!type1 || !type2) return MaxTypeDistance;

  float dist1 = 0;
  while (type1)
  {
    if (type2->IsKindOf(type1)) break;
    type1 = type1->_parentType;
    dist1++;
  }
  if (!type1) return MaxTypeDistance;

  float dist2 = 0;
  while (type2)
  {
    if (type2 == type1) return dist2;
    type2 = type2->_parentType;
    dist2++;
    if (dist2 >= dist1) return dist1;
  }
  return MaxTypeDistance;
}

float GetDistance(Vector3Par position1, Vector3Par position2)
{
  return position2.Distance(position1);
}

float GetDistance(float time1, float time2)
{
  return fabs(time2 - time1);
}

float GetDistance(Vector3Par position, const Area &area)
{
  float dist = area.center.Distance(position);
  return floatMax(dist - area.radius, 0);
}

float GetDistance(float time, const TimeInterval &interval)
{
  if (time < interval.timeMin) return interval.timeMin - time;
  if (time > interval.timeMax) return time - interval.timeMax;
  return 0;
}

struct TargetCandidate : public RefCount
{
  TargetCandidateItem<TargetId> idExact; ///< unique target identification (link to real object)
  TargetCandidateItem<TargetSide> side; ///< expected target side
  TargetCandidateItem< Ref<const EntityAIType>, Ref<const EntityType> > type; ///< expected target type
  TargetCandidateItem<Vector3, Area> position; ///< last known target position
  TargetCandidateItem<float, TimeInterval> time; ///< when target was last seen
  float coefTotal;
  int order; // helper for aggregation

  TargetCandidate();

  void Init(Target *target);
  float Check(const TargetListQuery &query);

  float GetDistance(TargetCandidate *candidate);
};

TargetCandidate::TargetCandidate()
{
  order = -1; // order is defined after sort
}

void TargetCandidate::Init(Target *target)
{
  idExact.value = target->idExact;
  side.value = target->GetSide();
  type.value = target->GetType();
  position.value = target->GetPosition();
  time.value = Glob.time - target->GetLastSeen();
}

float TargetCandidate::Check(const TargetListQuery &query)
{
  side.CalculateMatching(query.side);
  type.CalculateMatching(query.type);
  position.CalculateMatching(query.area);
  time.CalculateMatching(query.interval);
  coefTotal = side.coef * type.coef * position.coef * time.coef;
  return coefTotal;
}

float TargetCandidate::GetDistance(TargetCandidate *candidate)
{
  return 
    MetricSide * ::GetDistance(candidate->side.value, side.value) +
    MetricType * ::GetDistance(candidate->type.value, type.value, EntityTypePtr) +
    MetricPosition * ::GetDistance(candidate->position.value, position.value) +
    MetricTime * ::GetDistance(candidate->time.value, time.value);
}

static int CmpCandidates(Ref<TargetCandidate> *c1, Ref<TargetCandidate> *c2)
{
  TargetCandidate *candidate1 = *c1;
  TargetCandidate *candidate2 = *c2;

  float diff = candidate2->coefTotal - candidate1->coefTotal; // descending sort order
  if (diff != 0) return sign(diff);

  return ComparePointerAddresses(candidate1, candidate2);
}

class CollectTargetCandidates : public TargetFunctor
{
protected:
  const TargetListQuery &_query;
  StaticArrayAuto< Ref<TargetCandidate> > &_candidates;
  const Object *_me;
  const Object *_you;

public:
  CollectTargetCandidates(
    const TargetListQuery &query, StaticArrayAuto< Ref<TargetCandidate> > &candidates,
    const Object *me, const Object *you)
    : _query(query), _candidates(candidates), _me(me), _you(you) {}

    // return true to break enumeration
    virtual bool operator ()(Target *target) const;
};

bool CollectTargetCandidates::operator ()(Target *target) const
{
  // only valid targets
  if (target->IsVanishedOrDestroyed() || !target->IsKnown() /* ??? */) return false;
  // avoid dialog participants
  if (_me && target->idExact == _me || _you && target->idExact == _you) return false;
  // select only vehicles
  if (!target->GetType()->IsKindOf(GWorld->Preloaded(VTypeAllVehicles))) return false;

  Ref<TargetCandidate> candidate = new TargetCandidate();
  candidate->Init(target);
  if (candidate->Check(_query) > ThresholdCandidate) _candidates.Add(candidate);
  return false; // continue with enumeration
}

GameValue KBTargetsQuery(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  GameValue value = GGameState.CreateGameValue(GameArray);

  // check and load parameters
  Object *me = GetObject(oper1);
  Person *person = dyn_cast<Person>(me);
  if (!person) return value;
  AIBrain *brain = person->Brain();
  if (!brain) return value;

  const GameArrayType &array = oper2;
  if (array.Size() != 5)
  {
    state->SetError(EvalDim, array.Size(), 5);
    return value;
  }
  Object *you = GetObject(array[0]);
  TargetListQuery query;
  query.side.Init(array[1]);
  query.type.Init(array[2]);
  query.area.Init(array[3]);
  query.interval.Init(array[4]);

  // create list of candidates
  AUTO_STATIC_ARRAY(Ref<TargetCandidate>, candidates, 32);
  CollectTargetCandidates func(query, candidates, me, you);
  brain->ForEachTarget(func);
  int n = candidates.Size();
  if (n == 0) return value; // no candidates

  // sort candidates
  QSort(candidates.Data(), n, CmpCandidates);

  // create the output
  GameArrayType &result = value;
  result.Realloc(n);
  result.Resize(n);
  for (int i=0; i<n; i++)
  {
    result[i] = GGameState.CreateGameValue(GameArray);
    GameArrayType &array = result[i];
    array.Realloc(6);
    array.Resize(6);
    
    const TargetCandidate *candidate = candidates[i];
    array[0] = candidate->coefTotal;
    array[1] = GameValueExt((TargetType*)candidate->idExact.value);
    array[2] = GameValueExt(candidate->side.value);
    array[3] = candidate->type.value->GetName();
    array[4] = GGameState.CreateGameValue(GameArray);
    GameArrayType &position = array[4];
    position.Realloc(2);
    position.Resize(2);
    position[0] = candidate->position.value.X();
    position[1] = candidate->position.value.Z();
    array[5] = candidate->time.value;
  }

  return value;
}

// KBTargetsAggregate

static const float ConcretnessSide = 1.0f;
static const float ConcretnessType = 1.0f;
static const float ConcretnessPosition = 1.0f;
static const float ConcretnessTime = 1.0f;

static const float ExpAmount = 2.0f;

TargetSide GetParent(TargetSide side1, TargetSide side2)
{
  return side1 == side2 ? side1 : TSideUnknown;
}

float GetConcretness(TargetSide side)
{
  return side == TSideUnknown ? 0.0f : 1.0f;
}

const EntityAIType *GetParent(const EntityAIType *type1, const EntityAIType *type2)
{
  if (type2 == NULL) return NULL;
  while (type1)
  {
    if (type2->IsKindOf(type1)) return type1;
    type1 = dynamic_cast<const EntityAIType *>(type1->_parentType.GetRef());
  }
  return NULL;
}

float GetConcretness(const EntityType *type)
{
  float coef = 0;
  while (type)
  {
    type = type->_parentType;
    coef += 0.1;
  }
  return coef;
}

Area GetParent(const Area &area, Vector3Par position)
{
  float dist = position.Distance(area.center);
  if (dist <= area.radius) return area;

  Area result;
  float offset = 0.5 * (dist - area.radius);
  Vector3 dir = (position - area.center).Normalized();
  result.center = area.center + offset * dir;
  result.radius = area.radius + offset;
  return result;
}

float GetConcretness(const Area &area)
{
  return 1.0f / (1.0f + 0.01f * area.radius);
}

TimeInterval GetParent(const TimeInterval &interval, float time)
{
  TimeInterval result = interval;
  saturateMin(result.timeMin, time);
  saturateMax(result.timeMax, time);
  return result;
}

float GetConcretness(const TimeInterval &interval)
{
  return 1.0f / (1.0f + 0.001 * (interval.timeMax - interval.timeMin));
}

struct TargetAggregation : public RefCount
{
  TargetSide side;
  Ref<const EntityAIType> type;
  Area area;
  TimeInterval time;
  float matched;

  RefArray<TargetCandidate> candidates;

  void Add(TargetCandidate *candidate);
  float GetDistance(TargetCandidate *candidate);

  // how good this aggregation is
  float GetQuality() const;
};

void TargetAggregation::Add(TargetCandidate *candidate)
{
  if (candidates.Size() == 0)
  {
    side = candidate->side.value;
    type = candidate->type.value;
    area.center = candidate->position.value;
    area.radius = 0;
    time.timeMin = time.timeMax = candidate->time.value;
    matched = candidate->coefTotal;
  }
  else
  {
    side = GetParent(side, candidate->side.value);
    type = GetParent(type, candidate->type.value.GetRef());
    area = GetParent(area, candidate->position.value);
    time = GetParent(time, candidate->time.value);
    saturateMax(matched, candidate->coefTotal);
  }

  candidates.Add(candidate);
}

float TargetAggregation::GetDistance(TargetCandidate *candidate)
{
  return 
    MetricSide * ::GetDistance(candidate->side.value, side) +
    MetricType * ::GetDistance(candidate->type.value, type, EntityTypePtr) +
    MetricPosition * ::GetDistance(candidate->position.value, area) +
    MetricTime * ::GetDistance(candidate->time.value, time);
}

float TargetAggregation::GetQuality() const
{
  // Level of concreteness
  float concretness =
    ConcretnessSide * GetConcretness(side) *
    ConcretnessType * GetConcretness(type.GetRef()) *
    ConcretnessPosition * GetConcretness(area) *
    ConcretnessTime * GetConcretness(time);

  // Amount of candidates
  float amount = pow(candidates.Size(), ExpAmount);
  /*
  LogF("*** Quality of aggregation:");
  LogF("  side concretness: %.2f", GetConcretness(side));
  LogF("  type concretness: %.2f", GetConcretness(type));
  LogF("  area concretness: %.2f", GetConcretness(area));
  LogF("  time concretness: %.2f", GetConcretness(time));
  LogF("  total concretness: %.2f", concretness);
  LogF("  amount: %d ^ %.2f = %.2f", candidates.Size(), ExpAmount, amount);
  LogF("  quality: %.2f", amount * concretness);
  */
  return amount * concretness;
}

template <typename Type>
class Symmetric2DArray : public AutoArray<Type>
{
public:
  typedef AutoArray<Type> base;
  Symmetric2DArray(int size)
  {
    int n = size * (size - 1) / 2;
    base::Realloc(n);
    base::Resize(n);
  }
  Type &operator () (int i, int j)
  {
    Assert(i != j);
    return base::Set(Index(i, j));
  }
  const Type &operator () (int i, int j) const
  {
    Assert(i != j);
    return base::Get(Index(i, j));
  }
protected:
  int Index(int i, int j) const
  {
    int row, column;
    if (i > j)
    {
      row = i; column = j;
    }
    else
    {
      row = j; column = i;
    }
    return row * (row - 1) / 2 + column;
  }
};

static void SplitAggregation(TargetAggregation *src, TargetAggregation *dst1, TargetAggregation *dst2, Symmetric2DArray<float> &distances)
{
  // select the representants of the new aggregations
  int c1 = -1, c2 = -1;
  int n = src->candidates.Size();
  float maxDist = -FLT_MAX;
  for (int i=1; i<n; i++)
  {
    int order1 = src->candidates[i]->order;
    for (int j=0; j<i; j++)
    {
      float dist = distances(order1, src->candidates[j]->order);
      if (dist > maxDist)
      {
        c1 = i; c2 = j; maxDist = dist;
      }
    }
  }
  Assert(c1 >= 0 && c2 >= 0);
  dst1->Add(src->candidates[c1]);
  dst2->Add(src->candidates[c2]);

  // split the rest of candidates
  for (int i=0; i<n; i++)
  {
    if (i == c1 || i == c2) continue; // already done

    TargetCandidate *candidate = src->candidates[i];
    if (dst1->GetDistance(candidate) < dst2->GetDistance(candidate))
      dst1->Add(candidate);
    else
      dst2->Add(candidate);
  }
}

static int CmpAggregations(Ref<TargetAggregation> *a1, Ref<TargetAggregation> *a2)
{
  TargetAggregation *agg1 = *a1;
  TargetAggregation *agg2 = *a2;

  float diff = agg2->matched - agg1->matched; // descending sort order
  if (diff != 0) return sign(diff);

  return ComparePointerAddresses(agg1, agg2);
}

static bool IsEqual(const TimeInterval &inner, const TimeInterval &outer)
{
  return inner.timeMin >= outer.timeMin && inner.timeMax <= outer.timeMax;
}

// Helper structure to keep type with valid textSingular/plural, hopefully no Unknowns here
struct KBAggrEntityType
{
  Ref<const EntityAIType> type;
  int count;
  float sortVal;
  KBAggrEntityType() { count=1; };
};
TypeIsMovable(KBAggrEntityType)

// Use only the name of the most important candidate as a result
RString AggregatedKBTargetsName( const RefArray<TargetCandidate> &candidates, int &ucount )
{
  AutoArray<KBAggrEntityType, MemAllocLocal<KBAggrEntityType, 64> > targets;
  // collect each candidate with the same textSingular only once (store the count)
  for (int i=0; i<candidates.Size(); i++)
  {
    const EntityAIType* type = candidates[i]->type.value;
    if (type)
    {
      RString tname = type->GetTextSingular();
      bool found = false;
      for (int j=0, siz=targets.Size(); j<siz; j++)
      {
        if (tname==targets[j].type->GetTextSingular())
        {
          targets[j].count++;
          found = true;
          break;
        }
      }
      if (!found)
      {
        int ix = targets.Add();
        targets[ix].type = type;
        targets[ix].sortVal = GWorld->GetMartaImportance(type);
      }
    }
  }
  if (!targets.Size()) return RString();
  // get the target with max importance
  for (int j=0; j<targets.Size(); j++) //update sortVal
    targets[j].sortVal *= targets[j].count;
  float maxVal = targets[0].sortVal;
  int maxIx = 0;
  for (int i=1; i<targets.Size(); i++)
  {
    if (targets[i].sortVal > maxVal)
    {
      maxVal = targets[i].sortVal; maxIx = i;
    }
  }
  RString name = targets[maxIx].type->GetName();
  ucount = targets[maxIx].count;
  if ( ucount > 1 ) name = RString("*") + name;
  return name;
}

GameValue KBTargetsAggregate(const GameState *state, GameValuePar oper1, GameValuePar oper2)
{
  GameValue value = GGameState.CreateGameValue(GameArray);

  // read speaker and question
  // check and load parameters
  const GameArrayType &array1 = oper1;
  if (array1.Size() != 5)
  {
    state->SetError(EvalDim, array1.Size(), 5);
    return value;
  }
  Object *me = GetObject(array1[0]);
  if (!me) return value;

  TargetListQuery query;
  query.side.Init(array1[1]);
  query.type.Init(array1[2]);
  query.area.Init(array1[3]);
  query.interval.Init(array1[4]);

  // read the list of candidates
  const GameArrayType &array = oper2;
  int n = array.Size();
  if (n == 0) return value;

  AUTO_STATIC_ARRAY(Ref<TargetCandidate>, candidates, 32);
  candidates.Resize(n);
  for (int i=0; i<n; i++)
  {
    // check the array structure
    if (array[i].GetType() != GameArray)
    {
      state->TypeError(GameArray, array[i].GetType());
      return value;
    }
    const GameArrayType &subarray = array[i];
    if (subarray.Size() != 6)
    {
      state->SetError(EvalDim, array.Size(), 6);
      return value;
    }
    if (subarray[0].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, subarray[0].GetType());
      return value;
    }
    if (subarray[1].GetType() != GameObject)
    {
      state->TypeError(GameObject, subarray[1].GetType());
      return value;
    }
    if (subarray[2].GetType() != GameSide)
    {
      state->TypeError(GameSide, subarray[2].GetType());
      return value;
    }
    if (subarray[3].GetType() != GameString)
    {
      state->TypeError(GameString, subarray[3].GetType());
      return value;
    }
    if (subarray[4].GetType() != GameArray)
    {
      state->TypeError(GameArray, subarray[4].GetType());
      return value;
    }
    if (subarray[5].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, subarray[5].GetType());
      return value;
    }
    const GameArrayType &position = subarray[4];
    if (position.Size() < 2)
    {
      state->SetError(EvalDim, array.Size(), 2);
      return value;
    }
    if (position[0].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, position[0].GetType());
      return value;
    }
    if (position[1].GetType() != GameScalar)
    {
      state->TypeError(GameScalar, position[1].GetType());
      return value;
    }

    // read the values
    TargetCandidate *candidate = new TargetCandidate();
    candidates[i] = candidate;
    candidate->coefTotal = subarray[0];
    candidate->idExact.value = dyn_cast<EntityAI>(GetObject(subarray[1]));
    candidate->side.value = GetSide(subarray[2]);
    RString name = subarray[3];
    candidate->type.value = dynamic_cast<EntityAIType *>(VehicleTypes.New(name));
    candidate->position.value[0] = position[0];
    candidate->position.value[1] = 0;
    candidate->position.value[2] = position[1];
    candidate->time.value = subarray[5];
    candidate->order = i;
  }

  // calculate distances for each pair of candidates
  Symmetric2DArray<float> distances(n);
  for (int i=1; i<n; i++)
    for (int j=0; j<i; j++)
      distances(i, j) = candidates[i]->GetDistance(candidates[j]);

  // create initial aggregation
  AUTO_STATIC_ARRAY(Ref<TargetAggregation>, aggregations, 32);
  Ref<TargetAggregation> aggregation = new TargetAggregation;
  for (int i=0; i<n; i++) aggregation->Add(candidates[i]);
  aggregations.Add(aggregation);

  // recursively split aggregations
  n = 0;
  while (n < aggregations.Size())
  {
    aggregation = aggregations[n];
    if (aggregation->candidates.Size() <= 1)
    {
      // cannot split
      n++;
      continue;
    }
    Ref<TargetAggregation> aggregation1 = new TargetAggregation;
    Ref<TargetAggregation> aggregation2 = new TargetAggregation;
    SplitAggregation(aggregation, aggregation1, aggregation2, distances);
    if (aggregation1->GetQuality() + aggregation2->GetQuality() > aggregation->GetQuality())
    {
      // aggregation is better after split
      aggregations[n] = aggregation1;
      aggregations.Add(aggregation2);
    }
    else n++;
  }

  // sort aggregations
  n = aggregations.Size();
  QSort(aggregations.Data(), aggregations.Size(), CmpAggregations);

  // create the output
  GameArrayType &result = value;
  result.Realloc(n);
  result.Resize(n);
  for (int i=0; i<n; i++)
  {
    result[i] = GGameState.CreateGameValue(GameArray);
    GameArrayType &array = result[i];
    array.Realloc(7);
    array.Resize(7);

    const TargetAggregation *aggregation = aggregations[i];
    int count = aggregation->candidates.Size();
    {
      // count
      array[0] = GGameState.CreateGameValue(GameArray);
      GameArrayType &arg = array[0];
      arg.Realloc(2);
      arg.Resize(2);
      arg[0] = Format("COUNT%d", i);
      arg[1] = (float)count;
    }
    {
      // side - write if not equal as answered
      array[1] = GGameState.CreateGameValue(GameArray);
      GameArrayType &arg = array[1];
      arg.Realloc(2);
      arg.Resize(2);
      arg[0] = Format("SIDE%d", i);
      if (query.side.valid && aggregation->side == query.side.value)
        arg[1] = "$"; // equal to queried
      else
        arg[1] = GameValueExt(aggregation->side);
    }
    {
      // type - use pronounce if equal as answered
      array[2] = GGameState.CreateGameValue(GameArray);
      GameArrayType &arg = array[2];
      arg.Realloc(2);
      arg.Resize(2);
      arg[0] = Format("UNIT%d", i);
      RString name;
      if (query.type.valid && aggregation->type == query.type.value)
        name = "$"; // equal to queried
      else
        name = aggregation->type->GetName();
      if (count > 1) name = RString("*") + name;
      arg[1] = name;
    }
    {
      // place - use pronounce if equal as answered
      array[3] = GGameState.CreateGameValue(GameArray);
      GameArrayType &arg = array[3];
      arg.Realloc(2);
      arg.Resize(2);
      arg[0] = Format("PLACE%d", i);
      arg[1] = GGameState.CreateGameValue(GameArray);
      GameArrayType &area = arg[1];
      area.Realloc(3);
      area.Resize(3);
      area[0] = GGameState.CreateGameValue(GameArray);
      GameArrayType &center = area[0];
      center.Realloc(2);
      center.Resize(2);
      area[1] = aggregation->area.radius;
      // area[2] used as a reference point
      Vector3Val position = aggregation->area.center;
#if USE_LANDSCAPE_LOCATIONS
      Location *location = GLandscape->GetLocations().FindNearestWithDubbing(position, FLT_MAX);
      if (location)
      {
        if (position.DistanceXZ2(me->WorldPosition(me->FutureVisualState())) > position.DistanceXZ2(location->_position))
        {
          // some named location is closer than myself
          if (query.area.valid && query.area.value.center.DistanceXZ2(location->_position) < 1.0f)
          {
            area[2] = "$"; // equal to queried
            Vector3 relPos = position - query.area.value.center;
            center[0] = relPos.X();
            center[1] = relPos.Z();
          }
          else
          {
            area[2] = ""; // absolute
            center[0] = position.X();
            center[1] = position.Z();
          }
        }
        else
        {
          area[2] = "."; // here
          Vector3 relPos = position - me->WorldPosition(me->FutureVisualState());
          center[0] = relPos.X();
          center[1] = relPos.Z();
        }
      }
#else
      area[2] = "."; // here
      Vector3 relPos = position - me->WorldPosition(me->FutureVisualState());
      center[0] = relPos.X();
      center[1] = relPos.Z();
#endif
    }
    {
      // time - write if not equal as answered
      array[4] = GGameState.CreateGameValue(GameArray);
      GameArrayType &arg = array[4];
      arg.Realloc(2);
      arg.Resize(2);
      arg[0] = Format("TIME%d", i);
      if (query.interval.valid && IsEqual(aggregation->time, query.interval.value))
      {
        arg[1] = "$"; // equal to queried
      }
      else
      {
        arg[1] = GGameState.CreateGameValue(GameArray);
        GameArrayType &time = arg[1];
        time.Realloc(2);
        time.Resize(2);
        time[0] = aggregation->time.timeMin;
        time[1] = aggregation->time.timeMax;
      }
    }
    {
      // count and type with higher accuracy avoiding use of UNKNOWN (I saw several West Unknowns just now)
      array[5] = GGameState.CreateGameValue(GameArray);
      GameArrayType &arg = array[5];
      arg.Realloc(2);
      arg.Resize(2);
      int ucount = 0;
      RString name = AggregatedKBTargetsName(aggregation->candidates, ucount);
      // UCOUNT
      arg[0] = Format("UCOUNT%d", i);
      arg[1] = (float)ucount;
      // UTYPE
      array[6] = GGameState.CreateGameValue(GameArray);
      {
        GameArrayType &arg = array[6];
        arg.Realloc(2);
        arg.Resize(2);
        arg[0] = Format("UTYPE%d", i);
        arg[1] = name;
      }
    }
  }

  return value;
}

/*
// Functions registrations
#define FUNCTIONS_KBEXT(XX, Category) \
  XX(GameArray, "targetsAggregate", KBTargetsAggregate, GameArray, "candidates", "Aggregate candidates.", "", "", "2.92", "", Category) \

static const GameFunction ExtUnary[]=
{
  FUNCTIONS_KBEXT(REGISTER_FUNCTION, "Conversations")
};

#if DOCUMENT_COMREF
static ComRefFunc ExtComRefFunc1[] =
{
  FUNCTIONS_KBEXT(COMREF_FUNCTION, "Conversations")
};
#endif
*/

// Operators registration
#define OPERATORS_KBEXT(XX, Category) \
  XX(GameArray, "targetsQuery", function, KBTargetsQuery, GameObject, GameArray, "speaker", "[receiver, side, unit, place, time]", "Find all targets known to sender matching given query.", "", "", "2.92", "", Category) \
  XX(GameArray, "targetsAggregate", function, KBTargetsAggregate, GameArray, GameArray, "[speaker, side, unit, place, time]", "candidates", "Aggregate candidates.", "", "", "2.92", "", Category) \

OPERATORS_KBEXT(JNI_OPERATOR, "Conversations")

static const GameOperator ExtBinary[]=
{
  OPERATORS_KBEXT(REGISTER_OPERATOR, "Conversations")
};

#if DOCUMENT_COMREF
static ComRefFunc ExtComRefFunc2[] =
{
  OPERATORS_KBEXT(COMREF_OPERATOR, "Conversations")
};
#endif

#include <El/Modules/modules.hpp>

INIT_MODULE(KBExt, 2)
{
/*
  for (int i=0; i<lenof(ExtUnary); i++)
  {
    GGameState.NewFunction(ExtUnary[i]);
  }
*/
  for (int i=0; i<lenof(ExtBinary); i++)
  {
    GGameState.NewOperator(ExtBinary[i]);
  }

#if DOCUMENT_COMREF
/*
  for (int i=0; i<lenof(ExtComRefFunc1); i++)
  {
    GGameState.AddComRefFunction(ExtComRefFunc1[i]);
  }
*/
  for (int i=0; i<lenof(ExtComRefFunc2); i++)
  {
    GGameState.AddComRefFunction(ExtComRefFunc2[i]);
  }
#endif
}

#endif
