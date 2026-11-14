
// Poseidon - includes
#include "../wpch.hpp"

#include "dampers.hpp"
#include "../transport.hpp"
#include "../AI/ai.hpp" 
#include "../rtAnimation.hpp"

/*
 TODO: remove ai.hpp dependency
 The dependency is caused by OLinkPermNO(AIGroup) groupOwner member of TargetNormal.
*/

AnimationSourceDamper::AnimationSourceDamper(const RStringB &name, int index)
:base(name)
{
  // bone will be initialized later
  _bone = SkeletonIndexNone;
  _index = index;
  // unless proven wrong, assume damper is unused
  _unused = true;
}

AnimationSourceDamper::AnimationSourceDamper(const RStringB &name, int index, const RStringB &selection)
:base(name)
{
  // we can initialize bone anytime
  // bone will be initialized later
  _bone = SkeletonIndexNone;
  _boneName = selection;
  _index = index;
  // unless proven wrong, assume damper is unused
  _unused = true;
}

float AnimationSourceDamper::GetPhase(const ObjectVisualState &vs, const Animated *obj) const
{
  if (_index<0) return 0;
  float damper = static_cast_checked<TransportVisualState const&>(vs).GetDamperState(_index);
  return damper;
}

//const float MaxDamper=0.2;
//const float scaleDamper=0.5;
//
//const float initDamper=-2*scaleDamper/MaxDamper;
//const float adaptDamper=7.5*scaleDamper;


DamperHolder::DamperHolder() :_maxAmplitude(0), _stiffnesScale(1), _dampingScale(1), _used(false)
{}

AnimationSource *DamperHolder::CreateAnimationSource()
{
  // TODO: disable anonymous dampers
  AnimationSourceDamper *source = new AnimationSourceDamper(RStringB(),_sources.Size());
  _sources.Add(source);
  return source;
}

AnimationSource *DamperHolder::CreateAnimationSource(ParamEntryPar entry)
{
  // check landcontact selection
  RStringB selection = entry>>"selection";
  AnimationSourceDamper *source = new AnimationSourceDamper(entry.GetName(),_sources.Size(),selection);
  _sources.Add(source);
  return source;
}

void DamperHolder::Init(float maxAmplitude, float stiffnesScale, float dampingScale)
{
  _maxAmplitude = maxAmplitude;
  _stiffnesScale = stiffnesScale;
  _dampingScale = dampingScale;
}

void DamperHolder::InitLandcontact(
  LODShape *shape, int level,
  const AnimationHolder &anims, const AnimationSourceHolder &sources
)
{
  Assert(level==shape->FindLandContactLevel());
  const Shape *contact = shape->GetLevelLocked(level);

  // prepare contact information
  _whichWheelContact.Resize(contact->NPos()); // conversion from landcontact

  Skeleton *skeleton = shape->GetSkeleton();
  // LODShape is already loaded, and all animations are registered
  // for each damper scan animations for a corresponding skeleton bone
  _sources.Compact();
  // prepare wheel index information
  for (int w=0; w<_sources.Size(); w++)
  {
    AnimationSourceDamper *source = _sources[w];

    if (source->_boneName.GetLength()>0)
    {
      // explicit bone
      source->_bone = skeleton ? skeleton->NewBone(source->_boneName) : SkeletonIndexNone;
    }
    else
    {
      // implicit bone - based on animation
      int index = sources.Find(source);
      Assert(index>=0);
      AnimationType *type = anims[index];
      source->_bone = type->GetBone(level);
    }
    
    
  }
  
  for (int i=0; i<contact->NPos(); i++)
  {
    int wheel=-1;
    if (skeleton)
    {
      const AnimationRTWeight &weight = contact->Weight(i);
      if (weight.Size()>0)
      {
        // assume damper points are not blended
        // if they are, we are deep in trouble anyway
        Assert(weight.Size()==1);
        SubSkeletonIndex subBone = weight[0].GetSel();
        SkeletonIndex vertexBone = contact->GetSkeletonIndexFromSubSkeletonIndex(subBone);
        // check which source controls this bone
        for (int w=0; w<_sources.Size(); w++)
        {
          SkeletonIndex bone = _sources[w]->_bone;
          if (bone==vertexBone)
          {
            _sources[w]->_unused = false;
            wheel = w;
            _used = true;
            break;
          }
        }
      }
    }
#if _ENABLE_REPORT
    if (wheel<0)
    {
      Vector3Val pos = contact->Pos(i);
      RptF(
        "Error in %s: Landcontact point %g,%g,%g not connected to any wheel",
        cc_cast(shape->GetName()),pos[0],pos[1],pos[2]
      );
    }
#endif
    _whichWheelContact[i] = wheel;
  }

  // Initialize damper springs. Calculate the different stiffness from the front and back wheels according to
  // center of mass
  float mass = shape->Mass();
  Vector3 centerOfMass = shape->CenterOfMass();  

  float negativeR = 0;
  float positiveR = 0;

  int positiveN = 0;
  int negativeN = 0;

  for(int i = 0; i < contact->NPos(); i++)
  {    
    /*if (_whichWheelContact[i] < 0)  // use all contact points
      continue;*/ 

    float animCenter = contact->Pos(i)[2] - centerOfMass[2];

    if (animCenter > 0)
    {      
      positiveR += animCenter;
      positiveN++;
    }
    else
    {      
      negativeR -= animCenter;
      negativeN++;
    }
  }
  float massProp = mass / (positiveN + negativeN);
  float unbalancedMoment = -massProp * (positiveR - negativeR); // moment to balance if every dampers push by force massProp

  float positiveF = 0; 
  float negativeF = 0; 
  Assert(negativeN > 0); // other wise vehicle cannot stand
  if (fabs(unbalancedMoment) > 0.01f // numerical barrier
    &&  negativeN > 0) // negative moment cannot be negative...
  {    
    positiveF = unbalancedMoment / (positiveR + negativeR * positiveN / negativeN );   
    negativeF = -positiveF * positiveN / negativeN;   
  }

  // finally set up springs
  _springs.Resize(Count());
  for(int i = 0; i < contact->NPos(); i++)
  {
    if (_whichWheelContact[i] < 0)
      continue; 

    int wheel = _whichWheelContact[i];    

    float animCenter = contact->Pos(i)[2] - centerOfMass[2];
    if (animCenter > 0)
    {     
      _springs[wheel].Init(2*_maxAmplitude, _stiffnesScale * (massProp + positiveF) * 2 * G_CONST, (massProp + positiveF) * _dampingScale);
    }
    else
    {      
      _springs[wheel].Init(2*_maxAmplitude,  _stiffnesScale * (massProp + negativeF) * 2 * G_CONST, (massProp + negativeF) * _dampingScale);
    }
  }
}

struct MatchBone
{
  bool &matched;
  int vertexBone;
  MatchBone(bool &matched, int vertexBone):matched(matched),vertexBone(vertexBone){}

  int operator () (int bone, int ctx) const
  {
    if (vertexBone==bone) matched = true;
    return 0;
  }
};

void DamperHolder::InitGeometry(
  LODShape *shape, int level,
  const AnimationHolder &anims, const AnimationSourceHolder &sources
)
{
  Assert(level==shape->FindGeometryLevel());
  const Shape *contact = shape->GetLevelLocked(level);
  // dampers should work with geometry as well
  _whichWheelGeometry.Resize(contact->NPos()); // conversion from landcontact
  _used = false;
  for (int i=0; i<contact->NPos(); i++)
  {
    int wheel=-1;
    if (shape->GetSkeleton())
    {
      const AnimationRTWeight &weight = contact->Weight(i);
      if (weight.Size()>0)
      {
        const BoneTree &tree = shape->GetSkeleton()->GetTree();
        // assume damper points are not blended
        // if they are, we are deep in trouble anyway
        Assert(weight.Size()==1);
        SubSkeletonIndex subBone = weight[0].GetSel();
        SkeletonIndex vertexBone = contact->GetSkeletonIndexFromSubSkeletonIndex(subBone);
        // check which source controls this bone
        for (int w=0; w<_sources.Size(); w++)
        {
          SkeletonIndex bone = _sources[w]->_bone;
          bool matched = false;
          tree.Process(MatchBone(matched,GetSkeletonIndex(vertexBone)+1),GetSkeletonIndex(bone)+1);
          // check if vertexBone is anywhere in the bone subtree
          if (bone==vertexBone || matched)
          {
            _sources[w]->_unused = false;
            wheel = w;        
            break;
          }
        }
      }
    }
    _whichWheelGeometry[i] = wheel;
  }
}

void DamperHolder::Clear()
{
  _used = false; 
  _whichWheelContact.Clear();
  _whichWheelGeometry.Clear();
  _sources.Clear();
  _springs.Clear();
}

float DamperHolder::GetInitPos() const 
{
  float pos = -(1.0f - 1.0f/_stiffnesScale) * _maxAmplitude;
  saturate(pos,-_maxAmplitude, _maxAmplitude); 
  return pos; 
}



bool DamperHolder::CheckWheel(int vertexIndex, bool landContact) const
{
  // currently landcontact contains only wheels
  if (landContact) return true;
  // check which damper is affected
  // once landcontact contains other parts of the vehicle, we should use this:
  //int wheel = landContact ? _whichWheelContact[vertexIndex] : _whichWheelGeometry[vertexIndex];
  int wheel = _whichWheelGeometry[vertexIndex];
  return wheel>=0;
}



void DamperState::Init(const DamperHolder &type)
{
  _pos.Init(type.Count());
  for( int i=0; (size_t)i<_pos.Size(); i++ ) _pos[i]=type.GetInitPos();
}

float DamperState::Energy(const DamperHolder &type) const
{
  float e = 0;
  for(int i = 0; (size_t)i < _pos.Size(); i++)
    e += type._springs[i].Energy(type._maxAmplitude - _pos[i]);

  return e;
}

void DamperState::Interpolate(DamperState const& t1state, float t, DamperState& res) const
{
  Assert(_pos.Size()==res._pos.Size());
  Assert(_pos.Size()==t1state._pos.Size());
  for (int i=0; (size_t)i < _pos.Size(); i++)
    res._pos[i] = _pos[i] + t*(t1state._pos[i] - _pos[i]);
}


DamperForces::DamperForces( const DamperHolder &type)
:_processedDampers(_storage,sizeof(_storage))
{
  _processedDampers.Realloc(type.Count());
  _processedDampers.Resize(type.Count());

  for (int w=0; w<_processedDampers.Size(); w++ )
  {
    _processedDampers[w]=false;
  }
}

#define DIAG_DAMPERS 0

void DamperForces::SimulateContact(
  float &forceUp, float &frictionUp, DamperState &state,
  const DamperHolder &type, int vertexIndex, bool landContact,
  float &under, float pointSpeedUp
)
{
  forceUp = 0;
  frictionUp = 0;
  
  // check which damper is affected
  int wheel = (
    landContact ?
    type._whichWheelContact[vertexIndex] :
    type._whichWheelGeometry[vertexIndex]
  );
  if (wheel < 0)
    return;
  
  // some wheel affected  
  if (!_processedDampers[wheel])
  {
    Assert(!type._sources[wheel]->_unused);
    
    _processedDampers[wheel] = true;
    #if DIAG_DAMPERS
      Log("v=%d, W=%d, Pos=%.3f, under=%.3f",vertexIndex,wheel,state._pos[wheel],under);
    #endif
    state._pos[wheel] += under; // move damper on land
    if (under > 0)
    {
      if (state._pos[wheel] > type._maxAmplitude) 
      {
        under = state._pos[wheel] - type._maxAmplitude;
        state._pos[wheel] = type._maxAmplitude;
      }
      else
      {
        under = 0; 
      }
    }

    saturateMax(state._pos[wheel],-type._maxAmplitude); 
   
    forceUp = type._springs[wheel].Force(type._maxAmplitude - state._pos[wheel]);
    frictionUp = - type._springs[wheel].Damping(pointSpeedUp);
  }  
}


void DamperForces::Finish(DamperState &state, const DamperHolder &type, float deltaT)
{
  // ignore dampers which are not attached to any landcontact point
  const float speed = 0.15f; // how fast damper without contact to land will be narrowed 
  for (int w=0; w< type.Count(); w++ )
  {
    if (!_processedDampers[w] && !type._sources[w]->_unused)
    {
      #if DIAG_DAMPERS
         Log("W=%d, Pos=%.3f",w,state._pos[w]);
        #endif
       state._pos[w] -= speed * deltaT; 
       saturate(state._pos[w],-type._maxAmplitude,+type._maxAmplitude); 
    }
  }
  #if DIAG_DAMPERS
    Log("Finish: dT=%.4f",deltaT);
  #endif
}

