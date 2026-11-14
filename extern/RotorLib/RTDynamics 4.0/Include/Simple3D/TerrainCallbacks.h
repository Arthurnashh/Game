#pragma once

#include "Simple3D/ShipAnimator.h"

/*
    This class uses Simple3D's terrain intersection capabilities to compute terrain intersection
    queries.
    Every vehicle has a TerrainQueryCallback. Every frame, query method of this callback is
    called to get information about the terrain. Users have to implement this method to provide
    terrain information to RTDynamics vehicles and bots.
*/
class Simple3DTerrainQueryCallback : public RTD::TerrainQueryCallback
{
public:

	Simple3DTerrainQueryCallback( RTD::World* world , Simple3D* s3d ):RTD::TerrainQueryCallback( world )
    {
        m_simple3D = s3d;
    }

    /**
        The vehicle or the bot calls this method periodically with changing start and end vectors. These vectors
        define a linear intersector's start and end points in world coordinate system. Developers have to
        compute intersection of the terrain with this intersector line and pass the information back to
        the vehicle in hitXYZ, hitNormal, hitVelocity and frictionFactor.

        Note that Simple3D is RTDynamics' 3D engine for demo purposes. In your applications you will need
        to use your own gaphics system calls. Most graphics engines (Vega Prime, OSG, Performer etc... ) provide
        necessary functionality to make line intersections with terrain.
    */
    virtual bool query( const smVec3d& start, const smVec3d& end ,
                            smVec3d& hitXYZ , smVec3d& hitNormal, smVec3d& hitVelocity,
							smReal& frictionFactor,
							bool& isLiquid )
    {
		// Friction parameter that is defined in the ground contact definition is multiplied with
		// the following value to obtain final friction coefficient. Developers can simulate various
		// ground types (icy, asphalt, soil etc. ) using frictionFactor.
		frictionFactor = 1.0f;

		isLiquid = false;

        smVec3r tmpXYZ;
        smVec3r tmpNormal;
        if ( m_simple3D->getCollision( smVec3dToVec3r(start), smVec3dToVec3r(end) , tmpXYZ , tmpNormal ) )
        {
            hitXYZ = smVec3rToVec3d(tmpXYZ);
            hitNormal = smVec3rToVec3d(tmpNormal);
            hitVelocity.set( 0.0 , 0.0 , 0.0 );  // Terrain does not move so velocity is always zero.
            return true; // Since intersector hits the terrain return true.
        }
        return false; // If execution reaches here we do not have a hit. Return false.
    }

protected:
     Simple3D*  m_simple3D;
};

/*
   This class implements a terrain query callback similar to the previous one. The difference is that
   Simple3DTerrainQueryCallbackForShipdeckLanding supports moving platforms.

   For correct interaction between the vehicle and ship deck we need to provide position, 
   normal and velocity information on the point of contact.
   
   This callback's query method is periodically called by vehicles's ground contacts (each one makes
   a separate call every frame) to get information about the terrain and/or ship deck below them.
*/
class Simple3DTerrainQueryCallbackForShipdeckLanding : public RTD::TerrainQueryCallback
{
public:

	Simple3DTerrainQueryCallbackForShipdeckLanding( RTD::World* world , Simple3D* s3d , ShipAnimator* ShipAnimator ):RTD::TerrainQueryCallback( world )
	{
		m_simple3D = s3d;
		m_ShipAnimator = ShipAnimator;
	}

	/*
		start and end define a linear intersector's end points in world coordinate system. Developers
		have to cast this intersector to the scene and compute the position, normal and velocity of the hit point.
		In this example we assumed that all hits occur between ship and intersector hence we do not check
		the object type.
	*/
	virtual bool query( const smVec3d& start, const smVec3d& end ,
							smVec3d& hitXYZ , smVec3d& hitNormal, smVec3d& hitVelocity,
							smReal& frictionFactor,
							bool& isLiquid )
	{
		// Friction parameter that is defined in the ground contact definition is multiplied with
		// the following value to obtain final friction coefficient. Developers can simulate various
		// ground types (icy, asphalt, soil etc. ) using frictionFactor.
		frictionFactor = 1.0f;

		isLiquid = false;

		smVec3r tmpXYZ;
		smVec3r tmpNormal;
		if ( m_simple3D->getCollision( smVec3dToVec3r(start), smVec3dToVec3r(end) , tmpXYZ , tmpNormal ) )
		{
			hitXYZ = smVec3rToVec3d(tmpXYZ);
			hitNormal = smVec3rToVec3d(tmpNormal);

			// In this example we do not have a terrain. We can safely assume that if the intersection
			// occurs below 1 meters we hit the sea.
			if ( hitXYZ[2] < 1.0 )
			{
				return false;
			}
			else
			{
				/*
					Here we give the position, orientation, translational and angular velocity of the ship along with
					point of contact (hitXYZ) to a special function which computes the velocity of the point of contact.
				*/
				hitVelocity = RTD::computeVelocityAtAPointOnObject(	m_ShipAnimator->getXYZ() , m_ShipAnimator->getHPR() , 
																	m_ShipAnimator->getVelocity() , m_ShipAnimator->getRotVel() , 
																	hitXYZ );
			}

			
			return true;
		}
		return false;
	}

protected:
	 Simple3D*	m_simple3D;
	 ShipAnimator*		m_ShipAnimator;
};