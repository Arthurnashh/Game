#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FIREPLACE_HPP
#define _FIREPLACE_HPP

#include "vehicleAI.hpp"
#include "smokes.hpp"
#include "lights.hpp"
#include "dynSound.hpp"

class Fireplace: public EntityAI
{
	typedef EntityAI base;

protected:
  EffectsSourceRT _effects;
/*
	Ref<LightPointOnVehicle> _light;
	Color _lightColor;
	SmokeSource _smoke;
	Ref<SoundObject> _sound;
*/
	bool _burning;

public:
	Fireplace(const EntityAIType *name);

	bool Burning() const {return _burning;}
	void Inflame(bool fire = true) {_burning = fire;}

	void Simulate( float deltaT, SimulationImportance prec );

	void Sound( bool inside, float deltaT );
	void UnloadSound();

	bool QIsManual() const {return false;}

	AnimationStyle IsAnimated( int level ) const;

	virtual void GetActions(const std::function<void(const UIAction &action)> &add, AIBrain *unit) override;
#if _HELISIM_PRECISE_INTERACTION
  void GetDrawActions3D(UIActions &actions, AIBrain *unit);
#endif
  virtual bool GetActionParams(UIActionParams &params, const Action *action, AIBrain *unit) const override;
	virtual void PerformAction(const Action *action, AIBrain *unit) override;

	LSError Serialize(ParamArchive &ar);

	DECLARE_NETWORK_OBJECT
	static NetworkMessageFormat &CreateFormat
	(
		NetworkMessageClass cls,
		NetworkMessageFormat &format
	);
	TMError TransferMsg(NetworkMessageContext &ctx);
	float CalculateError(NetworkMessageContextWithError &ctx);
	
	USE_CASTING(base)
};

#endif
