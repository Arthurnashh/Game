// Poseidon - vehicle class
// (C) 1997, Suma
#include "wpch.hpp"

#include "invisibleVeh.hpp"
#include "AI/ai.hpp"

#include "scene.hpp"
#include "landscape.hpp"

InvisibleVehicleType::InvisibleVehicleType( ParamEntryPar param )
:base(param)
{
}

void InvisibleVehicleType::Load(ParamEntryPar par, const char *shape)
{
	base::Load(par,shape);
}

void InvisibleVehicleType::InitShape()
{
	if (!_shape) return;
	base::InitShape();
}


Object* InvisibleVehicleType::CreateObject(bool unused) const
{
  return new InvisibleVehicle(this, unused);
}


InvisibleVehicle::InvisibleVehicle( const EntityAIType *name, bool fullCreate )
:base(name,fullCreate)
{
}

void InvisibleVehicle::DrawDiags()
{
	Ref<Object> obj=new ObjectColored(GScene->Preloaded(SphereModel),VISITOR_NO_ID);
	Color color(1,1,0);
	obj->SetPosition(RenderVisualState().Position());
	obj->SetScale(3);
	obj->SetConstantColor(PackedColor(color));
	GScene->ShowObject(obj);
}

InvisibleVehicle::~InvisibleVehicle()
{
}

void InvisibleVehicle::Simulate( float deltaT, SimulationImportance prec )
{
  SimulatePost(deltaT,prec);
}

ActionContextBase *InvisibleVehicle::CreateActionContextUIAction(const Action *action, ManAction moveAction)
{
  Fail("InvisibleVehicle cannot perform actions");
  return NULL;
}

void InvisibleVehicle::CreateActivityGetIn(Transport *veh, UIActionType pos, Turret *turret, int cargoIndex)
{
  Fail("InvisibleVehicle cannot be used as a vehicle crew");
}
