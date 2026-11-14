
#include "../wpch.hpp"

#include <El/Evaluator/expressImpl.hpp>

#include "../world.hpp"


#include "WorldRes.h"

#include "WorldLoot.h"
#include "WorldSpawn.h"

//-----------------------------------------------------------------------------
// TODO: this should become part of world data?
WRResource* g_pWorldResource = NULL;

WRResource* GetWResource( void )
{
	return g_pWorldResource;
};
WRResource* CreateWResource( void )
{
	if( g_pWorldResource )
		return g_pWorldResource;

	g_pWorldResource = new WRResource();
	return g_pWorldResource;
};
void DestroyWResource( void )
{
	if( g_pWorldResource )
	{
		delete g_pWorldResource;
		g_pWorldResource = NULL;
	};
};
//-----------------------------------------------------------------------------

#define BUF_OPT ( 64L*1024 )
#define BUF_MIN ( 1024 )

static int fputiw( int W, FILE *f )
{
	if( fputc((byte)W,f)<0 ) return EOF;
	return fputc(W>>8,f);
}/*
static int fputi24( long W, FILE *f )
{
	if( fputc((byte)W,f)<0 ) return EOF;
	if( fputc((byte)(W>>8),f)<0 ) return EOF;
	if( fputc((byte)(W>>16),f)<0 ) return EOF;
	return 0;
}*/

//-----------------------------------------------------------------------------
WRDebug::WRDebug() :
	m_iWidth(WRDBEBUG_MAPSIZE),
	m_iHeight(WRDBEBUG_MAPSIZE),
	m_iBpp(WRDBEBUG_MAPBPP),
	m_fLevel0(1.0f),
	m_fLevel1(1.0f),
	m_fLevel2(1.0f),
	m_pData(NULL),
	m_pLevel(NULL),
	m_fX1(0.0f),
	m_fZ1(0.0f),
	m_fX2(0.0f),
	m_fZ2(0.0f)
{
	Alloc();
	Clear();

	TGA24Save("test.tga",m_iWidth,m_iHeight,m_iBpp,m_pData);
}
//-----------------------------------------------------------------------------
WRDebug::~WRDebug()
{
	Release();
}
//-----------------------------------------------------------------------------
int WRDebug::TGA24Save( const char* sName, int iWidth, int iHeight, int iBpp, unsigned char* pRGB )
{
	/* color map uncompressed */
	FILE *f;
	int r;
	f=fopen(sName,"wb");
	r=-1;
	if( f )
	{
		long L=(long)iWidth*iHeight*iBpp;
		
		fputc(0,f); /*  Number of Characters in Identification Field. */
		fputc(0,f); /*  Color Map Type. */
		fputc(2,f); /*  Image Type Code. */ /* CM uncompress. */
		
		/*   3   : Color Map Specification. */
		fputiw(0,f); /* beg */
		fputiw(0,f); /* count */
		fputc(0,f); /* bit RGBA */
		
		/*   8   : Image Specification. */
		fputiw(0,f); /*  X Origin of Image. */
		fputiw(0,f); /*  Y Origin of Image. */
		fputiw(iWidth,f); /*  Width Image. */
		fputiw(iHeight,f); /*  Height Image. */
		fputc(8*iBpp,f); /*  Image Pixel Size. */
		fputc(0,f); /*  Image Descriptor Byte. */
		
		fwrite(pRGB,sizeof(char),L,f);

		/* 18 */
//		for( I=0; I<NC; I++ )
//			fputi24(RGB[I],f);

		r=0;

		fclose(f);
	}
	return r;
}
//-----------------------------------------------------------------------------
void WRDebug::Alloc( void )
{
	if( m_pData == NULL )
		m_pData = new unsigned char[m_iWidth*m_iHeight*m_iBpp];

	if( m_pLevel == NULL )
		m_pLevel = new float[m_iWidth*m_iHeight];
}
//-----------------------------------------------------------------------------
void WRDebug::Release( void )
{
	if( m_pData )
	{
		delete[] m_pData;
		m_pData = NULL;
	}
	if( m_pLevel )
	{
		delete[] m_pLevel;
		m_pLevel = NULL;
	}
}
//-----------------------------------------------------------------------------
void WRDebug::Clear( void )
{
	if( m_pData )
	{
		for( int i = 0; i < (m_iWidth*m_iHeight*m_iBpp); i++ )
			m_pData[i] = 0;
	};

	if( m_pLevel )
	{
		for( int i = 0; i < (m_iWidth*m_iHeight); i++ )
			m_pLevel[i] = 0.0f;
	};
}
//-----------------------------------------------------------------------------
int WRDebug::Output( const char* sName )
{
/*	for( int y = 0; y < m_iHeight; y++ )
	{
		for( int x = 0; x < m_iWidth; x++ )
		{
			float val = (x<300.0f)?x:300.0f;
			m_pLevel[y*m_iWidth+x] = val;
		}
	}*/

	Convert(0,NULL);

	return TGA24Save(sName,m_iWidth,m_iHeight,m_iBpp,m_pData);
}
//-----------------------------------------------------------------------------
unsigned int WRDebug::InterpolateColor( unsigned int iColorA, unsigned int iColorB, float fMult ) 
{
	unsigned char rA = (iColorA>>16)&0xff;
	unsigned char gA = (iColorA>>8)&0xff;
	unsigned char bA = iColorA&0xff;

	unsigned char rB = (iColorB>>16)&0xff;
	unsigned char gB = (iColorB>>8)&0xff;
	unsigned char bB = iColorB&0xff;

	float fr = (float)rA*(1.0f-fMult) + (float)rB*fMult;
	float fg = (float)gA*(1.0f-fMult) + (float)gB*fMult;
	float fb = (float)bA*(1.0f-fMult) + (float)bB*fMult;

	unsigned char r = (unsigned char)fr;
	unsigned char g = (unsigned char)fg;
	unsigned char b = (unsigned char)fb;

	unsigned int col = (r<<16)|(g<<8)|b;
	return col;
}
//-----------------------------------------------------------------------------
unsigned int WRDebug::Interpolate( float fValue, int iColorCount, const unsigned int* pColors ) 
{
	// do not even care
	if( fValue <= 0.0f )
		return 0x000000;

	float sub = 1.0f/(float)iColorCount;

	float fA = fValue*(float)iColorCount;
	float fB = (fValue+sub)*(float)iColorCount;

	int iA = (int)fA;
	int iB = (int)fB;

	if( iA < 0 )
		iA = 0;
	else if( iA >= iColorCount )
		iA = (iColorCount-1);
	
	if( iB < 0 )
		iB = 0;
	else if( iB >= iColorCount )
		iB = (iColorCount-1);

	unsigned int colA = pColors[iA];
	unsigned int colB = pColors[iB];

	float mul = (float)iColorCount * (fValue - sub*(float)iA);

	unsigned int col = InterpolateColor(colA,colB,mul);

	return col;
}
//-----------------------------------------------------------------------------
void WRDebug::Convert( int iColorCount, const unsigned int* pColors )
{
	float max = GetMaxValue();
	if( max <= 0.0f )
		max = 1.0f;

	// default
	static const unsigned int defaultColors[] =
	{
		0x004a80,
		0x92278f,
		0xff0000,
		0xf7941d,
		0xfff200,
		0xffffff,
	};

	const unsigned int* colors = pColors;
	if( (pColors==NULL) || (iColorCount<1) )
	{
		iColorCount = sizeof(defaultColors)/sizeof(defaultColors[0]);
		colors = &defaultColors[0];
	};

	for( int i = 0; i < (m_iWidth*m_iHeight); i++ )
	{
		float val = m_pLevel[i]/max;

		/*		val = m_pLevel[i];
				if( val > 1024.0f )
					val = 1024.0f;
				val /= 1024.0f;*/

		unsigned int col = Interpolate(val,iColorCount,colors);

		unsigned char r = (col>>16)&0xff;
		unsigned char g = (col>>8)&0xff;
		unsigned char b = col&0xff;

		int idx = i*3;

		m_pData[idx+0] = b;
		m_pData[idx+1] = g;
		m_pData[idx+2] = r;
	};
}
//-----------------------------------------------------------------------------
float WRDebug::GetMaxValue( void )
{
	float max = 0.0f;
	for( int i = 0; i < (m_iWidth*m_iHeight); i++ )
	{
		if( max < m_pLevel[i] )
			max = m_pLevel[i];
	};

	return max;
}
//-----------------------------------------------------------------------------
void WRDebug::Project( float fX, float fZ, float fValue )
{
	float w = (m_fX2-m_fX1);
	float h = (m_fZ2-m_fZ1);

	float maxx = (float)GetWidth()-1.0f;
	float maxz = (float)GetHeight()-1.0f;

	float x = (maxx*(fX-m_fX1))/w;
	float z = (maxz*(fZ-m_fZ1))/h;

	long ix = (long)x;
	long iz = (long)z;

	long offset = (ix+(iz*m_iWidth));

	m_pLevel[offset] += fValue;
}
//-----------------------------------------------------------------------------


bool DBAddItem( Object* pObject );
Entity* DBGetItem( int iItemID, bool bSpawn );
int DBGetItemCount( void );
int DBGetItemBatch( int iStartItemID, int iEndItemID, bool bSpawn );

//-----------------------------------------------------------------------------
WRResource::WRResource() :
	m_pWorldLoot(NULL),
	m_pDebug(NULL),
	m_bRespawnEnabled(false)
{
	m_pWorldLoot = new WRLoot(); 
	m_pWorldSpawn = new WRSpawn();
	m_pDebug = new WRDebug();
}
//-----------------------------------------------------------------------------
WRResource::~WRResource()
{
	if( m_pWorldLoot )
		delete m_pWorldLoot;
	if( m_pWorldSpawn )
		delete m_pWorldSpawn;
	if( m_pDebug )
		delete m_pDebug;
}
//-----------------------------------------------------------------------------
void WRResource::OnAddObject( Object* pObject, WRSpawnItem* pItem )
{
//	DBAddItem(pObject);

	if( m_pWorldSpawn )
		m_pWorldSpawn->OnAddObject(pObject,pItem);
}
//-----------------------------------------------------------------------------
void WRResource::OnRemoveObject( Object* pObject )
{
//	DBRemoveItem(pObject);

	if( m_pWorldSpawn )
		m_pWorldSpawn->OnRemoveObject(pObject);
}
//-----------------------------------------------------------------------------
void WRResource::RestoreAllObjects( void )
{
#ifdef USER_raist
	unsigned long int cntStart = GetTickCount();
#endif

	int countSpawned = 0;

	// get count of items in database
	int count = DBGetItemCount();
///*test limit*/count = 1000;
/*test batch*/int batch = 100;
	for( int c = 1; c < count; c += batch )
	{
		int beg = c;
		int end = beg + (batch-1);
		if( end > count )
			end = count;

		int cnt = DBGetItemBatch(beg,end,true);
		countSpawned += cnt;
		RptF( "  requested %d..%d got %d", beg, end, cnt );
	};

#ifdef USER_raist
	float sec = (float)(GetTickCount()-cntStart);
	RptF( "World items %d of %d loaded in %0.2f seconds", countSpawned, count, sec*0.001f );
#endif
}
//-----------------------------------------------------------------------------
void WRResource::AnalyzeSpawnPoints( void )
{
	// export map
	if( m_pDebug && m_pWorldLoot )
	{
		float x1, x2, z1, z2;

		m_pWorldLoot->GetBoundaries(x1,z1,x2,z2);

		m_pDebug->SetBoundaries(x1,z1,x2,z2);

		for( int group = 0; group < m_pWorldLoot->GetCount(); group++ )
		{
			WRLoot_Group* g = m_pWorldLoot->GetGroup(group);
			for( int container = 0; container < g->GetCount(); container++ )
			{
				WRLoot_Container* c = g->GetContainer(container);
				for( int proxy = 0; proxy < c->GetTotalCount(); proxy++ )
				{
					WRLoot_Point* p = c->GetProxy(proxy);

					float px = p->GetOrigin().X();
					float pz = p->GetOrigin().Z();

					m_pDebug->Project( px, pz, 1.0f );
				};
			};
		};

		m_pDebug->Output("proxymap.tga");
	};

}
//-----------------------------------------------------------------------------
void WRResource::OnUpdate( void )
{
	
	if( m_pWorldSpawn && m_bRespawnEnabled )
	{
#ifdef WRSPAWN_LOG_OVERALL
		unsigned long int cntStart = GetTickCount();
		RptF( "[!] Respawn initiated... total" );
#endif

		m_pWorldSpawn->OnUpdate();

#ifdef WRSPAWN_LOG_OVERALL
		float sec = (float)(GetTickCount()-cntStart);
		RptF( "[!] Respawn took %0.2f seconds for [%d] items", sec*0.001f, m_pWorldSpawn->GetLastRespawnCount() );
#endif
	};

}
//-----------------------------------------------------------------------------



void World::WorldResScheduler(float deltaT)
{
	if(Glob.time < _nextSchedulerUpdateReSpawn)
		return;

	_nextSchedulerUpdateReSpawn = Glob.time + 5.0f;


	if( GetWResource() )
		GetWResource()->OnUpdate();


	//why to do that on clients?
	//  if(GetNetworkManager().IsClient())
	//  return;

/*	Ref<EntityType> type = VehicleTypes.New("zombieBase");
	if(type.IsNull())
		return;

	AutoArray< Person*, MemAllocLocal<Person*, 128> > players;


	if (!GetNetworkManager().IsServer())
	{
		//when not in game yet
		if(GWorld->PlayerOn())
			players.Add(GWorld->PlayerOn());
	}
	else
	{
		AutoArray< NetPlayerInfo> netplayers;
		GetNetworkManager().GetPlayers(netplayers);
		for (int i=0; i<netplayers.Size(); i++)
		{
			if (netplayers[i].person)
			{
				Person *person = dyn_cast<Person,NetworkObject>(netplayers[i].person);
				if (person) players.Add(person);
			}
		}
	}

#if defined SERVER_CORPSE_PERFORMANCE_PATCH
	// @NOTE: skip player check, we want collect bodies even if no player in game
#else
	if(!players.Size())
		return;
#endif

	const bool isServer = GetNetworkManager().IsServer();
	const bool isClient = GetNetworkManager().IsClient();

#if defined SERVER_CORPSE_PERFORMANCE_PATCH
	ScheduleZombies schedule(_deadBodies, type, 500, isServer || (!isServer && !isClient));
#else
	ScheduleZombies schedule(_deadBodies, type, 500, false);
#endif

	//merge nearby players to minimize number of tests
	for(int n = 0; n < players.Size(); n++)
	{
		Vector3Val pos = players[n]->WorldPosition(players[n]->FutureVisualState());
		int p = 0;
		for(; p < schedule._playerOrigins.Size(); p++)
		{
			if(pos.Distance2(schedule._playerOrigins[p]) < Square(15.0f))
			{
				schedule._playerOrigins[p] = (schedule._playerOrigins[p] + pos) * 0.5f;
				break;
			}
		}
		if(p == schedule._playerOrigins.Size())
		{
			schedule._playerOrigins.Add(pos);
		}
	}
	AliveZombies = 0;
	SleepingZombies = 0;

	ForEachVehicle(schedule);

#if defined SERVER_CORPSE_PERFORMANCE_PATCH
	for (size_t i = 0, ie = _deadBodies.Size(); i < ie; ++i)
		GWorld->RemoveVehicle(_deadBodies[i]);
	_deadBodies.Clear();
#endif*/
}
