/*********************************************************************/
/*																	 */
/*	Poseidon Messages 												 */
/*									Copyright © Mentar, 1998    	 */
/*																	 */
/*********************************************************************/

#ifndef __MESSAGEDLL_H__
#define __MESSAGEDLL_H__
// komunikaèní knihovna pro editory Poseidon

/////////////////////////////////////////////////////////////
// Version of comunication protokol
#define MSG_VERSION 5

/////////////////////////////////////////////////////////////

#define MSG_MEM_COUNT	500		// max. poèet bufferovaných událostí
								// ostatní jsou pouze odloženy !



//////////////////////////////////////////////////////////////
// Ports
#define VISITOR_SERVER_FIRST 2500
#define VISITOR_SERVER_LAST 2510

#define DATA_CLIENT_FIRST 2511
#define DATA_CLIENT_LAST 2520

#define ALIVE_CLIENT_FIRST 2521
#define ALIVE_CLIENT_LAST 2530


//////////////////////////////////////////////////////////////
// Messages
class SPosMessage : public RefCount
{
public:
	int		_iMsgType;		// identifikace události

  SPosMessage() {};
  virtual ~SPosMessage() {};

  virtual int DataSize(unsigned char * header = NULL) const {return 0;};
  virtual int HeaderSize() const {return 0;};  
  virtual int SetData(unsigned char * data,unsigned char * header) {return 1;};  
  virtual void WriteData(unsigned char * dest) const {return;};
  virtual void WriteHeader(unsigned char * dest) const {return;};
};

typedef Ref<SPosMessage> PSPosMessage;

template<typename Data> 
class SPosMessageEx : public SPosMessage
{
public:
  Data _data;

  SPosMessageEx() {};
  virtual ~SPosMessageEx() {};

  virtual int DataSize(unsigned char * header = NULL) const {return sizeof(_data);};
  virtual int HeaderSize() const {return 0;};  
  virtual int SetData(unsigned char * data, unsigned char * header ) {memcpy(&_data, data, sizeof(_data)); return 1;}; 
  virtual void WriteData(unsigned char * dest) const {memcpy(dest, &_data, sizeof(_data));};
  virtual void WriteHeader(unsigned char * dest) const {return;};
};

template<typename Data> 
class SPosMessageMulti : public SPosMessage
{
public :

  AutoArray<Data> _data;

  SPosMessageMulti() {};
  virtual ~SPosMessageMulti() {};

  virtual int DataSize(unsigned char * header = NULL) const;
  virtual int HeaderSize() const {return sizeof(int);};  
  virtual int SetData(unsigned char * data, unsigned char * header);  
  virtual void WriteData(unsigned char * dest) const {memcpy(dest, _data.Data(), sizeof(Data) * _data.Size());};
  virtual void WriteHeader(unsigned char * dest) const {int n = _data.Size(); memcpy(dest, &n, sizeof(n));};
};

template<typename Data> 
int SPosMessageMulti<Data>::DataSize(unsigned char * header = NULL) const
{return (header == NULL) ? _data.Size() * sizeof(Data) : (*(int*)header) * sizeof(Data);};

template<typename Data> 
int SPosMessageMulti<Data>::SetData(unsigned char * data, unsigned char * header)
{
  _data.Resize(*(int*)header);

  memcpy(_data.Data(), data, _data.Size() * sizeof(Data));
  return 1;
}

template<typename Data> 
class SPosMessageVarLength : public SPosMessage
{
public :

  int _size;
  Data * _data;

  SPosMessageVarLength():_data(NULL), _size(0) {};
  virtual ~SPosMessageVarLength() {delete _data;};

  virtual int DataSize(unsigned char * header = NULL) const;
  virtual int HeaderSize() const {return sizeof(int);};  
  virtual int SetData(unsigned char * data, unsigned char * header);  
  virtual void WriteData(unsigned char * dest) const {memcpy(dest, _data, _size);};
  virtual void WriteHeader(unsigned char * dest) const {memcpy(dest, &_size, sizeof(_size));};
};

template<typename Data> 
int SPosMessageVarLength<Data>::DataSize(unsigned char * header = NULL) const
{return (header == NULL) ? _size : (*(int*)header);};

template<typename Data> 
int SPosMessageVarLength<Data>::SetData(unsigned char * data, unsigned char * header)
{
  delete _data;
  _size = *(int*)header;
  _data = (Data *) new unsigned char[_size];
  
  memcpy(_data, data, _size);
  return 1;
}

#define ADD_MESSAGE(ID,TYPE) typedef enum en_##ID; \
                             typedef TYPE type_##ID; \
                             typedef SPosMessageEx<TYPE> message_type_##ID; \
                             inline SPosMessage * AllocPosMessageEx( enum en_##ID) { return new SPosMessageEx<TYPE>;};\
                             inline TYPE& GetPosMessageData( enum en_##ID, SPosMessage * msg) {return (((SPosMessageEx<TYPE> *) msg)->_data);};\
                             inline const TYPE& GetPosMessageData( enum en_##ID, const SPosMessage * msg) {return (((SPosMessageEx<TYPE> *) msg)->_data);};

#define ADD_MESSAGE_VAR_LENGTH(ID,TYPE) typedef enum en_##ID; \
                                        typedef TYPE type_##ID; \
                                        typedef SPosMessageVarLength<TYPE> message_type_##ID; \
                                        inline SPosMessage * AllocPosMessageEx( enum en_##ID) { return new SPosMessageVarLength<TYPE>;};\
                                        inline TYPE& GetPosMessageData( enum en_##ID, SPosMessage * msg) {return *(((SPosMessageVarLength<TYPE> *) msg)->_data);};\
                                        inline const TYPE& GetPosMessageData( enum en_##ID, const SPosMessage * msg) {return *(((SPosMessageVarLength<TYPE> *) msg)->_data);};


#define ADD_MESSAGE_MULTI(ID,TYPE) typedef enum en_##ID; \
                            typedef AutoArray<TYPE> type_##ID; \
                            typedef SPosMessageMulti<TYPE> message_type_##ID; \
                            inline SPosMessage * AllocPosMessageEx( enum en_##ID) { return new SPosMessageMulti<TYPE>;};\
                            inline AutoArray<TYPE>& GetPosMessageData( enum en_##ID, SPosMessage * msg) {return (((SPosMessageMulti<TYPE> *) msg)->_data);};\
                            inline const AutoArray<TYPE>& GetPosMessageData( enum en_##ID, const SPosMessage * msg) {return (((SPosMessageMulti<TYPE> *) msg)->_data);};

                            
#define ADD_MESSAGE_SIMPLE(ID) typedef enum en_##ID; \
                               typedef SPosMessage message_type_##ID; \
                                inline SPosMessage * AllocPosMessageEx( enum en_##ID) { return new SPosMessage;};

#define ALLOC_MESSAGE_PART(ID) case ID: return AllocPosMessageEx(en_##ID());
#define MESSAGE_DATA(ID, pt) GetPosMessageData(en_##ID(), pt)
#define MESSAGE_DATA_VAR(ID,pt,var) type_##ID & var = GetPosMessageData(en_##ID(), pt);
#define MESSAGE_DATA_CONST_VAR(ID,pt,var) const type_##ID & var = GetPosMessageData(en_##ID(), pt);
#define MESSAGE_TYPE(ID) message_type_##ID
#define MESSAGE_DATA_TYPE(ID) type_##ID
#define MESSAGE_VAR(ID, var) message_type_##ID var; \
                             var._iMsgType = ID;

//////////////////////////////////////////////////////////////////////////
// Message data
struct SLoadSavePosMessData
{
  // Load, Save
  char szFileName[1];  
};

typedef SLoadSavePosMessData * PSLoadSavePosMessData;

//#define LEN_PATHNAME	64
/*struct STransferPosMessData
{
  // Transfer
  char szPathFrom[LEN_PATHNAME];
  char szPathTo[LEN_PATHNAME]; 
};*/

struct SMoveObjectPosMessData
{
  // Object
  int nID; 
  float Position[4][4];
  SMoveObjectPosMessData(int nID):nID(nID) {}
  SMoveObjectPosMessData() {}
};

TypeIsSimple(SMoveObjectPosMessData)

struct SObjectNamePosMessData : public SMoveObjectPosMessData
{
  // Object  
  int nameLenght;
  char szName[1];
};

TypeIsSimple(SObjectNamePosMessData)

typedef SObjectNamePosMessData * PSObjectNamePosMessData;

struct SObjectPosMessData : public SMoveObjectPosMessData
{
  // Object  
  bool bState;
  SObjectPosMessData() {}
  SObjectPosMessData(int nID, bool state):SMoveObjectPosMessData(nID),bState(state) {}
};

TypeIsSimple(SObjectPosMessData)



struct STexturePosMessData
{
  // Texture
  int nX;
  int nZ;
  float Y;
  int nTextureID;

  STexturePosMessData() {}
  STexturePosMessData(int nX, int nZ, float Y, int nTextureID):nX(nX),nZ(nZ),Y(Y),nTextureID(nTextureID) {}

};

TypeIsSimple(STexturePosMessData)

struct SLandscapePosMessData
{  
  // Init landscape parameters
  int textureRangeX,textureRangeY;
  int landRangeX,landRangeY;
  float landGridX,landGridY;
  char configName[1];
};

struct SLandSelPosMessData
{  
  // Init landscape parameters
  float xs,zs;
  float xe,ze;
  SLandSelPosMessData() {}
  SLandSelPosMessData(float xs,float zs,float xe,float ze):xs(xs),zs(zs),xe(xe),ze(ze) {}
};

TypeIsSimple(SLandSelPosMessData);

/////////////////////////////////////////////////////////////////
// Message definitions

#define SYSTEM_QUIT						1
#define SYSTEM_INIT						2
#define FILE_EXPORT						3
#define FILE_IMPORT						4
//#define FILE_TRANSFER					5
#define CURSOR_POSITION_SET				6
#define SELECTION_OBJECT_CLEAR		7
#define SELECTION_OBJECT_ADD			8
#define REGISTER_LANDSCAPE_TEXTURE		9
#define REGISTER_OBJECT_TYPE			10
#define LAND_HEIGHT_CHANGE				11
#define LAND_TEXTURE_CHANGE				12
#define OBJECT_CREATE					13
#define OBJECT_DESTROY					14
#define OBJECT_MOVE						  15
//#define OBJECT_TYPE_CHANGE			16
#define FILE_IMPORT_BEGIN				17
#define FILE_IMPORT_END				  18
#define SELECTION_LAND_CLEAR		23
#define SELECTION_LAND_ADD			24
#define BLOCK_MOVE              25
#define BLOCK_SELECTION_OBJECT  26
#define BLOCK_SELECTION_LAND    27
#define BLOCK_LAND_HEIGHT_CHANGE    28
#define COM_VERSION             29
#define BLOCK_LAND_HEIGHT_CHANGE_INIT 30
#define BLOCK_LAND_TEXTURE_CHANGE_INIT 31
#define BLOCK_WATER_HEIGHT_CHANGE_INIT 32

#define BLOCK_DUPLICATE_SEL						  101
#define BLOCK_REMOVE_SEL			          201

ADD_MESSAGE_SIMPLE(SELECTION_OBJECT_CLEAR)
ADD_MESSAGE_SIMPLE(SYSTEM_QUIT)
ADD_MESSAGE(CURSOR_POSITION_SET, SMoveObjectPosMessData)
ADD_MESSAGE_VAR_LENGTH(OBJECT_CREATE,SObjectNamePosMessData)
ADD_MESSAGE(OBJECT_DESTROY,SMoveObjectPosMessData)
ADD_MESSAGE(OBJECT_MOVE,SMoveObjectPosMessData)
ADD_MESSAGE(SELECTION_OBJECT_ADD,SObjectPosMessData)
ADD_MESSAGE_VAR_LENGTH(SYSTEM_INIT,SLandscapePosMessData)
ADD_MESSAGE(FILE_IMPORT_BEGIN,STexturePosMessData)
ADD_MESSAGE_SIMPLE(FILE_IMPORT_END)
ADD_MESSAGE_VAR_LENGTH(FILE_EXPORT,SLoadSavePosMessData)
ADD_MESSAGE_VAR_LENGTH(FILE_IMPORT,SLoadSavePosMessData)
//ADD_MESSAGE(FILE_TRANSFER,STransferPosMessData)
ADD_MESSAGE(LAND_HEIGHT_CHANGE,STexturePosMessData)
ADD_MESSAGE(LAND_TEXTURE_CHANGE,STexturePosMessData)
ADD_MESSAGE_VAR_LENGTH(REGISTER_OBJECT_TYPE,SObjectNamePosMessData)
//ADD_MESSAGE(OBJECT_TYPE_CHANGE,SMoveObjectPosMessData)
ADD_MESSAGE_VAR_LENGTH(REGISTER_LANDSCAPE_TEXTURE,SObjectNamePosMessData)
ADD_MESSAGE(SELECTION_LAND_ADD,SLandSelPosMessData)
ADD_MESSAGE_SIMPLE(SELECTION_LAND_CLEAR)
ADD_MESSAGE_MULTI(BLOCK_MOVE,SMoveObjectPosMessData)
ADD_MESSAGE_MULTI(BLOCK_DUPLICATE_SEL,SMoveObjectPosMessData)
ADD_MESSAGE_MULTI(BLOCK_REMOVE_SEL,SMoveObjectPosMessData)
ADD_MESSAGE_MULTI(BLOCK_SELECTION_OBJECT,SMoveObjectPosMessData)
ADD_MESSAGE_MULTI(BLOCK_SELECTION_LAND, SLandSelPosMessData)
ADD_MESSAGE_MULTI(BLOCK_LAND_HEIGHT_CHANGE, STexturePosMessData)
ADD_MESSAGE_MULTI(BLOCK_LAND_HEIGHT_CHANGE_INIT, float);
ADD_MESSAGE_MULTI(BLOCK_LAND_TEXTURE_CHANGE_INIT, int);
ADD_MESSAGE_MULTI(BLOCK_WATER_HEIGHT_CHANGE_INIT, float);
ADD_MESSAGE(COM_VERSION,int)

inline SPosMessage * AllocPosMessage(int msgType)
{
  switch (msgType)
  {    
    ALLOC_MESSAGE_PART(SELECTION_OBJECT_CLEAR);
    ALLOC_MESSAGE_PART(SYSTEM_QUIT);
    ALLOC_MESSAGE_PART(CURSOR_POSITION_SET);
    ALLOC_MESSAGE_PART(OBJECT_CREATE);
    ALLOC_MESSAGE_PART(OBJECT_DESTROY);
    ALLOC_MESSAGE_PART(OBJECT_MOVE);
    ALLOC_MESSAGE_PART(SELECTION_OBJECT_ADD);
    ALLOC_MESSAGE_PART(SYSTEM_INIT);
    ALLOC_MESSAGE_PART(FILE_IMPORT_BEGIN);
    ALLOC_MESSAGE_PART(FILE_IMPORT_END);
    ALLOC_MESSAGE_PART(FILE_EXPORT);
    ALLOC_MESSAGE_PART(FILE_IMPORT);
    //ALLOC_MESSAGE_PART(FILE_TRANSFER);
    ALLOC_MESSAGE_PART(LAND_HEIGHT_CHANGE);
    ALLOC_MESSAGE_PART(LAND_TEXTURE_CHANGE);
    ALLOC_MESSAGE_PART(REGISTER_OBJECT_TYPE);
    //ALLOC_MESSAGE_PART(OBJECT_TYPE_CHANGE);
    ALLOC_MESSAGE_PART(REGISTER_LANDSCAPE_TEXTURE);
    ALLOC_MESSAGE_PART(SELECTION_LAND_ADD);
    ALLOC_MESSAGE_PART(SELECTION_LAND_CLEAR);
    ALLOC_MESSAGE_PART(BLOCK_MOVE);
    ALLOC_MESSAGE_PART(BLOCK_DUPLICATE_SEL);
    ALLOC_MESSAGE_PART(BLOCK_REMOVE_SEL);    
    ALLOC_MESSAGE_PART(BLOCK_SELECTION_OBJECT);
    ALLOC_MESSAGE_PART(BLOCK_SELECTION_LAND);
    ALLOC_MESSAGE_PART(BLOCK_LAND_HEIGHT_CHANGE);
    ALLOC_MESSAGE_PART(BLOCK_LAND_HEIGHT_CHANGE_INIT);
    ALLOC_MESSAGE_PART(COM_VERSION);
    ALLOC_MESSAGE_PART(BLOCK_LAND_TEXTURE_CHANGE_INIT);
    ALLOC_MESSAGE_PART(BLOCK_WATER_HEIGHT_CHANGE_INIT);
  default:
    Assert(FALSE);
    LogF("Don't know how to alloc message %d",msgType);
    return NULL;
  }
}

//////////////////////////////////////////////////////////////
// fronta událostí

// stav aplikací
#define APP_STATE_NOT_READY	0
#define APP_STATE_ACTIVE	1


#ifdef _WIN32
#ifndef _MSG_DLL
#undef	MNT_EXT_CLASS
#define MNT_EXT_CLASS __declspec(dllimport)
#else
#undef	MNT_EXT_CLASS
#define MNT_EXT_CLASS __declspec(dllexport)
#endif
#else
#undef  MNT_EXT_CLASS
#define MNT_EXT_CLASS
#endif

/*
typedef struct SFrontItemTag
{
	SPosMessage		sMsg;
	void*			pNext;
}	SFrontItem;

class MNT_EXT_CLASS CMessageFront
{
public:
				  CMessageFront();
	virtual		  ~CMessageFront();

	// funkce pro server - Poseidon Editor

	// vrací poèet událostí ve frontì
	int  GetServerMsgCount();
	// pøeète událost Serveru
	BOOL GetServerMessage(SPosMessage&);
	// pošle událost na Clienta
	void PostMessageToClient(SPosMessage&);
	// reset fronty událostí ori client
	void ResetClientMessageFront();

	// vrací stav aplikace serveru
	int	 GetServerAppState();
	// nastaví stav aplikace serveru
	void SetServerAppState(int nState);

	// funkce pro client - Poseidon View

	// vrací poèet událostí ve frontì
	int	 GetClientMsgCount();
	// pøeète událost Clienta
	BOOL GetClientMessage(SPosMessage&);
	// pošle událost na Server
	void PostMessageToServer(SPosMessage&);
	// reset fronty událostí pro server
	void ResetServerMessageFront();


	// vrací stav aplikace clienta
	int	 GetClientAppState();
	// nastaví stav aplikace clienta
	void SetClientAppState(int nState);

	// funkce pro komunikaci mezi aplikacemi
	// funkce by mela být volána periodicky !!!
	BOOL TryToSendMessages();


public:
	void	AddItem(SPosMessage&,SFrontItem*& pFirst,SFrontItem*& pLast);
	void	RemItem(SFrontItem*& pFirst,SFrontItem*& pLast);

// fronty událostí
	SFrontItem*	m_pFirstToServer;
	SFrontItem*	m_pLastToServer;

	SFrontItem*	m_pFirstToClient;
	SFrontItem*	m_pLastToClient;
};
*/

#endif //__MESSAGEDLL_H__