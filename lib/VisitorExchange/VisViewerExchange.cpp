#include <Es/essencepch.hpp>
#include <Es/Common/win.h>
#include "visviewerexchange.h"

VisViewerExchange::VisViewerExchange(IVisViewerExchangeDocView &event):IVisViewerExchange(event)
{
  _defaultTimeout=10000;
}

VisViewerExchange::~VisViewerExchange(void)
{
}


#define MSG_REACTION(p,msg,fn) case msg: GetView().fn(GetPosMessageData(en_##msg(), &p));break;
#define MSG_REACTION_SIMPLE(p,msg,fn) case msg: GetView().fn();break;
#define MSG_REACTION_STR(p,msg,fn,txt) case msg: GetView().fn(GetPosMessageData(en_##msg(), &p),GetPosMessageData(en_##msg(), &p).txt);break;
#define MSG_REACTION_SIMPSTR(p,msg,fn,txt) case msg: GetView().fn(GetPosMessageData(en_##msg(),&p).txt);break;


VisViewerExchange::Status VisViewerExchange::DispatchMessage(const SPosMessage &msg)
{
  switch (msg._iMsgType)
  {
    MSG_REACTION_SIMPLE (msg,SYSTEM_QUIT,SystemQuit)
    MSG_REACTION_STR    (msg,SYSTEM_INIT,SystemInit,configName)
    MSG_REACTION_SIMPSTR(msg,FILE_IMPORT,FileImport,szFileName)
    MSG_REACTION_SIMPSTR(msg,FILE_EXPORT,FileExport,szFileName)
    MSG_REACTION        (msg,CURSOR_POSITION_SET,CursorPositionSet)
    MSG_REACTION_SIMPLE (msg,SELECTION_OBJECT_CLEAR,SelectionObjectClear)
    MSG_REACTION        (msg,SELECTION_OBJECT_ADD,SelectionObjectAdd)
    MSG_REACTION_STR(msg,REGISTER_LANDSCAPE_TEXTURE,RegisterLandscapeTexture,szName)
    MSG_REACTION_SIMPSTR(msg,REGISTER_OBJECT_TYPE,RegisterObjectType,szName)
    MSG_REACTION        (msg,LAND_HEIGHT_CHANGE,LandHeightChange)
    MSG_REACTION        (msg,LAND_TEXTURE_CHANGE,LandTextureChange)
    MSG_REACTION_STR    (msg,OBJECT_CREATE,ObjectCreate,szName)
    MSG_REACTION        (msg,OBJECT_DESTROY,ObjectDestroy)
    MSG_REACTION        (msg,OBJECT_MOVE,ObjectMove)
    MSG_REACTION        (msg,FILE_IMPORT_BEGIN,FileImportBegin)
    MSG_REACTION_SIMPLE (msg,FILE_IMPORT_END,FileImportEnd)
    MSG_REACTION_SIMPLE (msg,SELECTION_LAND_CLEAR,SelectionLandClear)
    MSG_REACTION        (msg,SELECTION_LAND_ADD,SelectionLandAdd)
    MSG_REACTION        (msg,BLOCK_MOVE,BlockMove)
    MSG_REACTION        (msg,BLOCK_DUPLICATE_SEL,BlockDuplicateSel)    
    MSG_REACTION        (msg,BLOCK_REMOVE_SEL,BlockRemoveSel)    
    MSG_REACTION        (msg,BLOCK_SELECTION_OBJECT,BlockSelectionObject)
    MSG_REACTION        (msg,BLOCK_SELECTION_LAND,BlockSelectionLand)
    MSG_REACTION        (msg,BLOCK_LAND_HEIGHT_CHANGE,BlockLandHeightChange)
    MSG_REACTION        (msg,BLOCK_LAND_HEIGHT_CHANGE_INIT,BlockLandHeightChangeInit)
    MSG_REACTION        (msg,BLOCK_LAND_TEXTURE_CHANGE_INIT,BlockLandTextureChangeInit)
    MSG_REACTION        (msg,BLOCK_WATER_HEIGHT_CHANGE_INIT ,BlockWaterHeightChangeInit)
    MSG_REACTION        (msg,COM_VERSION,ComVersion)
  default: return statUnknownMessage;
  }
  return statOk;
}


#define SEND_MSG_SIMPLE(id)   message_type_##id msg; \
                              msg._iMsgType = id;\
                              _lastStatus=SendMessage(msg);

#define SEND_MSG_DATA(id,data) message_type_##id msg; \
                              msg._iMsgType = id;\
                              msg._data=data;  \
                              _lastStatus=SendMessage(msg);

#define SEND_MSG_DATAARRAY(id,data) message_type_##id msg; \
                              msg._iMsgType = id;\
                              msg._data.Copy(data.Data(),data.Size());\
                              _lastStatus=SendMessage(msg);


#define SEND_MSG_DATA_VL(id,data,txt,txtname) message_type_##id msg; \
                              msg._iMsgType = id;\
                              msg._size = sizeof(*msg._data) + strlen(txt); \
                              msg._data = (type_##id *) new unsigned char[msg._size];\
                              memcpy(msg._data,&data,sizeof(*msg._data));\
                              strcpy(msg._data->txtname, txt);\
                              _lastStatus=SendMessage(msg);

#define SEND_MSG_DATA_VL2(id,data,txt,txtname) message_type_##id msg; \
                              msg._iMsgType = id;\
                              msg._size = sizeof(*msg._data) + strlen(txt); \
                              msg._data = (type_##id *) new unsigned char[msg._size];\
                              strcpy(msg._data->txtname, txt);\
                              _lastStatus=SendMessage(msg);

void VisViewerExchange::SystemQuit()
{
  SEND_MSG_SIMPLE(SYSTEM_QUIT)
}

void VisViewerExchange::SelectionObjectClear()
{
  SEND_MSG_SIMPLE(SELECTION_OBJECT_CLEAR)
}

void VisViewerExchange::CursorPositionSet(const SMoveObjectPosMessData &data)
{
  SEND_MSG_DATA(CURSOR_POSITION_SET, data)
}

void VisViewerExchange::ObjectCreate(const SMoveObjectPosMessData &data, const char *name)
{
  SEND_MSG_DATA_VL(OBJECT_CREATE, data,name,szName)
}

void VisViewerExchange::ObjectDestroy(const SMoveObjectPosMessData &data)
{
  SEND_MSG_DATA(OBJECT_DESTROY, data)
}

void VisViewerExchange::SelectionObjectAdd(const SObjectPosMessData &data)
{
  SEND_MSG_DATA(SELECTION_OBJECT_ADD, data)
}

void VisViewerExchange::SystemInit(const SLandscapePosMessData &data, const char *configName)
{
  SEND_MSG_DATA_VL(SYSTEM_INIT, data,configName,configName)
}

void VisViewerExchange::FileImportBegin(const STexturePosMessData& data)
{
  SEND_MSG_DATA(FILE_IMPORT_BEGIN,data)  
}

void VisViewerExchange::FileImportEnd()
{
  SEND_MSG_SIMPLE(FILE_IMPORT_END)    
}

void VisViewerExchange::FileImport(const char *name)
{
  SEND_MSG_DATA_VL2(FILE_IMPORT,data,name,szFileName)  
}

void VisViewerExchange::FileExport(const char *name)
{
  SEND_MSG_DATA_VL2(FILE_EXPORT,data,name,szFileName)  
}

void VisViewerExchange::LandHeightChange(const STexturePosMessData& data)
{
  SEND_MSG_DATA(LAND_HEIGHT_CHANGE,data)  
}

void VisViewerExchange::LandTextureChange(const STexturePosMessData& data)
{
  SEND_MSG_DATA(LAND_TEXTURE_CHANGE,data)  
}

void VisViewerExchange::RegisterObjectType(const char *name)
{
  SObjectNamePosMessData data;
  SEND_MSG_DATA_VL(REGISTER_OBJECT_TYPE,data,name,szName)  
}

void VisViewerExchange::RegisterLandscapeTexture(const SMoveObjectPosMessData &data, const char *name)
{  

  SEND_MSG_DATA_VL(REGISTER_LANDSCAPE_TEXTURE,data,name,szName)  
}

void VisViewerExchange::SelectionLandAdd(const SLandSelPosMessData& data)
{
  SEND_MSG_DATA(SELECTION_LAND_ADD,data)  
}

void VisViewerExchange::SelectionLandClear()
{
  SEND_MSG_SIMPLE(SELECTION_LAND_CLEAR)  
}

void VisViewerExchange::BlockMove(const Array<SMoveObjectPosMessData> &data)
{
  SEND_MSG_DATAARRAY(BLOCK_MOVE,data)  
}

//-----------------------------------------------------------------------------
void VisViewerExchange::BlockDuplicateSel(const Array<SMoveObjectPosMessData> &data)
{
  SEND_MSG_DATAARRAY(BLOCK_DUPLICATE_SEL,data)  
}

//-----------------------------------------------------------------------------
void VisViewerExchange::BlockRemoveSel(const Array<SMoveObjectPosMessData> &data)
{
  SEND_MSG_DATAARRAY(BLOCK_REMOVE_SEL,data)  
}

void VisViewerExchange::BlockSelectionObject(const Array<SMoveObjectPosMessData> &data)
{
  SEND_MSG_DATAARRAY(BLOCK_SELECTION_OBJECT,data)  
}

void VisViewerExchange::BlockSelectionLand(const Array<SLandSelPosMessData> &data)
{
  SEND_MSG_DATAARRAY(BLOCK_SELECTION_LAND,data)  
}

void VisViewerExchange::BlockLandHeightChange(const Array<STexturePosMessData> &data)
{
  SEND_MSG_DATAARRAY(BLOCK_LAND_HEIGHT_CHANGE,data)  
}

void VisViewerExchange::BlockLandHeightChangeInit(const Array<float> &heights)
{
  SEND_MSG_DATAARRAY(BLOCK_LAND_HEIGHT_CHANGE_INIT,heights)  
}

void VisViewerExchange::BlockLandTextureChangeInit(const Array<int> &ids)
{
  SEND_MSG_DATAARRAY(BLOCK_LAND_TEXTURE_CHANGE_INIT,ids)  
}

void VisViewerExchange::BlockWaterHeightChangeInit(const Array<float> &heights)
{
  SEND_MSG_DATAARRAY(BLOCK_WATER_HEIGHT_CHANGE_INIT,heights)  
}

void VisViewerExchange::ComVersion(int version)
{
  SEND_MSG_DATA(COM_VERSION,version);
}

void VisViewerExchange::ObjectMove(const SMoveObjectPosMessData& data)
{
  SEND_MSG_DATA(OBJECT_MOVE,data);
}

VisViewerExchange::Status VisViewerExchange::Import()
{
  int msg=FILE_IMPORT_END;
  PSPosMessage mess;
  return WaitMessage(Array<int>(&msg,1),mess);  
}
