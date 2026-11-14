/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


#include "btContactConstraint.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "LinearMath/btVector3.h"
#include "btJacobianEntry.h"
#include "btContactSolverInfo.h"
#include "LinearMath/btMinMax.h"
#include "BulletCollision/NarrowPhaseCollision/btManifoldPoint.h"

#define ASSERT2 btAssert

#define USE_INTERNAL_APPLY_IMPULSE 1



btContactConstraint::btContactConstraint(btPersistentManifold* contactManifold,btRigidBody& rbA,btRigidBody& rbB)
:btTypedConstraint(CONTACT_CONSTRAINT_TYPE,rbA,rbB),
	m_contactManifold(*contactManifold)
{

}

btContactConstraint::~btContactConstraint()
{

}

void	btContactConstraint::setContactManifold(btPersistentManifold* contactManifold)
{
	m_contactManifold = *contactManifold;
}

void btContactConstraint::getInfo1 (btConstraintInfo1* info)
{

}

void btContactConstraint::getInfo2 (btConstraintInfo2* info)
{

}

void	btContactConstraint::buildJacobian()
{

}




