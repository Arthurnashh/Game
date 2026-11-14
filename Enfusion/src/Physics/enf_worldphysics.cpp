//-----------------------------------------------------------------------------
// File: enf_worldphysics.cpp
//
// Desc: World management & physics
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

//#include "precomp.h"
//#include "enf_main.h"
//#include "bspworld/enf_bspworld.h"
//#include "baseworld/enf_entity.h"
//#include "baseworld/enf_baseworldtrace.h"
//#include "baseworld/enf_worldphysics.h"
#include "Physics\enf_worldphysics.h"
//#include "enf_profiler.h"
//#include "vobject/enf_vobject.h"
//#include "vobject/enf_particleeffectinternal.h"
//#include "common/enf_serialize.h"
//#include "common/enf_diag.h"
//#include "enf_input.h"
//#include "enf_shapes.h"
//#include "enf_scriptapi.h"

namespace enf
{

//-----------------------------------------------------------------------
PhysicsManager::~PhysicsManager()
{
}

//-----------------------------------------------------------------------
PhysicsManager::PhysicsManager()//: Module()
{
  g_pStatic = CreateStaticBuffer(1024*1024*2 + 256 * 1024, true);
}

//-----------------------------------------------------------------------
PhysicsMaterial::PhysicsMaterial(CStr name):
	//m_Name(name.Static()),
	m_iMaterialIndex(0)
{
  m_Name = g_pStatic->AddString(name.cstr());
}

//-----------------------------------------------------------------------
PhysicsMaterial::PhysicsMaterial():
	m_iMaterialIndex(0)
{
}

//-----------------------------------------------------------------------
const Vector3& PhysicsController::GetCenterOfMass() const
{
	static const Vector3 zero(0.0f);
	return zero;
}

//-----------------------------------------------------------------------
PhysicsController::~PhysicsController()
{
}

//-----------------------------------------------------------------------
void PhysicsController::Serialize(SerializeOutputAdapter* adapter)
{
}

//-----------------------------------------------------------------------
void PhysicsController::Serialize(SerializeInputAdapter* adapter)
{
}

//-----------------------------------------------------------------------
bool PhysicsUpdateEntity(Object *th, Object* wrld, const float timeslice, const uint framecount)
{
  RlsAssert2(0, "not implemented");
  return false;
  /*
	th->ThrowEvent(wrld, EV_FRAME, (void*)framecount);
	return !th->IsFlagSet(TFL_REMOVE);
  */
}

//-----------------------------------------------------------------------
bool AfterPhysicsUpdateEntity(Object *th, Object* wrld, const float timeslice, const uint framecount)
{
  RlsAssert2(0, "not implemented");
  return false;
  /*
	th->Update();

	Object* sibling;
	for(Object* child = th->GetChildren(); child; child = sibling)
	{
		sibling = child->GetSibling();
		if(PhysicsUpdateEntity(child, wrld, timeslice, framecount) && child->GetParent() == th)
		{
			AfterPhysicsUpdateEntity(child, wrld, timeslice, framecount);
		}
	}

	return !th->IsFlagSet(TFL_REMOVE);
  */
}

} //enf
/*
//-----------------------------------------------------------------------
void FlushRemovedEntities(Landscape* wrl, Array<Object*>& ents)
{
  RlsAssert2(0, "not implemented");
  
	for(unsigned int n = 0; n < ents.Size(); n++)
	{
		Object* ent = ents[n];
		wrl->RemoveEntity(ent);
		g_pScript->FreeInstance(ent);
	}
	ents.Clear();
  
}

//-----------------------------------------------------------------------
void Landscape::UpdateEntities()
{
  RlsAssert2(0, "not implemented");
  
	//FlushRemovedEntities(this, _removedEntities);

	//float timeslice = (float)GetTimeSlice();
	//uint framecount = GetFrameCount();

	//Object* wrldent = GetWorldEntity();

	//ENF_BEGIN_PROFILE(PROFILE_UPDATEENTITY_BEFORE);

	//PrepareActiveWindEmitters();

	//for(uint n = 0; n < m_ActiveEntities.GetCardinality(); n++)
	//{
	//	Object* th = m_ActiveEntities[n];
	//	if(n < (m_ActiveEntities.GetCardinality() - 1))
	//		ENF_PREFETCH(m_ActiveEntities[n + 1]);

	//	//skip removed
	//	if(th->IsFlagSet(TFL_REMOVE))
	//		continue;

	//	if(ENF_DIAG_BOOL(DM_SHOW_ACTIVE))
	//	{
	//		Shape::CreateBox(th->GetBBox().GetMins(), th->GetBBox().GetMaxs(), 0x1f4f0000, SS_NOZWRITE|SS_TRANSP|SS_ONCE);
	//	}

	//	th->Update();
	//}
	//FlushRemovedEntities(this, RemovedEntities);
	//ENF_END_PROFILE();
  
}

//-----------------------------------------------------------------------
void Landscape::InternalUpdateEntities()
{
  RlsAssert2(0, "not implemented");
  
	//if(!ENF_DIAG_RBOOL(DM_GAMEUPDATE))
	//	return;

	//ENF_BEGIN_PROFILE(PROFILE_UPDATEENTITIES);

	//FlushRemovedEntities(this, RemovedEntities);

	//float timeslice = (float)GetTimeSlice();
	//uint framecount = GetFrameCount();

	//g_pParticleSystem->BeginSimulation(this);

	////sparse updated entities (EV_TICK)
	//ENF_BEGIN_PROFILE(PROFILE_UPDATEENTITY_TICK);
	//for(uint n = 0; n < m_EntitiesToTick.GetCardinality(); n++)
	//{
	//	EntityToTick& et = m_EntitiesToTick[n];

	//	if(et.m_pEntity->IsFlagSet(TFL_REMOVE))
	//		continue;

	//	float time = et.m_fActualTime + timeslice;
	//	et.m_fActualTime = time;

	//	if(time < et.m_fPeriod)
	//		continue;

	//	double integer;

	//	modf(time / et.m_fPeriod, &integer);
	//	time -= et.m_fPeriod * integer;
	//	et.m_fActualTime = time;
	//	et.m_pEntity->ThrowEvent(m_peWorldEntity, EV_TICK, (void*)framecount);
	//}
	//ENF_END_PROFILE();

	//static Array<Object*> entitiestoupdate;
	//static FlushableObjectAgent< Array<Object*> > agent(&entitiestoupdate);

	//entitiestoupdate.Clear();

	//Object* wrldent = GetWorldEntity();

	//if(ENF_PROFILER_IS_ENABLED() && !m_ActiveEntities.IsEmpty())
	//	Timer::PushTime();

	//static int64 topTicks = 0;
	//static const Object* topEntity = NULL;
	//static SString<> topName, topObject;
	//const Object* lastEntity = topEntity;

	//ENF_BEGIN_PROFILE(PROFILE_UPDATEENTITY_BEFORE);
	//for(uint n = 0; n < m_ActiveEntities.GetCardinality(); n++)
	//{
	//	Object* th = m_ActiveEntities[n];
	//	if(n < (m_ActiveEntities.GetCardinality() - 1))
	//		ENF_PREFETCH(m_ActiveEntities[n + 1]);

	//	//skip removed
	//	if(th->IsFlagSet(TFL_REMOVE))
	//		continue;
	//	
	//	if(ENF_DIAG_BOOL(DM_SHOW_ACTIVE))
	//	{
	//		Shape::CreateBox(th->GetBBox().GetMins(), th->GetBBox().GetMaxs(), 0x1f4f0000, SS_NOZWRITE|SS_TRANSP|SS_ONCE);
	//	}

	//	PhysicsController* pc = th->GetPhysics();
	//	if(pc)
	//		ENF_PREFETCH(pc);
	//	
	//	VObjectInstance* vi = th->GetInstance();
	//	if(vi)
	//		ENF_PREFETCH(vi);

	//	if(th->GetPhysics())
	//	{
	//		if(PhysicsUpdateEntity(th, wrldent, timeslice, framecount))
	//		{
	//			if(((th->GetPhysics() && th->GetPhysics()->IsActive()) || (th->GetFlags() & TFL_UPDATE) != 0 || (th->GetEventMask() & EV_POSTFRAME) != 0) && th != wrldent)
	//				entitiestoupdate.Insert(th);
	//		}
	//	}
	//	else
	//	{
	//		th->Update();
	//		if(PhysicsUpdateEntity(th, wrldent, timeslice, framecount))
	//		{
	//			if((th->GetEventMask() & EV_POSTFRAME) != 0 && th != wrldent)
	//				entitiestoupdate.Insert(th);
	//			else
	//				th->Update();
	//		}
	//	}
	//
	//	if(ENF_PROFILER_IS_ENABLED())
	//	{
	//		int64 ticks;
	//		if(n < (m_ActiveEntities.GetCardinality() - 1))
	//			ticks = Timer::PopAndPushTick();
	//		else
	//			ticks = Timer::PopTick();
	//		if(ticks > topTicks)
	//		{
	//			topTicks = ticks;
	//			topEntity = th;
	//		}
	//	}
	//}
	//ENF_END_PROFILE();

	//if(ENF_PROFILER_IS_ENABLED())
	//{
	//	if(lastEntity != topEntity)
	//	{
	//		topName = topEntity->GetClass()->GetName();
	//		if(topEntity->GetInstance())
	//			topObject = topEntity->GetInstance()->GetResourceObject()->GetName();
	//		else
	//			topObject = "";
	//		SString<> str;
	//		str = topName.cstr();
	//		str += "/";
	//		str += topObject.cstr();
	//	//	ENF_PROFILE_SET_NAME(PROFILE_SLOWENTITY, topName.cstr());
	//	}

	//	//ENF_PROFILE_ADD_TICKS(PROFILE_SLOWENTITY, topTicks);
	//}


	//if(ENF_DIAG_RBOOL(DM_PHYSICS) && GetPhysics())
	//{
	//	GetPhysics()->Advance(timeslice);
	//}

	//if(wrldent)
	//{
	//	wrldent->Update();
	//	wrldent->ThrowEvent(wrldent, EV_POSTFRAME, (void*)framecount);
	//}

	//ENF_BEGIN_PROFILE(PROFILE_UPDATEENTITY_AFTER);
	//g_pParticleSystem->EndSimulation(this);
	//for(uint n = 0; n < entitiestoupdate.GetCardinality(); n++)
	//{
	//	Object* ent = entitiestoupdate[n];
	//	if(n < (entitiestoupdate.GetCardinality() - 1))
	//		ENF_PREFETCH(entitiestoupdate[n + 1]);

	//	if(ent->IsFlagSet(TFL_REMOVE) || ent->GetParent() != NULL)
	//		continue;

	//	ent->Update();
	//	ent->ThrowEvent(wrldent, EV_POSTFRAME, (void*)framecount);
	//}

	//FlushRemovedEntities(this, RemovedEntities);

	//if(ENF_DIAG_VALUE(DM_FORCE_UPDATE) || ENF_DIAG_VALUE(DM_FORCE_VISIBLE))
	//{
	//	bool forceUpdate = ENF_DIAG_VALUE(DM_FORCE_UPDATE) != 0;
	//	bool forceVisible = ENF_DIAG_VALUE(DM_FORCE_VISIBLE) != 0;
	//	forceUpdate |= forceVisible;

	//	CLink* start = &EntityChain;

	//	for(CLink* link = start->Next; link != start; link = link->Next)
	//	{
	//		Object* ent = Object::FromEntityChain(link);

	//		if(forceVisible)
	//			ent->SetFlags(TFL_VISIBLE);

	//		if(forceUpdate)
	//			ent->Update();
	//	}
	//}

	//ENF_END_PROFILE();
	//ENF_END_PROFILE();
}
*/