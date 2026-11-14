#pragma once

#include <string>
#include <Physics/smVec3d.h>



#ifdef RTDYNAMICS_TERRAIN_EXPORTS
#	define RTDYNAMICS_TERRAIN_EXPORT _declspec(dllexport)
#else
#	define RTDYNAMICS_TERRAIN_EXPORT _declspec(dllimport)
#endif

namespace RTDTerrain
{

class RTDYNAMICS_TERRAIN_EXPORT Terrain
{
public:
	Terrain();
	virtual ~Terrain();

	virtual bool loadTerrain( const std::string& terrainFile , const std::vector<std::string>& paths , std::string& error );

	virtual bool query( const smVec3d& start, const smVec3d& end , smVec3d& hitXYZ , smVec3d& hitNormal, smVec3d& hitVelocity );		

protected:

	void*	m_p;

};

}