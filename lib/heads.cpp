#include "wpch.hpp"

#if _ENABLE_NEWHEAD

#if _PROFILE && USER_Ondra
# pragma optimize( "", off )
#endif

#include <El/Common/randomGen.hpp>
#include <Es/Files/filenames.hpp>
#include "fileLocator.hpp"
#include "object.hpp"
#include "Shape/specLods.hpp"

#include "soldierOld.hpp"

#include "Network/network.hpp"
#include "heads.hpp"

bool GrimaceRTM::GetBone(Matrix4 &m, const SkeletonIndex &si,  Head &head, const HeadType *headType, Matrix4Par headOrigin, Vector3Par eyeDirModel, float eyeAccom) const
{
  Assert(_rtm);
  m = _rtm->GetMatrix(head.GetGrimacePhase(), si);
  return true;
}

GrimaceCompound::GrimaceCompound(RStringB name, const ParamEntryVal &grimace, const Skeleton *skeleton, const RefArray<Grimace> &currentGrimaces) : Grimace(name)
{
  int skeletonBones = skeleton->NBones();

  // Go through all the grimace parts
  const ParamEntryVal &items = grimace >> "Items";
  int grimacePartCount = items.GetEntryCount();
  _selections.Realloc(grimacePartCount);
  for (int j = 0; j < grimacePartCount; j++)
  {
    const ParamEntryVal &grimacePart = items.GetEntry(j);

    // Find the source grimace
    RString grimacePartName = grimacePart.GetName();
    grimacePartName.Lower();
    int currentGrimacesSize = currentGrimaces.Size();
    int sourceGrimaceIndex;
    for (sourceGrimaceIndex = 0; sourceGrimaceIndex < currentGrimacesSize; sourceGrimaceIndex++)
    {
      if (currentGrimaces[sourceGrimaceIndex]->Name() == grimacePartName) break;
    }

    // If found then continue
    if (sourceGrimaceIndex < currentGrimacesSize)
    {

      // Read the mask
      ConstParamEntryPtr mask = grimacePart.FindEntry("mask");
      if (mask && mask->IsArray())
      {
        
        // Add the new item
        GrimaceSelection &gs = _selections.Append();
        gs._grimace = currentGrimaces[sourceGrimaceIndex];

        int maskItems = mask->GetSize() / 2;
        if ((maskItems == 1) && ((*mask)[0].operator RStringB() == RStringB("*")))
        { // Mask contains all items
          gs._weights.Realloc(skeletonBones);
          gs._weights.Resize(skeletonBones);
          gs._boneMap.Realloc(skeletonBones);
          gs._boneMap.Resize(skeletonBones);
          float factor = (float)(*mask)[1];
          for (int i = 0; i < skeletonBones; i++)
          {
            gs._weights[i]._weight = factor;
            gs._weights[i]._bone = SetSkeletonIndex(i);
            gs._boneMap[i] = i;
          }
        }
        else
        { // Mask is selection of some items
          gs._boneMap.Realloc(skeletonBones);
          gs._boneMap.Resize(skeletonBones);
          for (int i=0; i<skeletonBones; i++) gs._boneMap[i] = -1;
          // Go through all the selection bones
          for (int i = 0; i < maskItems; i++)
          {
            RString bone = (*mask)[i * 2 + 0];
            float factor = (*mask)[i * 2 + 1];
            bone.Lower();

            // Find the bone in the skeleton
            SkeletonIndex si = skeleton->FindBone(bone);
            if (SkeletonIndexValid(si) && factor>0.001f)
            {
              gs._boneMap[GetSkeletonIndex(si)] = gs._weights.Size();
              GrimaceWeight &w = gs._weights.Append();
              w._bone = si;
              w._weight = factor;
            }
          }
        }
      }
    }
  }

  // Compact the selection array
  _selections.Compact();
}

bool GrimaceCompound::GetBone(Matrix4 &m, const SkeletonIndex &si,  Head &head, const HeadType *headType, Matrix4Par headOrigin, Vector3Par eyeDirModel, float eyeAccom) const
{
  bool notIdentity = false;
  m = MIdentity;
  for (int i = 0; i < _selections.Size(); i++)
  {
    const GrimaceSelection &gs = _selections[i];
    // check if the bone is present in the selection, and with what weight
    int index = gs._boneMap[GetSkeletonIndex(si)];
    if (index>=0)
    {
      float factor = gs._weights[index]._weight;
      Assert(gs._weights[index]._bone==si);
      if (factor>0.999f)
      {
        if (gs._grimace->GetBone(m, si, head, headType, headOrigin, eyeDirModel, eyeAccom))
        {
          notIdentity = true;
        }
        else
        {
          notIdentity = false;
        }
      }
      else
      {
        Matrix4 gMatrix;
        if (gs._grimace->GetBone(gMatrix, si, head, headType, headOrigin, eyeDirModel, eyeAccom))
        {
          m = gMatrix * factor + m * (1.0f - factor);
          notIdentity = true;
        }
      }
    }
  }
  return notIdentity;
}

GrimaceEyes::GrimaceEyes(RStringB name, const ParamEntryVal &grimace, const Skeleton *skeleton) : Grimace(name)
{

  _eyeMaxAngle = grimace >> "eyeMaxAngle";
  _eyelidUpStartAngle = grimace >> "eyelidUpStartAngle";
  _eyelidUpMaxAngle = grimace >> "eyelidUpMaxAngle";
  _eyelidDownStartAngle = grimace >> "eyelidDownStartAngle";
  _eyelidDownMaxAngle = grimace >> "eyelidDownMaxAngle";
}



void GrimaceEyes::GetEyeBone(Matrix4 &m, Vector3Par eyePos, Matrix4Par headOrigin, Vector3Par eyeDirModel) const
  {
  // Create the eye orientation matrix in the head space
  Vector3 eyeDirHead = headOrigin.Orientation().InverseGeneral() * eyeDirModel;
  Matrix4 eyeOrientHead(MDirection, eyeDirHead, VUp);
  // Translate eye to zero, rotate, translate back and transform with the head
  m = /*headOrigin **/ Matrix4(MTranslation,eyePos) * eyeOrientHead * Matrix4(MTranslation,-eyePos);
  }
void GrimaceEyes::GetPupilBone(Matrix4 &m, Vector3Par eyePos, Vector3Par pupilPos, Matrix4Par headOrigin, Vector3Par eyeDirModel,const float eyeAccom) const
{
  // Create the eye orientation matrix in the head space
  Vector3 eyeDirHead = headOrigin.Orientation().InverseGeneral() * eyeDirModel;
  Matrix4 eyeOrientHead(MDirection, eyeDirHead, VUp);
  // Translate eye to zero, rotate, translate back and transform with the head
  Matrix4 eyeOrigin = /*headOrigin **/ Matrix4(MTranslation,eyePos) * eyeOrientHead * Matrix4(MTranslation,-eyePos);

  float eyePupilFactor;
  {
    // Get the accommodation factor
    // assume logarithmic adaptation?
    // eye tries to keep the amount of the light constant
    // this is controlled by pupil area
    const float k = 30.0f; // empirical magic constant
    eyePupilFactor = sqrt(eyeAccom) * k;
    saturate(eyePupilFactor,0.2f,1.0f);
  }
  m = eyeOrigin * Matrix4(MTranslation, pupilPos) * Matrix4(MScale, eyePupilFactor) * Matrix4(MTranslation, -pupilPos);
}

void GrimaceEyes::GetLidBone(Matrix4 &m, Vector3Par eyePos, Matrix4Par headOrigin, Vector3Par eyeDirModel, const float startAngle, const float maxAngle, const float winkPhase, const int downOrUP) const
{
  // Convert the eye direction into the head space (before skinning - for it is easy to translate to eyePos)
  Vector3 eyeDirHead = headOrigin.Orientation().InverseGeneral() * eyeDirModel;

  // Calculate the final eye direction - limit the eye direction by some angle
  {
    // Vector to rotate the eye direction by
    Vector3 rotVector = VForward.CrossProduct(eyeDirHead);

    // Continue only in case the vector is not zero
    if (rotVector.SquareSize() > 0.00001f)
    {
      // Calculate the real angle
      float eyeDirHeadZ = eyeDirHead.Z();
      saturate(eyeDirHeadZ, -1.0f, 1.0f);
      float angle = floatMin(_eyeMaxAngle, acos(eyeDirHeadZ/* VForward.DotProduct(eyeDirHead)*/));

      // Make the rotation matrix and rotate
      Matrix3 rotMatrix;
      rotMatrix.SetRotationAxis(-rotVector, angle);
      eyeDirHead = VForward * rotMatrix;
    }
  }

  // Calculate the wink position according to the winkphase (to include the slowing at the borders)
  float winkPosition = 1.0f - winkPhase * winkPhase;

  // Calculate the lower eyelid angle
  float eyelidAngle = floatMin(floatMax(asin(eyeDirHead.Y()*downOrUP) + startAngle, 0.0f) * winkPosition, maxAngle);

    // Create the eyelid orientation matrix in the head space
  Matrix4 eyeLidRotationHead(MRotationX, -eyelidAngle*downOrUP);

    // Translate to zero, rotate, translate back and transform with the head
  m = /*headOrigin **/ Matrix4(MTranslation,eyePos) * eyeLidRotationHead * Matrix4(MTranslation,-eyePos);
  }

bool GrimaceEyes::GetBone(Matrix4 &m, const SkeletonIndex &si,  Head &head, const HeadType *headType, Matrix4Par headOrigin, Vector3Par eyeDirModel, float eyeAccom) const
  {
  // If eye bone is present then fill it out
  if (head._lEye==si)
  {
    GetEyeBone(m,headType->_lEyePos,headOrigin,eyeDirModel);
    return true;
  }
  if (head._rEye==si)
  {
    GetEyeBone(m,headType->_rEyePos,headOrigin,eyeDirModel);
    return true;
  }

  // If the down lid bone is present then fill it out
  if (head._lEyelidDown==si)
{
    GetLidBone(m,headType->_lEyePos, headOrigin, eyeDirModel,_eyelidDownStartAngle,_eyelidDownMaxAngle,head.GetWinkPhase(),-1);
    return true;
  }
  if (head._rEyelidDown==si)
  {
    GetLidBone(m,headType->_rEyePos, headOrigin, eyeDirModel,_eyelidDownStartAngle,_eyelidDownMaxAngle,head.GetWinkPhase(),-1);
    return true;
  }

  // If the up lid bone is present then fill it out
  if (head._lEyelidUp==si)
  {
    GetLidBone(m,headType->_lEyePos, headOrigin, eyeDirModel,_eyelidUpStartAngle,_eyelidUpMaxAngle,head.GetWinkPhase(),1);
    return true;
}
  if (head._rEyelidUp==si)
{
    GetLidBone(m,headType->_rEyePos, headOrigin, eyeDirModel,_eyelidUpStartAngle,_eyelidUpMaxAngle,head.GetWinkPhase(),1);
    return true;
  }

  // If the pupil bone is present then fill it out
  if (head._lPupil==si)
  {
    GetPupilBone(m,headType->_lEyePos, headType->_lPupilPos, headOrigin, eyeDirModel, eyeAccom);
    return true;
  }
  if (head._rPupil==si)
  {
    GetPupilBone(m,headType->_rEyePos, headType->_rPupilPos, headOrigin, eyeDirModel, eyeAccom);
    return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////

GrimaceLipsync::GrimaceLipsync(RStringB name, const ParamEntryVal &grimace, Skeleton *skeleton) : Grimace(name)
{
  ConstParamEntryPtr vizem = grimace.FindEntry("vizem");
  if (vizem && vizem->IsArray())
  {
    int vizemItems = vizem->GetSize();
    if (vizemItems == VizemNum)
    {
      for (int i = 0; i < vizemItems; i++)
      {
        AnimationRTName rtmName;
        rtmName.name = (*vizem)[i].operator RStringB();
        rtmName.skeleton = skeleton;
        rtmName.reversed = true;
        rtmName.streamingEnabled = false;
        _vizem[i] = new AnimationRT(rtmName);
      }
    }
    else
    {
      RptF("Error: Grimace %s doesn't have %d vizems defined", name.Data(), vizemItems);
      Fail("Error: Some grimace doesn't have specified number of vizems defined");
    }
  }
}

bool GrimaceLipsync::GetBone(Matrix4 &m, const SkeletonIndex &si,  Head &head, const HeadType *headType, Matrix4Par headOrigin, Vector3Par eyeDirModel, float eyeAccom) const
{

  // Get the grimace phase
  int vizemS=0,vizemD=0;
  float  factor = head.GetLipPhase(vizemS,vizemD);

  if (factor > 0.999)
  {
    m = _vizem[vizemS]->GetMatrix(0.0, si);
  }
  else
  {
    m = _vizem[vizemS]->GetMatrix(0.0, si) * factor + _vizem[vizemD]->GetMatrix(0.0, si) * (1.0f - factor);
  }
    return true;
  }

//////////////////////////////////////////////////////////////////////////

bool LipSyncInfo::AttachWaveReady(const char *wave, Answer lipFile)
{
  PROFILE_SCOPE_EX(attWR,sound);
  BString<256> buffer;
  strcpy(buffer, wave);
  strcpy(GetFileExt(GetFilenameExt(buffer)),".lip");
  if (lipFile==No) return true;
  if (lipFile==Maybe && !QFBankQueryFunctions::FileExists(buffer)) return true;
  QIFStreamB f;
  f.AutoOpen(buffer);
  return f.PreRead();
}

bool LipSyncInfo::AttachWave(AbstractWave *wave, Answer lipFile, float freq)
{
  _freq = freq; // if LIP doesn't exist use _freq at least for randomLip .
  if (!wave) return false;
  PROFILE_SCOPE_EX(attWa,sound);
  char buffer[256];
  strcpy(buffer, wave->Name());
  strcpy(GetFileExt(GetFilenameExt(buffer)),".lip");

  if (lipFile==No) return false;
  if (lipFile==Maybe && !QFBankQueryFunctions::FileExists(buffer)) return true;

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
  _time = 0;
  return true;
}

bool LipSyncInfo::Simulate(float deltaT)
{
  _time += deltaT;

  float offset = _freq * _time;
  // advance current to point to a current item
  for (; _current < _items.Size();_current++)
  {
    if (_items[_current].time > offset)
    {
      return false;
}

  }
  return true;
}



float LipSyncInfo::GetPhase(int& start,int& end) const
{
  start = 0;
  end = 0; 
  float endtime,startTime = 0.0f;
  float offset = _freq * _time;

  if (_current>=0 && _current < _items.Size())
  {
      end = _items[_current].phase;
      saturateMax(end,0);
      endtime = _items[_current].time;
 
      if (_current>0)
      {
        startTime = _items[_current-1].time;
        start = _items[_current-1].phase;
      }
      float ret, deltaT = endtime-startTime;
      if (deltaT>0)
         ret = ((endtime-offset)/(deltaT));
      else 
        ret = (endtime-offset);
      return ret;
  };
  return -1;
}

//////////////////////////////////////////////////////////////////////////

DEFINE_FAST_ALLOCATOR(HeadObject)

HeadObject::HeadObject(const HeadType *type, LODShape *shape, const CreateObjectId &id)
: base(shape, id)
{
  _type = type;
  _headWound = -1;
}

void HeadObject::Animate(AnimationContext &animContext, int level, bool setEngineStuff, Object *parentObject, float dist2)
{
  base::Animate(animContext, level, setEngineStuff, parentObject, dist2);

  // Personality material and texture change
  if (_faceMaterial) _type->_personality.SetMaterial(animContext, _shape, level, _faceMaterial);
  _type->_personality.SetTexture(animContext, _shape, level, _faceTexture);

  // Wound animation
  if (_headWound > 0)
  {
    _type->_headWound.ApplyModified(animContext, _shape, level, _headWound, _faceTextureOrig, _faceTextureWounded[_headWound-1], dist2);
  }
}

void HeadObject::Deanimate(int level, bool setEngineStuff)
{
  base::Deanimate(level, setEngineStuff);
}

int HeadObject::PassNum(int lod)
{
  return -3;
}
int HeadObject::GetAlphaLevel(int lod) const
{
  return -3;
}

bool HeadObject::CanBeInstanced(int level, float distance2, const DrawParameters &dp) const
{
  return false;
}

HeadType::HeadType():
  _grimaceIndexHurt(-1),
  _grimaceIndexNeutral(-1),
  _grimaceIndexDead(-1),
  _grimaceIndexDanger(-1),
  _grimaceIndexAware(-1),
  _grimaceIndexSafe(-1),
  _grimaceIndexCombat(-1)
{
  _shapeRef = 0;
  _lEyePos = Vector3(0, 0, 0);
  _rEyePos = Vector3(0, 0, 0);
  _lPupilPos = Vector3(0, 0, 0);
  _rPupilPos = Vector3(0, 0, 0);
}

void HeadType::Init(const char *name)
{
  ParamEntryVal cls = Pars >> "CfgHeads" >> name;
  _parClass = cls.GetClassInterface();
  _modelName = (*_parClass)>>"model";
}

void HeadType::ShapeAddRef()
{
  if (_shapeRef++ == 0)
  {
    if (_modelName.GetLength() > 0)
    {
      _model = new HeadObject(this, Shapes.New(GetShapeName(_modelName), true, true), VISITOR_NO_ID);
    }
    else
    {
      _model = NULL;
    }
    if (_model)
    {
      _model->GetShape()->AddLoadHandler(unconst_cast(this));
    }
    else
    {
      InitShape();
    }
  }
}

void HeadType::ShapeRelease()
{
  if (--_shapeRef == 0)
  {
    if (_model)
    {
      _model->GetShape()->RemoveLoadHandler(unconst_cast(this));
    }
    else
    {
      DeinitShape();
    }
    _model.Free();
  }
}

static const char *GetSelectionName(ParamEntryPar entry)
{
  if (entry.IsArray())
  {
    RString name = entry[0];
    return name;
  }

  RString name = entry;
  return name;
}

static const char *GetSelectionAltName(ParamEntryPar entry)
{
  if (entry.IsArray())
  {
    RString name = entry[1];
    return name;
  }

  return NULL;
}

#define SELECTION_NAMES(entry) GetSelectionName(entry), GetSelectionAltName(entry)

void HeadType::InitShape()
{
  if (_model && _model->GetShape())
  {
    LODShape *shape = _model->GetShape();

    shape->SetAutoCenter(false);
    shape->CalculateBoundingSphere();
    if (shape->GetSkeletonSourceName().GetLength() == 0)
    {
      shape->SetAnimationType(AnimTypeNone);
    }
    else
    {
      shape->SetAnimationType(AnimTypeHardware);
      shape->LoadSkeletonFromSource();
    }

    _proxies.Init(shape);

    // Entry in CfgHeads
    ParamEntryVal par = *_parClass;

    // Load content of the Wounds config class
    Ref<WoundInfo> woundInfo = new WoundInfo;
    woundInfo->Load(shape, par>>"wounds");

    // Initialize wound selection
    _headWound.Init(shape, woundInfo, SELECTION_NAMES(par >> "selectionHeadWound"));

    // Initialize personality section
    _personality.Init(shape, SELECTION_NAMES(par >> "selectionPersonality"));

    // Load the grimaces
    {
      Skeleton *skeleton = shape->GetSkeleton();
      if (skeleton)
      {
        ConstParamEntryPtr grimaces = _parClass->FindEntry("Grimaces");
        if (grimaces)
        {
          int grimacesCount = grimaces->GetEntryCount();
          for (int i = 0; i < grimacesCount; i++)
          {
            const ParamEntryVal &grimace = grimaces->GetEntry(i);
            RString grimaceName = grimace.GetName();
            grimaceName.Lower();
            RStringB grimaceTypeName = grimace >> "type";
            if (grimaceTypeName == RStringB("rtm"))
            {
              ConstParamEntryPtr anim = grimace.FindEntry("anim");
              AnimationRTName rtmName;
              rtmName.name = anim->GetValue();
              rtmName.skeleton = skeleton;
              rtmName.reversed = true;
              rtmName.streamingEnabled = false;
              _grimaces.Add(new GrimaceRTM(grimaceName, rtmName));
            }
            else if (grimaceTypeName == RStringB("compound"))
            {
              _grimaces.Add(new GrimaceCompound(grimaceName, grimace, skeleton, _grimaces));
            }
            else if (grimaceTypeName == RStringB("eyes"))
            {
              _grimaces.Add(new GrimaceEyes(grimaceName, grimace, skeleton));
            }
            else if (grimaceTypeName == RStringB("lipsync"))
            {
              _grimaces.Add(new GrimaceLipsync(grimaceName, grimace, skeleton));
            }
          }
          //get common grimace indices
          _grimaceIndexHurt = GetGrimaceIndex("hurt");
          _grimaceIndexNeutral = GetGrimaceIndex("neutral");
          _grimaceIndexDead = GetGrimaceIndex("dead");
          _grimaceIndexDanger = GetGrimaceIndex("danger");
          _grimaceIndexAware = GetGrimaceIndex("aware");
          _grimaceIndexSafe = GetGrimaceIndex("safe");
          _grimaceIndexCombat = GetGrimaceIndex("combat");
        }
      }
    }
  }
}

void HeadType::DeinitShape()
{
  if (_model && _model->GetShape())
  {
    _headWound.Unload();
    _proxies.Deinit(_model->GetShape());
  }
  _grimaces.Clear();
}

void HeadType::InitShapeLevel(int level)
{
  if (_model && _model->GetShape())
  {
    _proxies.InitLevel(_model->GetShape(), level);
    _headWound.InitLevel(_model->GetShape(), level);
    _personality.InitLevel(_model->GetShape(), level);
  }

  if (level == _model->GetShape()->FindSpecLevel(MEMORY_SPEC))
  {
    _lEyePos = _model->GetShape()->NamedPoint(level, "l_eye_center");
    _rEyePos = _model->GetShape()->NamedPoint(level, "r_eye_center");
    _lPupilPos = _model->GetShape()->NamedPoint(level, "l_pupila_center");
    _rPupilPos = _model->GetShape()->NamedPoint(level, "r_pupila_center");
  }
}

void HeadType::DeinitShapeLevel(int level)
{
  if (_model && _model->GetShape())
  {
    _proxies.DeinitLevel(_model->GetShape(), level);
    _headWound.DeinitLevel(_model->GetShape(), level);
    _personality.DeinitLevel(_model->GetShape(), level);
  }
}

void HeadType::LODShapeLoaded(LODShape *shape)
{
  Assert(shape==_model->GetShape());
  InitShape();
}

void HeadType::LODShapeUnloaded(LODShape *shape)
{
  Assert(shape==_model->GetShape());
  DeinitShape();
}

void HeadType::ShapeLoaded(LODShape *shape, int level)
{
  Assert(shape==_model->GetShape());
  InitShapeLevel(level);
}

void HeadType::ShapeUnloaded(LODShape *shape, int level)
{
  Assert(shape==_model->GetShape());
  DeinitShapeLevel(level);
}

void HeadType::PrepareGrimaceMatrices(Matrix4Array &matrices,  Head &head, const HeadType *headType, const Shape *shape, Matrix4Par headOrigin, Vector3Par eyeDirModel, float eyeAccom, int grimaceDest, int grimaceSrc, float factor) const
{
  // Grimace blending
  PROFILE_SCOPE(headGM);

  const Ref<Grimace> &dstGrimace = _grimaces[grimaceDest];
  const Ref<Grimace> &srcGrimace = _grimaces[grimaceSrc];
  int subSkeletonSize = shape->GetSubSkeletonSize();
  for (int i = 0; i < subSkeletonSize; i++)
  {
    SubSkeletonIndex ssi;
    SetSubSkeletonIndex(ssi, i);
    SkeletonIndex si = shape->GetSkeletonIndexFromSubSkeletonIndex(ssi);

    if (factor<0.001f || grimaceDest==grimaceSrc)
    {
      Matrix4 src;
      if (srcGrimace->GetBone(src, si, head, headType, headOrigin, eyeDirModel, eyeAccom))
      {
        matrices[GetSubSkeletonIndex(ssi)] = matrices[GetSubSkeletonIndex(ssi)] * src;
      }
    }
    else if (factor>0.999f)
    {
      Matrix4 dst;
      if (dstGrimace->GetBone(dst, si, head, headType, headOrigin, eyeDirModel, eyeAccom))
      {
        matrices[GetSubSkeletonIndex(ssi)] = matrices[GetSubSkeletonIndex(ssi)] * dst;
      }
    }
    else
    {
      Matrix4 dst;
      Matrix4 src;
      bool dstNotIdent = dstGrimace->GetBone(dst, si, head, headType, headOrigin, eyeDirModel, eyeAccom);
      bool srcNotIdent = srcGrimace->GetBone(src, si, head, headType, headOrigin, eyeDirModel, eyeAccom);
      if (srcNotIdent || dstNotIdent)
      {
        Matrix4 blend;
        if (!srcNotIdent)
        {
          blend = dst * factor + MIdentity * (1.0f - factor);
        }
        else if (!dstNotIdent)
        {
          blend = MIdentity * factor + src * (1.0f - factor);
        }
        else
        {
          blend = dst * factor + src * (1.0f - factor);
        }
        matrices[GetSubSkeletonIndex(ssi)] = matrices[GetSubSkeletonIndex(ssi)] * blend;
      }
    }
  }
}

int HeadType::GetGrimaceIndex(RStringB grimaceName) const
{
  for (int i = 0; i < _grimaces.Size(); i++)
  {
    if (_grimaces[i]->Name() == grimaceName) return i;
  }
  return -1;
}

HeadTypeBank HeadTypes;

Head::Head()
{
  _grimacePhase = 0;
  _winkPhase = 2;
  _forceWinkPhase = -1;
  _nextWink = 0;  // recalculate now
  _srcGrimace = _dstGrimace = _newGrimace = _forcedGrimace = -1;
  _factorGrimace = 1.0f;

  _randomLip = false;
  _actualRandomLip = 0;
  _wantedRandomLip = 0;

  _headBone = SkeletonIndexNone;
  _lEye = SkeletonIndexNone;
  _rEye = SkeletonIndexNone;
  _lEyelidUp = SkeletonIndexNone;
  _rEyelidUp = SkeletonIndexNone;
  _lEyelidDown = SkeletonIndexNone;
  _rEyelidDown = SkeletonIndexNone;
  _lPupil = SkeletonIndexNone;
  _rPupil = SkeletonIndexNone;
}

void Head::Simulate(float deltaT, SimulationImportance prec, bool dead)
{
  _grimacePhase += deltaT * 1.0f / 10.0f;
  _grimacePhase = fastFmod(_grimacePhase,1);
  if (!dead)
  {
    _nextWink -= deltaT;
    if (_nextWink <= 0)
    {
      const float winkSpeed = 6.0f;
      _winkPhase += winkSpeed * deltaT;
      if (_winkPhase >= 1)
      {
        _winkPhase = 0;
        _nextWink = 1.0f + 5.0f * GRandGen.RandomValue();
      }
    }
  }

  // Simulate grimaces
  {

    // If grimaces not in progress and new grimace differs then start new change
    if (_forcedGrimace>=0)
    {
      if (_forcedGrimace!=_dstGrimace)
      {
        _dstGrimace = _forcedGrimace; 
        _factorGrimace = 0.0f;
      }
    }
    else if (_dstGrimace == _srcGrimace)
    {
      if (_newGrimace != _dstGrimace)
      {
        _dstGrimace = _newGrimace; 
        _factorGrimace = 0.0f;
      }
    }

    // If grimace change in progress then calculate new progress (or finish the progress)
    const float grimaseChangeSpeed = 0.5f;
    if (_dstGrimace != _srcGrimace)
    {
      if (_factorGrimace < 1.0f)
      {
        _factorGrimace += deltaT / grimaseChangeSpeed;
      }
      else
      {
        _srcGrimace = _dstGrimace;
        _factorGrimace = 1.0f;
      }
    }
  }

  // Simulate random lip
  if (_randomLip)
  {
    _nextChangeRandomLip -= deltaT;
    if (_nextChangeRandomLip <= 0) NextRandomLip();
  }
  else
  {
    _actualRandomLip = 0;
  }

  // Simulate lip moving
  if (_lipInfo)
  {
    if (_lipInfo->Simulate(deltaT))
    {
      _lipInfo.Free();
}
  }
}

void Head::SetGrimace(int grimaceIndex)
{
  _newGrimace = grimaceIndex;
}

void Head::SetForcedGrimace(int grimaceIndex)
{
  _forcedGrimace = grimaceIndex;
}

float Head::GetWinkPhase() const
{
  float winkPhase;
  if (_forceWinkPhase >= 0)
  {
    winkPhase = 2.0f * _forceWinkPhase;
  }
  else
  {
    winkPhase = 2.0f * _winkPhase;
  }
  if (winkPhase > 1) winkPhase = 2.0 - winkPhase;
  return winkPhase;
}

void Head::AttachWave(AbstractWave *wave, float freq)
{
  _lipInfo = new LipSyncInfo();
  LipSyncInfo::Answer lipExists = LipSyncInfo::Maybe;
  
  // note: instead of FileIsTakenFromBank a weaker function should be used, only testing the existance, not "patching"
  BString<256> lipFile;
  strcpy(lipFile, wave->Name());
  strcpy(GetFileExt(GetFilenameExt(lipFile)),".lip");

  if (QFBankQueryFunctions::FileIsTakenFromBank(lipFile))
    lipExists = LipSyncInfo::Yes;
  else if (!strcmp(_lipExists._file,lipFile))
  {
    if (_lipExists._exists.IsReady())
      lipExists = _lipExists._exists.GetResult() ? LipSyncInfo::Yes : LipSyncInfo::No;
}
  if (!_lipInfo->AttachWave(wave,lipExists,freq)) _lipInfo = NULL;
}

bool Head::AttachWaveReady(const char *wave)
{
  BString<256> lipFile;
  strcpy(lipFile, wave);
  strcpy(GetFileExt(GetFilenameExt(lipFile)),".lip");
  LipSyncInfo::Answer lipExists = LipSyncInfo::Maybe;
  // when in bank, no need to test any external files
  if (QFBankQueryFunctions::FileIsTakenFromBank(lipFile))
    lipExists = LipSyncInfo::Yes;
  else
  {
    if (strcmp(_lipExists._file,lipFile))
    {
      _lipExists._file = lipFile.cstr();
      _lipExists._exists = GFileServerFunctions->RequestTimeStamp(lipFile);
}
    // check if the future is ready
    // if not, attach not ready yet
    if (!_lipExists._exists.IsReady())
     return false;
    lipExists = _lipExists._exists.GetResult() ? LipSyncInfo::Yes : LipSyncInfo::No;
  }
  // actual result does not matter, we only need to be sure the handle is here
  return LipSyncInfo::AttachWaveReady(wave,lipExists);
}

float Head::GetLipPhase(int & start,int & end) const
{
  if (_randomLip && (!_lipInfo || _lipInfo->IsEmpty()))
  {
    start = _actualRandomLip;
    end = _wantedRandomLip;
    return _nextChangeRandomLip*_speedRandomLip;
  }
  else
  {
    if (_lipInfo) 
    {
      float ret = _lipInfo->GetPhase(start,end);
      if (ret < 0) 
      {
        return 0;
      }
      return ret;
    }
  }
  return 0.0f;
}

void Head::SetRandomLip(bool set)
{
  if (set && !_randomLip)
  {
    _wantedRandomLip = 0;
    NextRandomLip();
  }
  _randomLip = set;
}

void Head::NextRandomLip()
{
  _actualRandomLip = _wantedRandomLip;
  do
  {
    _wantedRandomLip = 1 + toIntFloor(GRandGen.RandomValue()*(VizemNum-1));
    saturate(_wantedRandomLip, 0, VizemNum - 1);
  } while (_actualRandomLip==_wantedRandomLip);

  float time = 0.04 + 0.04 * GRandGen.RandomValue();

  if (_lipInfo)
    time *= _lipInfo->GetSpeed();

  _nextChangeRandomLip = time;
  _speedRandomLip = 1.0 / time;
}

void GlassesType::Init(const char *name)
{
  _name = name;
  ParamEntryVal cls = Pars >> "CfgGlasses" >> name;
  _modelName = cls >> "model";
}

void GlassesType::ShapeAddRef()
{
  if (_shapeRef++ == 0)
  {
    // model need to be loaded
    if (_modelName.GetLength() > 0)
      _model = new Object(Shapes.New(GetShapeName(_modelName), true, true), VISITOR_NO_ID);
    else
      _model = NULL;

    if (_model) _model->GetShape()->AddLoadHandler(this);
  }
}

void GlassesType::ShapeRelease()
{
  if (--_shapeRef == 0)
  {
    if (_model) _model->GetShape()->RemoveLoadHandler(this);
    _model.Free();
  }
}

GlassesTypeBank GlassesTypes;

#endif
