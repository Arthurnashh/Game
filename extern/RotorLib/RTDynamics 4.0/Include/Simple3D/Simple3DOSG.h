#pragma once

#include "Simple3DBase.h"

namespace RLDynamicsUtil
{
    class DX8Joystick ;
}

class Simple3DOSGData ;

/**
*   The Simple3D class provides a simple 3D OpenGL world and a vehicle model.
*   It's main purpose is to keep the examples of RotorLib simple, uncluttered
*   by 3d code.
*/
class SIMPLE3D_EXPORT Simple3DOSG : public Simple3DBase
{
public:
    

public:
    Simple3DOSG( SceneType scene = Plane, bool jsNeeded = false , bool dontOpenWindow = false ) ;
    virtual ~Simple3DOSG( ) ;

    void    showMessageBox( const std::string& title, const std::string& msgString );
    void    setInfoText(const std::string& info );

    virtual void update() ;

    double  getFrameTime() ;
    double  getFrameDeltaTime() ;
    bool    isExitRequested() ;
    bool    getKeyState( size_t key ) ;
    bool    isKeyDown( size_t key ) ;
    bool    isKeyUp( size_t key ) ;

    /** Returns true if a joystick is available and ready to use. */
    bool    joystickAvailable();
    bool    pedalsAvailable();

	virtual void   setMousePos( const float& x , const float& y );
    virtual float   getMouseX() const;
    virtual float   getMouseY() const;
    bool    getMouseWheelScrolledUp() const;
    bool    getMouseWheelScrolledDown() const;
    bool    getMouseLeftButton() const;
    bool    getMouseRightButton() const;
    bool    getMouseMiddleButton() const;

    void    useAttachedCam( smPositionVec offset, smAngleDVec hprOffset = smAngleDVec(), bool rotCamWithVehicle = true ) ;
    void    useAttachedCam( unsigned int entityIndex , smPositionVec offset, smAngleDVec hprOffset = smAngleDVec(), bool rotCamWithVehicle = true ) ;
    void    useFreeCam( smPositionVec offset, smAngleDVec hpr, bool lookAtVehicle, smReal distToModel = 0 ) ;
    void    useDualVehicleCam( ) ;
	void    usePrimaryToSecondaryCam( ) ;
	void    useSecondaryToPrimaryCam( ) ;
    void    setFOV( const smAngleD& h, const smAngleD& v, smReal n, smReal f ) ;
    void    getFOV( smAngleD& h, smAngleD& v, smReal& n, smReal& f );

    bool    getTerrainHeight( const smVec3r& xyz, float& h ) ;
    bool    getCollision( const smVec3r& sXYZ, const smVec3r& eXYZ, smVec3r& contactPos  , smVec3r& contactNormal) ;


	virtual void	getCameraInfo( smVec3r& position , smVec3r& forward , smVec3r& up , smVec3r& velocity );

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

    /**
		Sets the position of the LSD41 ship model in the 3D world.
	*/
    void    setLSD41_XYZ( const smVec3r& xyz );

    /**
		Sets the orientation of the LSD41 ship model in the 3D world. \e hpr
		contains heading, pitch and roll angles in degrees.
	*/
    void    setLSD41_HPR( const smVec3r& hpr );

    /**
		Returns the position of the LSD41 ship model.
	*/
    smVec3r getLSD41_XYZ() const;

    /**
		Returns the orientation of the LSD41 ship model.\e hpr
		contains heading, pitch and roll angles in degrees.
	*/	
	smVec3r getLSD41_HPR() const;


	/**
		Sets the position of the aircraft carrier model in the 3D world.
	*/
    void    setCVN68_XYZ( const smVec3r& xyz );

   /**
		Sets the orientation of the aircraft carrier model in the 3D world. \e hpr
		contains heading, pitch and roll angles in degrees.
	*/
    void    setCVN68_HPR( const smVec3r& hpr );

    /**
		Returns the position of the aircraft carrier model.
	*/
    smVec3r getCVN68_XYZ() const;

    /**
		Returns the orientation of the aircraft carrier model.\e hpr
		contains heading, pitch and roll angles in degrees.
	*/
    smVec3r getCVN68_HPR() const;

    /** 
	*	Draws a vector from a vehicle's center to a point in the world.
	*
    *   @param  vehicle     the vehicle to be used as start point
    *   @param  pointInWCS  pointInWCS is a point in WCS. A vector which points from body center
    *                       to this point will be drawn.
    *   @param  scale       scale
    *   @param  color1      color at the starting point of the vector
    *   @param  color2      color at the ending point of the vector
    *  
    */
    void    drawVectorFromVehicleToPoint( const RTD::VehicleBase* vehicle, smVec3r pointInWCS, smReal scale = 1.0f, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) );

    /**
        Draws a vector \e vectorInBCS in body coordinate system. Vector is positioned at \e pointInBCS .
    */
    void    drawVectorBCS(  const RTD::VehicleBase* vehicle, const smVec3r& pointInBCS, const smVec3r& vectorInBCS , smReal scale = 1.0f, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) );

    void    drawVectorWCS(  const smVec3r& pointInWCS, const smVec3r& vectorInWCS , smReal scale = 1.0f, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) );


    void    drawAxesBCS(  const RTD::VehicleBase* vehicle, const smMatrix& axisSystemInBCS , smReal scale = 1.0f );

    void    drawVectorBCS( const RTD::VehicleBase* vehicle , RTD::ForceGenerator* fg, smReal scale, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) );

    void    setVehicle( unsigned int entIndex  , VehicleType vt , RTD::VehicleBase* vehicle , bool useBlueRibbon );
	RTD::VehicleBase*    getVehicle( unsigned int entIndex );

	virtual int			 getVehicleIndex( const  RTD::VehicleBase* vehicle ) const ;

	void    setScene( const SceneType& st );

	void    setVehicleScale( unsigned int entIndex , smReal scale  );
	void    setVehicleScales( smReal scale  );

	void	resetAllVehicles();

	void	setWindowSize( unsigned int x , unsigned int y );

	virtual void	addGenericSymbol( const std::string& texture , float depth );
	virtual void	removeAllGenericSymbols();
	virtual unsigned int	getNumGenericSymbols() const;
	virtual void	enableGenericSymbol(unsigned int index , bool enable ) ;
	virtual void	setGenericSymbolPos( unsigned int index ,const smVec2r& pos );
	virtual void	setCursorVisibility( bool enable ) ;
private:
    Simple3DOSGData*					m_data ;

    RLDynamicsUtil::DX8Joystick*        mJoystick ;
    RLDynamicsUtil::DX8Joystick*        mPedals;

	void	updateDualVehicleCam();
	void	updateXtoYCam( bool isPrimaryToSecondary );
	void	enlargeVectorList( unsigned int requestedIndex );
};

