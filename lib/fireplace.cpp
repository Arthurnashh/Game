#include "wpch.hpp"
#include "fireplace.hpp"
#include "scene.hpp"
#include <El/Common/randomGen.hpp>
#include "AI/ai.hpp"
#include "UI/uiActions.hpp"
#include "stringtableExt.hpp"
#include "Network/network.hpp"

DEFINE_CASTING(Fireplace)

Fireplace::Fireplace(const EntityAIType *type)
: base(type) /*, _smoke((*type->_par) >> "Smoke")*/
{
	_burning = false;
	const ParamEntry &cls = *type->_par;
  _effects.Init(cls >> "effects", this, EVarSet_Fire);

/*
	_light = new LightPointOnVehicle(this, VZero);
	_light->Switch(_burning);
	_light->Load(cls >> "Light");
	_lightColor = _light->GetDiffuse();
	GLOB_SCENE->AddLight(_light);

	RString sound = cls >> "sound";
	_sound = new SoundObject(sound, this, true, true);
*/
}
	
void Fireplace::Simulate( float deltaT, SimulationImportance prec )
{
/*
  if (_burning)
	{
		Vector3 position = Position();
		float fade = GRandGen.PlusMinus(0.8, 0.2);
		if (_light)
		{
			_light->Switch(true);
			_light->SetDiffuse(_lightColor * fade);
			position = _light->Transform().Position();
		}
		_smoke.Simulate(position, Speed(), deltaT, prec);
		if (_sound)
		{
			_sound->Pause(false);
			_sound->Simulate(this,deltaT, prec);
		}
	}
	else
	{
		if (_light) _light->Switch(false);
		if (_sound)
		{
			_sound->Pause();
			_sound->Simulate(this,deltaT, prec);
		}
	}
*/

  float fade = GRandGen.PlusMinus(0.8, 0.2);
  float effectValues[] = {1.0f,fade};
  _effects.Simulate(deltaT, prec, effectValues, lenof(effectValues), _burning);

	base::Simulate(deltaT, prec);
  SimulatePost(deltaT,prec);
}

AnimationStyle Fireplace::IsAnimated( int level ) const
{
	return Object::IsAnimated(level);
}

void Fireplace::Sound( bool inside, float deltaT )
{
}

void Fireplace::UnloadSound()
{
}

LSError Fireplace::Serialize(ParamArchive &ar)
{
	CHECK(base::Serialize(ar))
	CHECK(ar.Serialize("burning", _burning, 1, false))
	return LSOK;
}

#define FIREPLACE_MSG_LIST(XX) \
  XX(UpdateGeneric, UpdateFireplace)

DEFINE_NETWORK_OBJECT(Fireplace, base, FIREPLACE_MSG_LIST)

#define UPDATE_FIREPLACE_MSG(MessageName, XX) \
  XX(MessageName, bool, burning, NDTBool, bool, NCTNone, DEFVALUE(bool, false), DOC_MSG("Fire is burning"), TRANSF, ET_NOT_EQUAL, ERR_COEF_MODE)

DECLARE_NET_INDICES_EX_ERR(UpdateFireplace, UpdateVehicleAI, UPDATE_FIREPLACE_MSG)
DEFINE_NET_INDICES_EX_ERR(UpdateFireplace, UpdateVehicleAI, UPDATE_FIREPLACE_MSG, NoErrorInitialFunc)

NetworkMessageFormat &Fireplace::CreateFormat
(
	NetworkMessageClass cls,
	NetworkMessageFormat &format
)
{
	switch (cls)
	{
	case NMCUpdateGeneric:
		base::CreateFormat(cls, format);
    UPDATE_FIREPLACE_MSG(UpdateFireplace, MSG_FORMAT_ERR)
		break;
	default:
		base::CreateFormat(cls, format);
		break;
	}
	return format;
}

TMError Fireplace::TransferMsg(NetworkMessageContext &ctx)
{
	switch (ctx.GetClass())
	{
	case NMCUpdateGeneric:
		TMCHECK(base::TransferMsg(ctx))
		{
			PREPARE_TRANSFER(UpdateFireplace)

			TRANSF(burning)
		}
		break;
	default:
		return base::TransferMsg(ctx);
	}
	return TMOK;
}

float Fireplace::CalculateError(NetworkMessageContextWithError &ctx)
{
	float error = 0;
	switch (ctx.GetClass())
	{
	case NMCUpdateGeneric:
		error += base::CalculateError(ctx);
		{
			PREPARE_TRANSFER(UpdateFireplace)

			ICALCERR_NEQ(bool, burning, ERR_COEF_MODE)
		}
		break;
	default:
		error += base::CalculateError(ctx);
		break;
	}
	DoAssert(_finite(error));
	return error;
}

void Fireplace::GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit)
{
	base::GetActions(add, unit);
	if (!unit) return;

	if (unit->IsFreeSoldier())
	{
		Person *person = unit->GetPerson();
		if (person->CheckActionProcessing(ATFireInflame, unit)) return;
		if (person->CheckActionProcessing(ATFirePutDown, unit)) return;

		if (_burning)
    {
      UIAction action(new ActionBasic(ATFirePutDown, this));
      add(action);
    }
		else
    {
      UIAction action(new ActionBasic(ATFireInflame, this));
      add(action);
    }
	}
}

#if _HELISIM_PRECISE_INTERACTION
void Fireplace::GetDrawActions3D(UIActions &actions, AIBrain *unit)
{
  if (!unit) return;

  if (unit->IsFreeSoldier())
  {
    if (_burning)
    {
      UIAction action(new ActionBasic(ATFirePutDown, this));
      actions.Add(action);
    }
    else
    {
      UIAction action(new ActionBasic(ATFireInflame, this));
      actions.Add(action);
    }
  }

  base::GetDrawActions3D(actions, unit);
}
#endif

bool Fireplace::GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const
{
	switch (action->GetType())
	{
		case ATFireInflame:
		case ATFirePutDown:
      // No parameters
      return true;
	}
	return base::GetActionParams(params, action, unit);
}

void Fireplace::PerformAction(const Action *action, AIBrain *unit)
{
	switch (action->GetType())
	{
		case ATFireInflame:
      // only owner can process actions
      DoAssert(GetNetworkManager().IsServerOwner());
			Inflame(true);
			return;
		case ATFirePutDown:
      // only owner can process actions
      DoAssert(GetNetworkManager().IsServerOwner());
			Inflame(false);
			return;
	}
	base::PerformAction(action,unit);
}
