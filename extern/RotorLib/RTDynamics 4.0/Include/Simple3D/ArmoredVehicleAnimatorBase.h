#pragma once
#include "Simple3D/Common.h"
#include "Physics/smVecTypes.h"

class SIMPLE3D_EXPORT ArmoredVehicleAnimatorBase
{
public:
	ArmoredVehicleAnimatorBase(){}
	virtual ~ArmoredVehicleAnimatorBase(){}

	virtual void	setLeftRearWheel( const smVec3r& XYZ , const smVec3r& HPR ){}
	virtual void	setRightRearWheel( const smVec3r& XYZ , const smVec3r& HPR ){}
		
	virtual void	setLeftFrontWheel( const smVec3r& XYZ , const smVec3r& HPR ){}
	virtual void	setRightFrontWheel( const smVec3r& XYZ , const smVec3r& HPR ){}
};

