/*
 * Copyright (c) 2005 Erwin Coumans http://continuousphysics.com/Bullet/
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies.
 * Erwin Coumans makes no representations about the suitability 
 * of this software for any purpose.  
 * It is provided "as is" without express or implied warranty. 
*/

#include "LinearMath/btVector3.h"
#include "btRaycastVehicle.h"

#include "BulletDynamics/ConstraintSolver/btSolve2LinearConstraint.h"
#include "BulletDynamics/ConstraintSolver/btJacobianEntry.h"
#include "LinearMath/btQuaternion.h"
#include "BulletDynamics/Dynamics/btDynamicsWorld.h"
#include "btVehicleRaycaster.h"
#include "btWheelInfo.h"
#include "LinearMath/btMinMax.h"
#include "LinearMath/btIDebugDraw.h"
#include "BulletDynamics/ConstraintSolver/btContactConstraint.h"

//btIDebugDraw* g_debugDrawer = NULL;

btRigidBody& btActionInterface::getFixedBody()
{
	static btRigidBody s_fixed(0, 0,0);
	s_fixed.setMassProps(btScalar(0.),btVector3(btScalar(0.),btScalar(0.),btScalar(0.)));
	return s_fixed;
}

btRaycastVehicle::btRaycastVehicle(btRigidBody* chassis,	btVehicleRaycaster* raycaster )
:m_vehicleRaycaster(raycaster),
m_pitchControl(btScalar(0.))
{
	m_chassisBody = chassis;
	m_indexRightAxis = 0;
	m_indexUpAxis = 2;
	m_indexForwardAxis = 1;
	m_waterLevel = -FLT_MAX;
	m_waterMaterial = NULL;
	m_waterObject = NULL;
	defaultInit();
}


void btRaycastVehicle::defaultInit()
{
	m_currentVehicleSpeedKmHour = btScalar(0.);
	m_steeringValue = btScalar(0.);
	
}

	

btRaycastVehicle::~btRaycastVehicle()
{
}


//
// basically most of the code is general for 2 or 4 wheel vehicles, but some of it needs to be reviewed
//
btWheelInfo&	btRaycastVehicle::addWheel( const btVector3& connectionPointCS, const btVector3& wheelDirectionCS0,const btVector3& wheelAxleCS, btScalar suspensionRestLength, btScalar wheelRadius,const btVehicleTuning& tuning, bool isFrontWheel)
{

	btWheelInfoConstructionInfo ci;

	ci.m_chassisConnectionCS = connectionPointCS;
	ci.m_wheelDirectionCS = wheelDirectionCS0;
	ci.m_wheelAxleCS = wheelAxleCS;
	ci.m_suspensionRestLength = suspensionRestLength;
	ci.m_wheelRadius = wheelRadius;
	ci.m_suspensionStiffness = tuning.m_suspensionStiffness;
	ci.m_wheelsDampingCompression = tuning.m_suspensionCompression;
	ci.m_wheelsDampingRelaxation = tuning.m_suspensionDamping;
	ci.m_btMaterial = tuning.m_btMaterial;
	ci.m_bIsFrontWheel = isFrontWheel;
	ci.m_maxSuspensionTravel = tuning.m_maxSuspensionTravel;
	ci.m_maxSuspensionForce = tuning.m_maxSuspensionForce;

	m_wheelInfo.push_back( btWheelInfo(ci));
	updateWheelTransform(getNumWheels() - 1);
	return m_wheelInfo[getNumWheels() - 1];
}

void	btRaycastVehicle::updateWheelTransform( int wheelIndex)
{
	
	btWheelInfo& wheel = m_wheelInfo[ wheelIndex ];
	updateWheelTransformsWS(wheel);
	btVector3 up = -wheel.m_raycastInfo.m_wheelDirectionWS;
	const btVector3& right = wheel.m_raycastInfo.m_wheelAxleWS;
	btVector3 fwd = up.cross(right);
	fwd = fwd.normalize();

	//rotate around steering over de wheelAxleWS
	btScalar steering = wheel.m_steering;
	
	btMatrix3x3 rotatingMat;
	rotatingMat.setEulerYPR(-steering,-wheel.m_rotation,0);

	btMatrix3x3 basis2;

#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(119);
	vec_float4 vec[3];
	vec[m_indexForwardAxis] = fwd.mVec;
	vec[m_indexRightAxis] = right.mVec;
	vec[m_indexUpAxis] = up.mVec;

	//transpose
	vec_float4 tmp = vec_vmrghw(vec[0], vec[2]);	//ax cx ay cy
	basis2[0] = vec_vmrghw(tmp, vec[1]); //ax bx cx by
	basis2[1] = vec_vmrglw(tmp, vec_sld(vec[1], vec[1], 12));	//ay by cy bz
	basis2[2] = vec_vmrghw(vec_vmrglw(vec[0], vec[2]), vec_sld(vec[1], vec[1],	8)); //az bz cz x
#else
	basis2[0][m_indexForwardAxis] = fwd.getX();
	basis2[1][m_indexForwardAxis] = fwd.getY();
	basis2[2][m_indexForwardAxis] = fwd.getZ();
	basis2[0][m_indexRightAxis] = right.getX();
	basis2[1][m_indexRightAxis] = right.getY();
	basis2[2][m_indexRightAxis] = right.getZ();
	basis2[0][m_indexUpAxis] = up.getX();
	basis2[1][m_indexUpAxis] = up.getY();
	basis2[2][m_indexUpAxis] = up.getZ();
#endif

	wheel.m_worldTransform.setBasis(basis2 * rotatingMat);
	wheel.m_worldTransform.setOrigin(
		wheel.m_raycastInfo.m_hardPointWS + wheel.m_raycastInfo.m_wheelDirectionWS * wheel.m_raycastInfo.m_suspensionLength
	);
/*
	if(g_debugDrawer)
	{
		btVector3 org = wheel.m_raycastInfo.m_hardPointWS + wheel.m_raycastInfo.m_wheelDirectionWS * wheel.m_raycastInfo.m_suspensionLength;

		btVector3 fwd(wheel.m_worldTransform.getBasis()[0][m_indexForwardAxis],
							wheel.m_worldTransform.getBasis()[1][m_indexForwardAxis],
							wheel.m_worldTransform.getBasis()[2][m_indexForwardAxis]);
		btVector3 right(wheel.m_worldTransform.getBasis()[0][m_indexRightAxis],
							wheel.m_worldTransform.getBasis()[1][m_indexRightAxis],
							wheel.m_worldTransform.getBasis()[2][m_indexRightAxis]);
		btVector3 up(wheel.m_worldTransform.getBasis()[0][m_indexUpAxis],
							wheel.m_worldTransform.getBasis()[1][m_indexUpAxis],
							wheel.m_worldTransform.getBasis()[2][m_indexUpAxis]);

		g_debugDrawer->drawLine(org, org + fwd * 1.95, btVector3(1,0,0));
		g_debugDrawer->drawLine(org, org + right * 1.95, btVector3(0,1,0));
		g_debugDrawer->drawLine(org, org + up * 1.95, btVector3(0,0,1));
	}*/
}

void btRaycastVehicle::resetSuspension()
{

	int i;
	for (i=0;i<m_wheelInfo.size();	i++)
	{
			btWheelInfo& wheel = m_wheelInfo[i];
			wheel.m_suspensionRelativeVelocity = btScalar(0.0);
			//wheel_info.setContactFriction(btScalar(0.0));
			wheel.m_clippedInvContactDotSuspension = btScalar(1.0);

			wheel.m_raycastInfo.m_suspensionLength = wheel.getSuspensionRestLength();
			wheel.m_raycastInfo.m_fraction = btScalar(1.0f);			
			wheel.m_raycastInfo.m_contactNormalWS = - wheel.m_raycastInfo.m_wheelDirectionWS;
	}
}

void	btRaycastVehicle::updateWheelTransformsWS(btWheelInfo& wheel)
{
	wheel.m_raycastInfo.m_isInContact = WC_NOCONTACT;

	btTransform chassisTrans = getChassisWorldTransform();

	wheel.m_raycastInfo.m_hardPointWS = chassisTrans( wheel.m_chassisConnectionPointCS );
	wheel.m_raycastInfo.m_wheelDirectionWS = chassisTrans.getBasis() *  wheel.m_wheelDirectionCS ;
	wheel.m_raycastInfo.m_wheelAxleWS = chassisTrans.getBasis() * wheel.m_wheelAxleCS;
}

btScalar btRaycastVehicle::rayCast(btWheelInfo& wheel)
{
	btScalar raylen = wheel.getSuspensionRestLength() + wheel.m_wheelsRadius;

	btVector3 rayvector = wheel.m_raycastInfo.m_wheelDirectionWS * (raylen);
	const btVector3& source = wheel.m_raycastInfo.m_hardPointWS;
	btVector3 target = source + rayvector;

	btVehicleRaycaster::btVehicleRaycasterResult	rayResults;

	btAssert(m_vehicleRaycaster);

	btCollisionObject* object = m_vehicleRaycaster->castRay(getRigidBody(), source, target, rayResults);

	btScalar param = rayResults.m_distFraction;
	wheel.m_raycastInfo.m_groundObject = object;

	WheelContact type = object ? WC_SOLID_GROUND : WC_NOCONTACT;

	btScalar wheelZ = target[m_indexUpAxis];

	//Optimize. Don't ask for water level, if we are high above surface
	if(wheelZ <= (m_waterLevel + 1.0f)) //HACK! get it from parameter
	{
#ifdef BT_PS3PPU_VEC
		//btVector3::btHitPrint(120);
		//GetWaterLevel returns 0.f...no need to solve it
		btScalar waterLevel = m_waterLevel;// + m_vehicleRaycaster->GetWaterLevel(source.getX(), source.getY());

		if(wheelZ < waterLevel)
		{
			btVector3 t = vec_sub(source.mVec, target.mVec);
			float *tmp = btVector3(vec_vmrglw(t.mVec, source.mVec)).m_floats;
			
			btScalar t0 = tmp[1] - waterLevel;
			btScalar waterParam = t0 / tmp[0];
			btClamp(waterParam, btScalar(0.0f), btScalar(1.0f));

			if(type == WC_NOCONTACT || waterParam < param)
			{
				param = waterParam;
				type = WC_WATER;
			}
		}
#else
		btScalar waterLevel = m_waterLevel + m_vehicleRaycaster->GetWaterLevel(source.getX(), source.getY());

		if(wheelZ < waterLevel)
		{
			btScalar t = source.getZ() - target.getZ();
			btScalar t0 = source.getZ() - waterLevel;
			btScalar waterParam = t0 / t;
			btClamp(waterParam, btScalar(0.0f), btScalar(1.0f));

			if(type == WC_NOCONTACT || waterParam < param)
			{
				param = waterParam;
				type = WC_WATER;
			}
		}
#endif
	}

	btScalar hitDistance = raylen * param;

	if (type == WC_SOLID_GROUND)
	{
		wheel.m_raycastInfo.m_contactNormalWS  = rayResults.m_hitNormalInWorld;
		wheel.m_raycastInfo.m_isInContact = WC_SOLID_GROUND;		
		wheel.m_raycastInfo.m_material = rayResults.m_material;

		wheel.m_raycastInfo.m_suspensionLength = hitDistance - wheel.m_wheelsRadius;
		wheel.m_raycastInfo.m_fraction = param;
		//clamp on max suspension travel

		btScalar minSuspensionLength = wheel.getSuspensionRestLength() - wheel.m_maxSuspensionTravel;
		btScalar maxSuspensionLength = wheel.getSuspensionRestLength() + wheel.m_maxSuspensionTravel;
		btClamp(wheel.m_raycastInfo.m_suspensionLength, minSuspensionLength, maxSuspensionLength);

		wheel.m_raycastInfo.m_contactPointWS = rayResults.m_hitPointInWorld;

		btScalar denominator = wheel.m_raycastInfo.m_contactNormalWS.dot( wheel.m_raycastInfo.m_wheelDirectionWS );

		btVector3 relpos = wheel.m_raycastInfo.m_contactPointWS - getRigidBody()->getCenterOfMassPosition();
		btScalar projVel = wheel.m_raycastInfo.m_contactNormalWS.dot( getRigidBody()->getVelocityInLocalPoint(relpos) );

		if ( denominator >= btScalar(-0.1))
		{
			wheel.m_suspensionRelativeVelocity = btScalar(0.0);
			wheel.m_clippedInvContactDotSuspension = btScalar(1.0) / btScalar(0.1);
		}
		else
		{
			btScalar inv = btScalar(-1.) / denominator;
			wheel.m_suspensionRelativeVelocity = projVel * inv;
			wheel.m_clippedInvContactDotSuspension = inv;
		}
			
	}
	else if(type == WC_WATER)
	{
#ifdef BT_PS3PPU_VEC
		wheel.m_raycastInfo.m_contactNormalWS = btVector3(btIdentVec2);
#else
		wheel.m_raycastInfo.m_contactNormalWS = btVector3(0.0f, 0.0f, 1.0f);
#endif
		wheel.m_raycastInfo.m_isInContact = WC_WATER;
		wheel.m_raycastInfo.m_groundObject = m_waterObject;
		wheel.m_raycastInfo.m_material = m_waterMaterial;

		wheel.m_raycastInfo.m_suspensionLength = hitDistance - wheel.m_wheelsRadius;
		wheel.m_raycastInfo.m_fraction = param;
		
		//clamp on max suspension travel
		btScalar minSuspensionLength = wheel.getSuspensionRestLength() - wheel.m_maxSuspensionTravel;
		btScalar maxSuspensionLength = wheel.getSuspensionRestLength() + wheel.m_maxSuspensionTravel;
		btClamp(wheel.m_raycastInfo.m_suspensionLength, minSuspensionLength, maxSuspensionLength);

		wheel.m_raycastInfo.m_contactPointWS = (target - source) * param + source;

		btScalar denominator = wheel.m_raycastInfo.m_contactNormalWS.dot( wheel.m_raycastInfo.m_wheelDirectionWS );

		btVector3 relpos = wheel.m_raycastInfo.m_contactPointWS - getRigidBody()->getCenterOfMassPosition();
		btScalar projVel = wheel.m_raycastInfo.m_contactNormalWS.dot( getRigidBody()->getVelocityInLocalPoint(relpos) );

		if ( denominator >= btScalar(-0.1))
		{
			wheel.m_suspensionRelativeVelocity = btScalar(0.0);
			wheel.m_clippedInvContactDotSuspension = btScalar(1.0) / btScalar(0.1);
		}
		else
		{
			btScalar inv = btScalar(-1.) / denominator;
			wheel.m_suspensionRelativeVelocity = projVel * inv;
			wheel.m_clippedInvContactDotSuspension = inv;
		}
	}
	else
	{
		//put wheel info as in rest position
		wheel.m_raycastInfo.m_suspensionLength = wheel.getSuspensionRestLength();
		wheel.m_raycastInfo.m_fraction = btScalar(1.0f);
		wheel.m_suspensionRelativeVelocity = btScalar(0.0);
		wheel.m_raycastInfo.m_contactNormalWS = - wheel.m_raycastInfo.m_wheelDirectionWS;
		wheel.m_clippedInvContactDotSuspension = btScalar(1.0);
		hitDistance = -1;
	}

	return hitDistance;
}


const btTransform& btRaycastVehicle::getChassisWorldTransform() const
{
	return getRigidBody()->getCenterOfMassTransform();
}


inline void addBounds(btVector3& mins, btVector3& maxs, const btVector3& p)
{
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(121);
	mins.mVec = vec_sel(mins.mVec, p.mVec, vec_cmpgt(mins.mVec, p.mVec));
	maxs.mVec = vec_sel(maxs.mVec, p.mVec, vec_cmplt(maxs.mVec, p.mVec));
#else
	for (int i=0;i<3;i++)
	{
		if (mins[i] > p[i])
		{
			mins[i] = p[i];
		}
		if (maxs[i] < p[i])
		{
			maxs[i] = p[i];
		}
	}
#endif
}

void btRaycastVehicle::updateVehicle( btScalar step )
{
	//return;
	m_currentVehicleSpeedKmHour = btScalar(3.6) * getRigidBody()->getLinearVelocity().length();
	
	const btTransform& chassisTrans = getChassisWorldTransform();
	
#ifdef BT_PS3PPU_VEC
	//btVector3::btHitPrint(122);
	btVector3 forwardW = chassisTrans.getBasis().getColumn(m_indexForwardAxis);
#else
	btVector3 forwardW (
		chassisTrans.getBasis()[0][m_indexForwardAxis],
		chassisTrans.getBasis()[1][m_indexForwardAxis],
		chassisTrans.getBasis()[2][m_indexForwardAxis]);
#endif

	if (forwardW.dot(getRigidBody()->getLinearVelocity()) < btScalar(0.))
	{
		m_currentVehicleSpeedKmHour *= btScalar(-1.);
	}

	bool wakeUp = getRigidBody()->isActive();
	for (int i = 0; i < m_wheelInfo.size(); i++)
	{
		if(!wakeUp && BT_FABS(m_wheelInfo[i].m_engineForce) > 1.0f)
		{
			wakeUp = true;
			getRigidBody()->activate();
		}
	}

	for (int i = 0; i < m_wheelInfo.size(); i++)
		m_wheelInfo[i].m_raycastInfo.m_groundObject = NULL;

	if(!getRigidBody()->isActive())
	{
		return;
	}

	btVector3 castMins(FLT_MAX, FLT_MAX, FLT_MAX), castMaxs(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (int i = 0; i < m_wheelInfo.size(); i++)
	{
		updateWheelTransform(i);
		btWheelInfo& wheel = m_wheelInfo[i];
		btScalar raylen = wheel.getSuspensionRestLength() + wheel.m_wheelsRadius;
		btVector3 rayvector = wheel.m_raycastInfo.m_wheelDirectionWS * (raylen);
		const btVector3& source = wheel.m_raycastInfo.m_hardPointWS;
		btVector3 target = source + rayvector;
		wheel.m_raycastInfo.m_contactPointWS = target;
		addBounds(castMins, castMaxs, source);
		addBounds(castMins, castMaxs, target);
	}

	m_vehicleRaycaster->prepare(getRigidBody(), castMins, castMaxs);
	for (int i = 0; i < m_wheelInfo.size(); i++)
	{
		rayCast(m_wheelInfo[i]);
	}

	//
	// simulate suspension
	//

	btVector3 chassisCOM = getRigidBody()->getCenterOfMassPosition();
	btScalar chassisMass = btScalar(1.) / m_chassisBody->getInvMass();

	for (int i = 0; i < m_wheelInfo.size(); i++)
	{
		//apply suspension force
		btWheelInfo& wheel = m_wheelInfo[i];

		btScalar suspensionForce = 0;
		wheel.m_wheelsSuspensionForce = 0;

		if ( wheel.m_raycastInfo.m_isInContact != WC_NOCONTACT)
		{
			//	Spring
			btScalar	susp_length = wheel.getSuspensionRestLength();
			btScalar	current_length = wheel.m_raycastInfo.m_suspensionLength;
			btScalar length_diff = (susp_length - current_length);

			if(BT_FABS(length_diff) > 0.01f)
			{
				btScalar force = wheel.m_suspensionStiffness
					* length_diff * wheel.m_clippedInvContactDotSuspension;
			
				// Damper
				btScalar projected_rel_vel = wheel.m_suspensionRelativeVelocity;
				btScalar	susp_damping = ( projected_rel_vel < btScalar(0.0) ) ? wheel.m_wheelsDampingCompression : wheel.m_wheelsDampingRelaxation;
				susp_damping *= ((wheel.m_raycastInfo.m_isInContact == WC_WATER) ? 0.75f : 1.0f);

				force -= susp_damping * projected_rel_vel;

				// RESULT
				suspensionForce = btMax(force * chassisMass, btScalar(0.0f));
				wheel.m_wheelsSuspensionForce = suspensionForce;
				btScalar maxSuspForce = wheel.m_maxSuspensionForce * ((wheel.m_raycastInfo.m_isInContact == WC_WATER) ? 0.85f : 1.0f);
				suspensionForce = btMin(suspensionForce, maxSuspForce);

				btVector3 impulse = wheel.m_raycastInfo.m_contactNormalWS * suspensionForce * step;
				btVector3 relpos = wheel.m_raycastInfo.m_contactPointWS - chassisCOM;
				
				getRigidBody()->applyImpulse(impulse, relpos);
			}
		}
	}

	updateFriction( step);

	for (int i = 0; i < m_wheelInfo.size(); i++)
	{
		btWheelInfo& wheel = m_wheelInfo[i];
		btVector3 relpos = wheel.m_raycastInfo.m_hardPointWS - chassisCOM;
		btVector3 vel = getRigidBody()->getVelocityInLocalPoint( relpos );

		btScalar deltaRotation = 0.0f;
		if (wheel.m_raycastInfo.m_isInContact != WC_NOCONTACT)
		{
			const btTransform&	chassisWorldTransform = getChassisWorldTransform();

#ifdef BT_PS3PPU_VEC
			//btVector3::btHitPrint(123);
			btVector3 fwd(chassisWorldTransform.getBasis().getColumn(m_indexForwardAxis));
#else
			btVector3 fwd (
				chassisWorldTransform.getBasis()[0][m_indexForwardAxis],
				chassisWorldTransform.getBasis()[1][m_indexForwardAxis],
				chassisWorldTransform.getBasis()[2][m_indexForwardAxis]);
#endif
			btScalar proj = fwd.dot(wheel.m_raycastInfo.m_contactNormalWS);
			fwd -= wheel.m_raycastInfo.m_contactNormalWS * proj;

			btScalar proj2 = fwd.dot(vel);
			
			deltaRotation = (proj2 * step) / (wheel.m_wheelsRadius);
		}

		btScalar deltaRotation2 = wheel.m_engineForce * step  / wheel.m_wheelsRadius * 0.005f;

		if(deltaRotation2 > 0)
		{
			if(deltaRotation2 > deltaRotation)
				wheel.m_deltaRotation = (deltaRotation + deltaRotation2) * 0.5f;
			else
				wheel.m_deltaRotation = deltaRotation;
		}
		else
		{
			if(deltaRotation2 < deltaRotation)
				wheel.m_deltaRotation = (deltaRotation + deltaRotation2) * 0.5f;
			else
				wheel.m_deltaRotation = deltaRotation;
		}

		wheel.m_rotation += wheel.m_deltaRotation;

		wheel.m_deltaRotation *= btScalar(0.999);//damping of rotation when not in contact
	}
}


void	btRaycastVehicle::setSteeringValue(btScalar steering,int wheel)
{
	btWheelInfo& wheelInfo = getWheelInfo(wheel);
	wheelInfo.m_steering = steering;
}



btScalar	btRaycastVehicle::getSteeringValue(int wheel) const
{
	return getWheelInfo(wheel).m_steering;
}


void	btRaycastVehicle::applyEngineForce(btScalar force, int wheel)
{
	btWheelInfo& wheelInfo = getWheelInfo(wheel);
	wheelInfo.m_engineForce = force;
}


const btWheelInfo&	btRaycastVehicle::getWheelInfo(int index) const
{
	btAssert((index >= 0) && (index < 	getNumWheels()));
	
	return m_wheelInfo[index];
}

btWheelInfo&	btRaycastVehicle::getWheelInfo(int index) 
{
	btAssert((index >= 0) && (index < 	getNumWheels()));
	
	return m_wheelInfo[index];
}

void btRaycastVehicle::setBrake(btScalar brake,int wheelIndex)
{
	btAssert((wheelIndex >= 0) && (wheelIndex < 	getNumWheels()));
	getWheelInfo(wheelIndex).m_brake = brake;
}

struct btWheelContactPoint
{
	btRigidBody* m_body0;
	btCollisionObject* m_body1;
	btVector3	m_frictionPositionWorld;
	btVector3	m_frictionDirectionWorld;
	btScalar	m_jacDiagABInv;
	btScalar	m_maxImpulse;


	btWheelContactPoint(btRigidBody* body0,btCollisionObject* body1,const btVector3& frictionPosWorld,const btVector3& frictionDirectionWorld, btScalar maxImpulse)
		:m_body0(body0),
		m_body1(body1),
		m_frictionPositionWorld(frictionPosWorld),
		m_frictionDirectionWorld(frictionDirectionWorld),
		m_maxImpulse(maxImpulse)
	{
		btScalar denom0 = body0->computeImpulseDenominator(frictionPosWorld,frictionDirectionWorld);
		btScalar denom1 = 0;
		btRigidBody* rbody1 = btRigidBody::upcast(body1);
		if(rbody1)
			denom1 = rbody1->computeImpulseDenominator(frictionPosWorld,frictionDirectionWorld);
		btScalar	relaxation = 1.f;
		m_jacDiagABInv = relaxation/(denom0+denom1);
	}



};

btScalar calcRollingFriction(btWheelContactPoint& contactPoint)
{

	btScalar j1=0.f;

	const btVector3& contactPosWorld = contactPoint.m_frictionPositionWorld;

	btVector3 rel_pos1 = contactPosWorld - contactPoint.m_body0->getCenterOfMassPosition(); 
	btVector3 rel_pos2 = contactPosWorld - contactPoint.m_body1->getCenterOfMassPosition();
	
	btScalar maxImpulse  = contactPoint.m_maxImpulse;
	
	btVector3 vel1 = contactPoint.m_body0->getVelocityInLocalPoint(rel_pos1);
	btVector3 vel = vel1;

	btRigidBody* rbody1 = btRigidBody::upcast(contactPoint.m_body1);
	if(rbody1)
		vel -= rbody1->getVelocityInLocalPoint(rel_pos2);

	btScalar vrel = contactPoint.m_frictionDirectionWorld.dot(vel);

	// calculate j that moves us to zero relative velocity
	j1 = -vrel * contactPoint.m_jacDiagABInv;
	btSetMin(j1, maxImpulse);
	btSetMax(j1, -maxImpulse);

	return j1;
}

btScalar sideFrictionStiffness2 = btScalar(1.0);

///resolveSingleBilateral is an obsolete methods used for vehicle friction between two dynamic objects
void resolveSingleBilateral(btRigidBody* body1, const btMatrix3x3& invOrientation1, const btVector3& rel_pos1,
                      btCollisionObject* body2, const btVector3& rel_pos2,
                      const btVector3& normal,btScalar& impulse)
{
	btScalar normalLenSqr = normal.length2();
	btAssert(btFabs(normalLenSqr) < btScalar(1.1));
	if (normalLenSqr > btScalar(1.1))
	{
		impulse = btScalar(0.);
		return;
	}
	//this jacobian entry could be re-used for all iterations

	btRigidBody* rbody2 = btRigidBody::upcast(body2);

	btVector3 vel;
	btVector3 invInertiaDiagLocal2;
	btVector3 linearVelocity2;
	btVector3 angularVelocity2;
	float invMass2 = 0;

	if(rbody2)
	{
		vel = body1->getVelocityInLocalPoint(rel_pos1) - rbody2->getVelocityInLocalPoint(rel_pos2);
		invMass2 = rbody2->getInvMass();
		invInertiaDiagLocal2 = rbody2->getInvInertiaDiagLocal();
		linearVelocity2 = rbody2->getLinearVelocity();
		angularVelocity2 = rbody2->getAngularVelocity();
	}
	else
	{
		vel = body1->getVelocityInLocalPoint(rel_pos1);
		invInertiaDiagLocal2.setZero();
		linearVelocity2.setZero();
		angularVelocity2.setZero();
	}

	btMatrix3x3 invOrientation2 = body2->getCenterOfMassTransform().getBasis().transpose();
	btJacobianEntry jac(invOrientation1,
								invOrientation2,
								rel_pos1,rel_pos2,normal,body1->getInvInertiaDiagLocal(),body1->getInvMass(),
								invInertiaDiagLocal2,invMass2);

	btScalar jacDiagAB = jac.getDiagonal();
	btScalar jacDiagABInv = btScalar(1.) / jacDiagAB;

	btScalar rel_vel = jac.getRelativeVelocity(
									body1->getLinearVelocity(),
									invOrientation1 * body1->getAngularVelocity(),
									linearVelocity2,
									invOrientation2 * angularVelocity2); 

	rel_vel = normal.dot(vel);

	//todo: move this into proper structure
	btScalar contactDamping = btScalar(0.2);

#ifdef ONLY_USE_LINEAR_MASS
	btScalar massTerm = btScalar(1.) / (body1->getInvMass() + body2->getInvMass());
	impulse = - contactDamping * rel_vel * massTerm;
#else	
	impulse = -contactDamping * rel_vel * jacDiagABInv;
#endif
}

void	btRaycastVehicle::updateFriction(btScalar	timeStep)
{

	//calculate the impulse, so that the wheels don't move sidewards
	int numWheel = getNumWheels();
	if (!numWheel)
		return;

	bool sliding = false;


	btVector3 chassisCOM = m_chassisBody->getCenterOfMassPosition();
	btMatrix3x3 transOrientation1 = m_chassisBody->getCenterOfMassTransform().getBasis().transpose();


	//collapse all those loops into one!
	for (int i=0;i<numWheel;i++)
	{
		btWheelInfo& wheelInfo = m_wheelInfo[i];

		m_wheelInfo[i].m_skidInfo= btScalar(1.);

		btScalar sideImpulse = btScalar(0.);
		btScalar forwardImpulse = btScalar(0.);

		btCollisionObject* groundObject = wheelInfo.m_raycastInfo.m_groundObject;

		if (groundObject)
		{
			const btTransform& wheelTrans = wheelInfo.m_worldTransform;

			btMatrix3x3 wheelBasis0 = wheelTrans.getBasis();

#ifdef BT_PS3PPU_VEC
			//btVector3::btHitPrint(124);
			btVector3 axle = wheelBasis0.getColumn(m_indexRightAxis);
#else
			btVector3 axle = btVector3(
				wheelBasis0[0][m_indexRightAxis],
				wheelBasis0[1][m_indexRightAxis],
				wheelBasis0[2][m_indexRightAxis]);
#endif
			const btVector3& surfNormalWS = wheelInfo.m_raycastInfo.m_contactNormalWS;
			btScalar proj = axle.dot(surfNormalWS);
			axle -= surfNormalWS * proj;
			axle = axle.normalize();
			
			btVector3 forwardWS = surfNormalWS.cross(axle).normalized();

			btVector3 rel_pos1 = wheelInfo.m_raycastInfo.m_contactPointWS - chassisCOM; 
			btVector3 rel_pos2 = wheelInfo.m_raycastInfo.m_contactPointWS - groundObject->getCenterOfMassPosition();
	
			resolveSingleBilateral(m_chassisBody, transOrientation1, rel_pos1,
										  groundObject, rel_pos2,
										  axle,sideImpulse);

/*			if(g_debugDrawer)
			{
				btVector3 org = wheelInfo.m_raycastInfo.m_hardPointWS + wheelInfo.m_raycastInfo.m_wheelDirectionWS * wheelInfo.m_raycastInfo.m_suspensionLength;

				btVector3 fwd(wheelInfo.m_worldTransform.getBasis()[0][m_indexForwardAxis],
									wheelInfo.m_worldTransform.getBasis()[1][m_indexForwardAxis],
									wheelInfo.m_worldTransform.getBasis()[2][m_indexForwardAxis]);
				btVector3 right(wheelInfo.m_worldTransform.getBasis()[0][m_indexRightAxis],
									wheelInfo.m_worldTransform.getBasis()[1][m_indexRightAxis],
									wheelInfo.m_worldTransform.getBasis()[2][m_indexRightAxis]);
				btVector3 up(wheelInfo.m_worldTransform.getBasis()[0][m_indexUpAxis],
									wheelInfo.m_worldTransform.getBasis()[1][m_indexUpAxis],
									wheelInfo.m_worldTransform.getBasis()[2][m_indexUpAxis]);

				g_debugDrawer->drawLine(org, org + axle * (sideImpulse * 0.5), btVector3(1,0,0));
				//g_debugDrawer->drawLine(org, org + right * 1.95, btVector3(0,1,0));
				//g_debugDrawer->drawLine(org, org + up * 1.95, btVector3(0,0,1));
			}*/

			sideImpulse *= sideFrictionStiffness2;

			//sliding
			//const btScalar sideFactor = btScalar(0.5);
			//const btScalar fwdFactor = btScalar(0.5);
		
			if (wheelInfo.m_engineForce != 0.f)
			{
				forwardImpulse = -wheelInfo.m_engineForce * timeStep;
			}
			else
			{
				btScalar defaultRollingFrictionImpulse = 0.f;
				btScalar maxImpulse = (wheelInfo.m_brake != btScalar(0.f)) ? wheelInfo.m_brake : defaultRollingFrictionImpulse;
				btWheelContactPoint contactPt(m_chassisBody,groundObject,wheelInfo.m_raycastInfo.m_contactPointWS,forwardWS,maxImpulse);
				forwardImpulse = calcRollingFriction(contactPt);
			}

			const btMaterial* wheelMat = wheelInfo.m_btMaterial;

			//switch between active rolling (throttle), braking and non-active rolling friction (no throttle/break)
#ifdef BT_PS3PPU_VEC
			//btVector3::btHitPrint(125);
			
			btScalar combinedFriction = wheelMat->getFriction() * wheelInfo.m_raycastInfo.m_material->getFriction();
			btVector3 frictionXY = wheelMat->getAnisotropicFriction();
			if(wheelMat->hasCombiners())
			{
				const btMaterial* combiner = wheelMat->findCombiner(wheelInfo.m_raycastInfo.m_material);
				if(combiner)
				{
					combinedFriction = combiner->getFriction();
					frictionXY = combiner->getAnisotropicFriction();
				}
			}

			btScalar maximp = wheelInfo.m_wheelsSuspensionForce * timeStep * combinedFriction;
			btScalar maximpSquared = maximp * maximp;
		
			btScalar x = forwardImpulse / frictionXY.getX();
			btScalar y = sideImpulse / frictionXY.getY();

			btScalar impulseSquared = (x*x + y*y);
#else
			btScalar combinedFriction = wheelMat->getFriction() * wheelInfo.m_raycastInfo.m_material->getFriction();
			btScalar frictionX = wheelMat->getAnisotropicFriction().getX();
			btScalar frictionY = wheelMat->getAnisotropicFriction().getY();
			if(wheelMat->hasCombiners())
			{
				const btMaterial* combiner = wheelMat->findCombiner(wheelInfo.m_raycastInfo.m_material);
				if(combiner)
				{
					combinedFriction = combiner->getFriction();
					frictionX = combiner->getAnisotropicFriction().getX();
					frictionY = combiner->getAnisotropicFriction().getY();
				}
			}

			btScalar maximp = wheelInfo.m_wheelsSuspensionForce * timeStep * combinedFriction;
			btScalar maximpSquared = maximp * maximp;
		
			btScalar x = forwardImpulse / frictionX;
			btScalar y = sideImpulse / frictionY;

			btScalar impulseSquared = (x*x + y*y);
#endif	
			if (impulseSquared > maximpSquared)
			{
				btScalar factor = maximp / btSqrt(impulseSquared);

				m_wheelInfo[i].m_skidInfo = factor;
				
				if (sideImpulse != btScalar(0.))
				{
					if (factor < btScalar(1.))
					{
						forwardImpulse *=	factor;
						sideImpulse *= factor;
					}
				}
			}

			// apply the impulses
			if (forwardImpulse != btScalar(0.))
			{
				m_chassisBody->applyImpulse(forwardWS * forwardImpulse, rel_pos1);
			}
			
			if (sideImpulse != btScalar(0.))
			{
				btVector3 sideImp = axle * sideImpulse;

				//rel_pos1[m_indexUpAxis] *= wheelInfo.m_rollInfluence;
				m_chassisBody->applyImpulse(sideImp, rel_pos1);

				btRigidBody* groundRB = btRigidBody::upcast(groundObject);
				if(groundRB)
				{
					//apply friction impulse on the ground
					groundRB->applyImpulse(-sideImp, rel_pos2);
				}
			}
		} 
	}	
}



void	btRaycastVehicle::debugDraw(btIDebugDraw* debugDrawer)
{
//	g_debugDrawer = debugDrawer;

	for (int v=0;v<this->getNumWheels();v++)
	{
		btVector3 wheelColor(0,1,1);
		if (getWheelInfo(v).m_raycastInfo.m_isInContact != WC_NOCONTACT)
		{
			wheelColor.setValue(0,0,1);
		} else
		{
			wheelColor.setValue(1,0,1);
		}

		btVector3 wheelPosWS = getWheelInfo(v).m_worldTransform.getOrigin();

		btVector3 axle = btVector3(	
			getWheelInfo(v).m_worldTransform.getBasis()[0][getRightAxis()],
			getWheelInfo(v).m_worldTransform.getBasis()[1][getRightAxis()],
			getWheelInfo(v).m_worldTransform.getBasis()[2][getRightAxis()]);

		//debug wheels (cylinders)
		debugDrawer->drawLine(wheelPosWS,wheelPosWS+axle,wheelColor);
		debugDrawer->drawLine(wheelPosWS,getWheelInfo(v).m_raycastInfo.m_contactPointWS,wheelColor);

	}
}


class VehicleResultCallback: public	btCollisionWorld::ClosestRayResultCallback
{
public:
	btCollisionObject* m_exclude;

	virtual bool needsCollision(btBroadphaseProxy* proxy0) const
	{
		if(proxy0->m_clientObject == (void*)m_exclude)
			return false;

		return btCollisionWorld::ClosestRayResultCallback::needsCollision(proxy0);
	}
	VehicleResultCallback(btCollisionObject* exclude, const btVector3&	rayFromWorld,const btVector3&	rayToWorld):
		btCollisionWorld::ClosestRayResultCallback(rayFromWorld, rayToWorld),
		m_exclude(exclude)
	{
	}
};

bool VehicleAabbQueryResultCallback::needsCollision(const btBroadphaseProxy* proxy0) const
{
	if(!btCollisionWorld::AabbQueryResultCallback::needsCollision(proxy0))
		return false;
	const btCollisionObject* cobj = static_cast<btCollisionObject*>(proxy0->m_clientObject);
	if(m_exclude == cobj)
		return false;

	return true;
}

void btDefaultVehicleRaycaster::prepare(btCollisionObject* exclude, const btVector3& mins, const btVector3& maxs)
{
	m_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setCurrent();
	m_queryCallback.m_collisionObjects.clear();
	m_queryCallback.m_collisionFilterGroup = btBroadphaseProxy::AllFilter;
	m_queryCallback.m_interactionLayer = exclude->getInteractionLayer();
	m_dynamicsWorld->aabbQuery(mins, maxs, m_queryCallback);
}

btCollisionObject* btDefaultVehicleRaycaster::castRay(btCollisionObject* exclude, const btVector3& from,const btVector3& to, btVehicleRaycasterResult& result)
{
#if 1
	VehicleResultCallback rayCallback(exclude, from,to);
	btTransform rayFromTrans(from);
	btTransform rayToTrans(to);
	rayCallback.m_interactionLayer = exclude->getInteractionLayer();

	for(int n = 0; n < m_queryCallback.m_collisionObjects.size(); n++)
	{
		if (rayCallback.m_closestHitFraction == btScalar(0.f))
			break;

		m_dynamicsWorld->rayTestSingle(rayFromTrans, rayToTrans,
										m_queryCallback.m_collisionObjects[n],
										m_queryCallback.m_collisionObjects[n]->getCollisionShape(),
										m_queryCallback.m_collisionObjects[n]->getWorldTransform(),
										rayCallback,
										rayCallback.m_interactionLayer);
	}

	if(rayCallback.m_collisionObject)
	{
		result.m_distFraction = rayCallback.m_closestHitFraction;
		result.m_hitNormalInWorld = rayCallback.m_hitNormalWorld.normalized();
		result.m_hitPointInWorld = rayCallback.m_hitPointWorld;
		result.m_material = rayCallback.m_material;
		return rayCallback.m_collisionObject;
	}
	return 0;
#else
	VehicleResultCallback rayCallback(exclude, from,to);

	rayCallback.m_collisionFilterGroup = btBroadphaseProxy::AllFilter;
	rayCallback.m_interactionLayer = exclude->getInteractionLayer();

	m_dynamicsWorld->rayTest(from, to, rayCallback);

	if (rayCallback.hasHit())
	{
		
		btCollisionObject* body = rayCallback.m_collisionObject;
      if (body && body->hasContactResponse())
		{
			result.m_hitPointInWorld = rayCallback.m_hitPointWorld;
			result.m_hitNormalInWorld = rayCallback.m_hitNormalWorld.normalized();
			result.m_distFraction = rayCallback.m_closestHitFraction;
			result.m_material = rayCallback.m_material;
			return body;
		}
	}
	return 0;
#endif
}

