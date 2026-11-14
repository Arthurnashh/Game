#ifdef _MSC_VER
#pragma once
#endif

#ifndef _INVISIBLE_VEH_HPP
#define _INVISIBLE_VEH_HPP

#include "vehicleAI.hpp"
#include "person.hpp"

class InvisibleVehicleType: public VehicleType
{
	typedef VehicleType base;

	friend class InvisibleVehicle;

	public:
	InvisibleVehicleType( ParamEntryPar param );
	virtual void Load(ParamEntryPar par, const char *shape);
	void InitShape();
	bool AbstractOnly() const {return false;}
  virtual bool IsPerson() const {return true;}

  virtual Object* CreateObject(bool unused) const;

  virtual bool ScanTargets() const {return false;}
  virtual bool ScannedAsTarget() const {return false;}
};

class InvisibleVehicle: public Person
{
	typedef Person base;

	public:
	const InvisibleVehicleType *Type() const
	{
		return static_cast<const InvisibleVehicleType *>(GetType());
	}

	InvisibleVehicle( const EntityAIType *name, bool fullCreate = true );
	~InvisibleVehicle();

	Visible VisibleStyle() const {return ObjInvisible;}
	
	void DrawDiags();

  void ResetMovement(float speed, int action) {}
	void ResetMovement(float speed, RString action) {}

	void Simulate(float deltaT, SimulationImportance prec);
	void CreateActivityGetIn(Transport *veh, UIActionType pos, Turret *turret = NULL, int cargoIndex = -1);
  virtual ActionContextBase *CreateActionContextUIAction(const Action *action, ManAction moveAction) override;
  void PreciseGetInAction(const RString action, const RString actionAfter, int doorIndex) {}
  void PreciseGetOutAction(const RString action) {};

  float NeedsAmbulance() const {return 0;} // avoid recursion
  bool IsAbleToStand() const {return true;} // avoid recursion
};

#endif

