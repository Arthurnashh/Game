#pragma once
#include "Simple3D/Common.h"
#include <string>
#include "Physics/smVecTypes.h"


class SIMPLE3D_EXPORT FighterJetAnimatorBase
{
public:
	FighterJetAnimatorBase(){ }
	virtual ~FighterJetAnimatorBase(){ }

	virtual void	setNoseGear( const smVec3r& XYZ , const smVec3r& HPR ){ }
	virtual void	setLeftMainGear( const smVec3r& XYZ , const smVec3r& HPR ){ }
	virtual void	setRightMainGear( const smVec3r& XYZ , const smVec3r& HPR ){ }

	/**
		Status is a value between 0 and 1.
		0 Means fully retracted gears and 1 means fully extended.
	*/
	virtual void	setSteeringGearStatus		( float status ){ }	
	virtual void	setLeftMainGearStatus		( float status ){ }	
	virtual void	setRightMainGearStatus		( float status ){ }	

	// The angle(in radians) between the ailerons and wing plane.
	// Positive values mean aileron moves upward.
	virtual void	setLeftAileronDeflection( float angle ){ }
	virtual void	setRightAileronDeflection( float angle ){ }

	// The angle(in radians) between the elevator and tail wing plane.
	// Positive values mean elevator moves upward.
	virtual void	setLeftElevatorDeflection( float angle ){ }
	virtual void	setRightElevatorDeflection( float angle ){ }

	// The angle(in radians) between the vertical stabilizer and rudder. Positive values mean 
	// that rudder moves left.
	virtual void	setRudderDeflection( float angle ){ }

	virtual void	setAirbrakeDeflection( float angle ){ }
};