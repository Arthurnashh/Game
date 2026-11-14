#pragma once
#include <Es/Types/pointers.hpp>

struct SMoveObjectPosMessData;
struct SObjectPosMessData;
struct SLandscapePosMessData;
struct STexturePosMessData;
struct SLandSelPosMessData;
template<class T> class Array;
///Definition of Buldozer-Visitor interface
class IVisViewerExchangeDocView
{
public:
	enum Status
	{
		statOk, ///<Status OK
		statLost, ///<Other side is lost. Connection has been reset, or other com-error
		statError, ///<General Error - invalid message, invalid parameters, etc.
		statTimeout, ///<Timeout elapses during waiting on other side
		statUnknownMessage, ///<Received an unknown message. 
		statNoMessage ///<Status OK, but no message arrived.
	};
protected:
	///Last status
	Status _lastStatus;
public:
	///Receives last status.
	/**
	When function is called, internal status is reset to statOk.
	*/
	Status GetLastStatus() 
	{
		Status res = _lastStatus;
		_lastStatus = statOk;
		return res;
	}

	///Message SystemQuit
	/**Forces viewer to quit */
	virtual void SystemQuit() = 0;
	///Message SelectionObjectClear
	/** Notifies other side, that selection was cleared*/
	virtual void SelectionObjectClear() = 0;
	///Message CursorPositionSet
	/** Changes cursor position on other side */
	virtual void CursorPositionSet(const SMoveObjectPosMessData& data) = 0;  
	///Message ObjectCreate
	/** Creates new object on other side */
	virtual void ObjectCreate(const SMoveObjectPosMessData& data, const char* name) = 0;
	///Message ObjectDestroy
	/** Destroys new object on other side */
	virtual void ObjectDestroy(const SMoveObjectPosMessData& data) = 0;
	///Message SelectionObjectAdd
	/** Adds new selection on other side */
	virtual void SelectionObjectAdd(const SObjectPosMessData& data) = 0;  
	///Message SystemInit
	/** Initializes other side */
	virtual void SystemInit(const SLandscapePosMessData& data, const char* configName) = 0;
	///Message FileImportBegin
	/** Starts import. Other side is switched to import mode until FileImportEnd is received */
	virtual void FileImportBegin(const STexturePosMessData& data) = 0;
	///Message FileImportEnd
	/** Exits import mode. */
	virtual void FileImportEnd() = 0;
	///Message FileImport
	/**
	Currently not implemented
	*/
	virtual void FileImport(const char* data) = 0;
	///Message FileExport
	/** Exports land from buldozer */  
	virtual void FileExport(const char* data) = 0;
	/// Message LandHeightChange
	/** Changes land height on other side */
	virtual void LandHeightChange(const STexturePosMessData& data) = 0;
	/// Message LandTextureChange
	/** Changes land texture in other side */
	virtual void LandTextureChange(const STexturePosMessData& data) = 0;
	/// Message RegisterObjectType
	/** Registers object on other side */
	virtual void RegisterObjectType(const char* name) = 0;
	/// Message RegisterLandscapeTexture
	/** Registers land texture on other side. Each texture must be registered before it can be used */
	virtual void RegisterLandscapeTexture(const SMoveObjectPosMessData& data, const char* name) = 0;
	/// Message SelectionLandAdd
	/** Adds land selection */
	virtual void SelectionLandAdd(const SLandSelPosMessData& data) = 0;
	/// Message SelectionLandClear
	/** Clears current land selection */
	virtual void SelectionLandClear() = 0;  
	/// Message BlockMove
	/** Moves block of objects */
	virtual void BlockMove(const Array<SMoveObjectPosMessData>& data) = 0;
	virtual void BlockDuplicateSel(const Array<SMoveObjectPosMessData>& data) = 0;
	virtual void BlockRemoveSel(const Array<SMoveObjectPosMessData>& data) = 0;
	/// Message BlockSelectionObject
	/** Selects block of objects */
	virtual void BlockSelectionObject(const Array<SMoveObjectPosMessData>& data) = 0;
	/// Message BlockSelectionLand
	/** Selects multiple land blocks */
	virtual void BlockSelectionLand(const Array<SLandSelPosMessData>& data) = 0;
	/// Message BlockLandHeightChange
	/** Changes height on multiple places */
	virtual void BlockLandHeightChange(const Array<STexturePosMessData>& data) = 0;
	/// Message BlockLandHeightChangeInit
	/** Used during FileImportBegin. Call this repeatedly until full island is not initialized */
	virtual void BlockLandHeightChangeInit(const Array<float>& heights) = 0;
	/// Message BlockLandTextureChangeInit
	/** Used during FileImportBegin. Call this repeatedly until full island is not initialized */
	virtual void BlockLandTextureChangeInit(const Array<int>& ids) = 0;
	/// Message BlockWaterHeightChangeInit
	/** Used during FileImportBegin. Call this repeatedly until full island is not initialized */
	virtual void BlockWaterHeightChangeInit(const Array<float>& heights) = 0;
	/// Message ComVersion
	/** Sends interface version to other side. Depend on version, communication will be accepted or terminated */
	virtual void ComVersion(int version) = 0;  
	/// Message ObjectMove
	/** Moves one object */
	virtual void ObjectMove(const SMoveObjectPosMessData& data) = 0;

	///Optional AddRef function. 
	/**
	Useful, if you want to share this interface. Exchange system will call this members
	to notify implementation about sharing instance. 

	If you don't need ref counting, leave it empty
	@return function returns count of references after AddRef. It should return nonzero, if refcounting is not used
	*/
	virtual int AddRef() const = 0;

	///Optional Release function. 
	/**
	Useful, if you want to share this interface. Exchange system will call this members
	to notify implementation about sharing instance. 

	If you don't need ref counting, leave it empty
	@return function returns count of references after Release. It should return zero, if refcounting is not used
	*/
	virtual int Release() const = 0;
};

///Interface extension for interface pooling
class IVisViewerExchange : public IVisViewerExchangeDocView
{
public:

private:
	///Reference to event receiver.
	Ref<IVisViewerExchangeDocView> _event;

protected:
	///Gets event receiver
	IVisViewerExchangeDocView& GetView()
	{
		Assert(_event.NotNull());
		return *_event.GetRef();
	}
public:  
	///Initializes exchange system with event receiver (must be implemented by the App)
	IVisViewerExchange(IVisViewerExchangeDocView& event)
	: _event(&event) 
	{
	}
  
	///Checks the state of connection
	/**
	Function checks the state of connection. If there is message in queue, processes it
	and calls one of member function on event receiver. 
	@return statOk - message has been processed, statNoMessage - no message has been processed,
    statError - error state, statLost - connectionLost, statUnknownMessage - unknown message received.
	*/
	virtual Status CheckInterfaceStatus() = 0;

	///Not implemented.
	virtual Status Import() = 0;

	///Starts buffering.
	/**
	When buffer is enabled, messages are not posted immediately, but they are stored
    in buffer. When buffer is full, whole content is send to other side, and buffer is
    cleared. This may rapidly increase communication speed.
    
    If you want to stop buffering, call EndBuffer

    @param bufferSize size of buffer.
    @return state of the interface. If buffering is not supported, function returns statError

    @note during buffering, interface state is still checked, so application must be prepared 
    to process all incoming events.
	*/
	virtual Status StartBuffer(size_t bufferSize) 
	{
		return statError;
	}

	///Ends buffering
	/**
	If buffer is not empty, it is flushed to other side. Then buffer is deallocated.
	@return state of the interface. If buffering is not supported, function returns statError
	*/
	virtual Status EndBuffer() 
	{
		return statError;
	}
};