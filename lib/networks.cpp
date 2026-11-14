#include "wpch.hpp"

#include "landscape.hpp"
/*
#include "poly.hpp"
#include "object.hpp"

#include "engine.hpp"
#include "global.hpp"
*/

void Landscape::TerrainChanged( float x, float z, float limit )
{
	FlushCache();
	int xMin,xMax,zMin,zMax;
	xMin=toIntFloor((x-limit)*_invLandGrid);
	xMax=toIntCeil((x+limit)*_invLandGrid);
	zMin=toIntFloor((z-limit)*_invLandGrid);
	zMax=toIntCeil((z+limit)*_invLandGrid);
	if( xMin<0 ) xMin=0;if( xMax>_landRangeMask ) xMax=_landRangeMask;
	if( zMin<0 ) zMin=0;if( zMax>_landRangeMask ) zMax=_landRangeMask;
	//AutoArray<NetLine *>changeNets;
	for( int zz=zMin; zz<=zMax; zz++ ) for( int xx=xMin; xx<=xMax; xx++ )
	{
		const ObjectList &oList=_objects(xx,zz);
		int n=oList.Size();
		for( int i=0; i<n; i++ )
		{
			Object *obj=oList[i];
			if( !obj ) continue;
			if( obj->GetType()==Primary || obj->GetType()==Network )
			{
				Vector3 offCenter=obj->DirectionModelToWorld(obj->GetShape()->BoundingCenter());
				Point3 pos=obj->Position()-offCenter;
				pos[1]=SurfaceY(pos[0],pos[2]);
				obj->SetPosition(pos+offCenter);
			}
			/*
			else if( obj->GetType()==Network )
			{
				NetLine *line=static_cast<NetLine *>(obj->UserData());
				Assert( line );
				for( int i=0; i<changeNets.Size(); i++ )
				{
					if( changeNets[i]==line ) changeNets.Delete(i),i--;
				}
				changeNets.Add(line);
			}
			*/
		}
	}
	/*
	int iLine;
	for( iLine=0; iLine<changeNets.Size(); iLine++ )
	{
		NetLine *line=changeNets[iLine];
		RecreateNetLine(line);
	}
	*/
}
