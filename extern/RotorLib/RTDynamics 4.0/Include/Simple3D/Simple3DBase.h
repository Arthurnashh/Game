#pragma once



#include "Simple3D/Common.h"

#include "RTD/VehicleBase.h"
#include "RTD/ForceGenerator.h"

#include "Simple3D/A310AnimatorBase.h"
#include "Simple3D/UH60AnimatorBase.h"
#include "Simple3D/FighterJetAnimatorBase.h"
#include "Simple3D/ArmoredVehicleAnimatorBase.h"


/**
*   The Simple3D class provides a simple 3D OpenGL world and a vehicle model.
*   It's main purpose is to keep the examples of RotorLib simple, uncluttered
*   by 3d code.
*/
class SIMPLE3D_EXPORT Simple3DBase
{
public:
    enum SceneType
    {
        /** Plane is a simple grid plane.   */
        Plane,

        /** Plane2 is a simple grid plane.  */
        Plane2,

        /** Terrain is a general purpose terrain with valleys.  */
        Terrain,

        /** Terrain2 is a general purpose terrain with valleys. */
        Terrain2,

        /** Terrain2 is a general purpose terrain   */
        Terrain3,

        /** Sea is a sea scene with ships.  */
        Sea,

        /** Large Area Database */
        LADB,

        /** Island Database */
        Island,
    };

    enum VehicleType
    {
        Helicopter,
        Airplane,
        A310,
		F16,
        ArmoredVehicle,
        SimplestAircraft,
		Bomb,
        Cube,    //@tmp
		F16_Transparent
    };

    enum CameraType
    {
        AttachedCam,
        FreeCam,
        DualVehicleCam,
		PrimaryToSecondaryCam,
		SecondaryToPrimaryCam,
    };

	enum SymbolKeys
	{
            KEY_Space           = 0x20,
            
            KEY_BackSpace       = 0xFF08,        /* back space, back char */
            KEY_Tab             = 0xFF09,
            KEY_Return          = 0xFF0D,        /* Return, enter */
            KEY_Pause           = 0xFF13,        /* Pause, hold */
            KEY_Escape          = 0xFF1B,
            KEY_Delete          = 0xFFFF,        /* Delete, rubout */


            /* Cursor control & motion */

            KEY_Home            = 0xFF50,
            KEY_Left            = 0xFF51,        /* Move left, left arrow */
            KEY_Up              = 0xFF52,        /* Move up, up arrow */
            KEY_Right           = 0xFF53,        /* Move right, right arrow */
            KEY_Down            = 0xFF54,        /* Move down, down arrow */
            KEY_Page_Up         = 0xFF55,
            KEY_Page_Down       = 0xFF56,
            KEY_End             = 0xFF57,        /* EOL */
            KEY_Begin           = 0xFF58,        /* BOL */

            KEY_KP_Space        = 0xFF80,        /* space */
            KEY_KP_Tab          = 0xFF89,
            KEY_KP_Enter        = 0xFF8D,        /* enter */
            KEY_KP_F1           = 0xFF91,        /* PF1, KP_A, ... */
            KEY_KP_F2           = 0xFF92,
            KEY_KP_F3           = 0xFF93,
            KEY_KP_F4           = 0xFF94,
            KEY_KP_Home         = 0xFF95,
            KEY_KP_Left         = 0xFF96,
            KEY_KP_Up           = 0xFF97,
            KEY_KP_Right        = 0xFF98,
            KEY_KP_Down         = 0xFF99,
            KEY_KP_Prior        = 0xFF9A,
            KEY_KP_Page_Up      = 0xFF9A,
            KEY_KP_Next         = 0xFF9B,
            KEY_KP_Page_Down    = 0xFF9B,
            KEY_KP_End          = 0xFF9C,
            KEY_KP_Begin        = 0xFF9D,
            KEY_KP_Insert       = 0xFF9E,
            KEY_KP_Delete       = 0xFF9F,
            KEY_KP_Equal        = 0xFFBD,        /* equals */
            KEY_KP_Multiply     = 0xFFAA,
            KEY_KP_Add          = 0xFFAB,
            KEY_KP_Separator    = 0xFFAC,       /* separator, often comma */
            KEY_KP_Subtract     = 0xFFAD,
            KEY_KP_Decimal      = 0xFFAE,
            KEY_KP_Divide       = 0xFFAF,

            KEY_KP_0            = 0xFFB0,
            KEY_KP_1            = 0xFFB1,
            KEY_KP_2            = 0xFFB2,
            KEY_KP_3            = 0xFFB3,
            KEY_KP_4            = 0xFFB4,
            KEY_KP_5            = 0xFFB5,
            KEY_KP_6            = 0xFFB6,
            KEY_KP_7            = 0xFFB7,
            KEY_KP_8            = 0xFFB8,
            KEY_KP_9            = 0xFFB9,
            
            KEY_F1              = 0xFFBE,
            KEY_F2              = 0xFFBF,
            KEY_F3              = 0xFFC0,
            KEY_F4              = 0xFFC1,
            KEY_F5              = 0xFFC2,
            KEY_F6              = 0xFFC3,
            KEY_F7              = 0xFFC4,
            KEY_F8              = 0xFFC5,
            KEY_F9              = 0xFFC6,
            KEY_F10             = 0xFFC7,
            KEY_F11             = 0xFFC8,
            KEY_F12             = 0xFFC9,
            KEY_F13             = 0xFFCA,
            KEY_F14             = 0xFFCB,
            KEY_F15             = 0xFFCC,
            KEY_F16             = 0xFFCD,
            KEY_F17             = 0xFFCE,
            KEY_F18             = 0xFFCF,
            KEY_F19             = 0xFFD0,
            KEY_F20             = 0xFFD1,

            KEY_Shift_L         = 0xFFE1,        /* Left shift */
            KEY_Shift_R         = 0xFFE2,        /* Right shift */
            KEY_Control_L       = 0xFFE3,        /* Left control */
            KEY_Control_R       = 0xFFE4,        /* Right control */
            KEY_Caps_Lock       = 0xFFE5,        /* Caps lock */
            KEY_Shift_Lock      = 0xFFE6,        /* Shift lock */
	};

public:
    Simple3DBase( SceneType scene = Plane, bool jsNeeded = false ) ;
    virtual ~Simple3DBase( ) ;

    void    showMessageBox( const std::string& title, const std::string& msgString );
    void    setInfoText(const std::string& info );

    virtual void update() ;


    virtual double  getFrameTime() = 0 ;
    virtual double  getFrameDeltaTime() = 0 ;
    virtual bool    isExitRequested() = 0 ;
    virtual bool    getKeyState( size_t key ) = 0 ;
    virtual bool    isKeyDown( size_t key ) = 0 ;
    virtual bool    isKeyUp( size_t key ) = 0 ;

    /** Returns true if a joystick is available and ready to use. */
    virtual bool    joystickAvailable()= 0 ;
    virtual bool    pedalsAvailable() = 0 ;
    virtual float   getJoystickAxes( int id )   {   return m_joystickAxes[id]; }
    virtual bool    getJoystickButton( int id ) {   return SM_SAFE_INT_BOOL_CAST( m_joystickButtons[id] ) ; }
    virtual bool    getJoystickButtonClicked( int id ) ;

    virtual float   getPedalAxes( int id )   {   return m_pedalAxes[id]; }

    virtual void    setMousePos( const float& x , const float& y ) = 0 ;

    virtual float   getMouseX() const= 0 ;
    virtual float   getMouseY() const= 0 ;
    virtual bool    getMouseWheelScrolledUp() const= 0 ;
    virtual bool    getMouseWheelScrolledDown() const= 0 ;
    virtual bool    getMouseLeftButton() const= 0 ;
    virtual bool    getMouseRightButton() const= 0 ;
    virtual bool    getMouseMiddleButton() const= 0 ;

    float   getJoystickPOVState() const {   return m_joystickPOV;   }

    /** @name Camera Modes
    *   Simple3D provides 3 different camera modes. The first one is selected
    *   with the useAttachedCam method and in this case the camera follows
    *   the vehicle as if it is attached to the vehicle.
    *
    *   The second type is selected with useFreeCam, it is freely positioned camera
    *   located at xyz and hpr.
    *
    *   If lookAtVehicle is selected, it is the third type. This camera looks at
    *   the vehicle from the specified xyz location. HPR is not used in this mode.
    */
    //@{
    virtual void    useAttachedCam( smPositionVec offset, smAngleDVec hprOffset = smAngleDVec(), bool rotCamWithVehicle = true  ) = 0 ;
    virtual void    useAttachedCam( unsigned int entityIndex , smPositionVec offset, smAngleDVec hprOffset = smAngleDVec(), bool rotCamWithVehicle = true ) = 0  ;
    virtual void    useFreeCam( smPositionVec offset, smAngleDVec hpr, bool lookAtVehicle, smReal distToModel = 0 ) = 0  ;
    virtual void    useDualVehicleCam( ) = 0  ;
	virtual void    usePrimaryToSecondaryCam( ) = 0  ;
	virtual void    useSecondaryToPrimaryCam( ) = 0  ;
    virtual void    setFOV( const smAngleD& h, const smAngleD& v, smReal n, smReal f ) = 0 ;
    virtual void    getFOV( smAngleD& h, smAngleD& v, smReal& n, smReal& f ) {};
    //@}

    /** @name Collision Detection
    *   Simplistic collision detection is also available. The getTerrainHeight() can do
    *   a simple test to find the height of terrain at a given xy position.
    *
    *   The getCollision() is more capable and it can be used to do a collision test
    *   with a ray (defined by the start and end points) and fills in the contact vector
    *   with the first contact point. Returns false if no collisions are detected, true
    *   otherwise.
    */
    //@{
    virtual bool    getTerrainHeight( const smVec3r& xyz, float& h ) = 0 ;
    virtual bool    getCollision( const smVec3r& sXYZ, const smVec3r& eXYZ, smVec3r& contactPos  , smVec3r& contactNormal) = 0 ;
    //@}


	virtual void	getCameraInfo( smVec3r& position , smVec3r& forward , smVec3r& up , smVec3r& velocity )= 0 ;

	CameraType		getActiveCameraType() const	{	return m_activeCamType;	}

    /**
        Returns the A310 animator instance of given entity.
    */
    virtual A310AnimatorBase*   getA310Animator(unsigned int entIndex = 0)= 0 ;

    /**
        Returns the fighter jet animator instance of given entity.
    */
    virtual FighterJetAnimatorBase*   getFighterJetAnimator(unsigned int entIndex = 0)= 0 ;

    /**
        Returns the Uh-60 animator instance of given entity.
    */
    virtual UH60AnimatorBase*   getUH60Animator(unsigned int entIndex = 0 )= 0 ;

    /**
        Returns the armored vehicle animator instance of given entity.
    */
    virtual ArmoredVehicleAnimatorBase* getArmoredVehicleAnimator(unsigned int entIndex = 0)= 0 ;

    /**
		Sets the position of the LSD41 ship model in the 3D world.
	*/
    virtual void    setLSD41_XYZ( const smVec3r& xyz )= 0 ;

    /**
		Sets the orientation of the LSD41 ship model in the 3D world. \e hpr
		contains heading, pitch and roll angles in degrees.
	*/
    virtual void    setLSD41_HPR( const smVec3r& hpr )= 0 ;

    /**
		Returns the position of the LSD41 ship model.
	*/
    virtual smVec3r getLSD41_XYZ() const= 0 ;

    /**
		Returns the orientation of the LSD41 ship model.\e hpr
		contains heading, pitch and roll angles in degrees.
	*/	
	virtual smVec3r getLSD41_HPR() const= 0 ;


	/**
		Sets the position of the aircraft carrier model in the 3D world.
	*/
    virtual void    setCVN68_XYZ( const smVec3r& xyz )= 0 ;

   /**
		Sets the orientation of the aircraft carrier model in the 3D world. \e hpr
		contains heading, pitch and roll angles in degrees.
	*/
    virtual void    setCVN68_HPR( const smVec3r& hpr )= 0 ;

    /**
		Returns the position of the aircraft carrier model.
	*/
    virtual smVec3r getCVN68_XYZ() const= 0 ;

    /**
		Returns the orientation of the aircraft carrier model.\e hpr
		contains heading, pitch and roll angles in degrees.
	*/
    virtual smVec3r getCVN68_HPR() const= 0 ;

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
    virtual void    drawVectorFromVehicleToPoint( const RTD::VehicleBase* vehicle, smVec3r pointInWCS, smReal scale = 1.0f, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) )= 0 ;

    /**
        Draws a vector \e vectorInBCS in body coordinate system of the given vehicle. Vector is positioned at \e pointInBCS .
    */
    virtual void    drawVectorBCS( const RTD::VehicleBase* vehicle , const smVec3r& pointInBCS, const smVec3r& vectorInBCS , smReal scale = 1.0f, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) )= 0 ;

    virtual void    drawVectorWCS(  const smVec3r& pointInWCS, const smVec3r& vectorInWCS , smReal scale = 1.0f, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) )= 0 ;


    virtual void    drawAxesBCS(const RTD::VehicleBase* vehicle ,  const smMatrix& axisSystemInBCS , smReal scale = 1.0f )= 0 ;

    virtual void    drawVectorBCS( const RTD::VehicleBase* vehicle , RTD::ForceGenerator* fg, smReal scale, smVec4r color1=smVec4r(1,0,0,1), smVec4r color2=smVec4r(1,1,0,1) )= 0 ;

    virtual void    setVehicle( unsigned int entIndex  , VehicleType vt , RTD::VehicleBase* vehicle , bool useBlueRibbon )= 0 ;
	virtual RTD::VehicleBase*    getVehicle( unsigned int entIndex )= 0 ;

	virtual  int			 	 getVehicleIndex( const  RTD::VehicleBase* vehicle ) const = 0;

	virtual void    setVehicleScale( unsigned int entIndex , smReal scale  )= 0 ;
	virtual void    setVehicleScales( smReal scale  )= 0 ;

	virtual std::string	getTerrainDBFileName()	{	return m_terrainDBFileName;	}

	virtual void	setWindowSize( unsigned int x , unsigned int y ) = 0;

	virtual void	resetAllVehicles() = 0;

	virtual void	addGenericSymbol( const std::string& texture , float depth ) = 0;
	virtual void	removeAllGenericSymbols() = 0;
	virtual unsigned int	getNumGenericSymbols() const = 0;
	virtual void	enableGenericSymbol( unsigned int index ,bool enable ) = 0;
	virtual void	setGenericSymbolPos( unsigned int index ,const smVec2r& pos ) = 0;
	virtual void	setCursorVisibility( bool enable  ) = 0;
protected:
    smReal  m_joystickAxes[8] ;
    smBool  m_joystickButtons[128] ;
    smBool  m_prevJoystickButtons[128] ;
    smReal  m_joystickPOV;

    smReal  m_pedalAxes[8] ;

	std::string	m_homeDir;
	std::string	m_dataDir;
	std::string	m_3dDataDir;
	std::string m_terrainDBFileName;

    CameraType  m_activeCamType ;
};
