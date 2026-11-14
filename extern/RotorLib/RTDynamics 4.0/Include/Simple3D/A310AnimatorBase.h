#pragma once
#include "Simple3D/Common.h"
#include <string>
#include "Physics/smVecTypes.h"

class SIMPLE3D_EXPORT A310AnimatorBase
{
public:
	A310AnimatorBase(){}
	virtual ~A310AnimatorBase(){}

	virtual void	setNoseGear( const smVec3r& XYZ , const smVec3r& HPR ){}
	virtual void	setLeftMainGear( const smVec3r& XYZ , const smVec3r& HPR ){}
	virtual void	setRightMainGear( const smVec3r& XYZ , const smVec3r& HPR ){}

	/*
		Status is a value between 0 and 1.
		0 Means fully retracted gears and 1 means fully extended.
	*/
	virtual void	setSteeringGearStatus( float status ){}	
	virtual void	setLeftMainGearStatus( float status ){}	
	virtual void	setRightMainGearStatus( float status ){}	

	/*
		Status is a value between 0 and 1.
		0 Means fully retracted spoilers and 1 means fully extended.
	*/
	virtual void	setSpoilerStatus( float status ){}

	// The angle(in degrees) between the ailerons and wing plane.
	// Positive values mean that trailing edge is down.
	virtual void	setLeftAileronDeflection( float angle ){}
	virtual void	setRightAileronDeflection( float angle ){}

	// The angle(in degrees) between the elevator and tail wing plane.
	// Positive values mean that trailing edge is down.
	virtual void	setLeftElevatorDeflection( float angle ){}
	virtual void	setRightElevatorDeflection( float angle ){}

	// The angle(in radians) between the vertical stabilizer and rudder. Positive values mean 
	// that the trailing edge moves left.
	virtual void	setRudderDeflection( float angle ){}

	virtual void	setLeftFlapSetting( float status ){}
	virtual void	setRightFlapSetting( float status ){}

	virtual void	setLeftEngineRotorAngle( float angle ){}
	virtual float	getLeftEngineRotorAngle() const{ return 0;}

	virtual void	setRightEngineRotorAngle( float angle ){}
	virtual float	getRightEngineRotorAngle() const{return 0;}
};