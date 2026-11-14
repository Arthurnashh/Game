#ifndef __LVC_GAME_API_H__
#define __LVC_GAME_API_H__

#include "LVCGame_API_Datatypes.h"

#if _VBS3
  class IRemoteLogger;
  class CNRToolsFactory;
#else
  #include "CNRToolsFactory.h"
  #include "IRemoteLogger.h"
  #include "Types.h"
#endif


#ifdef LVCGAME_DLL_EXPORTS
#define LVCGAME_DLL_API __declspec(dllexport)
#else
#define LVCGAME_DLL_API __declspec(dllimport)
#endif

namespace LVCGamePackage
{
class LVCGameExecutable;
}

namespace SGMiddleware
{
class SGClient;
}

class LVCGAME_DLL_API LVCGame_API
{
public:

    LVCGame_API();
    virtual ~LVCGame_API();

    /**
     * Initialise LVC Game API.
     * This will bring everything up, join the middleware session
     * and be ready to receive / send information on sim middleware.
     *
     * @return true if initialisation was successful or false otherwise
     */
    virtual bool init();

    /**
     * Initialise LVC Game API.
     * This will bring everything up, join the middleware session
     * and be ready to receive / send information on sim middleware.
     *
     * @param path the path (relative or absolute) to the LVC Game API 
     * configuration files
     *
     * @return true if initialisation was successful or false otherwise
     */
    virtual bool init(const char* path);

    /**
     * Indicates whether LVC Game API has been initialised correctly.
     * If the LVC Game API has initialised, it will be connected to the
     * sim middleware and ready to go.
     *
     * @return true if initialised or false otherwise
     */
    bool LVCGame_API::isInitialised();

    /**
     * Tick LVC Game.
     *
     * This will give control to LVC Game allowing up to n events to 
     * be processed. This method will return when either the event queue
     * is empty or n events have been processed.
     *
     * @param n the maximum number of events to process this time around
     *
     * @return -1 if error was encountered otherwise number of events
     *         that remain on the event queue.
     */
    virtual int tick(int n);

    /**
     * Shutdown the LVC Game
     *
     */
    virtual void shutdown();

    /**
     * Determines if the LVC Game lifecycle has stopped 
     */
    virtual bool isStopped();

    /**
     * Map origin for the current map in lat/long.
     * The map origin is the lower south east corner.
     *
     * @param latitude the latitude of the map origin in
     *        DD*MM'SS.ss" format
     * @param longitude the longitude of the map origin in
     *        DD*MM'SS.ss" format
     */
    virtual void setOrigin(
        const char* latitude,
        const char* longitude);

    /**
     * Map origin for the current map in lat/long.
     * The map origin is the lower south east corner.
     *
     * @param easting
     * @param northing
     * @param zone
     * @param hemisphere
     */
    virtual void setOrigin(
        double easting,
        double northing,
        long zone,
        char hemisphere);

    /**
     * An entity was created within the Serious Game
     *
     * @param data the available data on the entity.
     *
     * @return true if entity created or false otherwise
     */
    virtual bool entityCreated(EntityCreateData data);

    /**
     * An entity was updated within the Serious Game
     *
     * @param data the available data on the entity.
     */
    virtual void entityUpdated(EntityUpdateData data);

    /**
     * An entity or designator was removed from the Serious Game
     *
     * @param id the SG ID of the entity or designator.
     */
    virtual void entityDeleted(EntityDeleteData id);

    /**
     * A Laser Designator was created in the Serious Game
     *
     * @param data the available data on the designator.
     */
    virtual void designatorCreated(DesignatorCreateData data);

    /**
     * A Laser Designator was updated in the Serious Game
     *
     * @param data the available data on the designator.
     */
    virtual void designatorUpdated(DesignatorUpdateData data);

    /**
     * The state of the simulation changed within the Serious Game
     *
     * @param data the available data on the simulation state.
     */
    virtual void simulationStateChanged(SimStateData data);

    /**
     * Serious Game munition has been fired
     * 
     * Note: this may not actually be a visible 
     * munition in the game.
     */
    virtual void weaponFired(FireWeaponData data);

    /**
     * VBS2 munition entity has detonated
     */
    virtual void munitionDetonation(DetonationData data);

	/**
	 * Send a Set Data PDU
	 */
	virtual void sendSetDataPDU(DataScript scriptData);

	/**
	 * Send a Data PDU
	 */
	virtual void sendDataPDU(DataScript scriptData);

    /**
     * Callback registration for entity and weapon events
     */
    virtual void setCreateEntity(EntityId (*fPtr)(EntityCreateData data));
    virtual void setUpdateEntity(int (*fPtr)(EntityUpdateData data));
    virtual void setDeleteEntity(void (*fPtr)(EntityDeleteData data));
    virtual void setCreateDesignator(void (*fPtr)(DesignatorCreateData data));
    virtual void setUpdateDesignator(void (*fPtr)(DesignatorUpdateData data));
    virtual void setFireWeapon(void (*fPtr)(FireWeaponData data));
    virtual void setDetonateMunition(void (*fPtr)(DetonationData data));

	/* 
	 * Call back registratioin for script injecting into VBS
	 */	
	virtual void setExecuteCommand(int (*fPtr)(const char* command, char* result, int resultLength));


    /**
     * Enables all callback methods
     *
     * Once enabled callbacks will be invoked as appropriate
     * Ensure that initialisation of the API is complete and
     * callbacks have been registered.
     */
    virtual void enableCallbacks();

    /**
     * Disables all callback methods
     */
    virtual void disableCallbacks();

	/**
	 * Converts a Lat/Long to a VBS coordinate
	 */
	virtual bool LatLongToVBScoord(char *lat, char *lon, Coordinate3DType &coord);

	/**
	 * Converts a VBS coordinate to a Lat/Long
	 */
	virtual bool VBSCoordToLatLong(Coordinate3DType &coord, char *lat, char *lon, int length);


	/**
	 * Associates an RemoteLoggerInterface to an instance of a RemoteLogger
	 * Calls the RemoteLogger Factory
	 *
	 * @return 1 or 0 for failure or success
  	 **/
	virtual int CNRLogInitRemoteLogger();


	/**
	 * Release Logger for deletion. Removes instance of IRemoteLogger
	 *
	 * @return 1 for failure or success
	 */
	virtual int CNRLogReleaseRemoteLogger();


	/**
	 * Halts Logger Recording Session remotely
	 *
	 * @return 1 for failure or success
  	 **/
	virtual int CNRLogStopRecording();


	/**
	 * Initiates logger recording session remotely. Creates a recording file
	 *
	 * @return 1 for failure or success
  	 **/
	virtual int CNRLogStartRecording();


	/**
	 * Notifies caller whether recording session is in progress
	 *
	 * @return 1 for failure or success
  	 **/
	virtual int CNRLogIsRecording(bool &isRecording);


	/**
	 * Opens new session on the logger
	 *
	 * param@ sessionName as the session you want to open
	 * @return 1 for failure or success
  	 **/
	virtual int CNRLogOpenSession(const char* sessionName);

	/**
	* Gets one event at a specified index of the current session
	*
	* param@ int index. The index number starting from 0
	* specifies a recording index number
	*
	* param@ requires an event struct to pass into the method. If successful (true) 
	* the event will contain event information
	*
	* @return bool. Returns true if execution was successful.
	**/
	virtual bool CNRLogGetEvent(int index, CNRLogEvent& newEvent);

	/**
	 * Retrieves the number of recordings available in the current session
	 *
	 * return@ int. 0 means no recordings.
  	 **/
	virtual int CNRLogGetEventSize();


	/**
	 * Halts event playing on the logger
	 *
	 * @return 1 for failure or success
  	 **/
	virtual int CNRLogStopPlaying();


	/**
	 * Plays specified session event
	 *
	 * param@ index is the event id in the session
	 * @return 1 for failure or success
  	 **/
	virtual int CNRLogPlayOneRecord(int index);


	/**
	 * Plays all events available on the current session
	 * The logger throwa a no events to play exception
	 *
	 * @return 1 for failure or success
  	 **/
	virtual int CNRLogPlayAllRecords();


	/**
	 * Returns start time of the most current recording session.
	 *
	 * If there is no recording times availble. Returns a -1
  	 **/
	virtual long CNRLogGetStartTime();


	/**
	 * Removes session folder from the CNRLOG-Session directory. 
	 *
	 * param@ sessionName as the name of the session the user wants to the delete
	 * the name is case sensitive
	 * @return 1 for failure or success
  	 **/
	virtual int CNRLogDeleteSession(const char* sessionName);

private:
    
    LVCGamePackage::LVCGameExecutable* m_exe;   // handle to LVC Game controller
    SGMiddleware::SGClient* m_sgClient;         // handle to SG entity processing
    bool m_initialised;                         // indicates that LVC Game has initialised
    char m_path[256];                           // path to configuration files

	IRemoteLogger* m_cnrLogger;					// IRemoteLogger interface to handle calls
	CNRToolsFactory *m_factory;					//factory that will place a RemoteLogger against an interface
	
};



#endif

