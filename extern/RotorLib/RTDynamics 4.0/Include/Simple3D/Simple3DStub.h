#pragma once

#include "Simple3DBase.h"

/**
*   The Simple3D class provides a simple 3D OpenGL world and a vehicle model.
*   It's main purpose is to keep the examples of RotorLib simple, uncluttered
*   by 3d code.
*/
class SIMPLE3D_EXPORT Simple3DStub : public Simple3DBase
{
public:


public:
    Simple3DStub( SceneType scene = Plane, bool jsNeeded = false , bool dontOpenWindow = false ) ;
    virtual ~Simple3DStub( ) ;

    void    showMessageBox( const std::string& title, const std::string& msgString );
    void    setInfoText(const std::string& info );

    virtual void update() ;

    double  getFrameTime() ;
    double  getFrameDeltaTime() ;
    bool    isExitRequested() ;
	virtual bool    getKeyState( size_t key );
    virtual bool    isKeyDown( size_t key );
    virtual bool    isKeyUp( size_t key );


    /** Returns true if a joystick is available and ready to use. */
    virtual bool    joystickAvailable();
    virtual bool    pedalsAvailable();

    virtual float   getMouseX() const;
    float   getMouseY() const;
    bool    getMouseWheelScrolledUp() const;
    bool    getMouseWheelScrolledDown() const;
    bool    getMouseLeftButton() const;
    bool    getMouseRightButton() const;
    bool    getMouseMiddleButton() const;

	virtual void    setMousePos( const float& x , const float& y ) {};

    void    useAttachedCam( smPositionVec offset, smAngleDVec hprOffset = smAngleDVec(), bool rotCamWithVehicle = true ) ;
    void    useAttachedCam( unsigned int entityIndex , smPositionVec offset, smAngleDVec hprOffset = smAngleDVec(), bool rotCamWithVehicle = true ) ;
    void    useFreeCam( smPositionVec offset, smAngleDVec hpr, bool lookAtVehicle, smReal distToModel = 0 ) ;
    void    useDualVehicleCam( ) ;
    void    usePrimaryToSecondaryCam( ) ;
    void    useSecondaryToPrimaryCam( ) ;
    void    setFOV( const smAngleD& h, const smAngleD& v, smReal n, smReal f ) ;
    //@}

    bool    getTerrainHeight( const smVec3r& xyz, float& h ) ;
    bool    getCollision( const smVec3r& sXYZ, const smVec3r& eXYZ, smVec3r& contactPos  , smVec3r& contactNormal) ;


    virtual void    getCameraInfo( smVec3r& position , smVec3r& forward , smVec3r& up , smVec3r& velocity );

    /**
        Returns the A310 animator instance of given entity.
    */
    A310AnimatorBase*   getA310Animator(unsigned int entIndex = 0);

    /**
        Returns the fighter jet animator instance of given entity.
    */
    FighterJetAnimatorBase*   getFighterJetAnimator(unsigned int entIndex = 0);

    /**
        Returns the Uh-60 animator instance of given entity.
    */
    UH60AnimatorBase*   getUH60Animator(unsigned int entIndex = 0 );

    /**
        Returns the armored vehicle animator instance of given entity.
    */
    ArmoredVehicleAnimatorBase* getArmoredVehicleAnimator(unsigned int entIndex = 0);

    void    setLSD41_XYZ( const smVec3r& xyz );

    void    setLSD41_HPR( const smVec3r& hpr );

    smVec3r getLSD41_XYZ() const;

    smVec3r getLSD41_HPR() const;


    void    setCVN68_XYZ( const smVec3r& xyz );

    void    setCVN68_HPR( const smVec3r& hpr );

    smVec3r getCVN68_XYZ() const;

    smVec3r getCVN68_HPR() const;


    void    drawVectorFromVehicleToPoint( const RTD::VehicleBase* vehicle, smVec3r pointInWCS, smReal scale = 1.0f, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) );

    void    drawVectorBCS( const RTD::VehicleBase* vehicle, const smVec3r& pointInBCS, const smVec3r& vectorInBCS , smReal scale = 1.0f, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) );

    void    drawVectorWCS(  const smVec3r& pointInWCS, const smVec3r& vectorInWCS , smReal scale = 1.0f, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) );


    void    drawAxesBCS(  const RTD::VehicleBase* vehicle, const smMatrix& axisSystemInBCS , smReal scale = 1.0f );

    void    drawVectorBCS( const RTD::VehicleBase* vehicle , RTD::ForceGenerator* fg, smReal scale, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) );

    void    setVehicle( unsigned int entIndex  , VehicleType vt , RTD::VehicleBase* vehicle , bool useBlueRibbon );
    RTD::VehicleBase*    getVehicle( unsigned int entIndex );

	virtual  int			 	 getVehicleIndex( const  RTD::VehicleBase* vehicle ) const { return -1; }

    void    setScene( const SceneType& st );

    void    setVehicleScale( unsigned int entIndex , smReal scale  );
    void    setVehicleScales( smReal scale  );

    void    resetAllVehicles();

	virtual void	setWindowSize( unsigned int x , unsigned int y );

	virtual void	addGenericSymbol( const std::string& texture , float depth) {}
	virtual void	removeAllGenericSymbols() {}
	virtual unsigned int	getNumGenericSymbols() const { return 0; }
	virtual void	enableGenericSymbol(unsigned int index , bool enable ) {}
	virtual void	setGenericSymbolPos( unsigned int index ,const smVec2r& pos ){}
	virtual void	setCursorVisibility( bool enable ) {}
private:
    double m_simTime;
    std::vector<RTD::VehicleBase*>  m_vehicleList;
};

