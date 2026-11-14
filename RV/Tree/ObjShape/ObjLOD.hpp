#ifndef _OBJLOD_HPP
#define _OBJLOD_HPP

#include "objobje.hpp"

#define MAX_LOD_LEVELS 16

typedef bool (*LoadCallBack)( istream &f, void *context );
typedef bool (*SaveCallBack)( ostream &f, void *context );

WString LODResolToName( float resol );
float LODNameToResol( WString name );

class LODObject
{
    private:
    ObjectData *_obj[MAX_LOD_LEVELS];
    float _resolutions[MAX_LOD_LEVELS];
    
    int _active;
    int _n;
    
    bool  _dirty; // note - all _obj items have also their own dirty flag
   
    protected:
    void DoConstruct();
    void DoConstructEmpty();
    void DoConstruct( const LODObject &src );
    void DoDestruct();
    
    public:
    LODObject(){DoConstruct();}
    LODObject( const LODObject &src ){DoConstruct(src);}
    void operator = ( const LODObject &src ){DoDestruct();DoConstruct(src);}
    ~LODObject(){DoDestruct();}

    int ActiveLevel() const  {return _active;}
    ObjectData *Active() const  {return _obj[_active];}
    ObjectData *Level( int level ) const {return _obj[level];}
    
    operator ObjectData &() {return *_obj[_active];}
    
    void SelectLevel( int level ) {_active=level;}
    int NLevels() const {return _n;}
    
    int FindLevel( float resolution ) const;
    int FindLevelExact( float resolution ) const;
    int FindLevelUltraExact(float resolution) const;
    float Resolution( int level ) const {return _resolutions[level];}
    WString LODName( int level ) const;
    bool ChangeResolution( float oldRes, float newRes );
    int SetResolution( int level, float newRes );

    bool DeleteLevel( int level ); // copy/deallocate actual data
    int AddLevel( const ObjectData &obj, float resolution );

    bool RemoveLevel( int level ); // remove/add only the pointer
    int InsertLevel( ObjectData *obj, float resolution );

    // some global operations
    void ClearDirty();
    void SetDirty();
    bool Dirty();
    
    void CleanNormals();

    int Load( WFilePath filename, LoadCallBack callback, void *context );
    int Save( WFilePath filename, bool final, SaveCallBack callback, void *context );

    int Save( ostream &f, bool final );
    bool Merge( const LODObject &src );
		
		void CenterAll();
};

#endif

