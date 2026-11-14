// AI - implementation of AI radio messages

#include "../wpch.hpp"
#include "ai.hpp"
#include "../person.hpp"
#include "aiDefs.hpp"

#include "aiRadio.hpp"

#include "../world.hpp"
#include "../landscape.hpp"
//#include "loadStream.hpp"
#include <El/Common/randomGen.hpp>
#include "../global.hpp"

//#include "strIncl.hpp"
#include "../stringtableExt.hpp"
#include "../paramArchiveExt.hpp"

#include <El/ParamFile/paramFile.hpp>

#include "../sentences.hpp"
#include "../UI/uiActions.hpp"

#include <ctype.h>

#include "../UI/chat.hpp"

#include <El/Enum/enumNames.hpp>
#include <El/Evaluator/express.hpp>

#include "../Network/network.hpp"

#include <Es/Common/win.h>
#include <Es/Strings/bString.hpp>

#include "../gameDirs.hpp"

#include <El/HiResTime/hiResTime.hpp>

/*
#define PAUSE_AFTER_WORD				0.00F
#define PAUSE_IN_NUMBER					0.00F
#define PAUSE_AFTER_NUMBER			PAUSE_AFTER_WORD
#define PAUSE_IN_UNIT_LIST			0.00F
#define PAUSE_AFTER_UNIT_LIST		0.05F
*/

///////////////////////////////////////////////////////////////////////////////
// helper functions

static const EnumName UnitPosNames[]=
{
  EnumName(UPUp, "Up"),
  EnumName(UPMiddle, "Middle"),
  EnumName(UPDown, "Down"),
  EnumName(UPAuto, "Auto"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(UnitPosition dummy)
{
  return UnitPosNames;
}

static void Spell(RadioSentence &sentence, const char *buffer, float pauseAfter, const RadioChannelParams &cfg)
{
  int i, n = strlen(buffer);
  for (i=0; i<n; i++)
  {
    float pause = (i == n - 1) ? pauseAfter : cfg.pauseInNumber;
    char c = tolower(buffer[i]);
    switch (c)
    {
#if 0
    case '0':
      sentence.Add("zero", pause);
      break;
    case '1':
      sentence.Add("one", pause);
      break;
    case '2':
      sentence.Add("two", pause);
      break;
    case '3':
      sentence.Add("three", pause);
      break;
    case '4':
      sentence.Add("four", pause);
      break;
    case '5':
      sentence.Add("five", pause);
      break;
    case '6':
      sentence.Add("six", pause);
      break;
    case '7':
      sentence.Add("seven", pause);
      break;
    case '8':
      sentence.Add("eight", pause);
      break;
    case '9':
      sentence.Add("nine", pause);
      break;
#else
    case '0':
      sentence.Add("zero2", pause);
      break;
    case '1':
      sentence.Add("one2", pause);
      break;
    case '2':
      sentence.Add("two2", pause);
      break;
    case '3':
      sentence.Add("three2", pause);
      break;
    case '4':
      sentence.Add("four2", pause);
      break;
    case '5':
      sentence.Add("five2", pause);
      break;
    case '6':
      sentence.Add("six2", pause);
      break;
    case '7':
      sentence.Add("seven2", pause);
      break;
    case '8':
      sentence.Add("eight2", pause);
      break;
    case '9':
      sentence.Add("nine2", pause);
      break;
#endif
    case 'a':
      sentence.Add("alpha", pause);
      break;
    case 'b':
      sentence.Add("bravo", pause);
      break;
    case 'c':
      sentence.Add("charlie", pause);
      break;
    case 'd':
      sentence.Add("delta", pause);
      break;
    case 'e':
      sentence.Add("echo", pause);
      break;
    case 'f':
      sentence.Add("foxtrot", pause);
      break;
    case 'g':
      sentence.Add("golf", pause);
      break;
    case 'h':
      sentence.Add("hotel", pause);
      break;
    case 'i':
      sentence.Add("india", pause);
      break;
    case 'j':
      sentence.Add("juliet", pause);
      break;
    case 'k':
      sentence.Add("kilo", pause);
      break;
    case 'l':
      sentence.Add("lima", pause);
      break;
    case 'm':
      sentence.Add("mike", pause);
      break;
    case 'n':
      sentence.Add("november", pause);
      break;
    case 'o':
      sentence.Add("oscar", pause);
      break;
    case 'p':
      sentence.Add("papa", pause);
      break;
    case 'q':
      sentence.Add("quebec", pause);
      break;
    case 'r':
      sentence.Add("romeo", pause);
      break;
    case 's':
      sentence.Add("sierra", pause);
      break;
    case 't':
      sentence.Add("tango", pause);
      break;
    case 'u':
      sentence.Add("uniform", pause);
      break;
    case 'v':
      sentence.Add("victor", pause);
      break;
    case 'w':
      sentence.Add("whiskey", pause);
      break;
    case 'x':
      sentence.Add("xray", pause);
      break;
    case 'y':
      sentence.Add("yankee", pause);
      break;
    case 'z':
      sentence.Add("zulu", pause);
      break;
    case '.':
    case ',':
    case ':':
    case ';':
    case '?':
    case '!':
    case '-':
    case '(':
    case ')':
      break;
    default:
      Fail("Character");
      break;
    }
  }
}

static void SayUnitNumber(RadioSentence &sentence, int number, float pause, const RadioChannelParams &cfg)
{
  if (number > 100)
  {
    Spell(sentence, Format("%d", number), pause, cfg);
    return;
  }
  if (number == 100)
  {
    sentence.Add("hundred", pause);
    return;
  }
  int tens = 0;
  if (number >= 20)
  {
    tens = number / 10;
    number -= tens * 10;
  }
  float pauseAfterTens = number == 0 ? pause : cfg.pauseInNumber;
  switch (tens)
  {
  case 0:
    break;
  case 2:
    sentence.Add("twenty", pauseAfterTens);
    break;
  case 3:
    sentence.Add("thirty", pauseAfterTens);
    break;
  case 4:
    sentence.Add("forty", pauseAfterTens);
    break;
  case 5:
    sentence.Add("fifty", pauseAfterTens);
    break;
  case 6:
    sentence.Add("sixty", pauseAfterTens);
    break;
  case 7:
    sentence.Add("seventy", pauseAfterTens);
    break;
  case 8:
    sentence.Add("eighty", pauseAfterTens);
    break;
  case 9:
    sentence.Add("ninety", pauseAfterTens);
    break;
  default:
    Fail("Unexpected number");
    break;
  }
  switch (number)
  {
  case 0:
    break;
  case 1:
    sentence.Add("one", pause);
    break;
  case 2:
    sentence.Add("two", pause);
    break;
  case 3:
    sentence.Add("three", pause);
    break;
  case 4:
    sentence.Add("four", pause);
    break;
  case 5:
    sentence.Add("five", pause);
    break;
  case 6:
    sentence.Add("six", pause);
    break;
  case 7:
    sentence.Add("seven", pause);
    break;
  case 8:
    sentence.Add("eight", pause);
    break;
  case 9:
    sentence.Add("nine", pause);
    break;
  case 10:
    sentence.Add("ten", pause);
    break;
  case 11:
    sentence.Add("eleven", pause);
    break;
  case 12:
    sentence.Add("twelve", pause);
    break;
  case 13:
    sentence.Add("thirteen", pause);
    break;
  case 14:
    sentence.Add("fourteen", pause);
    break;
  case 15:
    sentence.Add("fifteen", pause);
    break;
  case 16:
    sentence.Add("sixteen", pause);
    break;
  case 17:
    sentence.Add("seventeen", pause);
    break;
  case 18:
    sentence.Add("eighteen", pause);
    break;
  case 19:
    sentence.Add("nineteen", pause);
    break;
  default:
    Fail("Unexpected number");
    break;
  }
}

static void SayNumber
(
  RadioSentence &sentence, int number, float pauseAfter, const RadioChannelParams &cfg,
  const char *format = "%d", int mode=1
)
{
  switch (mode)
  {
    default:
      char buffer[32];
      sprintf(buffer, format, number);
      Spell(sentence, buffer, pauseAfter, cfg);
    break;
    case 2:
      SayUnitNumber(sentence, number, pauseAfter, cfg);
    break;
  }
}

void PositionToAA11(Vector3Val pos, char *buffer);

Object *FindNearestMoveObject(Vector3Par pos)
{
  if (!GWorld->CameraOn()) return NULL;
  float dist2 = pos.Distance2(GWorld->CameraOn()->FutureVisualState().Position());
  if (dist2 > Square(400)) return NULL;
  float diff = 0.25 * sqrt(dist2);
  saturate(diff, 5.0, 100.0);
  int xMin,xMax,zMin,zMax;
  ObjRadiusRectangle(xMin,xMax,zMin,zMax,pos,pos,diff);

  Object *best = NULL;
  const float minDist2 = diff * diff;
  float minFunc = FLT_MAX;
  for (int x=xMin; x<=xMax; x++)
    for (int z=zMin; z<=zMax; z++)
    {
      const ObjectListUsed &list = GLandscape->UseObjects(x,z);
      int n = list.Size();
      for (int i=0; i<n; i++)
      {
        Object *obj = list[i];
        if (obj->GetType() == Network) continue;
        if (obj->GetType() == Temporary) continue;
        if (obj->GetType() == TypeTempVehicle) continue;
#if _ARMA3_TERRAIN_DECALS
        if (obj->GetType() == LandDecal) continue;
#endif
        if (!obj->IsMoveTarget()) continue;
        float dist2 = pos.Distance2(obj->FutureVisualState().Position());
        if (dist2 >= minDist2) continue;
        float func = dist2 * obj->GetInvMass();
        if (func >= minFunc) continue;
        minFunc = func ;
        best = obj;
      }
    } // (for all near objects)
    return best;
}

static const RStringB azimSay[13]=
{
  "at12","at1",
  "at2","at3",
  "at4","at5",
  "at6","at7",
  "at8","at9",
  "at10","at11",
  "at12"
};
static const int *azimWrite[13]=
{
  &IDS_WORD_AT12,&IDS_WORD_AT1,
  &IDS_WORD_AT2,&IDS_WORD_AT3,
  &IDS_WORD_AT4,&IDS_WORD_AT5,
  &IDS_WORD_AT6,&IDS_WORD_AT7,
  &IDS_WORD_AT8,&IDS_WORD_AT9,
  &IDS_WORD_AT10,&IDS_WORD_AT11,
  &IDS_WORD_AT12
};

void SentenceParams::AddAzimutRelDir(Vector3Par dir)
{
  int azimut = toInt(atan2(dir.X(), dir.Z()) * (6 / H_PI));
  if (azimut < 0) azimut += 12;

  //int ret = -1;
  if (azimut>=0 && azimut<=12)
  {
    AddWord(azimSay[azimut],LocalizeString(*azimWrite[azimut]));
  }
}

static const RStringB distSay[]=
{
  "dist50", // 0..50
  "dist100", // 50..150
  "dist200","dist200",
  "dist500",
  "dist500", // 500
  "dist500",
  "dist1000","dist1000","dist1000",
  "dist1000", // 1000
  "dist1000","dist1000","dist1000","dist1000","dist1000",
  "dist2000","dist2000","dist2000","dist2000",
  "dist2000", // 2000
  "dist2000","dist2000","dist2000","dist2000","dist2000",
  "far" // 2500 (and more)
};

static const int *distWrite[]=
{
  &IDS_WORD_DIST50,
  &IDS_WORD_DIST100, // 50..150
  &IDS_WORD_DIST200,&IDS_WORD_DIST200,
  &IDS_WORD_DIST500,
  &IDS_WORD_DIST500, // 500
  &IDS_WORD_DIST500,
  &IDS_WORD_DIST1000,&IDS_WORD_DIST1000,
  &IDS_WORD_DIST1000,
  &IDS_WORD_DIST1000, // 1000
  &IDS_WORD_DIST1000,&IDS_WORD_DIST1000,
  &IDS_WORD_DIST1000,&IDS_WORD_DIST1000,
  &IDS_WORD_DIST1000,
  &IDS_WORD_DIST2000,&IDS_WORD_DIST2000,
  &IDS_WORD_DIST2000,&IDS_WORD_DIST2000,
  &IDS_WORD_DIST2000, // 2000
  &IDS_WORD_DIST2000,&IDS_WORD_DIST2000,
  &IDS_WORD_DIST2000,&IDS_WORD_DIST2000,
  &IDS_WORD_DIST2000,
  &IDS_WORD_DISTFAR // 2500 (and more)
};

void SentenceParams::AddDistance(float dist)
{
  int dist100 = toInt(dist*0.01);
  const int maxDist = sizeof(distWrite)/sizeof(*distWrite)-1;
  saturate(dist100,0,maxDist);

  AddWord(distSay[dist100],LocalizeString(*distWrite[dist100]));
}

void SentenceParams::AddDistance(float dist, int martaTypeIx, const RadioMessage *msg)
{
  static float defaultDistLevels[2] = {50.0f, 150.0f};
  if ( martaTypeIx>=0 )
    base::Add(new SentenceParamDistance(dist, GWorld->_martaType[martaTypeIx]._distanceLevels, msg));
  else
    base::Add(new SentenceParamDistance(dist, defaultDistLevels, msg));
}

void SentenceParams::AddDirection(float relDir, float absDir, bool forceCompass, const RadioMessage *msg)
{
  base::Add(new SentenceParamDirection(msg, relDir, absDir, forceCompass));
}

void SentenceParams::AddLocation(const Location *location)
{
  base::Add(new SentenceParamLocation(location));
}

void SentenceParams::AddAggrTargets(AutoArray<AggrTargetWord> &speechList, RString text, const RadioMessage *msg)
{
  base::Add(new SentenceParamAggrTargets(speechList, text, msg));
}

void SentenceParams::AddRelativePosition(Vector3Par dir)
{
  // convert position to relative position
  int azimut = toInt(0.1 * atan2(dir.X(), dir.Z()) * (180 / H_PI));
  if (azimut < 0) azimut += 36;
  Add("%02d", azimut);
  //int dist = toInt(0.1 * dir.SizeXZ());
  //Add(dist);
}

void SentenceParams::AddAzimut(Vector3Par pos)
{
  AIBrain *unit = GWorld->FocusOn();
  // FIXED for dedicated server
  if (!unit)
  {
    AddAzimutRelDir(VForward);
    return;
  }

  AddAzimutRelDir(GWorld->CameraOn()->FutureVisualState().GetInvTransform().FastTransform(pos));
 //	GWorld->UI()->ShowGroupDir();
}

void SentenceParams::AddAzimutDir(Vector3Par dir)
{
  AIBrain *unit = GWorld->FocusOn();
  // FIXED for dedicated server
  if (!unit)
  {
    AddAzimutRelDir(VForward);
    return;
  }

  AddAzimutRelDir(GWorld->CameraOn()->FutureVisualState().GetInvTransform().Rotate(dir));
//  GWorld->UI()->ShowGroupDir();
}

/// list of sentence simple expression variables
#define SENTENCE_SE_VARIABLES(XXX) \
  XXX(Rank, GetSERank) \
  XXX(Captive, GetSECaptive) \
  XXX(Safe, GetSESafe) \
  XXX(Combat, GetSECombat) \
  XXX(Stealth, GetSEStealth) \
  XXX(Morale, GetSEMorale) \
  XXX(GroupCompactness, GetSEGroupCompactness) \
  XXX(DistanceToUnit, GetSEDistanceToUnit) \
  XXX(DistanceToSender, GetSEDistanceToSender) \
  XXX(DistanceToGroup, GetSEDistanceToGroup) \
  XXX(DistanceToLocation, GetSEDistanceToLocation) \
  XXX(GroupCoreRadius, GetSEGroupCoreRadius) \
  XXX(UnitDistanceFactor, GetSEUnitDistanceFactor) \
  XXX(InsideLocation, GetSEInsideLocation) \
  XXX(MoveToObject, GetSEMoveToObject) \
  XXX(DistanceToRecipients, GetSEDistanceToRecipients) \
  XXX(RecipientsRadius, GetSERecipientsRadius)

#define SENTENCE_SE_VAR_NAME(name, function) #name,
#define SENTENCE_SE_VAR_FUNC(name, function) &RadioMessage::function,

typedef float (RadioMessage::*SimpleExpressionFunc)() const;

// simple expression variables for sentence variants
float RadioMessage::GetSERank() const {if (GetSender()) return GetSender()->GetSERank(); else return 0; }
float RadioMessage::GetSECaptive() const {if (GetSender()) return GetSender()->GetSECaptive(); else return 0; }
float RadioMessage::GetSESafe() const {if (GetSender()) return GetSender()->GetSESafe(); else return 0; }
float RadioMessage::GetSECombat() const {if (GetSender()) return GetSender()->GetSECombat(); else return 0; }
float RadioMessage::GetSEStealth() const {if (GetSender()) return GetSender()->GetSEStealth(); else return 0; }
float RadioMessage::GetSEMorale() const {if (GetSender()) return GetSender()->GetSEMorale(); else return 0; }

// helper function for SelectVariant
static ParamEntryPtr SelectVariant(RadioMessage *msg, ParamEntryPar cls, float rnd);
static ParamEntryPtr RecurseSelectVariant(RadioMessage *msg, RString sentenceName)
{
  // select radio protocol by the speaker
  Speaker *speaker = msg->GetSpeaker();
  if (speaker)
  {
    RString protocol = speaker->GetProtocol();
    ConstParamEntryPtr protocolCls = protocol.GetLength() > 0 ? Pars.FindEntry(protocol) : ParamEntryPtr(NULL);
    if (protocolCls) 
    { 
      ParamEntryPtr sentence = protocolCls->FindEntry(sentenceName);
      if (sentence) return SelectVariant(msg, (*sentence), GRandGen.RandomValue());
    }
  }
  return ParamEntryPtr();
}

#define DIAG_RECURSIVE_SENTENCES 0
static ParamEntryPtr SelectVariant(RadioMessage *msg, ParamEntryPar cls, float rnd)
{
  // Check for recursive sentence selection
  ParamEntryPtr versions = cls.FindEntry("versions");
  bool recurse = false;
  if (!versions)
  {
    recurse = true;
    versions = cls.FindEntry("sentences");
  }
  if (versions)
  {
    // TODO: precompile weight expressions (bank/cache of sentences?)
    int n = versions->GetSize() / 2;
    if (n * 2 != versions->GetSize())
    {
      RptF("Sentence %s - size of variants should be even", cc_cast(cls.GetName()));
      return ParamEntryPtr();
    }

    // names of simple expression variables
    static RString varNames[] =
    {
      SENTENCE_SE_VARIABLES(SENTENCE_SE_VAR_NAME)
    };

    // prepare all simple expression variables
    const int nValues = lenof(varNames);
    float varValues[nValues];
    bool  varInitialized[nValues];
    for (int i=0; i<nValues; i++) varInitialized[i] = false;
    SimpleExpressionFunc varFunc[] = 
    {
      SENTENCE_SE_VARIABLES(SENTENCE_SE_VAR_FUNC)
    };

    // collect weights of variants
    float totalWeight = 0;
    AutoArray< float, MemAllocLocal<float, 16> > weights;
    weights.Resize(n);
    for (int i=0; i<n; i++)
    {
      RString expText = (*versions)[2 * i + 1];
      ExpressionCode exp;
      bool ok = exp.Compile(expText, varNames, lenof(varNames));
      if (!ok)
      {
        RptF("Error in expression %s - sentence %s", cc_cast(expText), cc_cast(versions->GetName()));
        weights[i] = 0;
        continue;
      }
      ExpressionCodeVars vars = exp.GetUsedVariables();
      for (int i=0; i<vars.Size(); i++)
      {
        int vi = vars[i];
        if (!varInitialized[vi]) 
        {
          varValues[vi] = msg ? (msg->*varFunc[vi])() : NULL;
          varInitialized[vi] = true;
        }
      }

      float weight;
      ok = exp.Evaluate(weight, varValues, lenof(varValues));  
      if (ok)
      {
        weights[i] = weight;
        totalWeight += weight;
      }
      else weights[i] = 0;
    }
    if (totalWeight <= 0) return ParamEntryPtr();

    // find where weighted random number fit
    float value = rnd * totalWeight;
#if DIAG_RECURSIVE_SENTENCES
    if (recurse)
    {
      LogF("*** RecurseSelectVariant:");
      for (int i=0; i<n; i++)
      {
        LogF("  %s = %f", cc_cast((*versions)[2*i].GetValue()), weights[i]);
      }
    }
#endif
    for (int i=0; i<n-1; i++)
    {
      value -= weights[i];
      if (value <= 0)
      {
        RStringB version = (*versions)[2 * i];
        if (version.IsEmpty()) return ParamEntryPtr();
        if (recurse) return RecurseSelectVariant(msg, version);
        else
        {
          return cls.FindEntry(version);
        }
      }
    }
    RStringB version = (*versions)[2 * (n - 1)];
    if (version.IsEmpty()) return ParamEntryPtr();
    if (recurse) return RecurseSelectVariant(msg, version);
    else
    {
      return cls.FindEntry(version);
    }
  }
  return ParamEntryPtr();
}

void SentenceParamText::Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const
{
  Spell(sentence, _text, cfg.pauseAfterWord, cfg);
}

RString SentenceParamText::Write(int mode) const
{
  return _text;
}

DEFINE_FAST_ALLOCATOR(SentenceParamText)

void SentenceParamNumber::Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const
{
  SayNumber(sentence, _number, cfg.pauseAfterWord, cfg, "%d", mode);
}

RString SentenceParamNumber::Write(int mode) const
{
  return Format("%d", _number);
}

DEFINE_FAST_ALLOCATOR(SentenceParamNumber)

void SentenceParamFormat::Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const
{
  SayNumber(sentence, _number, cfg.pauseAfterWord, cfg, _format, mode);
}

RString SentenceParamFormat::Write(int mode) const
{
  return Format(_format, _number);
}

DEFINE_FAST_ALLOCATOR(SentenceParamFormat)

void SentenceParamWordText::Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const
{
  sentence.Add(_say, cfg.pauseAfterWord);
}

RString SentenceParamWordText::Write(int mode) const
{
  return _write;
}

DEFINE_FAST_ALLOCATOR(SentenceParamWordText)

RString SentenceParamPhrase::Write(int mode) const
{
  return _write;
}

void SentenceParamPhrase::Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const
{
  for (int i=0; i<_say.Size(); i++)
    sentence.Add(_say[i], cfg.pauseAfterWord);
}

DEFINE_FAST_ALLOCATOR(SentenceParamPhrase)

#if _ENABLE_INDEPENDENT_AGENTS

void SentenceParamTeamMember::Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const
{
  if (_member)
  {
    bool ok = false;
    AutoArray<RString, MemAllocLocal<RString, 16> > words;
    switch (mode)
    {
    case 1:
      // short call sign
      ok = _member->GetCallSignShortSpeech(words);
      break;
    case 2:
      // call sign
      ok = _member->GetCallSignSpeech(words);
      break;
    }
    if (ok)
    {
      for (int i=0; i<words.Size(); i++)
        sentence.Add(words[i], cfg.pauseAfterWord);
    }
  }
}

RString SentenceParamTeamMember::Write(int mode) const
{
  if (_member)
  {
    switch (mode)
    {
    case 1:
      // short call sign
      return _member->GetCallSignShort();
    case 2:
      // call sign
      return _member->GetCallSign();
    }
  }
  return RString();
}

DEFINE_FAST_ALLOCATOR(SentenceParamTeamMember)

static bool GetMemberPos(Vector3 &ret, AITeamMember *member)
{
  if (!member) return false;
  AIAgent *agent = member->GetAgent();
  while (!agent)
  {
    AITeam *team = member->GetTeam();
    if (!team) return false;
    member = team->GetLeader();
    if (!member) return false;
    agent = member->GetAgent();
  }
  ret = agent->Position(agent->GetFutureVisualState());
  return true;
}

static RString GetRadioProtocol(AITeamMember *member)
{
  if (!member) return RString();
  AIAgent *agent = member->GetAgent();
  while (!agent)
  {
    AITeam *team = member->GetTeam();
    if (!team) return RString();
    member = team->GetLeader();
    if (!member) return RString();
    agent = member->GetAgent();
  }

  // speaker is known now
  Speaker *speaker = agent->GetSpeaker();
  if (!speaker) return RString();
  
  return speaker->GetProtocol();
}

static void SaySentence
(
  RadioSentence &sentence,
  ParamEntryPar clsVariant, SentenceParams &params,
  float pauseAfterMessage, const RadioChannelParams &cfg
);

void SentenceParamGameValue::Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const
{
  switch (mode)
  {
  case 1: // position
    {
      bool GetPos(const GameState *state, Vector3 &ret, GameValuePar oper);
      Vector3 pos, posS, posR;
      if (GetPos(GWorld->GetGameState(), pos, _value) && GetMemberPos(posS, _sender) && GetMemberPos(posR, _receiver))
      {
        const float limit2 = Square(400);
        if (pos.Distance2(posS) > limit2 || pos.Distance2(posR) > limit2)
        {
          // position in the map
          //VBS supports bigger grids
          char buffer[32];
          PositionToAA11(pos, buffer);
          Spell(sentence, buffer, cfg.pauseAfterWord, cfg);
        }
        else
        {
          // find correct radio protocol
          RString protocol = GetRadioProtocol(_sender);
          ConstParamEntryPtr protocolCls = protocol.GetLength() > 0 ? Pars.FindEntry(protocol) : ParamEntryPtr(NULL);
          if (protocolCls)
          {
            // position in the world
            Object *obj = FindNearestMoveObject(pos);
            if (obj)
            {
              ConstParamEntryPtr cls = protocolCls->FindEntry("ClausePositionNear");
              if (cls)
              {
                SentenceParams params;
                params.AddWord(obj->GetNameSound(), obj->GetShortName());
                params.AddAzimut(obj->FutureVisualState().Position());
                SaySentence(sentence, *cls, params, 0, cfg);
              }
            }
            else
            {
              ConstParamEntryPtr cls = protocolCls->FindEntry("ClausePosition");
              if (cls)
              {
                SentenceParams params;
                params.AddAzimut(pos);
                Vector3 dir = pos - (GWorld->CameraOn() ? GWorld->CameraOn()->FutureVisualState().Position() : posS);
                RString distance = Format("%d", toInt(0.1 * dir.SizeXZ()));
                params.Add(distance);
                SaySentence(sentence, *cls, params, 0, cfg);
              }
            }
          }
        }
      }
    }
    break;
  }
}

static RString WriteSentence(RString format, SentenceParams &params);

RString SentenceParamGameValue::Write(int mode) const
{
  switch (mode)
  {
  case 1: // position
    {
      bool GetPos(const GameState *state, Vector3 &ret, GameValuePar oper);
      Vector3 pos, posS, posR;
      if (GetPos(GWorld->GetGameState(), pos, _value) && GetMemberPos(posS, _sender) && GetMemberPos(posR, _receiver))
      {
        const float limit2 = Square(400);
        if (pos.Distance2(posS) > limit2 || pos.Distance2(posR) > limit2)
        {
          // position in the map
          //VBS supports bigger grids
          char buffer[32];
          PositionToAA11(pos, buffer);
          return buffer;
        }
        else
        {
          // find correct radio protocol
          RString protocol = GetRadioProtocol(_sender);
          ConstParamEntryPtr protocolCls = protocol.GetLength() > 0 ? Pars.FindEntry(protocol) : ParamEntryPtr(NULL);
          if (protocolCls)
          {
            // position in the world
            Object *obj = FindNearestMoveObject(pos);
            if (obj)
            {
              ConstParamEntryPtr cls = protocolCls->FindEntry("ClausePositionNear");
              if (cls)
              {
                SentenceParams params;
                params.AddWord(obj->GetNameSound(), obj->GetShortName());
                params.AddAzimut(obj->FutureVisualState().Position());
                return WriteSentence((*cls) >> "text", params);
              }
            }
            else
            {
              ConstParamEntryPtr cls = protocolCls->FindEntry("ClausePosition");
              if (cls)
              {
                SentenceParams params;
                params.AddAzimut(pos);
                Vector3 dir = pos - (GWorld->CameraOn() ? GWorld->CameraOn()->FutureVisualState().Position() : posS);
                RString distance = Format("%d", toInt(0.1 * dir.SizeXZ()));
                params.Add(distance);
                return WriteSentence((*cls) >> "text", params);
              }
            }
          }
        }
      }
    }
  }
  return RString();
}

DEFINE_FAST_ALLOCATOR(SentenceParamGameValue)

#endif

static void LoadSpeech(AutoArray<RString, MemAllocLocal<RString, 16> > &result, ParamEntryVal entry)
{
  Assert(entry.IsArray());
  int n = entry.GetSize();
  result.Realloc(n);
  result.Resize(n);
  for (int i=0; i<n; i++) result[i] = entry[i];
}

ParamEntryPtr SentenceParamDistance::GetVariantCls(int mode) const
{
  Speaker *speaker = _msg->GetSpeaker();
  if (speaker)
  {
    RString protocol = speaker->GetProtocol();
    ConstParamEntryPtr protocolCls = protocol.GetLength() > 0 ? Pars.FindEntry(protocol) : ParamEntryPtr(NULL);
    if (protocolCls) 
    { 
      ParamEntryPtr cls = protocolCls->FindEntry("Arguments");
      if (!cls) return ParamEntryPtr();
      char buffer[32];
      sprintf(buffer, "Distance%d", mode%100);
      cls = cls->FindEntry(buffer);
      if (!cls) return ParamEntryPtr();
      if (_distance < _distLevels[0]) return cls->FindEntry("Level0");
      else if (_distance < _distLevels[1]) return cls->FindEntry("Level1");
      else return cls->FindEntry("Level2");
    }
  }
  return ParamEntryPtr();
}

RString SentenceParamDistance::Write(int mode) const
{
  if (mode/100==ModeDistMeters)
  { // use meters in destination
    int dist100 = toInt(_distance*0.01);
    const int maxDist = sizeof(distWrite)/sizeof(*distWrite)-1;
    saturate(dist100,0,maxDist);
    return LocalizeString(*distWrite[dist100]);
  }
  else
  {
    ParamEntryPtr cls = GetVariantCls(mode);
    if (cls)
    {
      return (*cls) >> "text";
    }
  }
  return RString();
}

void SentenceParamDistance::Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const
{
  if (mode/100==ModeDistMeters)
  { // use meters in destination
    int dist100 = toInt(_distance*0.01);
    const int maxDist = sizeof(distSay)/sizeof(*distSay)-1;
    saturate(dist100,0,maxDist);
    sentence.Add(distSay[dist100], cfg.pauseAfterWord);
  }
  else
  {
    ParamEntryPtr cls = GetVariantCls(mode);
    if (cls)
    {
      AutoArray<RString, MemAllocLocal<RString, 16> > words;
      LoadSpeech(words, (*cls) >> "speech");
      for (int i=0; i<words.Size(); i++)
        sentence.Add(words[i], cfg.pauseAfterWord);
    }
  }
}

DEFINE_FAST_ALLOCATOR(SentenceParamDistance)

RString SentenceParamLocation::Write(int mode) const
{
  return _location ? _location->_text : RString("");
}

void SentenceParamLocation::Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const
{
#if _ENABLE_CONVERSATION
  if (_location)
  {
    for (int i=0; i<_location->_speech.Size(); i++)
      sentence.Add(_location->_speech[i], cfg.pauseAfterWord);
  }
#endif
}

DEFINE_FAST_ALLOCATOR(SentenceParamLocation)

RString SentenceParamAggrTargets::GetVariantCls(int mode) const
{
  RString varDefault("Default");
  Speaker *speaker = _msg->GetSpeaker();
  if (speaker)
  {
    RString protocol = speaker->GetProtocol();
    ConstParamEntryPtr protocolCls = protocol.GetLength() > 0 ? Pars.FindEntry(protocol) : ParamEntryPtr(NULL);
    if (protocolCls)
    { 
      ParamEntryPtr cls = protocolCls->FindEntry("Arguments");
      if (!cls) return varDefault;
      if (cls->IsClass())
      {
        cls = cls->FindEntry("AggrTargets");
        if (!cls) return varDefault;
        cls = cls->FindEntry("modes");
        if (!cls || !cls->IsArray()) return varDefault;
        if (mode>cls->GetSize()) mode=1;
        mode--; //make the mode an index into the array
        return (*cls)[mode];
      }
    }
  }
  return varDefault;
}

RString SentenceParamAggrTargets::Write(int mode) const
{
  return _text;
}

void SentenceParamAggrTargets::Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const
{
  // TODO: use mode to select specific speech variant
  for (int tn=0; tn<_speechList.Size(); tn++)
  {
    const EntitySpeechVariant *variant = _speechList[tn]._type->GetSpeechVariant(GetVariantCls(mode));
    const AutoArray<RString> &words = _speechList[tn]._singular ?
      (variant ? variant->_speechSingular : _speechList[tn]._type->GetSpeechSingular() ) : 
      (variant ? variant->_speechPlural : _speechList[tn]._type->GetSpeechPlurar() );
    if (tn>0) sentence.Add(RString("and"), cfg.pauseAfterWord); //note: 'and' should be defined in RadioProtocolBase.Words
    for (int i=0; i < words.Size(); i++)
      sentence.Add(words[i], cfg.pauseAfterWord);
  }
#if DIAG_AGGREGATED_TARGETS
  RString log;
  for (int i=0; i < sentence.Size(); i++)
    log = log + sentence[i].id + RString(" ");
  LogF("  AggrTargets::Say: %s", cc_cast(log));
#endif
}

DEFINE_FAST_ALLOCATOR(SentenceParamAggrTargets)

ParamEntryPtr SentenceParamDirection::GetVariantCls(int mode) const
{
  Speaker *speaker = _msg->GetSpeaker();
  if (speaker)
  {
    RString protocol = speaker->GetProtocol();
    ConstParamEntryPtr protocolCls = protocol.GetLength() > 0 ? Pars.FindEntry(protocol) : ParamEntryPtr(NULL);
    if (protocolCls) 
    { 
      ParamEntryPtr cls = protocolCls->FindEntry("Arguments");
      if (!cls) return ParamEntryPtr();
      cls = cls->FindEntry("Direction");
      if (!cls) return ParamEntryPtr();
      if (_forceCompass || (mode/100==ModeDirCompass))
      {
        char buffer[32];
        sprintf(buffer, "Compass%d", mode%100);
        cls = cls->FindEntry(buffer);
        if (!cls) return ParamEntryPtr();
        int azimuth = toInt(_absDir*4/H_PI)*45; //azimuth using 45 degrees step
        if (azimuth<0) azimuth+=360;
        azimuth %= 360;
        sprintf(buffer, "%d", azimuth);
        return cls->FindEntry(buffer);
      }
      else // use left/right/front/rear side directions
      {
        char buffer[32];
        sprintf(buffer, "Relative%d", mode%100);
        cls = cls->FindEntry(buffer);
        if (!cls) return ParamEntryPtr();
        int azimuth = toInt(_relDir*2/H_PI)*90; //azimuth using 90 degrees step
        if (azimuth<0) azimuth+=360;
        azimuth %= 360;
        sprintf(buffer, "%d", azimuth);
        return cls->FindEntry(buffer);
      }
    }
  }
  return ParamEntryPtr();
}

RString SentenceParamDirection::Write(int mode) const
{
  if (!_forceCompass && (mode/100==ModeDirHours))
  { // use hours as dir
    int azimuth = toInt(_relDir*6/H_PI); //azimuth using hours
    if (azimuth<0) azimuth+=12;
    if (azimuth>=0 && azimuth<=12)
    {
      return LocalizeString(*azimWrite[azimuth]);
    }
  }
  else
  {
    ParamEntryPtr cls = GetVariantCls(mode);
    if (cls)
    {
      return (*cls) >> "text";
    }
  }
  return RString();
}

void SentenceParamDirection::Say(RadioSentence &sentence, const RadioChannelParams &cfg, int mode) const
{
  if (!_forceCompass && (mode/100==ModeDirHours) )
  { // use hours as dir
    int azimuth = toInt(_relDir*6/H_PI); //azimuth using hours
    if (azimuth<0) azimuth+=12;
    if (azimuth>=0 && azimuth<=12)
    {
      sentence.Add(azimSay[azimuth], cfg.pauseAfterNumber);
    }
  }
  else
  {
    ParamEntryPtr cls = GetVariantCls(mode);
    if (cls)
    {
      AutoArray<RString, MemAllocLocal<RString, 16> > words;
      LoadSpeech(words, (*cls) >> "speech");
      for (int i=0; i<words.Size(); i++)
        sentence.Add(words[i], cfg.pauseAfterWord);
    }
  }
}

DEFINE_FAST_ALLOCATOR(SentenceParamDirection)

static void SaySentence
(
  RadioSentence &sentence,
  ParamEntryPar clsVariant, SentenceParams &params,
  float pauseAfterMessage, const RadioChannelParams &cfg
)
{
  ParamEntryVal cls = clsVariant >> "speech";

  ConstParamEntryPtr wordsClass = clsVariant.FindEntry("wordsClass");
  if (wordsClass)
  {
    sentence.wordsClass = *wordsClass;
  }

  int n = cls.GetSize();
  int nParams = params.Size();
  for (int i=0; i<n; i++)
  {
    RString word = cls[i];
    if (!word || word.GetLength() == 0) continue;
    if (word[0] == '\'') continue;
    if (word[0] == '%')
    {
      if (word[1] == 'q')
      {
        // do not say MicOut
        // do not wait
        return;
      }
      else
      {
        const char *format = word;
        format++;
        int index = 0;
        int mode = 1;
        while (isdigit(*format))
        {
          index *= 10;
          index += *format - '0';
          format++;
        }
        index--;
        if (*format == '.' && isdigit(*(format + 1)))
        {
          format++;
          mode = 0;
          do
          {
            mode *= 10;
            mode += *format - '0';
            format++;
          } while (isdigit(*format));
        }
        if (index < 0 || index >= nParams) continue;

        params[index]->Say(sentence, cfg, mode);
      }
    }
    else
      sentence.Add(word, cfg.pauseAfterWord);
  }

  sentence.Add("xmit", 0);
}

#if !defined _WIN32
#include <wctype.h>
int CharUpperBuffW(wchar_t *text, int len)
{
  for (int i=0; i<len; i++) 
  {
    *text = towupper(*text);
    text++;
  }
  return len; //There is no return value reserved to indicate an error (from: man towupper)
}
#endif

//! Function to perform uppercase in unicode space
RString StrUprUniversal(const char *text)
{
  // Convert UTF8 to unicode
  TempWString<> unicodeText;
  {
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    unicodeText.Resize(bufferSize);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, unicodeText.Data(), unicodeText.Size());
  }

  // Make the text uppercase
  CharUpperBuffW(unicodeText.Data(), unicodeText.Size());
  // Test - make only the first character uppercase
  // if (unicodeText.Size() > 0) CharUpperBuffW(unicodeText.Data(), 1);

  // Convert unicode back to UTF8
  RString textUTF8;
  {
    int dstLen = WideCharToMultiByte(CP_UTF8, 0, unicodeText.Data(), unicodeText.Size(), NULL, 0, NULL, NULL);
    char *data = textUTF8.CreateBuffer(dstLen);
    WideCharToMultiByte(CP_UTF8, 0, unicodeText.Data(), unicodeText.Size(), data, dstLen, NULL, NULL);
  }

  // Return UTF8 text
  return textUTF8;
}

static RString WriteSentence(RString format, SentenceParams &params)
{
  int nParams = params.Size();
  const int resultSize = 2048;
  char result[2048], c;
  const char *src = format;
  int len = 0;

  while ((c = *src)!=0)
  {
    if (c == '%')
    {
      src++;
      int index = 0;
      int mode = 1;
      while (isdigit(*src))
      {
        index *= 10;
        index += *src - '0';
        src++;
      }
      index--;
      if (*src == '.' && isdigit(*(src + 1)))
      {
        src++;
        mode = 0;
        do
        {
          mode *= 10;
          mode += *src - '0';
          src++;
        } while (isdigit(*src));
      }
      if (index < 0 || index >= nParams) continue;

      RString text = params[index]->Write(mode);
      int n = text.GetLength();
      if (len + n < resultSize)
      {
        strcpy(result + len, text);
        len += n;
      }
    }
    else
    {
      if (len + 1 >= resultSize) break;
      *(result + len) = c;
      src++;
      len++;
    }
  }
  *(result + len) = 0;

  return StrUprUniversal(result);
}

const float coefAa11 = 100.0 / (256*50);
const float invCoefAa11 = 1.0 / coefAa11;

///////////////////////////////////////////////////////////////////////////////
// class RadioMessage

static const EnumName RadioMessageTypeNames[]=
{
  EnumName(RMTText, "TEXT"),
  EnumName(RMTTalk, "TALK"),
  EnumName(RMTTask, "TASK"),
  EnumName(RMTTaskReceived, "TASK RECEIVED"),
  EnumName(RMTTaskResult, "TASK RESULT"),
  EnumName()
};
template<>
const EnumName *GetEnumNames(RadioMessageType dummy)
{
  return RadioMessageTypeNames;
}

RadioMessage::RadioMessage()
{
  _transmitted = false;
  _playerMsg = false;
}

float RadioMessage::GetDuration() const
{
  return 1.0;
}

Speaker *RadioMessage::GetSpeaker() const
{
  AIBrain *sender = GetSender();
  return sender ? sender->GetSpeaker() : NULL;
}

bool RadioMessage::CanSenderSpeak() const
{
  if (GetSenderName().GetLength() > 0) return true; // HQ can always speak

  // real person is speaking
  AIBrain *sender = GetSender();
  return sender && sender->LSCanSpeak();
}

RString RadioMessage::GetDebugName() const
{
  SentenceParams params;
  RString debugName = PrepareSentence(params);
  if (debugName.GetLength() == 0) debugName = unconst_cast(this)->GetWave();
  AIBrain *sender = GetSender();
  return debugName + " -  " + RString(sender ? sender->GetDebugName() : "<null>");
}

static RadioMessage *CreateRadioMessage(RadioMessageType type)
{
  switch (type)
  {
  case RMTText:
    return new RadioMessageText();
#if _ENABLE_CONVERSATION
  case RMTTalk:
    return new RadioMessageTalk();
#endif
#if _ENABLE_IDENTITIES
  case RMTTask:
    return new RadioMessageTask();
  case RMTTaskReceived:
    return new RadioMessageTaskReceived();
  case RMTTaskResult:
    return new RadioMessageTaskResult();
#endif
  }
  ErrF("Uknown radio message Type %d",type);
  return NULL;
}

RadioMessage *RadioMessage::CreateObject(ParamArchive &ar)
{
  RadioMessageType type;
  if (ar.SerializeEnum("type", type, 1) != LSOK) return NULL;
  return CreateRadioMessage(type);
}

LSError RadioMessage::Serialize(ParamArchive &ar)
{
  if (ar.IsSaving())
  {
    RadioMessageType type = (RadioMessageType)GetType();
    CHECK(ar.SerializeEnum("type", type, 1))
  }
  CHECK(ar.Serialize("priority", _priority, 1, 0))
  CHECK(::Serialize(ar, "timeout", _timeOut, 1))
  CHECK(ar.Serialize("playerMsg", _playerMsg, 1, false))
  return LSOK;
}

///////////////////////////////////////////////////////////////////////////////
// class RadioChannel

RadioMessage *RadioChannel::CreateMessage(int type)
{
  return CreateRadioMessage((RadioMessageType)type);
}

void RadioSentence::Say(Person *who, RadioChannel *channel, Speaker *speaker)
{
  bool transmit = false;
  for (int i=0; i<Size(); i++)
  {
    RString id = Get(i).id;
    if (id.GetLength()<=0)
    {
      // log all words
      char buf[1024];
      *buf=0;
      for (int s=0; s<Size(); s++)
      {
        strcat(buf,"'");
        strcat(buf,Get(s).id);
        strcat(buf,"' ");
      }
      RptF("Empty word in sentence %s",buf);
      continue;
    }
    else if (stricmp(id, "xmit") == 0)
    {
      transmit = true;
    }
    else
    {
      channel->Say(who, speaker, Get(i).id, Get(i).pauseAfter, transmit, wordsClass);
      transmit = false;
    }
  }
}

ConstParamEntryPtr FindRadio(RString name, SoundPars &pars, bool silent = false);

void RadioChannel::Say(RString waveName, AIBrain *sender, RString senderName, RString player, float duration)
{
  if (!sender && senderName.GetLength() == 0) return;

  SoundPars pars;
  if (waveName[0] == '#')
  {
    char buffer[256];
    strncpy(buffer, (const char *)waveName + 1, 256); buffer[255] = 0;
    char *ext = strrchr(buffer, '.');
    if (ext) *ext = 0;
    GChatList.Add(_chatChannel, sender, buffer, false, false);

    RString dir;
    if (player.GetLength() > 0)
    {
      RString GetClientCustomFilesDir();
      dir = GetClientCustomFilesDir() + RString("\\players\\") + EncodeFileName(player) + RString("\\sound\\");
    }
    else
    {
      RString GetUserDirectory();
      dir = GetUserDirectory() + RString("sound\\");
    }
    pars.name = dir + RString((const char *)waveName + 1);
    pars.freq = 1;
    pars.freqRnd = 1;
    pars.vol = 1;
    pars.volRnd = 1;
  }
  else
  {
    ConstParamEntryPtr cls = FindRadio(waveName, pars);

    if (cls)
    {
      if (sender)
        GChatList.Add(_chatChannel, sender, *cls >> "title", false, false);
      else
        GChatList.Add(_chatChannel, Pars >> "CfgHQIdentities" >> senderName >> "name", *cls >> "title", false, false);
    }
  }

  Person *who = NULL;
  if (_params.sound3D && sender) who = sender->GetPerson();

  if (pars.name.GetLength() > 0  && GWorld->GetAcceleratedTime() <= 1)
  {
    AbstractWave *wave = NULL;
    if (who)
    {
      const float volume3DCoef = 0.01f;
      wave = GSoundScene->OpenAndPlayOnce(pars.name, who, false, who->FutureVisualState().Position(), who->FutureVisualState().Speed(), volume3DCoef, pars.freq, pars.distance);
      if (wave)
      {
        who->SetDirectSpeaking(wave);
        who->AttachWave(wave,pars.freq);
        wave->SetKind(WaveEffect);
      }
    }
    else
    {
      wave = GSoundScene->OpenAndPlayOnce2D(pars.name, 1, pars.freq, false);
      if (wave)
      {
        wave->SetKind(WaveSpeech);
      }
    }
    if (!_saying)
    {
      // start queue
      _saying = wave;
      if (_saying)
      {
        _saying->Play(); // start playback (once)
      }
    }
    else
    {
      if (wave)
      {
        wave->Queue(_saying);
      }
    }
    _pauseAfterMessage = 0;
  }
  else
  {
    _pauseAfterMessage = duration;
  }

  if (sender)
  {
    _speaker = *sender->GetSpeaker();
  }
  else // if (senderName.GetLength() > 0)
  {
    ParamEntryVal cls = Pars >> "CfgHQIdentities" >> senderName;
    RString name = cls >> "speaker";
    float pitch = cls >> "pitch";
    Ref<BasicSpeaker> basicSpeaker = new BasicSpeaker(name);
    _speaker = Speaker(basicSpeaker, pitch);
  }
  // start playing noise
  if (_saying && _speaker.IsSpeakerValid())
  {
    if (_params.noise.GetLength() > 0)
    {
      _noise = _speaker.SayNoPitch(who, _params.noise, true, RString());
    }
  }
}

bool IsSpeakingDirect(AIBrain *sender);

bool RadioChannel::CheckSpeaking(AIBrain *sender) const
{
  const RadioMessage *msg = GetActualMessage();
  if (msg)
  {
    return msg->GetSender()==sender;
  }
  else
  {
    // nothing is being said right now - check first message in the queue
    if (_radioQueue.Size()>0)
    {
      msg = _radioQueue[0];
      return msg->GetSender()==sender;
    }
    return false;
  }
}

/// helper functor - perform a check if given channel is a more important one
class RadioCheck
{
  RadioChannel *_speaking;
  RadioChannel *_ignore;
  AIBrain *_sender;
  
  public:
  RadioCheck(AIBrain *sender, RadioChannel *ignore)
  :_speaking(NULL),_sender(sender),_ignore(ignore)
  {}

  void operator () (RadioChannel &channel)
  {
    if (&channel==_ignore) return;
    if (_speaking && _speaking->GetLevel()>channel.GetLevel()) return;
    if (!channel.CheckSpeaking(_sender)) return;
    _speaking = &channel;
  }
  operator RadioChannel *() const {return _speaking;}
};

/// determine what priority is currently being used
/**
@return highest priority channel currently in use
*/
RadioChannel *SpeakingOnRadioChannel(AIBrain *sender, RadioChannel *ignore)
{
  if (!sender) return NULL;

  // even when not speaking, but the message is about to start, we should consider it speaking
  // global channel
  RadioCheck check(sender,ignore);
  check(GWorld->GetRadio());

  // vehicle channel
  Transport *veh = sender->GetVehicleIn();
  if (veh)
  {
    check(veh->GetRadio());
  }

  // custom radios
  for (int i=0; i<CCCustomN; i++)
  {
    CustomChannelInfo &info = GCustomChannels.GetCustomChannelInfo(CCCustom1+i);
    if (info.active)
    {
      check(GWorld->GetCustomRadio(i));
    }
  }

  // direct speaking
  Person *person = sender->GetPerson();
  RadioChannel *radio = person ? person->GetRadio() : NULL;
  if (radio) check(*radio);

  return check;
}

/*!
\patch 1.55 Date 5/7/2002 by Ondra
- Changed: All radio trafic that is done in mission init is now processed before the mission starts,
expect for messages transmitted via xxxRadio and xxxChat scripting commands.
\patch 1.78 Date 7/16/2002 by Jirka
- Fixed: Silent processing of radio messages sometimes freeze
*/

void RadioChannel::SilentProcess()
{
  if (_actualMsg)
  {
    if (!_actualMsg->IsTransmitted())
      _actualMsg->Transmitted(_chatChannel); // confirm xmit done
      // SetTransmitted is needless
    _actualMsg=NULL;
  }
/*
  for (int i=0; i<_radioQueue.Size(); i++)
  {
    RadioMessage *msg = _radioQueue[i];
    if (msg->GetType()!=RMTText)
    {
      msg->Transmitted(_chatChannel);
    }
  }
  _radioQueue.Clear();
*/
  // FIX
  while (_radioQueue.Size() > 0)
  {
    Ref<RadioMessage> msg = _radioQueue[0];
    _radioQueue.Delete(0);
    if (msg->GetType()!=RMTText)
      msg->Transmitted(_chatChannel);
  }
}

/*!
\patch 5129 Date 2/19/2007 by Jirka
- Fixed: MP - Radio messages echo on remote clients was missing
*/

void RadioChannel::NextMessage(NetworkObject *object)
{
  if (_actualMsg)
  {
    if (!_actualMsg->IsTransmitted())
      _actualMsg->Transmitted(_chatChannel); // confirm xmit done
      // SetTransmitted is needless
    _actualMsg=NULL;
  }

  while
  (
    _radioQueue.Size() > 0 &&
    ((!_radioQueue[0]) || Glob.time > _radioQueue[0]->GetTimeOut())
  )
  {
    if (_radioQueue[0]) _radioQueue[0]->Canceled();
    _radioQueue.Delete(0);
  }

  if (_radioQueue.Size() <= 0 ) return;

  AIBrain *sender = _radioQueue[0]->GetSender();
  if (IsSpeakingDirect(sender))
  {
    // if speaker is speaking on direct channel, we cannot do anything but wait
    _pauseAfterMessage = 0; // check in next simulation step
    return;
  }
  RadioChannel *speaking = SpeakingOnRadioChannel(sender,this);

  if (speaking)
  {
    // AI will always wait until it finished what it is currently saying
    if (speaking->GetLevel()>=GetLevel() || !sender->IsAnyPlayer() && speaking->GetActualMessage())
    {
      _pauseAfterMessage = 0; // check in next simulation step
      return;
    }
    // we are currently speaking on lower level, we can abort on postpone the message
    
    if(sender->IsAnyPlayer() && speaking->GetActualMessage() && speaking->GetActualMessage()->IsConversation())
    {  
      _radioQueue[0]->Transmitted(_chatChannel);  
      SentChatMessage(_radioQueue[0], sender, object);
      _radioQueue.Delete(0);
      return;
    }
    else 
    {     
      speaking->StopSpeaking(sender);
    }
  }

  if (_radioQueue.Size() <= 0 ) return;

  _actualMsg = _radioQueue[0];

  bool msgAudible = _audible && (GWorld->IsSentencesEnabled() || (_actualMsg && _actualMsg->IsConversation()));
  _radioQueue.Delete(0);

  if (!_actualMsg) return;

  float pauseAfterMessage = 0;
  if ((!sender || sender != GWorld->FocusOn()))
  {
    if (_radioQueue.Size() <= 5)
    {
      pauseAfterMessage = GRandGen.Gauss(0.1f, 0.4f, 2.5f); 
    }
    else
    {
      pauseAfterMessage = GRandGen.Gauss(0.1f, 0.15f, 0.3f); 
    }
  }
  
  // not all messages have real 3D sender
  Person *who = sender ? sender->GetPerson() : NULL;

  SentenceParams params;
  RString sentenceName = _actualMsg->PrepareSentence(params); // name of sentence
  ConstParamEntryPtr sentence;

  // select radio protocol by the speaker
  Speaker *speaker = _actualMsg->GetSpeaker();
  ConstParamEntryPtr argClass = ParamEntryPtr(NULL);
  if (speaker && !sentenceName.IsEmpty())
  {
    RString protocol = speaker->GetProtocol();
    ConstParamEntryPtr protocolCls = protocol.GetLength() > 0 ? Pars.FindEntry(protocol) : ParamEntryPtr(NULL);
    if (protocolCls) 
    {
      argClass = protocolCls->FindEntry("Arguments");
      sentence = protocolCls->FindEntry(sentenceName);
    }
  }

  if (sentence)
  {
    float rnd = GRandGen.RandomValue();
    ParamEntryPtr clsVariant = SelectVariant(_actualMsg, (*sentence), rnd);
    if (!clsVariant)
    {
      // nothing to tell
      _pauseAfterMessage = 0;
    }
    else
    {
      bool voiceEnabled = true;
  #if _VBS3
      if(!Glob.config.IsEnabled(DTAIVoices))
        voiceEnabled = false;
      else
        if(who && who->Brain())
          voiceEnabled = who->Brain()->CanSpeak();
  #endif

      if (who && sender->IsGestureEnabled(_chatChannel))
      {
        // first try searching in a variant
        // when not found, use a common entry
        ConstParamEntryPtr entry = (*clsVariant).FindEntry("gesture");
        if (!entry) entry = sentence->FindEntry("gesture");
        if (entry)
        {
          RString gesture = *entry;
          if (gesture.GetLength() > 0)
          {
            who->PlayActionGlobal(gesture);
            // voice can be disabled
            voiceEnabled = sender->IsVoiceEnabled(_chatChannel);
          }
        }
      }

      if (voiceEnabled)
      {
        RadioSentence sent;
        // fixed - voice communication was not audible
        if (GWorld->IsSentencesEnabled() || _actualMsg->IsConversation()) SaySentence(sent, *clsVariant, params, pauseAfterMessage, _params);
        if (GWorld->GetAcceleratedTime() <= 1)
        {
          sent.Say(who, this, speaker);
          _pauseAfterMessage=0;
        }
        else
        {
          _pauseAfterMessage = _actualMsg->GetDuration();
        }

        if (msgAudible || GWorld->GetMode() == GModeNetware)
        {
          RString text = WriteSentence((*clsVariant) >> "text", params);
#if DIAG_SELECT_ENEMY_DETECTED_SENTENCE
          if (stricmp(sentenceName, "SelectEnemyDetectedSentence")==0)
          {
            LogF("   Write: \"%s\"", cc_cast(text));
          }
#endif
          if (msgAudible) GChatList.Add(_chatChannel, _actualMsg->GetSender(), text, _actualMsg->IsPlayerMsg(), false);
          if (_actualMsg->GetSender() && _actualMsg->SendOverNetwork()) SendRadioChat(_chatChannel, _actualMsg->GetSender(), object, text, sent);
        }
      }
    }
  }
  else
  {
    _pauseAfterMessage = _actualMsg->GetDuration();

    if (msgAudible || GWorld->GetMode() == GModeNetware)
    {
      RString text = _actualMsg->GetText();
      if (msgAudible && text.GetLength() > 0)
      {
#if _VBS3
        // disable text and audio, if AI speech has been disabled
        if(who && who->Brain() && who->Brain()->CanSpeak())
#endif
        GChatList.Add(_chatChannel, _actualMsg->GetSender(), text, _actualMsg->IsPlayerMsg(), false);
      }

      RString wave = _actualMsg->GetWave();
      if (wave.GetLength() > 0)
      {
        _pauseAfterMessage = 0;
        if (msgAudible)
          Say(wave, _actualMsg->GetSender(), _actualMsg->GetSenderName(), "", _actualMsg->GetDuration());
        if (_actualMsg->GetSender() && _actualMsg->SendOverNetwork())
          SendRadioChatWave(_chatChannel, object, wave, _actualMsg->GetSender(), _actualMsg->GetSenderName());
      }
      else
      {
        RadioSentence sent;
        _actualMsg->GetSpeech(sent, _params);
        if (GWorld->GetAcceleratedTime() <= 1 && msgAudible && sent.Size() > 0)
        {
          sent.Say(who, this, speaker);
          _pauseAfterMessage = 0;
        }
        // SendRadioChat only when something relevant is present
        if ( _actualMsg->GetSender() && _actualMsg->SendOverNetwork() && (text.GetLength() > 0 || sent.Size() > 0) )
          SendRadioChat(_chatChannel, _actualMsg->GetSender(), object, text, sent);
      }
    }
  }
}

void RadioChannel::SentChatMessage(RadioMessage *message, AIBrain *sender, NetworkObject *object)
{
  if(!message) return;
  bool msgAudible = _audible && (GWorld->IsSentencesEnabled() || message->IsConversation());
    
  //float pauseAfterMessage = 0;
  // not all messages have real 3D sender
  Person *who = sender ? sender->GetPerson() : NULL;

  SentenceParams params;
  RString sentenceName = message->PrepareSentence(params); // name of sentence
  ConstParamEntryPtr sentence;

  // select radio protocol by the speaker
  Speaker *speaker = message->GetSpeaker();
  ConstParamEntryPtr argClass = ParamEntryPtr(NULL);
  if (speaker && !sentenceName.IsEmpty())
  {
    RString protocol = speaker->GetProtocol();
    ConstParamEntryPtr protocolCls = protocol.GetLength() > 0 ? Pars.FindEntry(protocol) : ParamEntryPtr(NULL);
    if (protocolCls) 
    {
      argClass = protocolCls->FindEntry("Arguments");
      sentence = protocolCls->FindEntry(sentenceName);
    }
  }

  if (sentence)
  {
    float rnd = GRandGen.RandomValue();
    ParamEntryPtr clsVariant = SelectVariant(message, (*sentence), rnd);
    if (!clsVariant)
    {
      // nothing to tell
      //_pauseAfterMessage = 0;
    }
    else
    {
      bool voiceEnabled = true;

      if (who && sender->IsGestureEnabled(_chatChannel))
      {
        // first try searching in a variant
        // when not found, use a common entry
        ConstParamEntryPtr entry = (*clsVariant).FindEntry("gesture");
        if (!entry) entry = sentence->FindEntry("gesture");
        if (entry)
        {
          RString gesture = *entry;
          if (gesture.GetLength() > 0)
          {
            who->PlayActionGlobal(gesture);
            // voice can be disabled
            voiceEnabled = sender->IsVoiceEnabled(_chatChannel);
          }
        }
      }

      if (voiceEnabled)
      {
        RadioSentence sent;

        if (msgAudible || GWorld->GetMode() == GModeNetware)
        {
          RString text = WriteSentence((*clsVariant) >> "text", params);
          if (msgAudible) GChatList.Add(_chatChannel, message->GetSender(), text, message->IsPlayerMsg(), false);
          if (message->GetSender() && message->SendOverNetwork()) SendRadioChat(_chatChannel, message->GetSender(), object, text, sent);
        }
      }
    }
  }
  else
  {
    if (msgAudible || GWorld->GetMode() == GModeNetware)
    {
      RString text = message->GetText();
      if (msgAudible && text.GetLength() > 0)
      {
          GChatList.Add(_chatChannel, message->GetSender(), text, message->IsPlayerMsg(), false);
      }

      RString wave = message->GetWave();
      if (wave.GetLength() > 0)
      {
        if (message->GetSender() && message->SendOverNetwork())
          SendRadioChatWave(_chatChannel, object, wave, message->GetSender(), message->GetSenderName());
      }
      else
      {
        RadioSentence sent;
        if ( message->GetSender() && message->SendOverNetwork() && (text.GetLength() > 0 || sent.Size() > 0) )
          SendRadioChat(_chatChannel, message->GetSender(), object, text, sent);
      }
    }
  }
}

RadioMoveToHelper::RadioMoveToHelper()
{
  // Helper values to answer later Simple Expression questions
  _distanceToGroup = RadioMessage::GetDefaultDistanceToGroup();
  _distanceToLocation = RadioMessage::GetDefaultDistanceToLocation();
  _distanceToUnit = RadioMessage::GetDefaultDistanceToUnit();
  _groupCoreRadius = RadioMessage::GetDefaultGroupCoreRadius();
  _groupCompactness = RadioMessage::GetDefaultGroupCompactness();
  _distanceToRecipients = RadioMessage::GetDefaultDistanceToRecipients();
  _recipientsRadius = RadioMessage::GetDefaultRecipientsRadius();
  _isInsideLocation = RadioMessage::GetDefaultInsideLocation();
  _moveToObject = RadioMessage::GetDefaultMoveToObject();
  _location = NULL;
}

void RadioMoveToHelper::PrepareSEHelperVariables(AIBrain *first, Vector3Par destination, Target *tgtE, TargetId tgtF)
{
  // prepare _distanceToUnit
  float best2 = first->Position(first->GetFutureVisualState()).DistanceXZ2(destination);
  _distanceToUnit = sqrt(best2);
  saturateMax(_distanceToUnit, 1); //at least 1 meter

  Vector3 corePosition = first->Position(first->GetFutureVisualState());
  Vector3 coreDirection = first->Direction(first->GetFutureVisualState());

  // Target relative moveTo command is processed different way
  Target *tgt = tgtE;
  if (tgt && tgt->GetType()->GetShortName().GetLength() > 0)
  { // target should be used, other parameters are not important
    // Prepare groupTgtDirection and groupTgtDistance
    Vector3 absDirVect = tgt->GetPosition()-corePosition;
    _distanceToGroup = absDirVect.SizeXZ();
    saturateMax(_distanceToGroup, 1); //at least 1 meter
    return; //done
  }

  // Prepare groupTgtDirection and groupTgtDistance
  Vector3 absDirVect = destination-corePosition;
  _distanceToGroup = absDirVect.SizeXZ();
  saturateMax(_distanceToGroup, 1); //at least 1 meter

  // LOCATIONS related parameters
  //float maxDist2 = min(0.25f*Square(_distanceToGroup),500.0f*500.0f); //not more than five hundred meters
  float maxDist2 = 1000*1000; //not more than 1000 meters
  _location = FindNearestLocationWithDubbing(destination, maxDist2);
#if PROFILE_LOCATION_FINDING
  /* //Testing on Chernaruss shows (time for 100 iterations in miliseconds)
   643.018: Location 400m: 0.469045
   643.020: Location 500m: 0.793977
   643.021: Location 600m: 0.805468
   643.023: Location 700m: 1.036633
   643.024: Location 800m: 1.149938
   643.025: Location 900m: 1.718684
   643.028: Location 1000m: 1.768494
   643.031: Location 1500m: 3.390542
  */
  {
    SectionTimeHandle _start;
    for (int ldist = 400; ldist < 1000; ldist +=100)
    {
      _start = StartSectionTime();
      maxDist2 = ldist*ldist;
      for (int i=0; i<100; i++) _location = FindNearestLocationWithDubbing(destination, maxDist2);
      LogF("Location %dm: %.2f", ldist, GetSectionTime(_start)*1000);
    }
    int ldist=1500;
    _start = StartSectionTime();
    maxDist2 = ldist*ldist;
    for (int i=0; i<100; i++) _location = FindNearestLocationWithDubbing(destination, maxDist2);
    LogF("Location %dm: %.2f", ldist, GetSectionTime(_start)*1000);
  }
#endif
  //_location = FindNearestLocationWithDubbing(_command._destination, maxDist2);
  _absLocationDir = 0;
  if (_location)
  {
#if 0
    // Diagnose to know locations params available
    LogF("Location: %s, a=%.1f, b=%.1f, rectangular=%d, angle=%.1f", cc_cast(_location->_text), _location->_a, _location->_b, _location->_rectangular, _location->_angle);
#endif
    _isInsideLocation = (_location && _location->IsInside(destination)) ?  1.0f : 0;
    if (_location->IsInside(corePosition))
    { // group is inside location, the description using this location has no sense
      _distanceToLocation = 1e10; // do not use this location
    }
    else
    {
      Vector3 absLocationVect = destination - _location->GetPosition();
      _distanceToLocation = absLocationVect.SizeXZ();
      _absLocationDir = atan2(absLocationVect.X(),absLocationVect.Z());
    }
  }
  else _distanceToLocation = 1e10; //same as no Location found
  saturateMax(_distanceToLocation, 1); //at least 1 meter

    _recipientsCenterPos = first->Position(first->GetFutureVisualState());
  float rad2 = RadioMessage::GetDefaultRecipientsRadius(); //avoid zero division
  _recipientsRadius = sqrt(rad2);
  _distanceToRecipients = _recipientsCenterPos.DistanceXZ(destination);
}

void RadioMoveToHelper::SetParams(SentenceParams &params, const RadioMessage *msg, AIBrain *first, Vector3Par destination)
{
  Vector3 corePosition = first->Position(first->GetFutureVisualState());
  Vector3 coreDirection = first->Direction(first->GetFutureVisualState());

  // Parameters for SelectCmdMoveSentence group of sentences
  // 1 - unit list ... already added
  // 2 - grid position
  // square in map
  // VBS supports bigger grids
  char buffer[32];
  PositionToAA11(destination, buffer);
  params.Add(RString(buffer));

  // Prepare groupTgtDirection and groupTgtDistance
  Matrix4 transf;
  transf.SetOriented(coreDirection, VUp);
  transf.SetPosition(corePosition);
  Vector3 dir =  transf.InverseRotation().FastTransform(destination);
  float relDir = atan2(dir.X(),dir.Z());
  Vector3 absDirVect = destination-corePosition;
  float absGrpDir = atan2(absDirVect.X(), absDirVect.Z());
  // Prepare Leader/Group mix position and direction
  Matrix4 transfLeader;
  float leaderAbsDir = 0;
  float distanceToLeader = 0;
  transfLeader = transf;

  int martaType = -1;

  Vector3 leaderDir =  transfLeader.InverseRotation().FastTransform(destination);
  float leaderRelDir = atan2(leaderDir.X(),leaderDir.Z());
  // 3 - distance from group
  params.AddDistance(_distanceToGroup, martaType, msg); //DISTANCE
  // 4 - direction from group
  params.AddDirection(relDir, absGrpDir, false, msg); //DIRECTION

  // LOCATIONS related parameters
  //LogF(" dist2Location: %.1f, dist2Group: %.1f, dist2Unit: %.1f", _distanceToLocation, _distanceToGroup, _distanceToUnit);
  // 5 - distance from location
  params.AddDistance(_distanceToLocation, martaType, msg);
  // 6 - direction from location
  params.AddDirection(relDir, _absLocationDir, true, msg);
  // 7 - location
  params.AddLocation(_location);

  // Object related parameters
  bool objParamsDone = false;
  if (!objParamsDone)
  { //add empty object related parameters
    params.AddWord(RString(""), RString(""));
    // 9 - direction to object
    params.AddDirection(0, 0, true, msg);
  }

  // Recipients related parameters
  Vector3 absRecipientsVect = destination - _recipientsCenterPos;
  float absRecipientsDir = atan2(absRecipientsVect.X(),absRecipientsVect.Z());
  // 10 - distance from recipients center
  params.AddDistance(_distanceToRecipients, martaType, msg); //DISTANCE
  // 11 - direction from recipients center
  params.AddDirection(0, absRecipientsDir, true, msg);
  // 12 - distance from Leader
  params.AddDistance(distanceToLeader, martaType, msg); //DISTANCE
  // 13 - direction from Leader
  params.AddDirection(leaderRelDir, leaderAbsDir, false, msg);
}

///////////////////////////////////////////////////////////////////////////////
// class RadioMessageText

LSError RadioMessageText::Serialize(ParamArchive &ar)
{
  CHECK(RadioMessage::Serialize(ar))
  CHECK(ar.Serialize("wave", _wave, 1))
  CHECK(ar.Serialize("ttl", _timeToLive, 1))
  CHECK(ar.SerializeRef("sender", _sender, 1))
  CHECK(ar.Serialize("senderName", _senderName, 1, RString()))

  return LSOK;
}

const char *RadioMessageText::GetPriorityClass()
{
  return "Design";
}

RString RadioMessageText::PrepareSentence(SentenceParams &params) const
{
  SoundPars pars;
  ConstParamEntryPtr cls = FindRadio(_wave, pars, true); // check only, avoid warning message
  if (cls) return RString(); // if possible, interpret _wave as an entry in CfgRadio

  return _wave; // otherwise, interpret it as a radio protocol sentence
}

void RadioMessageText::Transmitted(ChatChannel channel)
{
}

RString RadioMessageText::GetWave()
{
  return _wave;
}

float RadioMessageText::GetDuration() const
{
  return _timeToLive;
}

#if _ENABLE_CONVERSATION

///////////////////////////////////////////////////////////////////////////////
// class RadioMessageTalk

RadioMessageTalk::RadioMessageTalk(const KBMessageInfo *message, AIBrain *sender, AIBrain *receiver, float ttl)
{
  _message = unconst_cast(message); // store as non-const reference because of serialization
  _sender = sender;
  _receiver = receiver;
  _timeToLive = ttl;

  if (_sender) _sender->ChangeSpeaking(+1);
  if (_receiver) _receiver->ChangeListening(+1);
}

RadioMessageTalk::~RadioMessageTalk()
{
  if (_sender) _sender->ChangeSpeaking(-1);
  if (_receiver) _receiver->ChangeListening(-1);
}

RString RadioMessageTalk::GetWave()
{
  return RString();
}

RString RadioMessageTalk::GetText()
{
  if (!_sender || !_sender->LSCanSpeak() || !_sender->GetKBCenter()) return RString();
  if (!_message) return RString();

  const KBCenter *parent = _sender->GetKBParent();
  return _sender->GetKBCenter()->GetText(_message, parent);
}

float RadioMessageTalk::GetDuration() const
{
  if (!_sender || !_sender->LSCanSpeak() || !_sender->GetKBCenter()) return 0;
  if (!_message) return 0;

  const KBCenter *parent = _sender->GetKBParent();
  RString text = _sender->GetKBCenter()->GetText(_message, parent);

  // when no text is shown, we need not wait (empty sentences used as some control messages in SecOp system)
  if (text.GetLength() == 0) return 0;
  return _timeToLive;
}

void RadioMessageTalk::GetSpeech(RadioSentence &sentence, const RadioChannelParams &cfg)
{
  if (!_sender || !_sender->LSCanSpeak() || !_sender->GetKBCenter()) return;
  if (!_message) return;
  
  AutoArray<RString> speech;
  const KBCenter *parent = _sender->GetKBParent();
  _sender->GetKBCenter()->GetSpeech(speech, _message, parent);
  int n = speech.Size();
  if (n == 0) return;

  for (int i=0; i<n; i++)
    sentence.Add(speech[i], cfg.pauseAfterWord);
}

void RadioMessageTalk::Transmitted(ChatChannel channel)
{
  if (!_sender || !_sender->LSCanSpeak() || !_sender->GetKBCenter()) return;
  if (!_receiver || !_receiver->LSIsAlive()) return;
  if (!_message) return;
  if (!_receiver->CanHear(channel, _sender)) return;
  
#if _ENABLE_IDENTITIES
  _sender->GetPerson()->GetIdentity().AddSaid(_receiver, _message);

  const KBCenter *kbParent = _sender->GetKBParent();
  RString message = _sender->GetKBCenter()->GetText(_message, kbParent);
  if (message.GetLength() > 0)
  {
    // as unit use the person we are speaking with
    // as target use the speaker
    _sender->GetPerson()->GetIdentity().AddDiaryRecord("Conversation", RString(), message, NULL, TSNone, _receiver, _sender, true);
    _receiver->GetPerson()->GetIdentity().AddDiaryRecord("Conversation", RString(), message, NULL, TSNone, _sender, _sender, true);
  }
#endif

  _receiver->KBReact(_sender, _message);
}

const char *RadioMessageTalk::GetPriorityClass()
{
  return "Design";
}

LSError RadioMessageTalk::Serialize(ParamArchive &ar)
{
  CHECK(RadioMessage::Serialize(ar))

  CHECK(ar.SerializeRef("sender", _sender, 1))
  CHECK(ar.SerializeRef("receiver", _receiver, 1))
  CHECK(ar.Serialize("ttl", _timeToLive, 1))

  KBSaveContext context(_sender, &GGameState);
  void *oldParams = ar.SetParams(&context);
  CHECK(ar.Serialize("message", _message, 1))
  ar.SetParams(oldParams);

  if (ar.IsLoading() && ar.GetPass() == ParamArchive::PassSecond)
  {
    if (_sender) _sender->ChangeSpeaking(+1);
    if (_receiver) _receiver->ChangeListening(+1);
  }

  return LSOK;
}

#endif // _ENABLE_CONVERSATION

#if _ENABLE_IDENTITIES

#if _ENABLE_INDEPENDENT_AGENTS

AIBrain *GetSpeaker(AITeamMember *member)
{
  if (!member) return NULL;
  AIAgent *agent = member->GetAgent();
  while (!agent)
  {
    AITeam *team = member->GetTeam();
    if (!team) return NULL;
    member = team->GetLeader();
    if (!member) return NULL;
    agent = member->GetAgent();
  }
  return agent;
}

#endif

LSError RadioMessageTask::Serialize(ParamArchive &ar)
{
  CHECK(RadioMessage::Serialize(ar))
  CHECK(ar.Serialize("Task", _task, 1))

  void *old = ar.GetParams();
  ar.SetParams(&GGameState);
  CHECK(ar.Serialize("Params", _params, 1))
  ar.SetParams(old);

  return LSOK;
}

const char *RadioMessageTask::GetPriorityClass()
{
  return "NormalCommand";
}

void RadioMessageTask::Transmitted(ChatChannel channel)
{
#if _ENABLE_INDEPENDENT_AGENTS
  if (!GetSender()) return;
  
  Assert(_task);
  AITeamMember *to = _task->GetOwner();
  if (!to) return;
  to->AddTask(_task);

  // send confirmation
  AIBrain *speaker = ::GetSpeaker(to);
  if (speaker && !speaker->IsAnyPlayer())
  {
    to->SendMsg(new RadioMessageTaskReceived(_task));
  }
#endif
}

AIBrain *RadioMessageTask::GetSender() const
{
#if _ENABLE_INDEPENDENT_AGENTS
  AITeamMember *from = _task->GetSender();
  return ::GetSpeaker(from);
#else
  return NULL;
#endif
}

RString RadioMessageTask::PrepareSentence(SentenceParams &params) const
{
#if _ENABLE_INDEPENDENT_AGENTS
  if (!GetSender()) return RString();

  Assert(_task);
  AITeamMember *to = _task->GetOwner();
  if (!to) return RString();

  params.Add(to);
  if (_params.GetType() == GameArray)
  {
    const GameArrayType &array = _params;
    for (int i=0; i<array.Size(); i++) params.Add(array[i], _task->GetSender(), to);
  }
  else
  {
    params.Add(_params, _task->GetSender(), to);
  }

  return _task->GetSentence("SendTask");
#else
  return RString();
#endif
}

LSError RadioMessageTaskReceived::Serialize(ParamArchive &ar)
{
  CHECK(RadioMessage::Serialize(ar))
  CHECK(ar.SerializeRef("Task", _task, 1))
  return LSOK;
}

const char *RadioMessageTaskReceived::GetPriorityClass()
{
  return "Confirmation";
}

void RadioMessageTaskReceived::Transmitted(ChatChannel channel)
{
  // nothing to do
}

AIBrain *RadioMessageTaskReceived::GetSender() const
{
#if _ENABLE_INDEPENDENT_AGENTS
  AITeamMember *from = _task->GetOwner();
  return ::GetSpeaker(from);
#else
  return NULL;
#endif
}

RString RadioMessageTaskReceived::PrepareSentence(SentenceParams &params) const
{
#if _ENABLE_INDEPENDENT_AGENTS
  if (!GetSender()) return RString();
  
  Assert(_task);
  AITeamMember *to = _task->GetSender();
  if (!to) return RString();

  params.Add(to);

  return _task->GetSentence("SendTaskReceived");
#else
  return RString();
#endif
}

LSError RadioMessageTaskResult::Serialize(ParamArchive &ar)
{
  CHECK(RadioMessage::Serialize(ar))
  CHECK(ar.SerializeRef("Task", _task, 1))
  CHECK(ar.SerializeEnum("state", _state, 1))

  void *old = ar.GetParams();
  ar.SetParams(&GGameState);
  CHECK(ar.Serialize("Result", _result, 1))
  ar.SetParams(old);

  CHECK(ar.Serialize("sentence", _sentence, 1))
  return LSOK;
}

const char *RadioMessageTaskResult::GetPriorityClass()
{
  return "Completition";
}

void RadioMessageTaskResult::Transmitted(ChatChannel channel)
{
#if _ENABLE_INDEPENDENT_AGENTS
  if (!GetSender()) return;
  Assert(_task);
  AITeamMember *to = _task->GetSender();
  if (!to) return;

  _task->SetState(_state);
  _task->SetResult(_result);
#endif
}

AIBrain *RadioMessageTaskResult::GetSender() const
{
#if _ENABLE_INDEPENDENT_AGENTS
  AITeamMember *from = _task->GetOwner();
  return ::GetSpeaker(from);
#else
  return NULL;
#endif
}

RString RadioMessageTaskResult::PrepareSentence(SentenceParams &params) const
{
#if _ENABLE_INDEPENDENT_AGENTS
  if (!GetSender()) return RString();
  Assert(_task);
  AITeamMember *to = _task->GetSender();
  if (!to) return RString();

  params.Add(to);
  if (_result.GetType() == GameArray)
  {
    const GameArrayType &array = _result;
    for (int i=0; i<array.Size(); i++) params.Add(array[i], _task->GetOwner(), to);
  }
  else
  {
    params.Add(_result, _task->GetOwner(), to);
  }

  return _task->GetSentence(_sentence);
#else
  return RString();
#endif
}

#endif // _ENABLE_IDENTITIES
