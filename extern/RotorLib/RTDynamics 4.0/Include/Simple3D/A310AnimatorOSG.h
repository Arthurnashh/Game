#pragma once
#include "Simple3D/A310AnimatorBase.h"

class A310AnimatorData;

namespace Dina3D
{
	class DrawableObjectGeo;
}

namespace osg
{
	class Group;
}

namespace osgSim
{
	class DOFTransform;
}

class SIMPLE3D_EXPORT A310AnimatorOSG : public A310AnimatorBase
{
public:
	A310AnimatorOSG(Dina3D::DrawableObjectGeo*);
	virtual ~A310AnimatorOSG();

	void	setNoseGear( const smVec3r& XYZ , const smVec3r& HPR );
	void	setLeftMainGear( const smVec3r& XYZ , const smVec3r& HPR );
	void	setRightMainGear( const smVec3r& XYZ , const smVec3r& HPR );

	/*
		Status is a value between 0 and 1.
		0 Means fully retracted gears and 1 means fully extended.
	*/
	void	setSteeringGearStatus( float status );	
	void	setLeftMainGearStatus( float status );	
	void	setRightMainGearStatus( float status );	

	/*
		Status is a value between 0 and 1.
		0 Means fully retracted spoilers and 1 means fully extended.
	*/
	void	setSpoilerStatus( float status );

	// The angle(in degrees) between the ailerons and wing plane.
	// Positive values mean that trailing edge is down.
	void	setLeftAileronDeflection( float angle );
	void	setRightAileronDeflection( float angle );

	// The angle(in degrees) between the elevator and tail wing plane.
	// Positive values mean that trailing edge is down.
	void	setLeftElevatorDeflection( float angle );
	void	setRightElevatorDeflection( float angle );

	// The angle(in radians) between the vertical stabilizer and rudder. Positive values mean 
	// that the trailing edge moves left.
	void	setRudderDeflection( float angle );

	void	setLeftFlapSetting( float status );
	void	setRightFlapSetting( float status );

	void	setLeftEngineRotorAngle( float angle );
	float	getLeftEngineRotorAngle() const;

	void	setRightEngineRotorAngle( float angle );
	float	getRightEngineRotorAngle() const;

protected:	
	A310AnimatorData*		m_data;

	float	m_leftAileronDeflection;
	float	m_rightAileronDeflection;
	float	m_spoilerStatus;

	void	updateLeftWing();
	void	updateRightWing();

	bool getDOF( const std::string& nodeName ,  osg::Group* root , osgSim::DOFTransform*& nodePtr );
};