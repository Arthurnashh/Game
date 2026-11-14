
#include "../wpch.hpp"

#include "../object.hpp"

#include <El/Evaluator/expressImpl.hpp>

#include "../gameStateExt.hpp"
#include "../Shape/shape.hpp"

#include "WorldSpawn.h"
#include "WorldLoot.h"
#include "WorldRes.h"


//-----------------------------------------------------------------------------
WRMatrixContainer::WRMatrixContainer() :
	m_fLeft(0.0f),
	m_fTop(0.0f),
	m_fRight(0.0f),
	m_fBottom(0.0f),
	m_iInitialSpawn(0)
{

}
//-----------------------------------------------------------------------------
WRMatrixContainer::~WRMatrixContainer()
{
}
//-----------------------------------------------------------------------------
void WRMatrixContainer::OnAddObject( Object* pObject )
{
//	m_aLoot.Insert(pObject);
	m_iCurrentSpawn++;
}
//-----------------------------------------------------------------------------
bool WRMatrixContainer::OnRemoveObject( Object* pObject )
{
	Vector3 pos = pObject->GetCurrentPosition();

	if( CheckPos(pos.X(),pos.Z())
		&& (m_iCurrentSpawn > 0) )
	{
		m_iCurrentSpawn--;
		return true;
	};

/*	enf::uint idx = m_aLoot.GetIndexOf(pObject);
	if( idx != -1 )
		m_aLoot.RemoveElement(idx);

	return (idx!=-1);
*/
	return false;
}
//-----------------------------------------------------------------------------
WRLoot_Point* WRMatrixContainer::FindRandomPoint( void )
{
	if( m_aPoint.GetCardinality() < 1 )
		return NULL;

	if( m_aPoint.GetCardinality() == 1 )
		return m_aPoint[0];

	int idx = GRandGen.RandomInt(0,m_aPoint.GetCardinality());
	return m_aPoint[idx];
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
WRMatrix::WRMatrix() :
	m_fLeft(0.0f),
	m_fTop(0.0f),
	m_fWidth(15000.0f),
	m_fHeight(15000.0f),
	m_iSizeWidth(0),
	m_iSizeHeight(0),
	m_iInitialSpawn(0),
	m_iLowSpawn(0),
	m_iLastRespawnCount(0)
{

}
//-----------------------------------------------------------------------------
WRMatrix::~WRMatrix()
{
	Cleanup();
}
//-----------------------------------------------------------------------------
void WRMatrix::OnAddObject( Object* pObject )
{
	// project object
	Vector3 pos = pObject->GetCurrentPosition();

	float x = ((float)m_iSizeWidth*(pos.X()-m_fLeft))/m_fWidth;
	float z = ((float)m_iSizeHeight*(pos.Z()-m_fTop))/m_fHeight;

	int ix = (int)x;
	int iy = (int)z;

	if( ix < 0 )
		ix = 0;
	else if( ix >= m_iSizeWidth )
		ix = (m_iSizeWidth-1);

	if( iy < 0 )
		iy = 0;
	else if( iy >= m_iSizeHeight )
		iy = (m_iSizeHeight-1);

	int idx = (m_iSizeWidth*iy)+ix;

	if( idx < m_aLoot.GetCardinality() )
	{
		m_aLoot[idx]->OnAddObject(pObject);
	}
	else
	{
#ifdef WRSPAWN_LOG_DETAIL
		RptF("Wrong index... %0.2f,%0.2,%0.2" );
#endif
	};

}
//-----------------------------------------------------------------------------
void WRMatrix::OnRemoveObject( Object* pObject )
{
	// TODO: this is stupid... should be indexed directly
	for( enf::uint c = 0; c < m_aLoot.GetCardinality(); c++ )
	{
		if( m_aLoot[c]->OnRemoveObject(pObject) )
		{
#ifdef WRSPAWN_LOG_DETAIL
			RptF("Removing obj..." );
#endif
			break;
		};
	};
}
//-----------------------------------------------------------------------------
void WRMatrix::Init( int iWidth, int iHeight )
{
	m_iSizeWidth = iWidth;
	m_iSizeHeight = iHeight;

	float w = (m_fWidth-m_fLeft)/(float)iWidth;
	float h = (m_fHeight-m_fTop)/(float)iHeight;

	float l = m_fLeft;

	m_aLoot.SetSize(iWidth*iHeight);
	for( enf::uint x = 0; x < iWidth; x++ )
	{
		float t = m_fTop;

		for( enf::uint z = 0; z < iHeight; z++ )
		{
			int idx = x + (z*iWidth);

			m_aLoot[idx] = new WRMatrixContainer();
			m_aLoot[idx]->SetBoundings(l,t,l+w,t+h);

			t += h;
		};

		l += w;
	};
}
//-----------------------------------------------------------------------------
void WRMatrix::Cleanup( void )
{
	for( enf::uint c = 0; c < m_aLoot.GetCardinality(); c++ )
		delete m_aLoot[c];

	m_aLoot.Clear();
}
//-----------------------------------------------------------------------------
void WRMatrix::ListToConsole( void )
{
#ifdef WRSPAWN_LOG_OVERALL
	RptF("[&] Loot Matrix" );

	int totalLoot = 0;
	int totalPoint = 0;

	for( enf::uint c = 0; c < m_aLoot.GetCardinality(); c++ )
	{
		int cnt = m_aLoot[c]->GetCurrentSpawnCount();
		int pnt = m_aLoot[c]->GetPointCount();

		totalLoot += cnt;
		totalPoint += pnt;
		RptF("   [%03d] %d items %d points bounds(%0.1f,%0.1f,%0.1f,%0.1f)", c, cnt, pnt, m_aLoot[c]->GetLeft(), m_aLoot[c]->GetTop(), m_aLoot[c]->GetRight(), m_aLoot[c]->GetBottom() );
	};

	RptF("[total] %d items, %d point", totalLoot, totalPoint );
#endif
}
//-----------------------------------------------------------------------------
void WRMatrix::OnInitialized( void )
{
#ifdef WRSPAWN_LOG_OVERALL
	unsigned long int cntStart = GetTickCount();
	RptF( "[!] Defaults for respawn..." );
#endif

	// set default counts
	for( enf::uint c = 0; c < m_aLoot.GetCardinality(); c++ )
		m_aLoot[c]->SetInitialSpawnCount(m_aLoot[c]->GetCurrentSpawnCount());

	// distribute points to grid
	WRLoot* loot = GetWResource()->GetWRLoot();
	if( !loot )
		return;

	// process proxy data
	for( int gCount = 0; gCount < loot->GetCount(); gCount++ )
	{
		WRLoot_Group* g = loot->GetGroup(gCount);
		for( int cCount = 0; cCount < g->GetCount(); cCount++ )
		{
			WRLoot_Container* c = g->GetContainer(cCount);
			for( int pCount = 0; pCount < c->GetTotalCount(); pCount++ )
			{
				WRLoot_Point* p = c->GetProxy(pCount);
				if( p )
				{
					float x = p->GetOrigin().X();
					float z = p->GetOrigin().Z();

					for( enf::uint _m = 0; _m < m_aLoot.GetCardinality(); _m++ )
					{
						if( m_aLoot[_m]->CheckPos(x,z) )
						{
							m_aLoot[_m]->InsertPoint(p);
							p= NULL;
							break;
						};
					};

					// was not parsed
					if( p )
						RptF( "[!] Invalid spawn point %f,%f", x, z );

				};
			};
		};

//		if( !g->Distance(pos,radius) )
//			continue;

	};

#ifdef WRSPAWN_LOG_OVERALL
	float sec = (float)(GetTickCount()-cntStart);
	RptF( "[!] Defaults took %0.2f seconds", sec*0.001f );
#endif

}
//-----------------------------------------------------------------------------
void WRMatrix::OnUpdate( void )
{
	for( enf::uint c = 0; c < m_aLoot.GetCardinality(); c++ )
	{
		int respawnCount = m_aLoot[c]->IsPopulationLow();
		if( respawnCount > 0 )
		{
			RespawnAt(m_aLoot[c]);
			m_iLastRespawnCount += respawnCount;
		};
	};
}

/**/Entity* SpawnLootObject( WRClassHierarchy* pClass, const char* sName, WRLoot_Point* pProxy, Vector3Par vPosOverride );


//-----------------------------------------------------------------------------
void WRMatrix::RespawnAt( WRMatrixContainer* pContainer )
{
	WRLoot* loot = GetWResource()->GetWRLoot();
	if( !loot )
		return;

	WRLoot_Point* pp = pContainer->FindRandomPoint();
	if( pp )
	{
		// spawn entity
		WRLoot_Container* cc = pp->GetParent();
		if( cc )
		{
			WRLoot_Group* gg = cc->GetParent();
			if( gg )
			{
				// get proper class definition
				WRSpawnClass* spawnClass = loot->GetSetup()->FindByName(gg->GetName());
				if( spawnClass )
				{
					RptF(" Container... %s in group %s", cc->GetName(), gg->GetName() );
					WRSpawnLocation* loc = spawnClass->FindLocation(cc->GetName());
					if( loc )
					{
						WRContainer* spawn = loc->GetParent();
						if( spawn )
						{
							WRSpawnItem* item = spawn->GetRandomItem();

							Entity* veh = SpawnLootObject( item->GetClass(), item->GetName(), pp, pp->GetOrigin() );
							if( veh )
							{
								OnAddObject(veh);
#ifdef WRSPAWN_LOG_DETAIL
								RptF("Respawning... %s", item->GetName() );
#endif
							};
						};
					};
				};
			};
		};

	};
}
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
WRSpawn::WRSpawn() :
	m_pMatrix(NULL)
{
	Init(0.0f,0.0f,15000.0f,15000.0f);
}
//-----------------------------------------------------------------------------
WRSpawn::~WRSpawn()
{
}
//-----------------------------------------------------------------------------
void WRSpawn::Init( float fLeft, float fTop, float fWidth, float fHeight )
{
	if( m_pMatrix )
		Cleanup();

	m_pMatrix = new WRMatrix();
	m_pMatrix->SetBoundings(fLeft,fTop,fWidth,fHeight);
	m_pMatrix->Init(WRMatrix::DEFAULT_MATRIX_SIZE,WRMatrix::DEFAULT_MATRIX_SIZE);
}
//-----------------------------------------------------------------------------
void WRSpawn::Cleanup( void )
{
	if( m_pMatrix )
		delete m_pMatrix;
}
//-----------------------------------------------------------------------------
void WRSpawn::OnAddObject( Object* pObject, WRSpawnItem* pItem )
{
	if( m_pMatrix )
		m_pMatrix->OnAddObject(pObject);
}
//-----------------------------------------------------------------------------
void WRSpawn::OnRemoveObject( Object* pObject )
{
	if( m_pMatrix )
		m_pMatrix->OnRemoveObject(pObject);
}
//-----------------------------------------------------------------------------
void WRSpawn::ListToConsole( void )
{
	if( m_pMatrix )
		m_pMatrix->ListToConsole();
}
//-----------------------------------------------------------------------------
void WRSpawn::OnInitialized( void )
{
	if( m_pMatrix )
		m_pMatrix->OnInitialized();
}
//-----------------------------------------------------------------------------
void WRSpawn::OnUpdate( void )
{
	if( m_pMatrix )
		m_pMatrix->OnUpdate();
}
//-----------------------------------------------------------------------------
