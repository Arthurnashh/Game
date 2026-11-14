#pragma once


#include <set>
#include "RTD/WorldParticipant.h"



namespace RTD
{

    /** 
		This feature is experimantal.
    **/
    class RTDYNAMICS_EXPORT Terrain 
    {
		friend class World;
    public:
			Terrain();
						
			virtual bool	loadTerrain( const std::string& terrainFile , std::string& error );		

			virtual bool	loadTerrain( const std::string& terrainFile , const std::vector<std::string>& searchPaths , std::string& error );		


			virtual bool	query( const smVec3d& start, const smVec3d& end , smVec3d& hitXYZ , smVec3d& hitNormal, smVec3d& hitVelocity );		

			size_t			getNumReferers()	{	return m_referencingWorlds.size();	}

			bool			remove();

		protected:
		
			void		addWorld( World* w );
			void		removeWorld( World* w );

			std::set<World*>	m_referencingWorlds;
    
			void*	m_p;

			virtual ~Terrain();
			
    };
}

