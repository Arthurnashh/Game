
#ifndef WR_RES
	#define WR_RES


class WRLoot;
class WRSpawn;

class WRSpawnItem;

//! Debug
class WRDebug
{
private:

	static const int WRDBEBUG_MAPSIZE = 128; ///< Size of debug texture
	static const int WRDBEBUG_MAPBPP = 3; ///< 24bit

	long m_iWidth;
	long m_iHeight;
	long m_iBpp;

	float m_fLevel0;
	float m_fLevel1;
	float m_fLevel2;

	unsigned char* m_pData; ///< Image data

	float* m_pLevel; ///< Level data

	int TGA24Save( const char* sName, int iWidth, int iHeight, int iBpp, unsigned char* pRGB );

	void Alloc( void );
	void Release( void );

	float m_fX1;
	float m_fZ1;
	float m_fX2;
	float m_fZ2;

	//! interpolate single color
	unsigned int InterpolateColor( unsigned int iColorA, unsigned int iColorB, float fMult );
	//! Interpolate value 0.0 .. 1.0 into color array
	unsigned int Interpolate( float fValue, int iColorCount, const unsigned int* pColors );


public:
	WRDebug();
	~WRDebug();

	//! Clear content
	void Clear( void );
	//! Write to file
	int Output( const char* sName );

	long GetWidth( void ) const
		{ return m_iWidth; };
	long GetHeight( void ) const
		{ return m_iHeight; };

	//! Project value onto texture
	void Project( float fX, float fZ, float fValue );

	//! Set calculation boundaries
	void SetBoundaries( float fX1, float fZ1, float fX2, float fZ2 )
	{
		m_fX1 = fX1;
		m_fZ1 = fZ1;
		m_fX2 = fX2;
		m_fZ2 = fZ2;
	};

	//! Set level multiplies
	void SetLevels( float fLevel0, float fLevel1, float fLevel2 )
	{
		m_fLevel0 = fLevel0;
		m_fLevel1 = fLevel1;
		m_fLevel2 = fLevel2;
	};

	//! Get highest value
	float GetMaxValue( void );

	//! Convert values into RGB heightmap
	void Convert( int iColorCount, const unsigned int* pColors );

};


//! Content holder
class WRResource
{
private:

	WRLoot* m_pWorldLoot;
	WRSpawn* m_pWorldSpawn;

	WRDebug* m_pDebug;

	bool m_bRespawnEnabled; ///< Set if respawn should be running

public:
	WRResource();
	~WRResource();

	//! Get loot populator class
	WRLoot* GetWRLoot( void )
		{ return m_pWorldLoot; };
	//! Get loot spawn class
	WRSpawn* GetWRSpawn( void )
		{ return m_pWorldSpawn; };

	//! Get Debug
	WRDebug* GetWRDebug( void )
		{ return m_pDebug; };

	//! Add object to resource container
	void OnAddObject( Object* pObject, WRSpawnItem* pItem );
	//! Remove object from resource container
	void OnRemoveObject( Object* pObject );

	//! Restore all objects from DB
	void RestoreAllObjects( void );

	//! Analyze spawnpoints
	void AnalyzeSpawnPoints( void );

	//! Update content
	void OnUpdate( void );

};

WRResource* GetWResource( void );
WRResource* CreateWResource( void );
void DestroyWResource( void );


#endif
