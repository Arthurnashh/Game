#ifdef _MSC_VER
#pragma once
#endif

#ifndef __MARINASIM_HPP__
#define __MARINASIM_HPP__

// includes

#include <PhysicalLibs/stat.hpp>
#include <Marina/src/marina.hpp>
#include <Poseidon/lib/thingrb.hpp>
#include <Poseidon/lib/landscaperb.hpp>
#include <Poseidon/lib/AI/ai.hpp>


class MarinaSim : SimulationRB
{
protected:
  Stat _counter;

public:
  virtual bool Init();
  virtual bool AddBody(Ref<RigidBodyObject> body, LODShape& shape);
  virtual bool AddLandscape(const Landscape& land);

  //virtual BreakableFixedJointRB * CreateBreakableFixedJoint(Ref<RigidBodyObject> body0, Ref<RigidBodyObject> body1) {return NULL;};

  virtual bool Simulate(float time);
  virtual void DeInit();
};

#endif