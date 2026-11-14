#pragma once
#include "Simple3D/ArmoredVehicleAnimatorBase.h"

class ArmoredVehicleData;

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

class SIMPLE3D_EXPORT ArmoredVehicleAnimatorOSG : public ArmoredVehicleAnimatorBase
{
public:
	ArmoredVehicleAnimatorOSG(Dina3D::DrawableObjectGeo*);
	virtual ~ArmoredVehicleAnimatorOSG();

	void	setLeftRearWheel( const smVec3r& XYZ , const smVec3r& HPR );
	void	setRightRearWheel( const smVec3r& XYZ , const smVec3r& HPR );
		
	void	setLeftFrontWheel( const smVec3r& XYZ , const smVec3r& HPR );
	void	setRightFrontWheel( const smVec3r& XYZ , const smVec3r& HPR );

protected:	
	ArmoredVehicleData*		m_data;

	bool getDOF( const std::string& nodeName ,  osg::Group* root , osgSim::DOFTransform*& nodePtr );
};

