#ifndef _EDITOR_HPP
#define _EDITOR_HPP

#if _ENABLE_BULDOZER

#include "vehicle.hpp"
#include "landscape.hpp"
#include "VisitorExchange/MessageDll.h"

#if _GAMES_FOR_WINDOWS // using special Winsock library
#elif _WIN32
  #include <winsock2.h>
#else
  #include <sys/socket.h>
  typedef int SOCKET;
  #define SOCKET_ERROR   -1
  #define INVALID_SOCKET -1
  #define closesocket(s) ::close(s)
#endif

// Very simplified water height representation, just for prototyping
class Waterscape : public Array2D<float>
{  
  typedef Array2D<float> base;
protected:
  float _grid;
public:
  Waterscape() : _grid(0) {};

  void Destroy();
  void Dim(int sizex,int sizez, float grid) {base::Dim(sizex, sizez); _grid = grid;};
  void Draw(Vector3Val pos) const;
};

struct Rect 
{
  float _xs;
  float _zs;
  float _xe;
  float _ze;

  Rect(): _xs(0),_zs(0),_xe(0),_ze(0){};
  Rect(float xs, float zs, float xe, float ze): _xs(xs),_zs(zs),_xe(xe),_ze(ze){};

  float Width() const {return _xe - _xs;};
  float Height() const {return _ze - _zs;};
  
};

TypeIsSimple(Rect);

struct IntVector2
{
  int _v[2];

  int &operator[](const int i) {return _v[i];};  
  const int &operator[](const int i) const {return _v[i];};  
  bool operator==(const IntVector2& vec) const {return (_v[0] == vec._v[0]) && (_v[1] == vec._v[1]);};
  
};

TypeIsSimple(IntVector2);

class LandSelection
{
public:
  typedef FindArray<IntVector2> LandVertexesArray;
  typedef AutoArray<Rect> RectArray;
protected:
  RectArray _area;
  Rect _boundingRect;

  void CalculateBoundingRect();

public:

  LandSelection();

  void Add(const Rect& rect);
  void Remove(const Rect& rect);
  void Release();
  void GetSelectedVertexes(LandVertexesArray& vertexes) const;

  const RectArray& GetArea() const {return _area;};
  const Rect& GetBoundingRectangle() const {return _boundingRect;};
  void Draw() const;
};

#include "VisitorExchange/IVisViewerExchange.h"


class EditCursor;
struct TransferInfo;

class CSelection
{
protected:
	AutoArray<EditorSelectionItem> m_data;
    IVisViewerExchangeDocView *m_sendMsg;
	
public:
	CSelection() {m_sendMsg = NULL;}

	void SetOwner(IVisViewerExchangeDocView *owner) {m_sendMsg = owner;}
	int AddObject (Object *pObj)
	{
		return AddID(pObj->ID(), pObj->FutureVisualState().Position());
	}
	int AddID(const VisitorObjId &id, Vector3Par pos, bool sendEvent = true);
	bool RemoveAddObject(Object *pObj)
	{
		return RemoveID(pObj->ID());
	}
	bool RemoveID(int id);

	void Remove(int index);
	void RemoveAll();
	int Size() const {return m_data.Size();}
	const EditorSelectionItem &operator [] ( int i ) const {return m_data[i];}
	EditorSelectionItem &operator [] ( int i ) {return m_data[i];}
	AutoArray<EditorSelectionItem> &GetData() {return m_data;}
};
enum SelectionType
{
 stLandVertexes = 0,
 stObjects = 1,
};

#include <el/Tcpipbasics/Socket.h>


class EditCursor: public Entity
{
	typedef Entity base;

private:
  // axis enum
  struct EAxis
  {                    
    enum Type
    {
      axisX,
      axisY,
      axisZ
    };    
  };                

  struct ERotMode
  {
    enum Type
    {
      world,
      object
    };            
  };

	// describe orientation (Euler parameters)
	float _heading,_dive,_bank;

	float _camDistance;
	float _camHeight;
//	Matrix4 _mTrans,_mInvTrans; // user orientation
//	enum EditMode {EditObjs,EditNets,EditLand} _editMode;
	bool _visible;

	// Flags and modes
	bool m_bCtrl;		// CTRL key was down when LBUTTON DOWN
  bool m_bShift;  // SHIFT key was down when LBUTTON DOWN
	bool m_bOldMouseL;	// last state of left mouse button
	bool m_bOldMouseR;	// last state of right mouse button
	WORD m_wFlags;		// how move will be interpreted and other flags
	bool _showNode;
	
	bool _cameraOnEdit;
	OLink(Object) _animCamera;
	Time _animCameraMoved;
	Time _animCameraUpdated;

	// Actual selection
	CSelection m_Selection;
	Ref<Object> _drawRectangle;

	// State for moving, selecting and rotating
	Point3 m_ptStart;	// start point
	Point3 m_ptCurrent;	// current point
  Point3 _origin;
  Point3 _end;

  SelectionType _selectionType;
  LandSelection _landSelection; 
	EditorSelectionItem m_nPrimaryObject;
	bool m_bPrimarySelection;	// selection state of primary object
	AutoArray<EditorSelectionItem> m_MouseSelection;	// Objects in mouse selection cube

	// Last cursor position
	Matrix4	m_posLast;

    class EditCursorVisitorExchangeWrapper: public IVisViewerExchangeDocView
    {
    public: 
      
        EditCursor &_wrapTo;

        EditCursorVisitorExchangeWrapper(EditCursor &wrapTo):_wrapTo(wrapTo) {}
        void SystemQuit() {_wrapTo.SystemQuit();}
        void SelectionObjectClear() {_wrapTo.SelectionObjectClear();}
        void CursorPositionSet(const SMoveObjectPosMessData &data) {_wrapTo.CursorPositionSet(data);}
        void ObjectCreate(const SMoveObjectPosMessData &data, const char *name) {_wrapTo.ObjectCreate(data,name);}
        void ObjectDestroy(const SMoveObjectPosMessData &data) {_wrapTo.ObjectDestroy(data);}
        void SelectionObjectAdd(const SObjectPosMessData &data) {_wrapTo.SelectionObjectAdd(data);}
        void SystemInit(const SLandscapePosMessData &data, const char *configName) {_wrapTo.SystemInit(data,configName);}
        void FileImportBegin(const STexturePosMessData& data) {_wrapTo.FileImportBegin(data);}
        void FileImportEnd() {_wrapTo.FileImportEnd();}
        void FileImport(const char * data) {_wrapTo.FileImport(data);}
        void FileExport(const char * data) {_wrapTo.FileExport(data);}
        void LandHeightChange(const STexturePosMessData& data) {_wrapTo.LandHeightChange(data);}
        void LandTextureChange(const STexturePosMessData& data) {_wrapTo.LandTextureChange(data);}
        void RegisterObjectType(const char *name) {_wrapTo.RegisterObjectType(name);}
        void RegisterLandscapeTexture(const SMoveObjectPosMessData &data, const char *name) {_wrapTo.RegisterLandscapeTexture(data,name);}
        void SelectionLandAdd(const SLandSelPosMessData& data) {_wrapTo.SelectionLandAdd(data);}
        void SelectionLandClear() {_wrapTo.SelectionLandClear();}
        void BlockMove(const Array<SMoveObjectPosMessData> &data) {_wrapTo.BlockMove(data);}
        void BlockDuplicateSel(const Array<SMoveObjectPosMessData> &data) {_wrapTo.BlockDuplicateSel(data);}
        void BlockRemoveSel(const Array<SMoveObjectPosMessData> &data) {_wrapTo.BlockRemoveSel(data);}
        void BlockSelectionObject(const Array<SMoveObjectPosMessData> &data) {_wrapTo.BlockSelectionObject(data);}
        void BlockSelectionLand(const Array<SLandSelPosMessData> &data) {_wrapTo.BlockSelectionLand(data);}
        void BlockLandHeightChange(const Array<STexturePosMessData> &data) {_wrapTo.BlockLandHeightChange(data);}
        void BlockLandHeightChangeInit(const Array<float> &heights) {_wrapTo.BlockLandHeightChangeInit(heights);}
        void BlockLandTextureChangeInit(const Array<int> &ids) {_wrapTo.BlockLandTextureChangeInit(ids);}
        void BlockWaterHeightChangeInit(const Array<float> &heights) {_wrapTo.BlockWaterHeightChangeInit(heights);}
        void ComVersion(int version) {_wrapTo.ComVersion(version);}
        void ObjectMove(const SMoveObjectPosMessData& data) {_wrapTo.ObjectMove(data);}
        int AddRef() const {return 1;}
        int Release() const {return 1;}
    };

    EditCursorVisitorExchangeWrapper _exchangeWrapper;
    Ref<IVisViewerExchange> _exchange;

    TransferInfo *_transfer;
	RString _ip;
	//bool _connectedSend;
  //bool _connectedAlive;
  int _visitorPort;
#define TRY_TO_CONECT_WAIT_STEP 100
  int _tryToConnectWait;
	bool _afterConnect; // true - after connection, when first CURSOR_POSITION_SET is recieved changed to false;

  // Waterscape
  Waterscape _waterscape;
	
private:
	// Helper functions
	void CheckMouseSelection(Point3& ptMin, Point3& ptMax);

	void UpdateTerrain(Vector3 &position, float change);
  void MoveObject(Vector3 &offset);

  void TryToConnect();
	void HandleError(RStringB who, RStringB op);

	OLink(Object) GetSelectedObject(int i) const;

public:
	bool ProcessEvents();
//	void CCALL SendEvent(int nMsgID, ...);

	// Vehicle implementation
	void Simulate( float deltaT, SimulationImportance prec );
	void Draw(
    int cb, int forceLOD, ClipFlags clipFlags, const DrawParameters &dp,
    const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
    , int drawListId
#endif
  );

  AnimationStyle IsAnimated( int level ) const {return AnimatedGeometry;}; // return true to switch off instancing

	void Sound( bool inside, float deltaT ){}
	void UnloadSound(){}
	Matrix4 InsideCamera( CameraType camType ) const {return Matrix4(MTranslation,Vector3(0,0,-_camDistance));}
	float OutsideCameraDistance( CameraType camType ) const {return _camDistance;}
	//bool InsideVisible() const {return _visible;}
	//bool InsideShadowVisible() const {return _visible;}
	int InsideLOD( CameraType camType ) const {return 0;} // added by ONDRA 

	SimulationImportance WorstImportance() const {return SimulateVisibleNear;}

	void LimitVirtual
	(
		CameraType camType, float &heading, float &dive, float &fov
	) const;
	void InitVirtual
	(
		CameraType camType, float &heading, float &dive, float &fov
	) const;
	bool IsContinuous( CameraType camType ) const {return true;}

	void SwitchCamera();

	EditCursor(RString ip);
	~EditCursor();

    void SystemQuit();
    void SelectionObjectClear();
    void CursorPositionSet(const SMoveObjectPosMessData &data);
    void ObjectCreate(const SMoveObjectPosMessData &data, const char *name);
    void ObjectDestroy(const SMoveObjectPosMessData &data);
    void SelectionObjectAdd(const SObjectPosMessData &data);
    void SystemInit(const SLandscapePosMessData &data, const char *configName);
    void FileImportBegin(const STexturePosMessData& data);
    void FileImportEnd();
    void FileImport(const char * data) {}
    void FileExport(const char * data);
    void LandHeightChange(const STexturePosMessData& data);
    void LandTextureChange(const STexturePosMessData& data);
    void RegisterObjectType(const char *name);
    void RegisterLandscapeTexture(const SMoveObjectPosMessData &data, const char *name);
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
    void ComVersion(int version) {}
    void ObjectMove(const SMoveObjectPosMessData& data);


};


#endif // _ENABLE_BULDOZER

#endif

