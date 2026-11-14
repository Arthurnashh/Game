#pragma once

#include <El/TCPIPBasics/Socket.h>
#include <Es/Containers/array.hpp>
#include "messagedll.h"
#include <Es/Types/pointers.hpp>
#include <assert.h>

#include "IVisViewerExchange.h"



///Implements message exchanging, but doesn't define transfer technology.
class VisViewerExchange: public IVisViewerExchange, public RefCount
{
public:

private:
  Socket _sock;
  unsigned int _defaultTimeout;
protected:

  ///Sends a message
  /**
  @return status
  */
  virtual Status SendMessage(const SPosMessage& msg)=0;
  ///Receives a message
  /**
  @return status
  */
  virtual Status ReceiveMessage(PSPosMessage& msg)=0;
  
public:

  ///Dispatches message using some of virtual functions
  virtual Status DispatchMessage(const SPosMessage &msg);

  ///Waits on specific message
  /**
  @return 1-no message or message arrived, 0-connection lost, -1 connection error, -2 timeout
  */
  virtual Status WaitMessage(const Array<int> &msgIds, PSPosMessage& msg, unsigned long timeout=0xFFFFFFFF)=0;

public:
  VisViewerExchange(IVisViewerExchangeDocView &event);
  ~VisViewerExchange(void);


  virtual Status CheckInterfaceStatus()=0;


  void SystemQuit();

  void SelectionObjectClear();

  void CursorPositionSet(const SMoveObjectPosMessData &data);

  void ObjectCreate(const SMoveObjectPosMessData &data, const char *name);

  void ObjectDestroy(const SMoveObjectPosMessData &data);

  void SelectionObjectAdd(const SObjectPosMessData &data);
  
  void SystemInit(const SLandscapePosMessData &data, const char *configName);

  void FileImportBegin(const STexturePosMessData& data);

  void FileImportEnd();

  void FileImport(const char *name);

  void FileExport(const char *name);

  void LandHeightChange(const STexturePosMessData& data);

  void LandTextureChange(const STexturePosMessData& data);

  void RegisterObjectType(const char *name);

  virtual void RegisterLandscapeTexture(const SMoveObjectPosMessData &data, const char *name);

  void SelectionLandAdd(const SLandSelPosMessData& data);

  void SelectionLandClear();

  void BlockMove(const Array<SMoveObjectPosMessData> &data);
  void BlockDuplicateSel(const Array<SMoveObjectPosMessData> &data);
  void BlockRemoveSel(const Array<SMoveObjectPosMessData> &data);

  void BlockSelectionObject(const Array<SMoveObjectPosMessData> &data);

  void BlockSelectionLand(const Array<SLandSelPosMessData> &data);

  void BlockLandHeightChange(const Array<STexturePosMessData> &data);

  void BlockLandHeightChangeInit(const Array<float> &heights);

  void BlockLandTextureChangeInit(const Array<int> &ids);

  void BlockWaterHeightChangeInit(const Array<float> &heights);

  void ComVersion(int version);

  void ObjectMove(const SMoveObjectPosMessData& data);

  Status Import();

  virtual int AddRef() const {return RefCount::AddRef();}
  virtual int Release() const {return RefCount::Release();}  
};
