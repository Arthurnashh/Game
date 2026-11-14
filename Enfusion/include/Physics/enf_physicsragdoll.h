//-----------------------------------------------------------------------------
// File: enf_physicsragdoll.h
//
// Desc: Bullet ragdoll implementation
//
// Copyright (c) 2000-2014 Bohemia Interactive. All rights reserved
//-----------------------------------------------------------------------------

#ifndef ENF_PHYSICSBULLETRAGDOLL_H
#define ENF_PHYSICSBULLETRAGDOLL_H

#include "enf_config.h"
#include "common/enf_types.h"
//#include "common/enf_mathlib.h"
#include "El/Math/math3d.hpp"
//#include "Es\Memory\memAlloc.hpp"

namespace enf
{
  struct PhysicsMaterial;

/** @addtogroup Bullet
@{*/
//-----------------------------------------------------------------------
class RagdollDef
{
public:
	//-----------------------------------------------------------------------
	struct Geometry
	{
		const char*		Bone1;
		const char*		Bone2;
		float				Radius;
		PhysicsMaterial* Material;
	};

	//-----------------------------------------------------------------------
	struct Bone
	{
		const char*		Name;
		const char*		Parent;
		Vector3			Offset;
		bool				Added;
		float				Mass;
		bool				Simulated;
  };

	//-----------------------------------------------------------------------
	enum JointType
	{
		JOINT_BALL = 1,
		JOINT_HINGE = 2,
		JOINT_FIXED = 3
	};
		
	struct Joint
	{
		JointType		Type;
		const char*		Name;
		Vector3			VAxis;
		float				Angle1, Angle2;
		float				Damp;
	};

private:
friend class BulletWorld;
friend class BulletManager;

	enf::Array<Bone>			Bones;
	enf::Array<Geometry>	Geoms;
	enf::Array<Joint>		Joints;
	const char*			Name;

public:
	ENF_DECLARE_MMOBJECT(RagdollDef);

	const char* GetName()
	{
		return Name;
	}

	const Bone* FindBone(const char* name) const;

	RagdollDef(const char *name);
	~RagdollDef();
};
//@}
}
#endif //ENF_PHYSICSBULLETRAGDOLL_H