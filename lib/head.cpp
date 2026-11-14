// Operation FlashPoint - vehicle class
// (C) 2001, Bohemia Interactive Studio

#include "wpch.hpp"

#if !_ENABLE_NEWHEAD

#include "head.hpp"
#include "global.hpp"
#include <El/Common/randomGen.hpp>
#include <El/QStream/qbStream.hpp>
#include "paramFileExt.hpp"
#include <Es/Files/filenames.hpp>
#include "soundsys.hpp"
#include "vehicle.hpp"
#include "fileLocator.hpp"
#include "gameDirs.hpp"

// class ManLipInfo

bool ManLipInfo::AttachWaveReady(const char *wave)
{
  BString<256> buffer;
  strcpy(buffer, wave);
  strcpy(GetFileExt(GetFilenameExt(buffer)),".lip");
  if (!QFBankQueryFunctions::FileExists(buffer)) return true;
  QIFStreamB f;
  f.AutoOpen(buffer);
  return f.PreRead();
}

bool ManLipInfo::AttachWave(AbstractWave *wave, float freq)
{
  if (!wave) return false;
  char buffer[256];
  strcpy(buffer, wave->Name());
  strcpy(GetFileExt(GetFilenameExt(buffer)),".lip");
  if (!QFBankQueryFunctions::FileExists(buffer)) return false;

  // parse file
  QIFStreamB f;
  f.AutoOpen(buffer);
  while ( !f.eof() )
  {
    // read line
    f.readLine(buffer,sizeof(buffer));
    // process line
    float time;
    int phase;
    if (sscanf(buffer, "%f, %d", &time, &phase) == 2)
    {
      int index = _items.Add();
      _items[index].time = time;
      _items[index].phase = phase;
    }
    else
    {
      float frame;
      if (sscanf(buffer, "frame = %f", &frame) == 1)
      {
        _frame = frame;
        _invFrame = 1.0 / frame;
      }
    }
  }

  _current = 0;
  _freq = freq;
  _time = 0;
  return true;
}

void ManLipInfo::Simulate(float deltaT)
{
  _time += deltaT;
}

bool ManLipInfo::GetPhase(int &phase)
{
  float offset = _freq * _time;
  while(
    _current < _items.Size() &&
    _items[_current].time < offset
  ) _current++;
  if (_current > _items.Size()) return false;
  if (_current == 0)
    phase = -1;
  else
    phase = _items[_current - 1].phase;
  return true;
}

float ManLipInfo::GetPhase()
{
  float offset = _freq * _time;

  float floor = fastFloor(offset * _invFrame) * _frame;

  // find initial value for the frame
  while(
    _current < _items.Size() &&
    _items[_current].time < floor
  ) _current++;
  // if initial value is the end, we return end marker
  if (_current >= _items.Size()) return -1;

  float oldPhase = 0;
  if (_current > 0) oldPhase = _items[_current - 1].phase;

  // find final value for the frame
  int c = _current;
  while(
    c < _items.Size() &&
    _items[c].time < floor + _frame
  ) c++;
  // c may be the same as _current - the frame may reuse previous value

  float newPhase = 0;
  if (c > 0 && c < _items.Size()) newPhase = _items[c - 1].phase;
  
  // end value should be handled by termination
  // we interpolate to 0 instead
  if (newPhase<0) newPhase = 0;
  
  float dif = offset - floor; // range 0.._frame
  return (1.0 / 7) * _invFrame * (dif * newPhase + (_frame - dif) * oldPhase);
}

// class HeadTypeOld

HeadTypeOld::HeadTypeOld()
{
}

void HeadTypeOld::Load(ParamEntryPar cfg)
{
  _bone = cfg >> "boneHead";
  RStringB microName = cfg >> "microMimics";
  ParamEntryVal micro = Pars >> "CfgMimics" >> microName;
  _lBrowRandom.Load(micro>>"lBrow");
  _mBrowRandom.Load(micro>>"mBrow");
  _rBrowRandom.Load(micro>>"rBrow");
  _lMouthRandom.Load(micro>>"lMouth");
  _mMouthRandom.Load(micro>>"mMouth");
  _rMouthRandom.Load(micro>>"rMouth");
}

void HeadTypeOld::InitShape(ParamEntryPar cfg, LODShape *shape)
{
  _personality.Init(shape,(RString)(cfg>>"selectionPersonality"),NULL);

  _glasses.Init(shape,(RString)(cfg>>"selectionGlasses"),NULL);

  _lBrow.Init(shape,(RString)(cfg>>"selectionLBrow"),NULL);
  _mBrow.Init(shape,(RString)(cfg>>"selectionMBrow"),NULL);
  _rBrow.Init(shape,(RString)(cfg>>"selectionRBrow"),NULL);
  _lMouth.Init(shape,(RString)(cfg>>"selectionLMouth"),NULL);
  _mMouth.Init(shape,(RString)(cfg>>"selectionMMouth"),NULL);
  _rMouth.Init(shape,(RString)(cfg>>"selectionRMouth"),NULL);
  _eyelid.Init(shape,(RString)(cfg>>"selectionEyelid"),NULL);
  _lip.Init(shape,(RString)(cfg>>"selectionLip"),NULL);
}

void HeadTypeOld::InitShapeLevel(LODShape *shape, int level)
{
  _personality.InitLevel(shape,level);

  // get some texture from personality
  // note: there should be only one personality texture
  if (!_textureOrig) _textureOrig = _personality.GetTexture(shape);

  _glasses.InitLevel(shape,level);
  _lBrow.InitLevel(shape,level);
  _mBrow.InitLevel(shape,level);
  _rBrow.InitLevel(shape,level);
  _lMouth.InitLevel(shape,level);
  _mMouth.InitLevel(shape,level);
  _rMouth.InitLevel(shape,level);
  _eyelid.InitLevel(shape,level);
  _lip.InitLevel(shape,level);
}
void HeadTypeOld::DeinitShapeLevel(LODShape *shape, int level)
{
  _personality.DeinitLevel(shape,level);
  _glasses.DeinitLevel(shape,level);
  _lBrow.DeinitLevel(shape,level);
  _mBrow.DeinitLevel(shape,level);
  _rBrow.DeinitLevel(shape,level);
  _lMouth.DeinitLevel(shape,level);
  _mMouth.DeinitLevel(shape,level);
  _rMouth.DeinitLevel(shape,level);
  _eyelid.DeinitLevel(shape,level);
  _lip.DeinitLevel(shape,level);
}

// class HeadOld

HeadOld::HeadOld(const HeadTypeOld &type, LODShape *lShape, RString faceType)
{
  _winkPhase = 2;
  _forceWinkPhase = -1;
  _nextWink = 0;  // recalculate now

  _mimicMode = 0;

  _mimicPhase = 0;
  _nextMimicTime = 1e10;

  _lBrow = VZero;
  _mBrow = VZero;
  _rBrow = VZero;
  _lMouth = VZero;
  _mMouth = VZero;
  _rMouth = VZero;
  _lBrowOld = VZero;
  _mBrowOld = VZero;
  _rBrowOld = VZero;
  _lMouthOld = VZero;
  _mMouthOld = VZero;
  _rMouthOld = VZero;

  _rBrowRandom.SetWanted(VZero,0);
  _mBrowRandom.SetWanted(VZero,0);
  _lBrowRandom.SetWanted(VZero,0);

  _rMouthRandom.SetWanted(VZero,0);
  _mMouthRandom.SetWanted(VZero,0);
  _lMouthRandom.SetWanted(VZero,0);

  SetFace(type, faceType, lShape, "Default");
  SetGlasses(type, lShape, "None");
  SetMimic("Default");

  _randomLip = false;
  _actualRandomLip = 0;
  _wantedRandomLip = 0;
}

void HeadOld::AttachWave(AbstractWave *wave, float freq)
{
  _lipInfo = new ManLipInfo();
  if (!_lipInfo->AttachWave(wave, freq)) _lipInfo = NULL;
}

bool HeadOld::AttachWaveReady(const char *wave)
{
  return ManLipInfo::AttachWaveReady(wave);
}

void HeadOld::NextRandomLip()
{
  _wantedRandomLip = GRandGen.RandomValue();
  float time = 0.05 + 0.1 * GRandGen.RandomValue();
  _nextChangeRandomLip = time;
  _speedRandomLip = 1.0 / time;
}

void HeadOld::SetRandomLip(bool set)
{
  if (set && !_randomLip)
  {
    _actualRandomLip = 0;
    NextRandomLip();
  }
  _randomLip = set;
}

void HeadOld::Animate(
  const HeadTypeOld &type, AnimationContext &animContext, LODShape *lShape, int level, bool isDead, Matrix3Par trans,
  bool hiddenHead
)
{
  ShapeUsed shape = lShape->Level(level);

  // face
  type._personality.SetTexture(animContext, lShape, level, _texture);

  // glasses
  if (_glasses && !hiddenHead)
  {
    type._glasses.Unhide(animContext, lShape, level);
    type._glasses.SetTexture(animContext, lShape, level, _glasses);
  }
  else
    type._glasses.Hide(animContext, lShape, level);
}

void HeadOld::Deanimate(
  const HeadTypeOld &type, LODShape *lShape, int level, bool isDead, Matrix3Par trans,
  bool hiddenHead
)
{
}

void HeadOld::PrepareShapeDrawMatrices(const HeadTypeOld &type, Matrix4Array &matrices, const LODShape *lShape, int level, bool isDead, Matrix4Par headMatrix)
{

  // Get the shape and make sure it already exists
  //LODShape *lShape = obj->GetShape();
  DoAssert(lShape->IsLevelLocked(level));
  const Shape *shape = lShape->GetLevelLocked(level);

  // Get bone sets
  const SubSkeletonIndexSet& lBrowBone = shape->GetSubSkeletonIndexFromSkeletonIndex(type._lBrow.GetBone(level));
  const SubSkeletonIndexSet& mBrowBone = shape->GetSubSkeletonIndexFromSkeletonIndex(type._mBrow.GetBone(level));
  const SubSkeletonIndexSet& rBrowBone = shape->GetSubSkeletonIndexFromSkeletonIndex(type._rBrow.GetBone(level));
  const SubSkeletonIndexSet& lMouthBone = shape->GetSubSkeletonIndexFromSkeletonIndex(type._lMouth.GetBone(level));
  const SubSkeletonIndexSet& mMouthBone = shape->GetSubSkeletonIndexFromSkeletonIndex(type._mMouth.GetBone(level));
  const SubSkeletonIndexSet& rMouthBone = shape->GetSubSkeletonIndexFromSkeletonIndex(type._rMouth.GetBone(level));
  const SubSkeletonIndexSet& eyeLidBone = shape->GetSubSkeletonIndexFromSkeletonIndex(type._eyelid.GetBone(level));
  const SubSkeletonIndexSet& lipBone = shape->GetSubSkeletonIndexFromSkeletonIndex(type._lip.GetBone(level));

  // Get the maximum bone set size
  int maxBoneSetSize = 0;
  saturateMax(maxBoneSetSize, lBrowBone.Size());
  saturateMax(maxBoneSetSize, mBrowBone.Size());
  saturateMax(maxBoneSetSize, rBrowBone.Size());
  saturateMax(maxBoneSetSize, lMouthBone.Size());
  saturateMax(maxBoneSetSize, mMouthBone.Size());
  saturateMax(maxBoneSetSize, rMouthBone.Size());
  saturateMax(maxBoneSetSize, eyeLidBone.Size());
  saturateMax(maxBoneSetSize, lipBone.Size());

  // No new bones, exit
  if (maxBoneSetSize == 0) return;

  // Fill out all head bones with head matrix
  if (lBrowBone.Size() > 0) matrices[GetSubSkeletonIndex(lBrowBone[0])] = headMatrix;
  if (mBrowBone.Size() > 0) matrices[GetSubSkeletonIndex(mBrowBone[0])] = headMatrix;
  if (rBrowBone.Size() > 0) matrices[GetSubSkeletonIndex(rBrowBone[0])] = headMatrix;
  if (lMouthBone.Size() > 0) matrices[GetSubSkeletonIndex(lMouthBone[0])] = headMatrix;
  if (mMouthBone.Size() > 0) matrices[GetSubSkeletonIndex(mMouthBone[0])] = headMatrix;
  if (rMouthBone.Size() > 0) matrices[GetSubSkeletonIndex(rMouthBone[0])] = headMatrix;
  if (eyeLidBone.Size() > 0) matrices[GetSubSkeletonIndex(eyeLidBone[0])] = headMatrix;
  if (lipBone.Size() > 0) matrices[GetSubSkeletonIndex(lipBone[0])] = headMatrix;

  // Perform matrix movements
  if (!isDead)
  {
    // Winking
    float winkPhase = 0;
    if (_forceWinkPhase >= 0)
    {
      winkPhase = 2 * _forceWinkPhase;
    }
    else
    {
      winkPhase = 2.0 * _winkPhase;
    }
    if (winkPhase > 1) winkPhase = 2.0 - winkPhase;
    if (eyeLidBone.Size() > 0)
    {
      Matrix4 translationMatrix;
      translationMatrix.SetIdentity();
      translationMatrix.SetPosition(winkPhase * -0.015f * VUp * 2.0f);
      matrices[GetSubSkeletonIndex(eyeLidBone[0])] = headMatrix * translationMatrix;
    }

    // Lip moving
    if (_lipInfo)
    {
      float lipPhase = _lipInfo->GetPhase();
      if (lipPhase > -0.1)
      {
        if (lipBone.Size() > 0)
        {
          Matrix4 translationMatrix;
          translationMatrix.SetIdentity();
          translationMatrix.SetPosition(lipPhase * -0.02f * VUp * 2.0f);
          matrices[GetSubSkeletonIndex(lipBone[0])] = headMatrix * translationMatrix;
        }
        if (lMouthBone.Size() > 0)
        {
          Matrix4 translationMatrix;
          translationMatrix.SetIdentity();
          translationMatrix.SetPosition(lipPhase * -0.01f * VUp * 2.0f);
          matrices[GetSubSkeletonIndex(lMouthBone[0])] = headMatrix * translationMatrix;
        }
        if (rMouthBone.Size() > 0)
        {
          Matrix4 translationMatrix;
          translationMatrix.SetIdentity();
          translationMatrix.SetPosition(lipPhase * -0.01f * VUp * 2.0f);
          matrices[GetSubSkeletonIndex(rMouthBone[0])] = headMatrix * translationMatrix;
        }
      }
      else
      {
        _lipInfo = NULL;
      }
    }
    else if (_randomLip)
    {
      if (lipBone.Size() > 0)
      {
        Matrix4 translationMatrix;
        translationMatrix.SetIdentity();
        translationMatrix.SetPosition(_actualRandomLip * -0.02f * VUp * 2.0f);
        matrices[GetSubSkeletonIndex(lipBone[0])] = headMatrix * translationMatrix;
      }
      if (lMouthBone.Size() > 0)
      {
        Matrix4 translationMatrix;
        translationMatrix.SetIdentity();
        translationMatrix.SetPosition(_actualRandomLip * -0.01f * VUp * 2.0f);
        matrices[GetSubSkeletonIndex(lMouthBone[0])] = headMatrix * translationMatrix;
      }
      if (rMouthBone.Size() > 0)
      {
        Matrix4 translationMatrix;
        translationMatrix.SetIdentity();
        translationMatrix.SetPosition(_actualRandomLip * -0.01f * VUp * 2.0f);
        matrices[GetSubSkeletonIndex(rMouthBone[0])] = headMatrix * translationMatrix;
      }
    }
  }

  // Mimic
  float coefNew = _mimicPhase;
  float coefOld = 1.0f - _mimicPhase;
  const float coefRand = +1;

  if (rBrowBone.Size() > 0)
  {
    Matrix4 translationMatrix;
    translationMatrix.SetIdentity();
    translationMatrix.SetPosition((coefOld * _rBrowOld + coefNew * _rBrow + coefRand * _rBrowRandom) * 2.0f);
    matrices[GetSubSkeletonIndex(rBrowBone[0])] = headMatrix * translationMatrix;
  }

  if (mBrowBone.Size() > 0)
  {
    Matrix4 translationMatrix;
    translationMatrix.SetIdentity();
    translationMatrix.SetPosition((coefOld * _mBrowOld + coefNew * _mBrow + coefRand * _mBrowRandom) * 2.0f);
    matrices[GetSubSkeletonIndex(mBrowBone[0])] = headMatrix * translationMatrix;
  }

  if (lBrowBone.Size() > 0)
  {
    Matrix4 translationMatrix;
    translationMatrix.SetIdentity();
    translationMatrix.SetPosition((coefOld * _lBrowOld + coefNew * _lBrow + coefRand * _lBrowRandom) * 2.0f);
    matrices[GetSubSkeletonIndex(lBrowBone[0])] = headMatrix * translationMatrix;
  }

  if (rMouthBone.Size() > 0)
  {
    Matrix4 translationMatrix;
    translationMatrix.SetIdentity();
    translationMatrix.SetPosition((coefOld * _rMouthOld + coefNew * _rMouth + coefRand * _rMouthRandom) * 2.0f);
    matrices[GetSubSkeletonIndex(rMouthBone[0])] = headMatrix * translationMatrix;
  }

  if (mMouthBone.Size() > 0)
  {
    Matrix4 translationMatrix;
    translationMatrix.SetIdentity();
    translationMatrix.SetPosition((coefOld * _mMouthOld + coefNew * _mMouth + coefRand * _mMouthRandom) * 2.0f);
    matrices[GetSubSkeletonIndex(mMouthBone[0])] = headMatrix * translationMatrix;
  }

  if (lMouthBone.Size() > 0)
  {
    Matrix4 translationMatrix;
    translationMatrix.SetIdentity();
    translationMatrix.SetPosition((coefOld * _lMouthOld + coefNew * _lMouth + coefRand * _lMouthRandom) * 2.0f);
    matrices[GetSubSkeletonIndex(lMouthBone[0])] = headMatrix * translationMatrix;
  }

  // Replicating
  REPLICATE_FIRST_BONE(matrices, lBrowBone);
  REPLICATE_FIRST_BONE(matrices, mBrowBone);
  REPLICATE_FIRST_BONE(matrices, rBrowBone);
  REPLICATE_FIRST_BONE(matrices, lMouthBone);
  REPLICATE_FIRST_BONE(matrices, mMouthBone);
  REPLICATE_FIRST_BONE(matrices, rMouthBone);
  REPLICATE_FIRST_BONE(matrices, eyeLidBone);
  REPLICATE_FIRST_BONE(matrices, lipBone);
}

void RandomVector3Type::Load(ParamEntryPar cfg)
{
  rng = Vector3(cfg[0],cfg[1],cfg[2]);
  minT = cfg[3];
  maxT = cfg[4];
}


RandomVector3::RandomVector3()
{
  _spd = VZero;
  _cur = VZero;
  _timeToWanted = 0;
}

void RandomVector3::SetWanted(Vector3Par wanted, float time)
{
  /*
  LogF
  (
    "SetW %.4f,%.4f,%.4f,  %.4f",
    wanted[0],wanted[1],wanted[2],time
  );
  */
  if (time<=0)
  {
    _spd = VZero;
    _cur = wanted;
    _timeToWanted = 0;
  }
  else
  {
    _spd = (wanted-_cur)/time;
    _timeToWanted = time;
  }
}

bool RandomVector3::Simulate(float deltaT)
{
  if (_timeToWanted<=0) return true;
  float dt = deltaT;
  saturateMin(dt,_timeToWanted);
  _timeToWanted -= deltaT;
  _cur += _spd*dt;
  /*
  LogF
  (
    "cur %.4f,%.4f,%.4f,  spd %.4f,%.4f,%.4f,  %.4f",
    _cur[0],_cur[1],_cur[2],
    _spd[0],_spd[1],_spd[2],dt
  );
  */

  return false;
}

void RandomVector3::SetRandomTgt(Vector3Par rng, float minT, float maxT)
{
  float x = GRandGen.RandomValue()*rng.X()*2-rng.X();
  float y = GRandGen.RandomValue()*rng.Y()*2-rng.Y();
  float z = GRandGen.RandomValue()*rng.Z()*2-rng.Z();
  float t = GRandGen.RandomValue()*(maxT-minT)+minT;
  SetWanted(Vector3(x,y,z),t);
}

bool RandomVector3::SimulateAndSetRandomTgt
(
  float deltaT,
  Vector3Par rng, float minT, float maxT
)
{
  if (Simulate(deltaT))
  {
    SetRandomTgt(rng,minT,maxT);
    return true;
  }
  return false;
}

bool RandomVector3::SimulateAndSetRandomTgt
(
  float deltaT, const RandomVector3Type &type
)
{
  if (Simulate(deltaT))
  {
    SetRandomTgt(type.rng,type.minT,type.maxT);
    return true;
  }
  return false;
}



const float MimicPeriod = 5;

void HeadOld::Simulate(const HeadTypeOld &type, float deltaT, SimulationImportance prec, bool dead)
{
  _nextWink -= deltaT;
  if (_nextWink <= 0)
  {
    const float winkSpeed = 8.0;
    _winkPhase += winkSpeed * deltaT;
    if (_winkPhase >= 1)
    {
      _winkPhase = 0;
      _nextWink = 1.0f + 5.0f * GRandGen.RandomValue();
    }
  }

  const float mimicSpeed = 2.0;
  _mimicPhase += mimicSpeed * deltaT;
  saturateMin(_mimicPhase, 1.0f);

  if (_mimicPhase>=1)
  {
    // change mimic when in some mimic mode
    if (_forceMimic.GetLength()>0)
    {
    }
    else if (_mimicMode)
    {
      _nextMimicTime -= deltaT;
      if (_nextMimicTime<0)
      {
        // select random mimic
        float rand = GRandGen.RandomValue();
        RStringB name = (*_mimicMode)[_mimicMode->GetSize()-1];
        for (int i=0; i<_mimicMode->GetSize()-1; i+=2)
        {
          float prop = (*_mimicMode)[i+1];
          rand -= prop;
          if (rand<=0)
          {
            name = (*_mimicMode)[i];
            break;
          }
        }
        //LogF("Set mimic %s",(const char *)name);
        SetMimic(name);
        _nextMimicTime = MimicPeriod;
      }
    }

  }

  if (_randomLip)
  {
    float diff = _wantedRandomLip - _actualRandomLip;
    saturate(diff, -_speedRandomLip * deltaT, _speedRandomLip * deltaT);
    _actualRandomLip += diff;
    
    _nextChangeRandomLip -= deltaT;
    if (_nextChangeRandomLip <= 0) NextRandomLip();
  }
  else
  {
    _actualRandomLip = 0;
  }

  if (_lipInfo) _lipInfo->Simulate(deltaT);

  if (prec<=SimulateVisibleNear)
  {
    if (!dead)
    {
      _lBrowRandom.SimulateAndSetRandomTgt(deltaT,type._lBrowRandom);
      _mBrowRandom.SimulateAndSetRandomTgt(deltaT,type._mBrowRandom);
      _rBrowRandom.SimulateAndSetRandomTgt(deltaT,type._rBrowRandom);
      _lMouthRandom.SimulateAndSetRandomTgt(deltaT,type._lMouthRandom);
      _mMouthRandom.SimulateAndSetRandomTgt(deltaT,type._mMouthRandom);
      _rMouthRandom.SimulateAndSetRandomTgt(deltaT,type._rMouthRandom);
    }
    else
    {
      _lBrowRandom.SetWanted(VZero,0);
      _mBrowRandom.SetWanted(VZero,0);
      _rBrowRandom.SetWanted(VZero,0);
      _lMouthRandom.SetWanted(VZero,0);
      _mMouthRandom.SetWanted(VZero,0);
      _rMouthRandom.SetWanted(VZero,0);
    }
  }

  //LogF("_lBrowRandom %.4f,%.4f,%.4f,",_lBrowRandom.X(),_lBrowRandom.Y(),_lBrowRandom.Z());
  //LogF("_mBrowRandom %.4f,%.4f,%.4f,",_mBrowRandom.X(),_mBrowRandom.Y(),_mBrowRandom.Z());
  //LogF("_rBrowRandom %.4f,%.4f,%.4f,",_rBrowRandom.X(),_rBrowRandom.Y(),_rBrowRandom.Z());
}

/*!
\patch 1.43 Date 1/30/2002 by Ondra
- Fixed: Random crash when custom face texture file was corrupted.
*/

void HeadOld::SetFace(const HeadTypeOld &type, RString faceType, LODShape *lShape, RString name, RString player)
{
  ParamEntryVal cfg = Pars >> "CfgFaces" >> faceType;
  ConstParamEntryPtr cls = cfg.FindEntry(name);
  if (!cls)
  {
    RptF("SetFace error: class CfgFaces.%s.%s not found", cc_cast(faceType), cc_cast(name));
    cls = cfg.FindEntry("Default");
    if (!cls)
    {
      RptF("SetFace error: class CfgFaces.%s.Default not found", cc_cast(faceType));
      return;
    }
  }

  RString textName = GetPictureName(*cls >> "texture");
  Ref<Texture> text = GlobLoadTexture(textName);
  if (text)
    _texture = text;
  else
  {
    RptF("SetFace error: texture %s not found", cc_cast(textName));
  }

  if (stricmp(cls->GetName(), "custom") == 0)
  {
    RString GetUserDirectory();
    RString GetClientCustomFilesDir();
    
    RString dir;
    if (player.GetLength() > 0)
      dir = GetClientCustomFilesDir() + RString("\\players\\") + EncodeFileName(player) + RString("\\");
    else
      dir = GetUserDirectory();
    dir.Lower();
    RString name = dir + RString("face.paa");
    if (QIFileFunctions::FileExists(name))
    {
      Ref<Texture> text = GlobLoadTexture(name);
      if (text)
        _texture = text;
      else
      {
        RptF("SetFace error: cannot load custom face texture %s", cc_cast(name));
      }
    }
    else
    {
      name = dir + RString("face.jpg");
      if (QIFileFunctions::FileExists(name))
      {
        Ref<Texture> text = GlobLoadTexture(name);
        if (text)
          _texture = text;
        else
        {
          RptF("SetFace error: cannot load custom face texture %s", cc_cast(name));
        }
      }
      else
      {
        RptF("SetFace error: custom face texture not found in %s", cc_cast(dir));
      }
    }
  }

  // check if there is some wounded texture associated
  if (_texture)
  {
    ParamEntryVal cfg = Pars>>"CfgFaceWounds">>"wounds";
    for (int i=0; i<cfg.GetSize()-1; i+=2)
    {
      RStringB origName = cfg[i];
      RStringB woundName = cfg[i+1];
      // check if origName is name of currently set texture

      RStringB origTexName = GetDefaultName(origName, ".pac");
      if (strcmp(origTexName,_texture->GetName())) continue;
      RStringB woundTexName = GetDefaultName(woundName, ".pac");
      // TODO: 3-step wounds
      _textureWounded[0] = GlobLoadTexture(woundTexName);
      _textureWounded[1] = GlobLoadTexture(woundTexName);
    }
  }
}

void HeadOld::SetGlasses(const HeadTypeOld &type, LODShape *lShape, RString name)
{
  ConstParamEntryPtr cls = (Pars >> "CfgGlasses").FindEntry(name);
  if (!cls)
  {
    _glasses = NULL;
    return;
  }

  RString textName = GetPictureName(*cls >> "texture");
  if (textName.GetLength() == 0)
  {
    _glasses = NULL;
    return;
  }

  Ref<Texture> text = GlobLoadTexture(textName);
  if (text)
    _glasses = text;
  else
    WarningMessage("Cannot find texture: %s", (const char *)textName);

}

void HeadOld::SetMimicMode(RStringB modeName)
{
  if (modeName.GetLength()>0)
  {
    // was ParamEntryVal par = Pars >> "CfgMimics" >> modeName;
    // the way below is more efficient, as it avoid string comparison
    // in case of equality
    // TODO: use mimic mode handles instead
    if (!_mimicMode || _mimicMode->GetName()!=modeName)
    {
      ParamEntryVal par = Pars >> "CfgMimics" >> modeName;
      _mimicMode = &par;
      _nextMimicTime = 0;
    }
  }
  else
  {
    _mimicMode = NULL;
  }
}

RStringB HeadOld::GetMimicMode() const
{
  if (!_mimicMode) return RStringBEmpty;
  return _mimicMode->GetName();
}

void HeadOld::SetForceMimic(RStringB name)
{
  _forceMimic = name;
  // when setting to "", the current mimic will be replaced soon based on the mimic mode
  if (name.GetLength()>0)
  {
    SetMimic(name);
  }
}

/*!
\patch_internal 1.03 Date 07/13/2001 by Jirka
  - warning message removed
  - avoid WarningMessage in released campaign
*/
void HeadOld::SetMimic(RStringB name)
{
  ConstParamEntryPtr cls = (Pars >> "CfgMimics" >> "States").FindEntry(name);
  if (!cls)
  {
    // WarningMessage("Unknown mimic: %s", (const char *)name);
    ErrF("Unknown mimic: %s", (const char *)name);
    return;
  }

  // set current mimic as old
  float coefNew = _mimicPhase;
  float coefOld = 1.0f - _mimicPhase;
  _lBrowOld = coefOld * _lBrowOld + coefNew * _lBrow;
  _mBrowOld = coefOld * _mBrowOld + coefNew * _mBrow;
  _rBrowOld = coefOld * _rBrowOld + coefNew * _rBrow;
  _lMouthOld = coefOld * _lMouthOld + coefNew * _lMouth;
  _mMouthOld = coefOld * _mMouthOld + coefNew * _mMouth;
  _rMouthOld = coefOld * _rMouthOld + coefNew * _rMouth;
  _mimicPhase = 0;

  // load new mimic
  const float coef = 0.01;
  _lBrow[0] = (*cls >> "lBrow")[0]; _lBrow[0] *= coef;
  _lBrow[1] = (*cls >> "lBrow")[1]; _lBrow[1] *= coef;
  _mBrow[0] = (*cls >> "mBrow")[0]; _mBrow[0] *= coef;
  _mBrow[1] = (*cls >> "mBrow")[1]; _mBrow[1] *= coef;
  _rBrow[0] = (*cls >> "rBrow")[0]; _rBrow[0] *= coef;
  _rBrow[1] = (*cls >> "rBrow")[1]; _rBrow[1] *= coef;
  _lMouth[0] = (*cls >> "lMouth")[0]; _lMouth[0] *= coef;
  _lMouth[1] = (*cls >> "lMouth")[1]; _lMouth[1] *= coef;
  _mMouth[0] = (*cls >> "mMouth")[0]; _mMouth[0] *= coef;
  _mMouth[1] = (*cls >> "mMouth")[1]; _mMouth[1] *= coef;
  _rMouth[0] = (*cls >> "rMouth")[0]; _rMouth[0] *= coef;
  _rMouth[1] = (*cls >> "rMouth")[1]; _rMouth[1] *= coef;
}

#endif
