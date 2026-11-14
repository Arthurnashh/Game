#include <Poseidon/lib/wpch.hpp>

#if _ENABLE_RIGID_BODIES

#include "MarinaSim.hpp"
#include <Es/Common/global.hpp>
#include <Es/Framework/potime.hpp>
#include <Es/Strings/bstring.hpp>

bool MarinaSim::Init() 
{
  SimulationRB::SetExternalAccel(Vector3(0,-9.8f,0));

  _counter.Init(0.001f,0,100);
  return true;
};

bool MarinaSim::AddBody(Ref<RigidBodyObject> body, LODShape& shape)
{  
  body->_geometry->SetBody(body);

  SimulationRB::AddBody(body);
  return true;
};

bool MarinaSim::AddLandscape(const Landscape& land)
{
  Ref<RigidBodyObject> bodyObject = new RigidBodyObject();
  bodyObject->_static = true;
  bodyObject->_geometry = (IRBGeometry * )dynamic_cast<LandscapeRB *>(const_cast<Landscape *>(&land));
  bodyObject->_physBody = NULL;
  bodyObject->_group = NULL;

  bodyObject->_geometry->SetBody(bodyObject);
  SimulationRB::AddBody(bodyObject);
  return true;
}

bool MarinaSim::Simulate(float time) 
{
  if (time == 0)
    return true;

  SimulationRB::Simulate(time); 

  unsigned64 spendTime = getSystemTime();
  SimulationRB::Simulate(time);    

  spendTime = getSystemTime() - spendTime;
  _counter.AddValue(spendTime/ 1000000.0f / time);

  return true;
}
void MarinaSim::DeInit()
{
  _counter.WriteReport("Marina");
  SimulationRB::Reset();
};

#endif