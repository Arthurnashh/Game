#pragma once
#include "Simple3D/FighterJetAnimatorBase.h"

class FighterJetAnimatorData;

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

class SIMPLE3D_EXPORT FighterJetAnimatorOSG : public FighterJetAnimatorBase
{
public:
	FighterJetAnimatorOSG(Dina3D::DrawableObjectGeo*);
	virtual ~FighterJetAnimatorOSG();

	void	setNoseGear( const smVec3r& XYZ , const smVec3r& HPR );
	void	setLeftMainGear( const smVec3r& XYZ , const smVec3r& HPR );
	void	setRightMainGear( const smVec3r& XYZ , const smVec3r& HPR );

	/**
		Status is a value between 0 and 1.
		0 Means fully retracted gears and 1 means fully extended.
	*/
	void	setSteeringGearStatus		( float status );	
	void	setLeftMainGearStatus					( float status );	
	void	setRightMainGearStatus					( float status );	

	// The angle(in radians) between the ailerons and wing plane.
	// Positive values mean aileron moves upward.
	void	setLeftAileronDeflection( float angle );
	void	setRightAileronDeflection( float angle );

	// The angle(in radians) between the elevator and tail wing plane.
	// Positive values mean elevator moves upward.
	void	setLeftElevatorDeflection( float angle );
	void	setRightElevatorDeflection( float angle );

	// The angle(in radians) between the vertical stabilizer and rudder. Positive values mean 
	// that rudder moves left.
	void	setRudderDeflection( float angle );

	void	setAirbrakeDeflection( float angle );

protected:	
	FighterJetAnimatorData*		m_data;

	bool getDOF( const std::string& nodeName ,  osg::Group* root , osgSim::DOFTransform*& nodePtr );
};