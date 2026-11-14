#pragma once

/*
  This class animates a ship. The 3D ship model is already available within Simple3D and Simple3D also provides 
  the methods with which we can set/get the position and the orientation of the ship. 
*/
class ShipAnimator
{
public:
	ShipAnimator( Simple3D* s3d ): m_s3d( s3d )
	{
		reset();
	}

	smVec3d getVelocity()
	{
		return m_velocity;
	}
	
	smVec3d getRotVel()
	{
		return m_rotVel;
	}

	virtual void	update( double dt )
	{
		// Simple3D keeps the ship model internally. We get actual position and orientation from Simple3D
		smVec3d xyz = smVec3rToVec3d(m_s3d->getLSD41_XYZ());
		smVec3d hpr = smVec3rToVec3d(m_s3d->getLSD41_HPR());

		// Ships translational movement is computed here.
		xyz += m_velocity * dt;

		// Change in ship's pitch. We change the pitch *and* also compute the pitch rate. We need this rotational
		// speed later in the terrain query callback.
		{
			double  period = 0.3;
			double	amplitude = 1.0;
			hpr[1] = float(cos(m_animationTime * period - smMath::sHalfPI) * amplitude);
			m_rotVel[0] = smMath::degToRad(-sin(m_animationTime * period - smMath::sHalfPI) * period * amplitude );
		}

		// We do the same thing as above for roll.
		{
			double  period = 0.6;
			double	amplitude = 3.0;
			hpr[2] = float(sin(m_animationTime * period ) * amplitude);
			m_rotVel[1] = smMath::degToRad(cos(m_animationTime * period ) * period * amplitude );
		}

		// Set the new position and orientation of the 3D model. Note that it is crucial to update the
		// 3D model here. Of course, RotorLib does not care about the visual appearance; however, since the
		// 3D model is also used for collision detection it should be in correct place with correct orientation.
		m_s3d->setLSD41_XYZ( smVec3dToVec3r(xyz) );
		m_s3d->setLSD41_HPR( smVec3dToVec3r(hpr) );

		m_animationTime += dt;
	}

	virtual void	reset()
	{
		m_velocity.set( 0.0 , 5.0 , 0.0 );
		m_rotVel.set( 0.0 , 0.0 , 0.0 );

		m_animationTime = 0.0f;
		m_s3d->setLSD41_XYZ( smVec3r(0.0f, 45.0f , 0.0f ) );
		m_s3d->setLSD41_HPR( smVec3r(0.0f, 0.0f , 0.0f ) );
	}

	smVec3d getXYZ() const	{	return smVec3rToVec3d(m_s3d->getLSD41_XYZ());	}

	smVec3d getHPR() const	{	return smVec3rToVec3d(m_s3d->getLSD41_HPR());	}

protected:
	Simple3D*	m_s3d;

	double		m_animationTime;

	smVec3d		m_rotVel;
	smVec3d		m_velocity;

};