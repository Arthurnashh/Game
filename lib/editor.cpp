#include "wpch.hpp"

#if _ENABLE_BULDOZER

#include <Es/Common/win.h>
#include "object.hpp"
#include "editor.hpp"
#include "engine.hpp"
#include "scene.hpp"
#include "world.hpp"
#include "landscape.hpp"
#include "global.hpp"
#include "keyInput.hpp"
#include "progress.hpp"
#include <El/Interfaces/iAppInfo.hpp>
#include "Shape/material.hpp"
#include <malloc.h>
//#include "appInfoOFP.hpp"

#include <stdarg.h>

#include "dikCodes.h"

// modes
#define MODE(x)     (x & 0xff)
#define NOTMODE(x)    (x & 0xff00)
#define MODE_NORMAL   0
#define MODE_SELECT   1
#define MODE_MOVE   2
#define MODE_ROTATE   3
#define MODE_SCALE    4
// other flags
#define FLAG_POINTS   0x100 // magnetism for points
#define FLAG_PLANES   0x200 // magnetism for planes
#define FLAG_YFIXED   0x400 // magnetism in y axis forbidden

// speeds
#define SPEED_MOUSE   0.01f
#define SPEED_KEYB    1
#define SPEED_SCROLL  4

// other consts
#define INIT_SEL_ITEMS  64  // initial size of selection array
#define NEAR_OBJECT   -1
#define MIN_MOVEMENT  0
bool ConnectedToVisitor = false;

//////////////////////////////////////////////
//  class CSelection
//////////////////////////////////////////////

int CSelection::AddID(const VisitorObjId &id, Vector3Par pos,bool sendEvent/* = true*/)
{
  int nRet = m_data.Add();
  m_data[nRet].id = id;
  // fix pos
  OLink(Object) obj = GLandscape->GetObject(id,pos);
  if (!obj)
  {
    m_data[nRet].pos = pos;
  }
  else
  {
    m_data[nRet].pos = obj->FutureVisualState().Position();
  }
    
    if (sendEvent && m_sendMsg)
    {    
    m_sendMsg->SelectionObjectAdd(SObjectPosMessData(id, true));
    }
  return nRet;
}

bool CSelection::RemoveID(int id)
{
  for (int i=0; i<m_data.Size(); i++)
  {
    if (m_data[i].id == id)
    {
      m_data.Delete(i);
      return true;
    }
  }
  return false;
  // TODO: should we send even or not?
  // this function is used when object is destroyed
  // m_pOwner->SendEvent(SELECTION_OBJECT_ADD, nObj, false);
}

void CSelection::Remove(int index)
{
  VisitorObjId id = m_data[index].id;
  m_data.Delete(index);
  if (m_sendMsg) m_sendMsg->SelectionObjectAdd(SObjectPosMessData(id, true));
}

void CSelection::RemoveAll()
{
  bool sendEvent = m_data.Size() > 0;
  m_data.Init(INIT_SEL_ITEMS);
    if (sendEvent && m_sendMsg) m_sendMsg->SelectionObjectClear();
}

//////////////////////////////////////////////
//  class EditCursor
//////////////////////////////////////////////

OLink(Object) EditCursor::GetSelectedObject(int i) const
{
  const EditorSelectionItem &item = m_Selection[i];
  OLink(Object) obj = GLandscape->GetObject(item.id, item.pos);
  if (obj)
  {
    // if we found the object, we can update its position, so that next search is much faster
    item.pos = obj->FutureVisualState().Position();
  }
  return obj;
}

const float DefCamDistance=50.0;
const float DefCamHeight=1;
const float DefDive=0.5;

extern bool ForceRender; // draw frame as soon as possible

#if _ENABLE_BULDOZER
#include "VisitorExchange/VisViewerSocket.h"
#include "VisitorExchange/VisViewerPipe.h"
#endif

void EditCursor::TryToConnect()
{
#if _ENABLE_BULDOZER
  ConnectedToVisitor = false;
  if (_ip.GetLength() == 0)
    return;

  if (strnicmp(_ip,"pipe\\",5)==0)
  {
    //use pipes
    VisViewerPipe *expipe=new VisViewerPipe(_exchangeWrapper);
    if (expipe->Create(_ip.Data()+5)==FALSE) 
    {
      delete expipe;      
      return;
    }
    else
    {    
      _exchange=expipe;
      ConnectedToVisitor = true;
    }
  }
  else
  {
    //use sockets
    Socket socketSend;
    if (!socketSend)
    {

      socketSend.Create(SOCK_STREAM);

      if (!socketSend)    return;      

        //int yes = 1;
        ///*int ret = */setsockopt(socketSend,SOL_SOCKET, SO_REUSEADDR, (const char*) &yes, sizeof(yes));



          sockaddr_in addr;
          addr.sin_family = AF_INET;
          addr.sin_addr.s_addr = inet_addr(_ip);
          addr.sin_port = htons(VISITOR_SERVER_FIRST);
          if (socketSend.Connect(addr)) 
          {
          }
          else
          {
            socketSend.Close();
          }
      }  

    _afterConnect = socketSend.IsValid();
    if (_afterConnect)
    {
      ConnectedToVisitor = true;
      VisViewerSocket *exsocket=new VisViewerSocket(_exchangeWrapper);
      exsocket->Create(socketSend);
      _exchange=exsocket;
      //send first message with version info
    }
  }
 _exchange->ComVersion( MSG_VERSION);
 m_Selection.SetOwner(_exchange);
#endif
}


void EditCursor::HandleError(RStringB who, RStringB op)
{
#ifdef _WIN32
  int error = WSAGetLastError();
#else
  int error = 0;
#endif


  ConnectedToVisitor = false;
  //_connectedSend = _connectedAlive = false;
  _afterConnect = false;

  _visitorPort = -1; // connection lost forget visitor point.

  //if (send)
    WarningMessage("Sockets: Failed %s (cannot finish %s, error %d)", who, op, error);
  //else
  //  WarningMessage("Sockets: Connection lost (cannot finish recv, error %d)", error); 
}

#pragma warning(disable:4355)

EditCursor::EditCursor(RString ip)
:_camDistance(DefCamDistance),_camHeight(DefCamHeight),
_heading(0),_dive(DefDive),_bank(0),
// _editMode(EditObjs),
_visible(true),_cameraOnEdit(true),
_selectionType(stObjects),
Entity
( 
  // Shapes.New("data3d\\kursor.p3d",false,true),
  NULL,
  VehicleTypes.New("EditCursor"),CreateObjectId()
),
_exchangeWrapper(*this),
_transfer(0)
{
  // Set big simulation step. (In EditCursor are no vehicles, AIs etc... simulated).
  _simulationPrecision = 10.0f; 

#ifdef _WIN32
  _animCameraMoved=Glob.time-60;
  _animCameraUpdated=Glob.time;

  Matrix3Val rotY=Matrix3(MRotationY,_heading);
  Matrix3Val rotX=Matrix3(MRotationX,_dive);
  Matrix3Val rot=rotY*rotX;
  SetOrientation(rot);

  m_wFlags = MODE_NORMAL;
  m_bOldMouseL = false;
  m_bOldMouseR = false;
  _showNode = false;

  m_Selection.SetOwner(_exchange);

  m_ptStart = VZero;
  m_ptCurrent = VZero;

  m_nPrimaryObject.id = VisitorObjId();
  m_nPrimaryObject.pos = VZero;

  WSADATA data;
  WSAStartup(0x0101, &data);


  _ip = ip;
  //_connectedSend = false;
  //_connectedAlive = false;
  _visitorPort = -1;
  _afterConnect = false;

  _tryToConnectWait = TRY_TO_CONECT_WAIT_STEP * 2;
  if (ip.GetLength() > 0) 
    TryToConnect();
   
#endif

  m_posLast = FutureVisualState().Transform();
}

EditCursor::~EditCursor()
{
  _exchange=0;
#ifdef _WIN32
  WSACleanup();
#endif
}

void EditCursor::LimitVirtual
(
  CameraType camType, float &heading, float &dive, float &fov
) const
{
  Vehicle::LimitVirtual(camType,heading,dive,fov);
  switch( camType )
  {
    case CamGunner: case CamInternal:
      heading=dive=0;
    break;
  }
  fov=0.7;
}

void EditCursor::InitVirtual
(
  CameraType camType, float &heading, float &dive, float &fov
) const
{
  Vehicle::InitVirtual(camType,heading,dive,fov);
}

void EditCursor::CheckMouseSelection(Point3& ptMin, Point3& ptMax)
{
  int i;

  m_MouseSelection.Init(INIT_SEL_ITEMS);
  if (m_bCtrl || m_bShift)
    for (i=0; i<m_Selection.Size(); i++)
      m_MouseSelection.Add(m_Selection[i]);

  Landscape *land = GLOB_LAND;

  ptMin.Init();
  ptMax.Init();

  ptMin[0] = m_ptStart.X();
  ptMin[2] = m_ptStart.Z();
  ptMax[0] = m_ptCurrent.X();
  ptMax[2] = m_ptCurrent.Z();
  float pom;
  if (ptMin[0] > ptMax[0])
  {
    pom = ptMin[0];
    ptMin[0] = ptMax[0];
    ptMax[0] = pom;
  }
  if (ptMin[2] > ptMax[2])
  {
    pom = ptMin[2];
    ptMin[2] = ptMax[2];
    ptMax[2] = pom;
  }
  int nXMin = toIntFloor(ptMin.X() * InvObjGrid);
  int nXMax = toIntFloor(ptMax.X() * InvObjGrid);
  int nZMin = toIntFloor(ptMin.Z() * InvObjGrid);
  int nZMax = toIntFloor(ptMax.Z() * InvObjGrid);

  saturateMax(nXMin, 0);
  saturateMin(nXMax, land->GetLandRange() - 1);
  saturateMax(nZMin, 0);
  saturateMin(nZMax, land->GetLandRange() - 1);

  for (int nZ = nZMin; nZ <= nZMax; nZ++)
    for (int nX = nXMin; nX <= nXMax; nX++)
    {
      const ObjectListUsed &used = land->UseObjects(nX,nZ);
      
      for (i = 0; i < used.Size(); i++)
      {
        Ref<Object> pObj = used[i];
        if (pObj->GetType() == Primary
#if _ARMA3_TERRAIN_DECALS
          || pObj->GetType() == LandDecal
#endif           
          )
        {
          Point3 pos = pObj->FutureVisualState().Position();
          if (pos.X() >= ptMin.X() && pos.X() <= ptMax.X() &&
            pos.Z() >= ptMin.Z() && pos.Z() <= ptMax.Z())
          {
            int nObj = pObj->ID();
            if (m_bCtrl || m_bShift)
            {
              int nFound = -1;
              for (int j=0; j<m_MouseSelection.Size(); j++)
                if (m_MouseSelection[j].id == nObj)
                {
                  nFound = j;
                  break;
                }
              if (nFound < 0 && m_bCtrl)
              {
                int index = m_MouseSelection.Add();
                m_MouseSelection[index].id = VisitorObjId(nObj);
                m_MouseSelection[index].pos = pos;
              }
              else
                if (m_bShift)
                  m_MouseSelection.Delete(nFound);
            }
            else
            {
              int index = m_MouseSelection.Add();
              m_MouseSelection[index].id = VisitorObjId(nObj);
              m_MouseSelection[index].pos = pos;
            }
          }
        }
      }
    }
}




void EditLog( const char *format, ... )
{
  va_list arglist;
  va_start( arglist, format );

  // create or append to session log
  static bool notFirst;
  
  FILE *f;
  static const char logName[]="buldozer.log";
  if( notFirst )  f=fopen(logName,"at");
  else f=fopen(logName,"wt"),notFirst=true;
  if( !f ) return;
  vfprintf(f,format,arglist);
  fputc('\n',f);
  fclose(f);

  va_end( arglist );

}

void EditCursor::SwitchCamera()
{

  if( !_animCamera )
  {
    // find camera object
    int x,z;
    for( x=0; x<LandRange; x++ ) for( z=0; z<LandRange; z++ )
    {
      const ObjectList &list=GLOB_LAND->GetObjects(z,x);
      for( int i=0; i<list.Size(); i++ )
      {
        Object *obj=list[i];
        if( obj->GetShape() && !strcmpi(obj->GetShape()->Name(),"data3d\\camera.p3d") )
        {
          _animCamera=obj;
          break;
        }
      }
    }
  }
  _cameraOnEdit=!_cameraOnEdit;
  if( !_animCamera )
  {
    _cameraOnEdit=true;
  }
  if( _cameraOnEdit )
  {
    GLOB_WORLD->SwitchCameraTo(this,CamInternal,true);
  }
  else
  {
    GLOB_WORLD->SwitchCameraTo(_animCamera,CamInternal,true);
  }
}

#include "El/Debugging/debugTrap.hpp"


enum TransferState
{
  TSNone,
  TSTexturesRegistration,
  TSHeights,
  TSWaterHeights,
  TSTextures,
  TSObjectTypes,
  TSObjects
};

struct TransferInfo
{
  TransferState state;
  int objRegistered;
  int objInstances;
  int texRegistered;
  int landSize;
  int landTextures;
  int landWater;
  int xCoord, zCoord;
  Ref<ProgressHandle> progress;
};

bool CheckEntity(Entity *veh)
{
  int xx = toIntFloor(veh->FutureVisualState().Position().X() * InvLandGrid);
  int zz = toIntFloor(veh->FutureVisualState().Position().Z() * InvLandGrid);
  if (!ObjInRange(xx,zz))
  {
    // find nearest in-range square and use it
    if (xx < 0) xx = 0;
    else if (xx >= ObjRange) xx = ObjRange - 1;
    if (zz < 0) zz = 0;
    else if (zz >= ObjRange) zz = ObjRange - 1;
    Assert(ObjInRange(xx,zz));
  }
  const ObjectList &list = GLandscape->GetObjects(zz, xx);
  for (int i=0; i<list.Size(); i++)
  {
    if (list[i] == veh) return true;
  }
  return false;
}

bool EditCursor::ProcessEvents()
{
#if _ENABLE_BULDOZER
  if (_exchange.IsNull())
  {
    if (_tryToConnectWait > TRY_TO_CONECT_WAIT_STEP)
    {
      TryToConnect();
      _tryToConnectWait -= TRY_TO_CONECT_WAIT_STEP;
    }
    else
      _tryToConnectWait++;
  }
  
  if (_exchange.IsNull()) return false;

  GDebugger.NextAliveExpected(15*60*1000);

    IVisViewerExchange::Status res=_exchange->CheckInterfaceStatus();
    if (res==IVisViewerExchange::statOk)
    {
      ForceRender=true;
      return true;
    }
    else if (res==IVisViewerExchange::statNoMessage)
    {
      return false;
    }
    else
    {
      _exchange=0;
      return false;
    }

#else
  return false;
#endif
}

void EditCursor::UpdateTerrain(Vector3 &position, float change)
{
  float terrainGrid = GLandscape->GetTerrainGrid();

  LandSelection::LandVertexesArray vertexes;
  _landSelection.GetSelectedVertexes(vertexes);
  
  float dy = position[1] - GLandscape->SurfaceY(position[0], position[1]);
  if (vertexes.Size() == 0)
  {
    // no node in selection - change nearest
    int xx = toInt(position.X() / terrainGrid);
    int zz = toInt(position.Z() / terrainGrid);
    if (TerrainInRange(zz, xx))
    {
      GLandscape->HeightChange(xx, zz, GLandscape->GetData(xx, zz) + change);
    }
    float x = xx * terrainGrid;
    float z = zz * terrainGrid;
    float height = GLandscape->SurfaceY(x, z);
    position[0] = x;
    position[2] = z;
    position[1] = height;

    if (_exchange.NotNull()) _exchange->LandHeightChange(STexturePosMessData(xx,zz,height,0));        
  }
  else
  {    
      AutoArray<STexturePosMessData>  msgData;

      for (int i = 0; i < vertexes.Size(); i++)
      {
        const IntVector2& vertex = vertexes[i];   
        STexturePosMessData& data = msgData.Append();

        data.nX = vertex[0];
        data.nZ = vertex[1];
        data.Y = GLandscape->SurfaceY(vertex[0] * terrainGrid, vertex[1] * terrainGrid);      
      }

      if (_exchange.NotNull()) _exchange->BlockLandHeightChange(msgData);

   
  }
  GLandscape->FlushCache();
  position[1] = GLandscape->SurfaceY(position[0], position[1]) + dy;
};

void EditCursor::MoveObject(Vector3 &offset)
{
  Matrix4Val rotY = Matrix4(MRotationY,_heading);
  offset.SetRotate(rotY, offset);

  AutoArray<SMoveObjectPosMessData> msgData;

  for (int i=0; i<m_Selection.Size(); i++)
  {
    Ref<Object> pObj = GetSelectedObject(i);
    DoAssert(pObj);
    Point3 ptObj = pObj->FutureVisualState().Position();           
    ptObj += offset;  

    Matrix4 trans = pObj->FutureVisualState().Transform();
    float relHeight;
    pObj->DeSkew(GLOB_LAND, trans, relHeight);

    trans.SetPosition(ptObj);
    Vector3 bCenter = trans.FastTransform(-pObj->GetShape()->BoundingCenter());
    ptObj[1] += GLOB_LAND->SurfaceY(bCenter[0], bCenter[2]) + relHeight - bCenter[1];
    trans.SetPosition(ptObj);
    

    GLOB_LAND->MoveObjectFar(pObj,trans); // insert on new position          

    SMoveObjectPosMessData& data = msgData.Append();
    data.nID = m_Selection[i].id;
    {      
      for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
          data.Position[i][j] = trans.Orientation()(i, j);
      for (int i=0; i<3; i++)
        data.Position[i][3] = trans.Position()[i];
    }            
  }
  if (_exchange.NotNull()) _exchange->BlockMove(msgData);
}
extern float GetVisitorMouseSpeed();
void EditCursor::Simulate( float deltaT, SimulationImportance prec )
{
  if (GWorld->Options()) return;
  if (GInput.GetKeyToDo(DIK_F1))
  {
    AbstractOptionsUI *CreateBuldozerOptionsUI();
    GWorld->SetOptions(CreateBuldozerOptionsUI());
  }

  bool hasFocus = (CurrentAppInfoFunctions->GetAppHWnd() == (void *) GetFocus());

#ifdef _WIN32
  int i, /*j,*/ x, z;
  Landscape *land=GLOB_LAND;

  // process events from editor
  bool busy = ProcessEvents();

  Vector3 position=FutureVisualState().Position();

  Frame moveTrans;
  moveTrans.SetTransform(FutureVisualState());

  if (hasFocus)
  {
    if (GInput.GetActionToDo(UABuldSwitchCamera))
    {
      SwitchCamera();
    }

    // init
    
    float rotYAngle=0,rotXAngle=0;

    // User input
    float speedKeyb = deltaT * SPEED_KEYB; 
    float speedX=0;
    float speedZ=0;
    if (hasFocus)
    {
      speedX = (GInput.GetAction(UABuldMoveRight) - GInput.GetAction(UABuldMoveLeft)) * SPEED_MOUSE * GetVisitorMouseSpeed()*Input::MouseRangeFactor;
      speedZ = (GInput.GetAction(UABuldMoveForward) - GInput.GetAction(UABuldMoveBack)) * SPEED_MOUSE * GetVisitorMouseSpeed()*Input::MouseRangeFactor;
    }

#if _SPACEMOUSE
    //SpaceMouse
    speedX += GInput.spaceMouseAxis[0]*deltaT*5.0;
    speedZ += GInput.spaceMouseAxis[2]*deltaT*5.0;
    _camHeight += GInput.spaceMouseAxis[1]*deltaT*20.0;

    rotXAngle -= GInput.spaceMouseAxis[3]*deltaT *.5;
    rotYAngle = GInput.spaceMouseAxis[4]*deltaT *.5;
#endif

    bool faster = GInput.GetAction(UABuldTurbo) > 0.5f;
    if (faster)
    {
      speedX *= SPEED_SCROLL;
      speedZ *= SPEED_SCROLL;
      speedKeyb *= SPEED_SCROLL;
    }
    bool bReset = false;  // Reset camera position
    Point3 ptMin, ptMax;

    // process mouse moving
    if( _cameraOnEdit )
    {
      // edit cursor movement

      switch (MODE(m_wFlags))
      {
      case MODE_NORMAL:
        if (GInput.GetAction(UABuldFreeLook))
        {
          rotXAngle += speedZ;
          rotYAngle -= speedX;
        }
        else
        {
          Vector3 offset(speedX * _camDistance, 0, speedZ * _camDistance);
          Matrix4Val rotY=Matrix4(MRotationY,_heading);
          offset.SetRotate(rotY,offset);
          position+=offset;
        }
        break;
      case MODE_SELECT:
        {
          float xOffset = speedX * _camDistance / 3;
          float zOffset = speedZ * _camDistance / 3;
          Vector3 offset(xOffset,0,zOffset);
          Matrix4Val rotY=Matrix4(MRotationY,_heading);
          offset.SetRotate(rotY, offset);
          m_ptCurrent += offset;
          if (_selectionType == stObjects)
            CheckMouseSelection(ptMin, ptMax);

          position = m_ptCurrent;
        }
        break;
      case MODE_MOVE:
        {
          float xOffset = speedX * _camDistance / 3;
          float zOffset = speedZ * _camDistance / 3;
          if (xOffset == 0 && zOffset == 0)
            break;
          Vector3 offset(xOffset,0,zOffset);
          Matrix4Val rotY=Matrix4(MRotationY,_heading);
          offset.SetRotate(rotY, offset);

          for (i=0; i<m_Selection.Size(); i++)
          {
            Ref<Object> pObj = GetSelectedObject(i);
            DoAssert(pObj);
            Point3 ptObj = pObj->FutureVisualState().Position();           
            ptObj += offset;  

            Matrix4 trans = pObj->FutureVisualState().Transform();
            float relHeight;
            pObj->DeSkew(land, trans, relHeight);

            trans.SetPosition(ptObj);
            Vector3 bCenter = trans.FastTransform(-pObj->GetShape()->BoundingCenter());
            ptObj[1] += GLOB_LAND->SurfaceY(bCenter[0], bCenter[2]) + relHeight - bCenter[1];
            trans.SetPosition(ptObj);

            land->MoveObjectFar(pObj,trans); // insert on new position          
            //        SendEvent(OBJECT_MOVE, m_Selection[i], &pObj->Transform());
          }
          //m_ptStart += offset;
          //m_ptCurrent += offset;
          //position = m_ptCurrent;
          _end += offset; 
          position += offset;
        }
        break;
      case MODE_SCALE:
        {
          float yAngle = speedX;
          if (yAngle == 0)
            break;                            

          // process all objects from selection
          for (i=0; i<m_Selection.Size(); i++)
          {
            Ref<Object> pObj = GetSelectedObject(i);
            DoAssert(pObj);
            Point3 ptObj = pObj->FutureVisualState().Position();      
            Matrix4 trans = pObj->FutureVisualState().Transform();
            float relHeight;
            pObj->DeSkew(land, trans, relHeight);

            // scale transf
            float scale = trans.Scale();
            scale = scale + scale*yAngle; 
            trans.SetScale(scale);
            //////////////////////////////
            trans.SetPosition(ptObj);            
            Vector3 bCenter = trans.FastTransform(-pObj->GetShape()->BoundingCenter());
            ptObj[1] += GLOB_LAND->SurfaceY(bCenter[0], bCenter[2]) + relHeight - bCenter[1];
            trans.SetPosition(ptObj);            

            land->MoveObjectFar(pObj,trans);    // insert on new position
            //        SendEvent(OBJECT_MOVE, m_Selection[i], &pObj->Transform());
          }
        }
        break;        
      case MODE_ROTATE:
        {
          float yAngle = speedX;
          if (yAngle == 0)
            break; 

          // select axis
          EAxis::Type axisMode = EAxis::axisY;
          if (GInput.keys[DIK_X]>0) 
            axisMode = EAxis::axisX;
          else if (GInput.keys[DIK_Y]>0)
            axisMode = EAxis::axisZ;
          
          // select rotation mode
          ERotMode::Type rotMode = 
            GInput.keys[DIK_LCONTROL]>0 ? ERotMode::world : ERotMode::object; 
                              
          // rotation around axis
          Matrix3 axisRot = M3Identity;
          switch (axisMode)
          {
            case EAxis::axisX:axisRot.SetRotationX(yAngle);break;
            case EAxis::axisY:axisRot.SetRotationY(yAngle);break;
            case EAxis::axisZ:axisRot.SetRotationZ(yAngle);break;
          }
          
          // process all objects from selection
          for (i=0; i<m_Selection.Size(); i++)
          {
            Ref<Object> pObj = GetSelectedObject(i);
            DoAssert(pObj);
            Point3 ptObj = pObj->FutureVisualState().Position();                    
            if (!faster && axisMode==EAxis::axisY)
            {                        
              Vector3Val vector = axisRot * (ptObj - _origin);        // rotate position
            ptObj = _origin + vector;
            }

            Matrix4 trans = pObj->FutureVisualState().Transform();
            float relHeight;
            pObj->DeSkew(land, trans, relHeight);
            
            // rotate transf
            if (rotMode==ERotMode::world)                        
              trans.SetOrientation(axisRot*trans.Orientation());
            else  
              trans.SetOrientation(trans.Orientation()*axisRot);
            //////////////////////////////
            trans.SetPosition(ptObj);
            Vector3 bCenter = trans.FastTransform(-pObj->GetShape()->BoundingCenter());
            ptObj[1] += GLOB_LAND->SurfaceY(bCenter[0], bCenter[2]) + relHeight - bCenter[1];
            trans.SetPosition(ptObj);            

            land->MoveObjectFar(pObj,trans);    // insert on new position
            //        SendEvent(OBJECT_MOVE, m_Selection[i], &pObj->Transform());
          }
        }
        break;
      }

      // process keys
      if (GInput.GetActionToDo(UABuldSelect))
      {
        // Invert selection of nearest object
        Ref<Object> pObj = land->NearestObject(position, NEAR_OBJECT, 
          ObjectType(Primary|Network
#if _ARMA3_TERRAIN_DECALS
          |LandDecal
#endif       
          ));
        if (pObj && pObj->ID()>=0 )
        {
          int nObj = pObj->ID();
          int nFound = -1;
          for (i=0; i<m_Selection.Size(); i++)
            if (m_Selection[i].id == nObj)
            {
              nFound = i;
              break;
            }
            if (nFound < 0)
              m_Selection.AddID(VisitorObjId(nObj), pObj->FutureVisualState().Position());
            else
              m_Selection.Remove(nFound);
        }
      }
      if (GInput.GetActionToDo(UABuldResetCamera))
      {
        // Reset camera position
        bReset = true;
      }
      if (GInput.GetActionToDo(UABuldMagnetizePoints))
      {
        m_wFlags ^= FLAG_POINTS;
      }
      if (GInput.GetActionToDo(UABuldMagnetizePlanes))
      {
        m_wFlags ^= FLAG_PLANES;
      }
      if (GInput.GetActionToDo(UABuldMagnetizeYFixed))
      {
        m_wFlags ^= FLAG_YFIXED;
      }
      if (GInput.GetActionToDo(UABuldTerrainRaise1m))
      {
        DIAG_MESSAGE(200, "UABuldTerrainRaise1m");
        UpdateTerrain(position, +1.0);
      }
      if (GInput.GetActionToDo(UABuldTerrainLower1m))
      {
        DIAG_MESSAGE(200, "UABuldTerrainLower1m");
        UpdateTerrain(position, -1.0);
      }
      if (GInput.GetActionToDo(UABuldTerrainRaise5m))
      {
        DIAG_MESSAGE(200, "UABuldTerrainRaise5m");
        UpdateTerrain(position, +5.0);
      }
      if (GInput.GetActionToDo(UABuldTerrainLower5m))
      {
        DIAG_MESSAGE(200, "UABuldTerrainLower5m");
        UpdateTerrain(position, -5.0);
      }
      if (GInput.GetActionToDo(UABuldTerrainRaise10cm))
      {
        DIAG_MESSAGE(200, "UABuldTerrainRaise10cm");
        UpdateTerrain(position, +0.1f);
      }
      if (GInput.GetActionToDo(UABuldTerrainLower10cm))
      {
        DIAG_MESSAGE(200, "UABuldTerrainLower10cm");
        UpdateTerrain(position, -0.1f);
      }
      if (GInput.GetActionToDo(UABuldTerrainRaise50cm))
      {
        DIAG_MESSAGE(200, "UABuldTerrainRaise50cm");
        UpdateTerrain(position, +0.5f);
      }
      if (GInput.GetActionToDo(UABuldTerrainLower50cm))
      {
        DIAG_MESSAGE(200, "UABuldTerrainLower50cm");
        UpdateTerrain(position, -0.5f);
      }

      if (GInput.GetActionToDo(UABuldTerrainShowNode))
      {
        _showNode = !_showNode;
      }
      if (GInput.GetActionToDo(UABuldSelectionType))
      {
        if (_selectionType == stObjects)
          _selectionType = stLandVertexes;
        else
          _selectionType = stObjects;
      }
      if (GInput.GetActionToDo(UABuldLeft))
      {
        Vector3 offset(-0.1, 0, 0);
        MoveObject(offset);
        position += offset;
      }
      if (GInput.GetActionToDo(UABuldRight))
      {
        Vector3 offset(0.1, 0, 0);
        MoveObject(offset);
        position += offset;
      }
      if (GInput.GetActionToDo(UABuldForward))
      {
        Vector3 offset(0, 0, 0.1);
        MoveObject(offset);
        position += offset;
      }
      if (GInput.GetActionToDo(UABuldBack))
      {
        Vector3 offset(0, 0, -0.1);
        MoveObject(offset);
        position += offset;
      }
      rotYAngle += (GInput.GetAction(UABuldLookLeft)-GInput.GetAction(UABuldLookRight)) * speedKeyb;
      rotXAngle += (GInput.GetAction(UABuldLookUp)-GInput.GetAction(UABuldLookDown)) * speedKeyb;
      // zoom
      _camDistance += (GInput.GetAction(UABuldZoomOut)-GInput.GetAction(UABuldZoomIn)) * speedKeyb*20;
      saturate(_camDistance, 0, 500);
      if (GInput.GetAction(UABuldDown))
      {
        if (MODE(m_wFlags) == MODE_MOVE || 
            MODE(m_wFlags) == MODE_ROTATE||
            MODE(m_wFlags) == MODE_SCALE)
        {
          for (i=0; i<m_Selection.Size(); i++)
          {
            Ref<Object> pObj = GetSelectedObject(i);
            DoAssert(pObj);
            Point3 ptObj = pObj->FutureVisualState().Position();
            ptObj[1] -= GInput.GetAction(UABuldDown) * speedKeyb * _camDistance / 3;
            pObj->SetPosition(ptObj);
          }         
        }
        else
        {
          _camHeight -= GInput.GetAction(UABuldDown) * speedKeyb * 20;
          _camHeight = floatMax(_camHeight,-5);
        }
      }
      if (GInput.GetAction(UABuldUp))
      {
        if (MODE(m_wFlags) == MODE_MOVE   || 
            MODE(m_wFlags) == MODE_ROTATE ||
            MODE(m_wFlags) == MODE_SCALE)
        {
          
          for (i=0; i<m_Selection.Size(); i++)
          {
            Ref<Object> pObj = GetSelectedObject(i);
            DoAssert(pObj);
            Point3 ptObj = pObj->FutureVisualState().Position();
            ptObj[1] += GInput.GetAction(UABuldUp) * speedKeyb * _camDistance / 3;
            pObj->SetPosition(ptObj);            
          }
        }
        else
        {
          _camHeight += GInput.GetAction(UABuldUp) * speedKeyb * 20;
          _camHeight=floatMin(_camHeight,+500);
        }
      }

      // process mouse buttons

      if (GInput.mouseL && !m_bOldMouseL)
      {
        // mouseL down
        if (MODE(m_wFlags) == MODE_NORMAL)
        {
          m_bCtrl = (GInput.keys[DIK_LCONTROL] > 0 || GInput.keys[DIK_RCONTROL] > 0);
          m_bShift = (GInput.keys[DIK_LSHIFT] > 0 || GInput.keys[DIK_RSHIFT] > 0);
          m_ptCurrent = m_ptStart = position;
          _origin = _end = position;
          if (_selectionType == stObjects)
          {
            Ref<Object> pObj = land->NearestObject(position, NEAR_OBJECT, 
              ObjectType(Primary|Network
#if _ARMA3_TERRAIN_DECALS
              |LandDecal
#endif                    
              ));
            if (pObj && pObj->ID()>=0 )
            {
              int nObj = pObj->ID();
              int nFound = -1;
              for (i=0; i<m_Selection.Size(); i++)
              {
                if (m_Selection[i].id == nObj)
                {
                  nFound = i;
                  break;
                }
              }

              m_nPrimaryObject.id = VisitorObjId(nObj);
              m_nPrimaryObject.pos = pObj->FutureVisualState().Position();
              if (m_bCtrl)
              {
                m_bPrimarySelection = (nFound >= 0);
                if (!m_bPrimarySelection)
                {
                  m_Selection.AddID(VisitorObjId(nObj), pObj->FutureVisualState().Position());
                }
              }
              else
              {
                if (m_bShift)
                {
                  if (nFound >= 0)
                    m_Selection.Remove(nFound);
                }
                else
                {                
                  if (nFound < 0)
                  {
                    m_Selection.RemoveAll();
                    m_Selection.AddID(VisitorObjId(nObj), pObj->FutureVisualState().Position());
                  }
                }
              }
              m_wFlags = NOTMODE(m_wFlags) | MODE_MOVE;
            }
            else
            {
              m_wFlags = NOTMODE(m_wFlags) | MODE_SELECT;
              CheckMouseSelection(ptMin, ptMax);
            }
          }
          else
          {
            // make object selection still visible
            m_MouseSelection.Init(INIT_SEL_ITEMS);            
            for (i=0; i<m_Selection.Size(); i++)
               m_MouseSelection.Add(m_Selection[i]);
            m_wFlags = NOTMODE(m_wFlags) | MODE_SELECT;
          }

        }
      }
      else if (!GInput.mouseL && m_bOldMouseL)
      {
        // mouseL up
        if (MODE(m_wFlags) == MODE_SELECT)
        {
          switch(_selectionType)
          {
          case stObjects:
            {
              m_Selection.RemoveAll();
              if (m_MouseSelection.Size() == 0)
                break; 

              if (m_MouseSelection.Size() == 1)
              {
                m_Selection.AddID(m_MouseSelection[0].id, m_MouseSelection[0].pos);
                break;
              }
              
              AutoArray<SMoveObjectPosMessData> eventData;

              for (i=0; i<m_MouseSelection.Size(); i++)
              {
                SMoveObjectPosMessData& data = eventData.Append();
                data.nID = m_MouseSelection[i].id;                
                m_Selection.AddID(m_MouseSelection[i].id, m_MouseSelection[i].pos, false);
              }  
              if (_exchange.NotNull()) _exchange->BlockSelectionObject(eventData);
              break;              
            }
          case stLandVertexes:
            {

              if (m_bCtrl)
              {
                Rect rect(floatMin(m_ptStart[0],m_ptCurrent[0]),
                  floatMin(m_ptStart[2],m_ptCurrent[2]),
                  floatMax(m_ptStart[0],m_ptCurrent[0]),
                  floatMax(m_ptStart[2],m_ptCurrent[2]));

                _landSelection.Add(rect);
                if (_exchange.NotNull()) _exchange->SelectionLandAdd(SLandSelPosMessData(rect._xs,rect._zs,rect._xe,rect._ze));                
              }
              else
              {
                if (m_bShift)
                {            
                  _landSelection.Remove(Rect(floatMin(m_ptStart[0],m_ptCurrent[0]),
                    floatMin(m_ptStart[2],m_ptCurrent[2]),
                    floatMax(m_ptStart[0],m_ptCurrent[0]),
                    floatMax(m_ptStart[2],m_ptCurrent[2]))); 

                  //SendEvent(SELECTION_LAND_CLEAR); BLOCK_SELECTION_LAND is defaultly exclusive.
                  AutoArray<SLandSelPosMessData> msgData;
                  const LandSelection::RectArray& area = _landSelection.GetArea();
                  for(int i = 0; i < area.Size(); i++)
                  {
                    SLandSelPosMessData& data = msgData.Append();
                    data.xs = area[i]._xs;
                    data.xe = area[i]._xe;
                    data.zs = area[i]._zs;
                    data.ze = area[i]._ze;
                  }

                  if (_exchange.NotNull()) _exchange->BlockSelectionLand(msgData);
                }
                else
                {
                  _landSelection.Release();
                  if (_exchange.NotNull()) _exchange->SelectionLandClear();

#define MIN_SELECTION_SIZE 0.1
                  if (fabs(m_ptStart[0] - m_ptCurrent[0]) > MIN_SELECTION_SIZE && fabs(m_ptStart[2] - m_ptCurrent[2]) > MIN_SELECTION_SIZE)
                  {
                    Rect rect(floatMin(m_ptStart[0],m_ptCurrent[0]),
                      floatMin(m_ptStart[2],m_ptCurrent[2]),
                      floatMax(m_ptStart[0],m_ptCurrent[0]),
                      floatMax(m_ptStart[2],m_ptCurrent[2]));

                    _landSelection.Add(rect);
                    if (_exchange.NotNull()) _exchange->SelectionLandAdd(SLandSelPosMessData(rect._xs,rect._zs,rect._xe,rect._ze));                
                  }
                }
              }
              break;
            }
          }

          m_ptStart = m_ptCurrent = VZero;  
          m_MouseSelection.Init(INIT_SEL_ITEMS);
          m_wFlags = NOTMODE(m_wFlags) | MODE_NORMAL;
        }
        else if (MODE(m_wFlags) == MODE_MOVE)
        {
          AutoArray<SMoveObjectPosMessData>  msgData;

          for (i=0; i<m_Selection.Size(); i++)
          {
            Ref<Object> pObj = GetSelectedObject(i);
            DoAssert(pObj);
            SMoveObjectPosMessData& data = msgData.Append();
            data.nID = m_Selection[i].id;
            {
              Matrix4 Pos = pObj->FutureVisualState().Transform();
              float relHeight;
              pObj->DeSkew(land,Pos, relHeight);
              for (int i=0; i<3; i++)
                for (int j=0; j<3; j++)
                  data.Position[i][j] = Pos.Orientation()(i, j);
              for (int i=0; i<3; i++)
                data.Position[i][3] = Pos.Position()[i];
            }            
          }

          if (_exchange.NotNull()) _exchange->BlockMove(msgData);          

          m_nPrimaryObject.id = VisitorObjId();
          m_nPrimaryObject.pos = VZero;
          m_wFlags = NOTMODE(m_wFlags) | MODE_NORMAL;
        }
      }
      if (GInput.mouseR && !m_bOldMouseR)
      {
        // mouseR down
        if (MODE(m_wFlags) == MODE_NORMAL && m_Selection.Size() > 0)
        {
          _origin = position;
          
          // we have to check if user wants to switch to a
          // scale mode
          if (GInput.keys[DIK_E]>0)           
            m_wFlags = NOTMODE(m_wFlags) | MODE_SCALE;
          else   
          m_wFlags = NOTMODE(m_wFlags) | MODE_ROTATE;
        }
      }
      else if (!GInput.mouseR && m_bOldMouseR)
      {
        // mouseR up
        if (MODE(m_wFlags) == MODE_ROTATE || 
            MODE(m_wFlags) == MODE_SCALE)
        {
          m_wFlags = NOTMODE(m_wFlags) | MODE_NORMAL;
          AutoArray<SMoveObjectPosMessData>  msgData;

          for (i=0; i<m_Selection.Size(); i++)
          {
            Ref<Object> pObj = GetSelectedObject(i);
            DoAssert(pObj);
            SMoveObjectPosMessData& data = msgData.Append();
            data.nID = m_Selection[i].id;
            {
              Matrix4 Pos = pObj->FutureVisualState().Transform();
              float relHeight;
              pObj->DeSkew(land,Pos, relHeight);
              for (int i=0; i<3; i++)
                for (int j=0; j<3; j++)
                  data.Position[i][j] = Pos.Orientation()(i, j);
              for (int i=0; i<3; i++)
                data.Position[i][3] = Pos.Position()[i];
            }            
          }
          
          if (_exchange.NotNull()) _exchange->BlockMove(msgData);
        }
      }
      
      // delete current selection?
      if (!GInput.mouseR && !GInput.mouseL && GInput.GetKeyToDo(DIK_D))
      {
        AutoArray<SMoveObjectPosMessData>  msgData;

        for (i=0; i<m_Selection.Size(); i++)
        {
          Ref<Object> pObj = GetSelectedObject(i);
          DoAssert(pObj);
          SMoveObjectPosMessData& data = msgData.Append();
          data.nID = m_Selection[i].id;
          {
            Matrix4 Pos = pObj->FutureVisualState().Transform();
            float relHeight;
            pObj->DeSkew(land,Pos, relHeight);
            for (int i=0; i<3; i++)
              for (int j=0; j<3; j++)
                data.Position[i][j] = Pos.Orientation()(i, j);
            for (int i=0; i<3; i++)
              data.Position[i][3] = Pos.Position()[i];
          }            
        }

        if (_exchange.NotNull()) _exchange->BlockRemoveSel(msgData);      
      }
      // delete current selection?
      if (!GInput.mouseR && !GInput.mouseL && GInput.GetKeyToDo(DIK_C))
      {
        AutoArray<SMoveObjectPosMessData>  msgData;

        for (i=0; i<m_Selection.Size(); i++)
        {
          Ref<Object> pObj = GetSelectedObject(i);
          DoAssert(pObj);
          SMoveObjectPosMessData& data = msgData.Append();
          data.nID = m_Selection[i].id;
          {
            Matrix4 Pos = pObj->FutureVisualState().Transform();
            float relHeight;
            pObj->DeSkew(land,Pos, relHeight);
            for (int i=0; i<3; i++)
              for (int j=0; j<3; j++)
                data.Position[i][j] = Pos.Orientation()(i, j);
            for (int i=0; i<3; i++)
              data.Position[i][3] = Pos.Position()[i];
          }            
        }

        if (_exchange.NotNull()) _exchange->BlockDuplicateSel(msgData);      
      }
      
      
      m_bOldMouseL = GInput.mouseL;
      m_bOldMouseR = GInput.mouseR;

      // change position of cursor
      if (bReset)
      {
        _camDistance = DefCamDistance;
        _camHeight = DefCamHeight;
        _dive = DefDive;
      }
      else
      {
        _dive += rotXAngle;
      }
      _heading += rotYAngle;

      Matrix3Val rotY = Matrix3(MRotationY, _heading);
      Matrix3Val rotX = Matrix3(MRotationX, _dive);
      Matrix3Val rot = rotY * rotX;

      position[1] = GLOB_LAND->SurfaceY(position[0],position[2]) + _camHeight;
      saturateMax(position[1],0.1); // never allow under water camera

      moveTrans.SetOrientation(rot);
      moveTrans.SetPosition(position);


      // Send message CURSOR_POSITION_SET if position changed
      bool bChanged = false;

      if (FutureVisualState().Transform().Direction() != m_posLast.Direction())
      {
        bChanged = true;     
      }
      else
      {    
        if (FutureVisualState().Transform().DirectionUp() != m_posLast.DirectionUp())
        {
          bChanged = true;     
        }
        else
        {

          if (FutureVisualState().Transform().DirectionAside() != m_posLast.DirectionAside())
          {
            bChanged = true;     
          }
          else
          {
            if (FutureVisualState().Transform().Position() != m_posLast.Position())            
              bChanged = true;     
          }
        }
      }

      if (bChanged)
      {      
        SMoveObjectPosMessData data;
        const Matrix4 *pPos = &FutureVisualState().Transform();
        for (int i=0; i<3; i++)
          for (int j=0; j<3; j++)
            data.Position[i][j] = pPos->Orientation()(i, j);
        for (int i=0; i<3; i++)
          data.Position[i][3] = pPos->Position()[i];
        if (_exchange.NotNull()) _exchange->CursorPositionSet(SMoveObjectPosMessData(data));
        m_posLast = FutureVisualState().Transform();
      }
    }
    else
    { // camera movement
      // rotate
      rotXAngle -= GInput.GetAction(UABuldLookDown) * speedKeyb * 0.2;
      rotYAngle += GInput.GetAction(UABuldLookLeft) * speedKeyb * 0.2;
      rotYAngle -= GInput.GetAction(UABuldLookRight) * speedKeyb * 0.2;
      rotXAngle += GInput.GetAction(UABuldLookUp) * speedKeyb * 0.2;
      float zoom=0;
      zoom += GInput.GetAction(UABuldZoomOut) * speedKeyb * 20;
      zoom -= GInput.GetAction(UABuldZoomIn) * speedKeyb * 20;

      float speedY=0;
      speedY += GInput.GetAction(UABuldUp) * speedKeyb;
      speedY -= GInput.GetAction(UABuldDown) * speedKeyb;

      bool reset=false;
      if (GInput.GetActionToDo(UABuldFreeLook))
      {
        reset = true;
      }
      // move - speedX,speedY,moveUp

      if
        (
        speedX || speedY || speedZ || rotYAngle || rotXAngle || reset
        )
      {
        // change position of cursor
        Object::ProtectedVisualState<const ObjectVisualState> pvs = _animCamera->FutureVisualStateScope();

        Vector3 position = pvs->Position();
        Matrix3 rot = pvs->Orientation();
        float scale = pvs->Scale();

        float dive=-atan2(rot.Direction().Y(),rot.Direction().SizeXZ());
        float heading=-atan2(rot.Direction().X(),rot.Direction().Z());
        dive += rotXAngle;

        float logScale=log(scale);
        logScale+=zoom;
        scale=exp(logScale);

        if( reset ) dive=0,scale=1;

        heading += rotYAngle;

        Matrix3Val rotY = Matrix3(MRotationY, heading);
        Matrix3Val rotX = Matrix3(MRotationX, dive);

        rot = rotY * rotX;

        Vector3 offset(speedX,speedY,speedZ);
        position+=10*(rot*offset);

        float minY=GLOB_LAND->SurfaceY(position[0],position[2]);
        if( position[1]<minY ) position[1]=minY;

        _animCamera->SetScale(scale);
        _animCamera->SetOrientation(rot);
        _animCamera->SetPosition(position);   

        _animCameraMoved=Glob.time; 
      }
    }

    // Send selection to landscape
    if (MODE(m_wFlags) == MODE_SELECT)
    {
      land->SetSelection(m_MouseSelection);
      //ptMin[1] = GLOB_LAND->SurfaceY(ptMin[0],ptMin[2]);
      //ptMax[1] = GLOB_LAND->SurfaceY(ptMax[0],ptMax[2]);
      //land->SetSelRectangle(ptMin, ptMax);
    }
    else
      land->SetSelection(m_Selection.GetData());

  }

  if
  (
    _animCamera
    && Glob.time-_animCameraMoved>0.3
    && _animCameraMoved>=_animCameraUpdated
  )
  {
    _animCameraUpdated=Glob.time;
        SMoveObjectPosMessData data(_animCamera->ID());
    const Matrix4 *pPos = &_animCamera->FutureVisualState().Transform();
    for (int i=0; i<3; i++)
      for (int j=0; j<3; j++)
        data.Position[i][j] = pPos->Orientation()(i, j);
    for (int i=0; i<3; i++)
      data.Position[i][3] = pPos->Position()[i];

    if (_exchange.NotNull()) _exchange->ObjectMove(data);
  }

  if (_showNode)
  {
    x=toInt(position.X()*InvTerrainGrid);
    z=toInt(position.Z()*InvTerrainGrid);
    Point3 pos(x*TerrainGrid, land->SurfaceY(x*TerrainGrid,z*TerrainGrid), z*TerrainGrid);
    GScene->ShowArrow(pos);
  }


  // Send messages to editor in batch
//  while (m_Queue.GetServerAppState() == APP_STATE_ACTIVE
//        && m_Queue.GetServerMsgCount() > 0)
//    m_Queue.TryToSendMessages();

  char szComm[5];
/*
  if (m_Queue.GetServerAppState() == APP_STATE_ACTIVE)
    if (busy)
      strcpy(szComm, "BUSY");
    else
      strcpy(szComm, "CONN");
  else
    strcpy(szComm, "DISC");

  if (m_Queue.GetServerAppState() == APP_STATE_ACTIVE)
    m_Queue.TryToSendMessages();
*/

  if (busy)
    strcpy(szComm, "BUSY");
  else if (ConnectedToVisitor)    
    strcpy(szComm, "CONN");     
  else    
    strcpy(szComm, "DISC");      
    
  if (GInput.GetAction(UABuldTextureInfo) > 0)
  {
    // Display land texture and material
    int x = toIntFloor(FutureVisualState().Position().X() * land->GetInvLandGrid());
    int z = toIntFloor(FutureVisualState().Position().Z() * land->GetInvLandGrid());
    const Scenery::TextureInfo& info = land->ClippedTextureInfo(z,x);

    const char *material = info._mat ? info._mat->GetName() : "";
    GlobalShowMessage(
      INT_MAX, "%s Pos=(%.1f, %.1f, %.1f) rvmat=%s",
      szComm, position.X(), position.Y(), position.Z(),material
    );
  }
  else
  {  

    // Display status line
    if( _cameraOnEdit )
    {
      Ref<Object> pNearest = land->NearestObject(position, NEAR_OBJECT, 
        ObjectType(Primary|Network
#if _ARMA3_TERRAIN_DECALS
        |LandDecal
#endif       
        ));
      int nNearest;
      char szNearest[64];
      if (pNearest)
      {
        nNearest = pNearest->ID();
        const char *nameFull=pNearest->GetShape()->Name();
        const char *name=strrchr(nameFull,'\\');
        if( !name ) name=nameFull;
        else name++;
        strcpy(szNearest, name);
        char *ext=strrchr(szNearest,'.');
        if( ext ) *ext = 0;
      }
      else
      {
        nNearest = 0;
        strcpy(szNearest, "<none>");
      }
      char szMode[64];
      switch (MODE(m_wFlags))
      {
      case MODE_NORMAL:
        strcpy(szMode, "Normal");
        break;
      case MODE_MOVE:
        strcpy(szMode, "Move");
        break;
      case MODE_ROTATE:
        strcpy(szMode, "Rotate");
        break;
      case MODE_SCALE:
        strcpy(szMode, "Scale");
        break;        
      case MODE_SELECT:
        strcpy(szMode, "Select");
        break;
      }
      if (m_wFlags & FLAG_POINTS)
        strcat(szMode, " Pts");
      if (m_wFlags & FLAG_PLANES)
        strcat(szMode, " Plns");
      if (m_wFlags & FLAG_YFIXED)
        strcat(szMode, " YFix");
      int nSelected=m_Selection.Size();
      char szSelection[64];
      switch(_selectionType)
      {
      case stObjects:
        strcpy(szSelection, "Objects");
        break;
      case stLandVertexes:
        strcpy(szSelection, "Land");
        break;
      default:
        strcpy(szSelection, "Error");      
      }
      GlobalShowMessage
        (
        INT_MAX, "%s Pos=(%.1f, %.1f, %.1f) Obj=%s(%d) Mode=%s x %d SelMode=%s",
        szComm, position.X(), position.Y(), position.Z(),
        szNearest, nNearest, szMode,nSelected,szSelection
        );
    }
    else
    {
      GlobalShowMessage
        (
        INT_MAX, "%s Camera=(%.1f, %.1f, %.1f)",
        szComm, _animCamera->FutureVisualState().Position().X(), _animCamera->FutureVisualState().Position().Y(), _animCamera->FutureVisualState().Position().Z()
        );
    }
  }

  Move(moveTrans); // finally apply move
  DirectionWorldToModel(FutureVisualState()._modelSpeed, FutureVisualState()._speed);
#endif
}

static void DrawRectangle(float xs, float zs, float xe, float ze, const PackedColor& color, float up)
{
  Vector3 ss(xs, GLandscape->SurfaceYAboveWater(xs, zs) + up , zs);
  Vector3 es(xe, GLandscape->SurfaceYAboveWater(xe, zs) + up , zs);
  Vector3 se(xs, GLandscape->SurfaceYAboveWater(xs, ze) + up , ze);
  Vector3 ee(xe, GLandscape->SurfaceYAboveWater(xe, ze) + up , ze);
  
  Ref<ObjectColored> obj;
  if (!obj)
  {
    Ref<LODShape> lShape=new LODShape();
    Shape *shape=new Shape;
    lShape->AddShape(shape,0);

    // initialize LOD level
    shape->ReallocTable(6);
    for (int i=0; i<6; i++)
    {
      shape->SetPos(i) = VZero;
      shape->SetNorm(i) = Vector3Compressed::Up;
    }
    shape->SetClipAll(ClipAll);
    /*shape->SetUV(0,0,0);
    shape->SetUV(1,1,0);
    shape->SetUV(2,0,1);
    shape->SetUV(3,1,0);
    shape->SetUV(4,1,1);
    shape->SetUV(5,0,1);*/

    // precalculate hints for possible optimizations
    shape->CalculateHints();
    lShape->CalculateHints();

  	shape->ReserveFaces(2);
    // change faces parameters
    Poly face;
    face.Init();
    face.SetN(3);
    face.Set(0,0);
    face.Set(1,1);
    face.Set(2,2);
    shape->AddFace(face);
    face.Set(0,3);
    face.Set(1,4);
    face.Set(2,5);
    shape->AddFace(face);

    lShape->OrSpecial(IsColored|IsOnSurface|IsAlpha|IsAlphaFog|BestMipmap);

    obj = new ObjectColored(lShape, VisitorObjId());
    obj->SetOrientation(M3Identity);
    obj->SetConstantColor(color);
  }
  
  // use global object
  LODShape *lShape = obj->GetShape();
  Shape *shape = lShape->InitLevelLocked(0);

  shape->SetPos(0) = VZero;
  shape->SetPos(1) = (se - ss) ;
  shape->SetPos(2) = (es - ss) ;
  shape->SetPos(3) = (se - ss);
  shape->SetPos(4) = (ee - ss) ;
  shape->SetPos(5) = (es - ss) ;
  Vector3Compressed normal((se - ss).CrossProduct(es - ss).Normalized());
  shape->SetNorm(0) = normal;
  shape->SetNorm(1) = normal;
  shape->SetNorm(2) = normal;
  normal = Vector3Compressed((ee - se).CrossProduct(es - se).Normalized());
  shape->SetNorm(3) = normal;
  shape->SetNorm(4) = normal;
  shape->SetNorm(5) = normal;

  lShape->SetAutoCenter(false);
  lShape->CalculateMinMax(true);

  ShapeSectionInfo prop;
  prop.Init();
  prop.SetSpecial(ClampU|ClampV|lShape->Special());
  prop.SetTexture(NULL);

  shape->SetAsOneSection(prop);

  obj->SetPosition(ss);
#if _HELISIM_RENDER_TO_TEXTURE
  DoAssert(CheckMainThread())
  obj->Draw(-1, 0, ClipAll, DrawParameters::_default, InstanceParameters::_default, 0.1f, obj->GetFrameBase(), NULL, GScene->GetDrawObjectIndex());
#else
  obj->Draw(-1, 0, ClipAll, DrawParameters::_default, InstanceParameters::_default, 0.1f, obj->GetFrameBase(), NULL);
#endif
}

static void DrawPolygon(const AutoArray<Vector3>& polygon, const PackedColor& color)
{ 
  
  Ref<LODShape> lShape=new LODShape();
  Shape *shape=new Shape;
  lShape->AddShape(shape,0);

  // initalize lod level
  int nVertexes = 3 * (polygon.Size() - 2);
  shape->ReallocTable(nVertexes);
  shape->SetClipAll(ClipAll);
  /*shape->SetUV(0,0,0);
  shape->SetUV(1,1,0);
  shape->SetUV(2,0,1);
  shape->SetUV(3,1,0);
  shape->SetUV(4,1,1);
  shape->SetUV(5,0,1);*/

  // precalculate hints for possible optimizations
  shape->CalculateHints();
  lShape->CalculateHints();

  // change faces parameters
  Poly face;
  face.Init();
  face.SetN(3);
  for(int i = 0; i < polygon.Size() - 2; i++)
  {    
    face.Set(0,3*i);
    face.Set(1,3*i+1);
    face.Set(2,3*i+2);
    shape->AddFace(face);
  }

  lShape->OrSpecial(/*IsColored|IsOnSurface|*/IsAlpha|IsAlphaFog|BestMipmap);

  Ref<ObjectColored> obj = new ObjectColored(lShape, VisitorObjId());
  obj->SetOrientation(M3Identity);
  obj->SetConstantColor(color);
  //}

  // use global object
  LODShape *lodShape = obj->GetShape();
  Shape *shape2 = lodShape->InitLevelLocked(0);

  Vector3 normal;
  for(int i = 0; i < polygon.Size() - 2; i++)
  { 
    shape2->SetPos(3*i) = VZero;
    shape2->SetPos(3*i+1) = polygon[i + 2] - polygon[0];
    shape2->SetPos(3*i+2) = polygon[i + 1] - polygon[0];

    normal = -(polygon[i + 1] - polygon[0]).CrossProduct(polygon[i + 2] - polygon[0]).Normalized();

    Vector3Compressed cNormal(normal);
    shape2->SetNorm(3*i) = cNormal;
    shape2->SetNorm(3*i+1) = cNormal;
    shape2->SetNorm(3*i+2) = cNormal;
  }

  // fake shadowing just to see a water surface correctly
  float K = normal *  Vector3(0.707,-0.707,0);
  K = fabs(K);
  PackedColor shadowColor(color.R8() * K,color.G8() * K, color.B8() * K, color.A8());
  obj->SetConstantColor(shadowColor);

  lodShape->SetAutoCenter(false);
  lodShape->CalculateMinMax(true);

  ShapeSectionInfo prop;
  prop.Init();
  prop.SetSpecial(ClampU|ClampV|lodShape->Special());
  prop.SetTexture(NULL);

  shape2->SetAsOneSection(prop);

  obj->SetPosition(polygon[0]);
#if _HELISIM_RENDER_TO_TEXTURE
  DoAssert(CheckMainThread());
  obj->Draw(-1, 0, ClipAll, DrawParameters::_default, InstanceParameters::_default, 0.1f, obj->GetFrameBase(), NULL
    , GScene->GetDrawObjectIndex());
#else
  obj->Draw(-1, 0, ClipAll, DrawParameters::_default, InstanceParameters::_default, 0.1f, obj->GetFrameBase(), NULL);
#endif
}
static Vector3 IntersectLines(Vector3Val fromLine1, Vector3Val dirLine1, Vector3Val fromLine2, Vector3Val dirLine2)
{
  Vector3 normLine2(dirLine2[2], 0, -dirLine2[0]);

  float t = (fromLine2 - fromLine1).DotProduct(normLine2) / dirLine1.DotProduct(normLine2);
  return fromLine1 + dirLine1 * t;
}

/** Functions draws colored rectangle over lnadscape. Rectangle can be smaller than the base grid rectangle.
  */
static void DrawRectanglePart(float xs, float zs, float xe, float ze, const PackedColor& color, float up)
{
  AutoArray<Vector3> points;
  points.Add(Vector3(xs, 0 , zs));
  points.Add(Vector3(xe, 0 , zs));
  points.Add(Vector3(xe, 0 , ze));
  points.Add(Vector3(xs, 0 , ze));  

  for(int j = 0; j < points.Size(); j++)
    points[j][1] = GLandscape->SurfaceYAboveWater( points[j][0],  points[j][2]) + up;

  float terrainGrid = GLandscape->GetTerrainGrid();
  float invTerrainRange = GLandscape->GetInvTerrainGrid();

  Vector3 ls((int)((xs + 0.01) * invTerrainRange) * terrainGrid, 0, (int)((zs + 0.01) * invTerrainRange) * terrainGrid  + terrainGrid);
  //Vector3 le(ls[0] + landGrid, 0, ls[2] - landGrid);
  Vector3 line(terrainGrid, 0, -terrainGrid);
  Vector3 normal(terrainGrid, 0, terrainGrid);

  bool bPlus = normal * (points[0] - ls) > 0;
  int i = 1;
  for(; i < points.Size() && (bPlus == (normal * (points[i] - ls) > 0)); i++);
  
  if (i == points.Size())
  {
    // no intersection draw normal rectangle.
    DrawRectangle( xs, zs, xe, ze, color, up);
    return;
  }

  int firstIntersect = i ;
  bPlus = !bPlus;

  i++;
  if (i == points.Size())
    i = 0;

  for(;(bPlus == (normal * (points[i] - ls) > 0)); (i == points.Size() - 1)? i = 0 : i++);

  int secondIntersect = i;

  Vector3 firstPoint = IntersectLines(ls, line, points[firstIntersect - 1], points[firstIntersect] - points[firstIntersect - 1]);  
  firstPoint[1] = GLandscape->SurfaceYAboveWater( firstPoint[0],  firstPoint[2]) + up;

  int secIntPrev = (secondIntersect == 0) ? points.Size() - 1 : secondIntersect - 1;
  Vector3 secondPoint = IntersectLines(ls, line, points[secIntPrev], points[secondIntersect] - points[secIntPrev]);  
  secondPoint[1] = GLandscape->SurfaceYAboveWater( secondPoint[0],  secondPoint[2]) + up;

  AutoArray<Vector3> firstPoly;

  firstPoly.Add(secondPoint);
  firstPoly.Add(firstPoint);

  i = firstIntersect;
  for(;i != secondIntersect; (i == points.Size() - 1)? i = 0 : i++)
  {
     firstPoly.Add(points[i]);
  }
  
  AutoArray<Vector3> secondPoly;
  secondPoly.Add(firstPoint);
  secondPoly.Add(secondPoint);

  for(;i != firstIntersect; (i == points.Size() - 1)? i = 0 : i++)
  {
    secondPoly.Add(points[i]);
  }

  DrawPolygon(firstPoly, color);
  DrawPolygon(secondPoly, color);
}
 
void EditCursor::Draw(
  int cb, int forceLOD, ClipFlags clipFlags, const DrawParameters &dp,
  const InstanceParameters &ip, float dist2, const PositionRender &pos, SortObject *oi
#if _HELISIM_RENDER_TO_TEXTURE
  , int drawListId
#endif
)
{
  base::Draw(cb,forceLOD, clipFlags, dp, ip, dist2, pos, oi
#if _HELISIM_RENDER_TO_TEXTURE
    , drawListId
#endif
    );
  _landSelection.Draw();
  _waterscape.Draw(FutureVisualState().Position());

  float xs = m_ptStart.X();
  float zs = m_ptStart.Z();
  float xe = m_ptCurrent.X();
  float ze = m_ptCurrent.Z();

  //DrawRectangle(xs, zs, xs + 110, zs + 110);
  //DrawRectangle(xe, ze, xe + 50, ze + 50);

  if (xs > xe) Swap(xs, xe);
  if (zs > ze) Swap(zs, ze);

  if (xe - xs < 0.01 || ze - zs < 0.01) return;

  float terrainGrid = GLandscape->GetTerrainGrid();

  int xxs = toIntCeil(xs / terrainGrid);
  int zzs = toIntCeil(zs / terrainGrid);
  int xxe = toIntFloor(xe / terrainGrid);
  int zze = toIntFloor(ze / terrainGrid);

  //DrawRectangle(xs, zs, xe, ze);

  //return;
  PackedColor color(Color(1,0,0,0.3));
  float up = 0.6f;;

  if (xxs > xxe)
  {
    if (zzs > zze) DrawRectanglePart(xs, zs, xe, ze, color,up);
    else
    {
      DrawRectanglePart(xs, zs, xe, zzs * terrainGrid, color, up);
      for (int zz=zzs; zz<zze; zz++) DrawRectanglePart(xs, zz * terrainGrid, xe, (zz + 1) * terrainGrid, color, up);
      DrawRectanglePart(xs, zze * terrainGrid, xe, ze, color, up);
    }
  }
  else
  {
    if (zzs > zze)
    {
      DrawRectanglePart(xs, zs, xxs * terrainGrid, ze, color, up);
      for (int xx=xxs; xx<xxe; xx++) DrawRectanglePart(xx * terrainGrid, zs, (xx + 1) * terrainGrid, ze, color, up);
      DrawRectanglePart(xxe * terrainGrid, zs, xe, ze, color, up);
    }
    else
    {
      float x = xxs * terrainGrid;
      DrawRectanglePart(xs, zs, x, zzs * terrainGrid, color, up);
      for (int zz=zzs; zz<zze; zz++) DrawRectanglePart(xs, zz * terrainGrid, x, (zz + 1) * terrainGrid, color, up);
      DrawRectanglePart(xs, zze * terrainGrid, x, ze, color, up);

      for (int xx=xxs; xx<xxe; xx++)
      {
        float x = xx * terrainGrid;
        float x1 = x + terrainGrid;
        DrawRectanglePart(x, zs, x1, zzs * terrainGrid, color, up);
        for (int zz=zzs; zz<zze; zz++) DrawRectangle(x, zz * terrainGrid, x1, (zz + 1) * terrainGrid, color, up);
        DrawRectanglePart(x, zze * terrainGrid, x1, ze, color, up);
      }

      x = xxe * terrainGrid;
      DrawRectanglePart(x, zs, xe, zzs * terrainGrid, color, up);
      for (int zz=zzs; zz<zze; zz++) DrawRectanglePart(x, zz * terrainGrid, xe, (zz + 1) * terrainGrid, color, up);
      DrawRectanglePart(x, zze * terrainGrid, xe, ze, color, up);
    }
  }
}

void LandSelection::Draw() const
{
  for(int i = 0; i < _area.Size(); i++)
  {
    float xs = _area[i]._xs;
    float zs = _area[i]._zs;
    float xe = _area[i]._xe;
    float ze = _area[i]._ze;

    //DrawRectangle(xs, zs, xs + 110, zs + 110);
    //DrawRectangle(xe, ze, xe + 50, ze + 50);

    if (xs > xe) Swap(xs, xe);
    if (zs > ze) Swap(zs, ze);

    if (xe - xs < 0.01 || ze - zs < 0.01) continue;

    float terrainRange = GLandscape->GetTerrainGrid();
    float invTerrainRange = GLandscape->GetInvTerrainGrid();

    int xxs = toIntCeil(xs * invTerrainRange);
    int zzs = toIntCeil(zs * invTerrainRange);
    int xxe = toIntFloor(xe * invTerrainRange);
    int zze = toIntFloor(ze * invTerrainRange);

    //DrawRectangle(xs, zs, xe, ze);

    //return;
    PackedColor color(Color(0,1,0,0.3));
    float up = 0.5f;

    if (xxs > xxe)
    {
      if (zzs > zze) DrawRectanglePart(xs, zs, xe, ze, color, up);
      else
      {
        DrawRectanglePart(xs, zs, xe, zzs * terrainRange, color, up);
        for (int zz=zzs; zz<zze; zz++) DrawRectanglePart(xs, zz * terrainRange, xe, (zz + 1) * terrainRange, color, up);
        DrawRectanglePart(xs, zze * terrainRange, xe, ze, color, up);
      }
    }
    else
    {
      if (zzs > zze)
      {
        DrawRectanglePart(xs, zs, xxs * terrainRange, ze, color, up);
        for (int xx=xxs; xx<xxe; xx++) DrawRectanglePart(xx * terrainRange, zs, (xx + 1) * terrainRange, ze, color, up);
        DrawRectanglePart(xxe * terrainRange, zs, xe, ze, color, up);
      }
      else
      {
        float x = xxs * terrainRange;
        DrawRectanglePart(xs, zs, x, zzs * terrainRange, color, up);
        for (int zz=zzs; zz<zze; zz++) DrawRectanglePart(xs, zz * terrainRange, x, (zz + 1) * terrainRange, color, up);
        DrawRectanglePart(xs, zze * terrainRange, x, ze, color, up);

        for (int xx=xxs; xx<xxe; xx++)
        {
          float x = xx * terrainRange;
          float x1 = x + terrainRange;
          DrawRectanglePart(x, zs, x1, zzs * terrainRange, color, up);
          for (int zz=zzs; zz<zze; zz++) DrawRectangle(x, zz * terrainRange, x1, (zz + 1) * terrainRange, color, up);
          DrawRectanglePart(x, zze * terrainRange, x1, ze, color, up);
        }

        x = xxe * terrainRange;
        DrawRectanglePart(x, zs, xe, zzs * terrainRange, color, up);
        for (int zz=zzs; zz<zze; zz++) DrawRectanglePart(x, zz * terrainRange, xe, (zz + 1) * terrainRange, color, up);
        DrawRectanglePart(x, zze * terrainRange, xe, ze, color, up);
      }
    }
  }
}


inline BOOL IsIntesectLine(float a1, float a2, float b1, float b2)
{
  return !((a2 <= b1) || (b2 <= a1));
}

inline BOOL IsIntersectRect(const Rect& rect1, const Rect& rect2)
{
  return IsIntesectLine(rect1._xs,rect1._xe,rect2._xs,rect2._xe) && 
    IsIntesectLine(rect1._zs,rect1._ze,rect2._zs,rect2._ze);
}

inline BOOL IsLineInLine(float a1, float a2, float b1, float b2)
{
  return ((b1 <= a1) && (a2 <= b2));
}

inline BOOL IsRectInRect(const Rect& rect1, const Rect& rect2)
{
  return IsLineInLine(rect1._xs,rect1._xe,rect2._xs,rect2._xe) && 
    IsLineInLine(rect1._zs,rect1._ze,rect2._zs,rect2._ze);
}
typedef struct 
{
  BOOL bFromRemove;
  LONG s,e;
} INTERVAL;

TypeIsSimple(INTERVAL);

void LandSelection::Remove(const Rect& rectRem)
{
  AutoArray<Rect> cNewArea;
  cNewArea = _area;
  Release();

  for(int i = 0; i < cNewArea.Size(); i++)
  {
    const Rect& rect = cNewArea[i];
    if( IsIntersectRect(rectRem, rect) )
    {
      if (!IsRectInRect(rect, rectRem))
      {
        if (IsRectInRect(rectRem, rect))
        {
          Rect rectRes;
          rectRes._xs = rect._xs;
          rectRes._xe = rectRem._xs;
          rectRes._ze = rect._ze;
          rectRes._zs = rect._zs;

          if (rectRes.Width() >= 0 && rectRes.Height() >= 0)
            Add(rectRes);

          rectRes._xs = rectRem._xs;
          rectRes._xe = rectRem._xe;
          //rectRes._ze = rect._ze;
          rectRes._zs = rectRem._ze;

          if (rectRes.Width() >= 0 && rectRes.Height() >= 0)
            Add(rectRes);

          //rectRes._xs = rectRem._xs;
          //rectRes._xe = rectRem._xe;
          rectRes._ze = rectRem._zs;
          rectRes._zs = rect._zs;

          if (rectRes.Width() >= 0 && rectRes.Height() >= 0)
            Add(rectRes);

          rectRes._xs = rectRem._xe;
          rectRes._xe = rect._xe;
          rectRes._ze = rect._ze;
          rectRes._zs = rect._zs;

          if (rectRes.Width() >= 0 && rectRes.Height() >= 0)
            Add(rectRes);
        }
        else
        {

          AutoArray<INTERVAL> cIntervalsX;
          cIntervalsX.Realloc(3);

          INTERVAL inter;

          inter.s = rect._xs;
          if (rectRem._xs > rect._xs)
          {
            inter.e = rectRem._xs;
            inter.bFromRemove = FALSE;

            cIntervalsX.Add(inter);

            inter.s = rectRem._xs;
          }

          if (rect._xe > rectRem._xe)
          {
            inter.e = rectRem._xe;
            inter.bFromRemove = TRUE;

            cIntervalsX.Add(inter);

            inter.s = rectRem._xe;
            inter.e = rect._xe;
            inter.bFromRemove = FALSE;

            cIntervalsX.Add(inter);
          }
          else
          {
            inter.e = rect._xe;
            inter.bFromRemove = TRUE;

            cIntervalsX.Add(inter);
          }

          AutoArray<INTERVAL> cIntervalsY;
          cIntervalsY.Realloc(3);          

          inter.s = rect._zs;
          if (rectRem._zs > rect._zs)
          {
            inter.e = rectRem._zs;
            inter.bFromRemove = FALSE;

            cIntervalsY.Add(inter);

            inter.s = rectRem._zs;
          }

          if (rect._ze > rectRem._ze)
          {
            inter.e = rectRem._ze;
            inter.bFromRemove = TRUE;

            cIntervalsY.Add(inter);

            inter.s = rectRem._ze;
            inter.e = rect._ze;
            inter.bFromRemove = FALSE;

            cIntervalsY.Add(inter);
          }
          else
          {
            inter.e = rect._ze;
            inter.bFromRemove = TRUE;

            cIntervalsY.Add(inter);
          }

          for(int j = 0; j < cIntervalsX.Size(); j++)
          {
            for(int k = 0; k < cIntervalsY.Size(); k++)
            {
              if (cIntervalsX[j].bFromRemove && cIntervalsY[k].bFromRemove)
                continue;

              Rect rectNew(cIntervalsX[j].s, cIntervalsY[k].s,cIntervalsX[j].e, cIntervalsY[k].e);
              Add(rectNew);
            }
          }
        }
      }
    }
    else
      Add(rect);
  }
  CalculateBoundingRect();
}

void LandSelection::GetSelectedVertexes(LandVertexesArray& vertexes) const
{  
  float terrainGrid = GLandscape->GetTerrainGrid();

  for(int i = 0; i < _area.Size(); i++)
  {
    const Rect& rect = _area[i];
    
    int xxs = toIntCeil (rect._xs / terrainGrid);
    int zzs = toIntCeil (rect._zs / terrainGrid);
    int xxe = toIntFloor(rect._xe / terrainGrid);
    int zze = toIntFloor(rect._ze / terrainGrid);

    IntVector2 vertex;
    for (int zz = zzs; zz <= zze; zz++) for (int xx = xxs; xx <= xxe; xx++)
    {
      if (TerrainInRange(zz, xx))
      {
        vertex[0] = xx;
        vertex[1] = zz;

        vertexes.AddUnique(vertex);        
      }
    }
  }
}

void LandSelection::Add(const Rect& rect)
{
  _area.Add(rect);

  if (rect._xs < _boundingRect._xs)
    _boundingRect._xs = rect._xs;

  if (rect._zs < _boundingRect._zs)
    _boundingRect._zs = rect._zs;

  if (rect._xe > _boundingRect._xe)
    _boundingRect._xe = rect._xe;

  if (rect._ze > _boundingRect._ze)
    _boundingRect._ze = rect._ze;
}

void LandSelection::Release()
{
  _area.Resize(0);
  _boundingRect._xs = _boundingRect._zs = FLT_MAX;
  _boundingRect._xe = _boundingRect._ze = -FLT_MAX;
}

LandSelection::LandSelection()
{
  _boundingRect._xs = _boundingRect._zs = FLT_MAX;
  _boundingRect._xe = _boundingRect._ze = -FLT_MAX;
}

void LandSelection::CalculateBoundingRect()
{
  for(int i = 0; i < _area.Size(); i++)
  {
    const Rect& rect = _area[i];

    if (rect._xs < _boundingRect._xs)
      _boundingRect._xs = rect._xs;

    if (rect._zs < _boundingRect._zs)
      _boundingRect._zs = rect._zs;

    if (rect._xe > _boundingRect._xe)
      _boundingRect._xe = rect._xe;

    if (rect._ze > _boundingRect._ze)
      _boundingRect._ze = rect._ze;
  }
}

//-----------------------------------------------------
//  Water scape
//-----------------------------------------------------

void Waterscape::Draw(Vector3Val pos) const
{
  if (_grid == 0)
    return;

  bool diffGrid = fabs(_grid - GLandscape->GetTerrainGrid()) > 0.001; // If grid is different draw everything
  float invGrid = 1.0f/_grid;
  int around = 20;

  int xpos = pos[0] * invGrid;
  int zpos = pos[2] * invGrid;

  int xstart = std::max(0, xpos - around);
  int xend = std::min(xpos + around, GetXRange() - 1);

  int zstart = std::max(0, zpos - around);
  int zend = std::min(zpos + around, GetYRange() - 1);

  AutoArray<Vector3> tr;
  tr.Resize(3);
  for(int x = xstart; x < xend; x++) for(int z = zstart; z < zend; z++)
  {
    // first rectangle
#define COMPARE_LANDSCAPE(xx,zz) \
    (GLandscape->GetHeight(z+zz,x+xx) > Get(x + xx,z + zz)) 

#define SET_TRIANGLE_VECTOR(i,xx,zz) \
  tr[i][0] = (x +xx) * _grid; \
  tr[i][1] = Get(x + xx,z + zz); \
  tr[i][2] = (z + zz) * _grid;

    if ( diffGrid ||  (!COMPARE_LANDSCAPE(0,0) || !COMPARE_LANDSCAPE(1,0) || !COMPARE_LANDSCAPE(0,1)))
    {
      SET_TRIANGLE_VECTOR(0,0,0);
      SET_TRIANGLE_VECTOR(1,1,0);
      SET_TRIANGLE_VECTOR(2,0,1);

      DrawPolygon(tr, PackedColor(Color(0,0,1,0.5)));

    }

    if (diffGrid || (!COMPARE_LANDSCAPE(1,0) || !COMPARE_LANDSCAPE(1,1) || !COMPARE_LANDSCAPE(0,1)))
    {
      SET_TRIANGLE_VECTOR(0,1,0);
      SET_TRIANGLE_VECTOR(1,1,1);
      SET_TRIANGLE_VECTOR(2,0,1);

      DrawPolygon(tr, PackedColor(Color(0,0,1,0.5)));
    }
  }
}

void Waterscape::Destroy()
{
  _grid = 0;
  Clear();
}

void EditCursor::SystemQuit()
{
  Landscape *land=GLOB_LAND;
  land->Quit();
  Glob.exit=true;
}


void EditCursor::SystemInit(const SLandscapePosMessData &msgData, const char *configName)
{
  Landscape *land=GLOB_LAND;
  Ref<Entity> temp = this;
  GWorld->DeleteVehicle(this);
   land->Dim
    (
    msgData.textureRangeX,msgData.textureRangeY,
    msgData.landRangeX,msgData.landRangeY,
    msgData.landGridX
    );
  if (strlen(configName) == 0)
    land->Init(Pars>>"CfgWorlds">>"DefaultWorld");
  else
  {
    ParamEntryPar cls = Pars>>"CfgWorlds";
    if (cls.FindEntry(configName).IsNull())
    {
      RptF("Config for island %s not found. Using DefaultWorld instead",configName);
      land->Init(cls>>"DefaultWorld");          
    }
    else
      land->Init(cls>>configName);
  }  
  GWorld->AddVehicle(this);
  _waterscape.Destroy();
}


void EditCursor::FileExport(const char * data)
{
  Landscape *land=GLOB_LAND;
  land->SaveData(data);
}

void EditCursor::CursorPositionSet(const SMoveObjectPosMessData &msgData)
{
  Landscape *land=GLOB_LAND;
  _afterConnect = false;
  Matrix4 trans;
  Point3 pos;
  Matrix3Val ori=FutureVisualState().Transform().Orientation();
    for (int i=0; i<3; i++)
      pos[i] = msgData.Position[i][3];
  // Setle cursor on the ground
  pos[1] = land->SurfaceY(pos[0],pos[2]) + _camHeight;
  saturateMax(pos[1],0.1); // never allow under water camera

  trans.SetOrientation(ori);
  trans.SetPosition(pos);
  m_posLast = trans;
  Move(trans);
}

void EditCursor::SelectionObjectClear()
{
  m_Selection.GetData().Init(INIT_SEL_ITEMS);
}

static DWORD SLastTimeUpdate=0;
static bool UpdateProgress(Ref<ProgressHandle> &progress, TransferState &curstate, TransferState newstate, int &variable, const char *text)
{
  bool ret=false;
  if (curstate!=newstate)
  {
    if (newstate==TSNone)
    {
      ProgressFrame();
      ProgressFinish(progress);
      curstate=newstate;         
      return ret;
    }
    else
    {
      ProgressFinish(progress);
      ProgressReset();
      progress = ProgressStart(text);
      ProgressAdd(variable);
      ProgressFrame();
      curstate=newstate;         
      ret=true;
    }
  }
  ProgressAdvance(1);
  variable--;
  DWORD tm=GetTickCount();
  if (tm-SLastTimeUpdate>100) 
  {
    ProgressRefresh();
    SLastTimeUpdate=tm;
  }
  return ret;
}


void EditCursor::SelectionObjectAdd(const SObjectPosMessData &data)
{
  int nFound = -1;
  for (int i=0; i<m_Selection.Size(); i++)
    if (m_Selection[i].id == data.nID)
    {
      nFound = i;
      break;
    }
    if (data.bState)
    {
      if (nFound < 0)
      {           
        Vector3 Pos(data.Position[0][3], data.Position[1][3], data.Position[2][3]);
        m_Selection.AddID(VisitorObjId(data.nID),Pos,false);
      }
    }
    else
    {
      if (nFound >= 0)
        m_Selection.GetData().Delete(nFound);
    }
  ForceRender=true;
}

void EditCursor::RegisterLandscapeTexture(const SMoveObjectPosMessData &data, const char *name)
{
  Landscape *land=GLOB_LAND;
  // id currently encodes major pass
  // it could be better to change message format instead, but it is too time consuming
  int id = data.nID&0xffffff;
  int pass = (data.nID>>24)&0xff;
  DoAssert(id>=0 && id<=0xffff);
  DoAssert(pass<=4);
  land->RegisterTexture(data.nID, name, pass);
  if (_transfer) 
    UpdateProgress(_transfer->progress, _transfer->state,TSTexturesRegistration,_transfer->texRegistered,"Preparing surface materials");
}

void EditCursor::BlockSelectionObject(const Array<SMoveObjectPosMessData> &msgData)
{
  for(int iO = 0; iO < msgData.Size(); iO++)
  {
    SelectionObjectAdd(static_cast<const SObjectPosMessData &>(msgData[iO]));
  }
}

void EditCursor::SelectionLandClear()
{
   _landSelection.Release();
}

void EditCursor::SelectionLandAdd(const SLandSelPosMessData& msgData)
{
  Rect rect;
        rect._xs = msgData.xs;
        rect._xe = msgData.xe;
        rect._zs = msgData.zs;
        rect._ze = msgData.ze;

        _landSelection.Add(rect);
}

void EditCursor::BlockSelectionLand(const Array<SLandSelPosMessData> &data)
{
  for (int i=0;i<data.Size();i++) SelectionLandAdd(data[i]);
}

void EditCursor::RegisterObjectType(const char *name)
{
  ///this function is currently not necessery
}

void EditCursor::LandHeightChange(const STexturePosMessData& msgData)
{
  Landscape *land=GLOB_LAND;
  land->HeightChange(msgData.nX, msgData.nZ, msgData.Y);      
  land->FlushCache();  
  if (_transfer)
    UpdateProgress(_transfer->progress,_transfer->state, TSHeights,_transfer->landSize,"Extruding hills and valleys");
}

void EditCursor::BlockLandHeightChange(const Array<STexturePosMessData> &data)
{
  for (int i=0;i<data.Size();i++) LandHeightChange(data[i]);
}

void EditCursor::BlockLandHeightChangeInit(const Array<float> &heights)
{
  Landscape *land=GLOB_LAND;
  if (_transfer)
  {
    if (UpdateProgress(_transfer->progress,_transfer->state, TSHeights,_transfer->landSize,"Extruding hills and valleys"))
    {
      _transfer->xCoord=_transfer->zCoord=0;
    }
    int terrainRange = land->GetTerrainRange();
    if (_transfer->xCoord >= terrainRange) return;

    for(int i = 0; i < heights.Size(); i++)
    {       
      land->HeightChange(_transfer->xCoord, _transfer->zCoord, heights[i]);
      _transfer->zCoord++;
      if (_transfer->zCoord >= terrainRange)
      {
        _transfer->zCoord = 0;
        _transfer->xCoord++;
      }
    }
    ProgressAdvance(heights.Size()-1);
//    land->FlushCache();  
  }
}

void EditCursor::LandTextureChange(const STexturePosMessData& msgData)
{
  GLandscape->TextureChange(msgData.nX, msgData.nZ, msgData.nTextureID);

  if (_transfer)
    UpdateProgress(_transfer->progress,_transfer->state,TSTextures,_transfer->landTextures,"Painting the ground");
  else
  {
    //GLandscape->ResetGeography();
    //GLandscape->InitGeography();
    GLandscape->FlushCache();
  }
}

void EditCursor::BlockLandTextureChangeInit(const Array<int> &ids)
{      
  Landscape *land=GLOB_LAND;
  if (_transfer)
  {
    if (UpdateProgress(_transfer->progress,_transfer->state,TSTextures,_transfer->landTextures,"Painting the ground"))
    {
      _transfer->xCoord=_transfer->zCoord=0;
    }
    int landRange = land->GetLandRange();
    if (_transfer->xCoord >= landRange) return;
    //LogF("  TextureChange");

    for(int i = 0; i < ids.Size(); i++)
    {       
      if (ids[i] != 0)
        land->TextureChange(_transfer->xCoord, _transfer->zCoord, ids[i]);

      _transfer->zCoord++;
      if (_transfer->zCoord >= landRange)
      {
        _transfer->zCoord = 0;
        _transfer->xCoord++;
      }
    }
    ProgressAdvance(ids.Size()-1);
  }
}

void EditCursor::ObjectCreate(const SMoveObjectPosMessData &msgData, const char *name)
{
  Landscape *land=GLOB_LAND;

  Matrix4 trans;
  Point3 pos;
  Matrix3 ori;

  for (int i=0; i<3; i++)
      for (int j=0; j<3; j++)
        ori(i, j) = msgData.Position[i][j];

  for (int i=0; i<3; i++)
    pos[i] = msgData.Position[i][3];
  trans.SetOrientation(ori);
  trans.SetPosition(pos);
  // SetTransform(trans);
  Object *obj = land->ObjectCreate(msgData.nID, name, trans, _transfer!=0);
  if (obj && obj->GetShape())
  {
    // obj->GetShape() shape is in ShapeBank
    //RegisterVBShape(obj->GetShape());
    obj->GetShape()->OptimizeRendering(VBStatic);
  }
  if (!_transfer && CurrentAppInfoFunctions->GetAppHWnd() != GetForegroundWindow()) // I am not working on buldozer
  {        
    Move(trans);
  }
  if (_transfer)
    UpdateProgress(_transfer->progress,_transfer->state,TSObjects,_transfer->objInstances,"Inserting some objects");
}


void EditCursor::ObjectDestroy(const SMoveObjectPosMessData &data)
{
  Landscape *land=GLOB_LAND;
  m_Selection.RemoveID(data.nID);
  // TODOLIST: right position

  EditorSelectionItem item;
  item.id = VisitorObjId(data.nID);        
  item.pos = Vector3(data.Position[0][3], data.Position[1][3], data.Position[2][3]);
  land->ObjectDestroy(item);
}

void EditCursor::ObjectMove(const SMoveObjectPosMessData& msgData)
{
  Landscape *land=GLOB_LAND;

  Matrix4 trans;
  Point3 pos;
  Matrix3 ori;
  
    for (int i=0; i<3; i++)
      for (int j=0; j<3; j++)
        ori(i, j) = msgData.Position[i][j];
  for (int i=0; i<3; i++)
    pos[i] = msgData.Position[i][3];

  trans.SetOrientation(ori);
  trans.SetPosition(pos);

  if (CurrentAppInfoFunctions->GetAppHWnd() != GetForegroundWindow()) // I am not working on buldozer                        
    Move(trans);        

  // TODOLIST: right position
  EditorSelectionItem item;
  item.id = VisitorObjId(msgData.nID);
  item.pos = pos;
  land->ObjectMove(item, trans);
}

void EditCursor::BlockMove(const Array<SMoveObjectPosMessData> &data)
{
  for (int i=0;i<data.Size();i++) ObjectMove(data[i]);
}

//-----------------------------------------------------------------------------
void EditCursor::BlockDuplicateSel(const Array<SMoveObjectPosMessData> &data)
{  
}

//-----------------------------------------------------------------------------
void EditCursor::BlockRemoveSel(const Array<SMoveObjectPosMessData> &data)
{  
}

void EditCursor::FileImportBegin(const STexturePosMessData& msgData)
{
#if _ENABLE_BULDOZER
  Landscape *land=GLOB_LAND;


  TransferInfo info;
  _transfer=&info;
  info.objRegistered = msgData.nX;
  info.objInstances = msgData.nZ;
  info.texRegistered = msgData.nTextureID;
  info.landSize=toLargeInt(Square(land->GetTerrainRange()));
  info.landTextures=toLargeInt(Square(land->GetLandRange()));
  info.landWater=1;
  info.xCoord=info.zCoord=0;

  land->ClearClutterMapping();
  
  int exitMsgs[]={SYSTEM_QUIT,SYSTEM_INIT,FILE_IMPORT_BEGIN,FILE_IMPORT_END};  
  PSPosMessage msg;
  VisViewerExchange::Status res=static_cast<VisViewerExchange *>(_exchange.GetRef())->WaitMessage(Array<int>(exitMsgs,lenof(exitMsgs)),msg);

  if (res==VisViewerExchange::statOk && msg->_iMsgType!=FILE_IMPORT_BEGIN && msg->_iMsgType!=FILE_IMPORT_END)
    static_cast<VisViewerExchange *>(_exchange.GetRef())->DispatchMessage(*msg);
  
  UpdateProgress(_transfer->progress,_transfer->state,TSNone,_transfer->landSize,"");
  _transfer=0;
  
  FileImportEnd();
  // transfer terminated - FileImportEnd was received
  ForceRender=true;

#endif
}

void EditCursor::FileImportEnd()
{
#if _ENABLE_BULDOZER
  Landscape *land=GLOB_LAND;
  // init as much geography as you can
  LogF("[Buldozer] ResetGeography ...");
  land->ResetGeography();
  LogF("[Buldozer] InitGeography ...");
  land->InitGeography();
  LogF("[Buldozer] FlushCache ...");
  land->FlushCache();
  
  LogF("[Buldozer] BuildClutterMapping ...");
  land->BuildClutterMapping();
  LogF("[Buldozer] InitWaterMap ...");
  land->InitWaterAndGrassMap(false);
  LogF("[Buldozer] InitCache ...");
  land->InitCache(true);
  // recalculate bounding information
  LogF("[Buldozer] Recalculate ...");
  land->Recalculate();
  LogF("[Buldozer] InitPrimaryTexture ...");
  land->InitPrimaryTexture();
#endif
}

void EditCursor::BlockWaterHeightChangeInit(const Array<float> &heights)
{
  if (_transfer)
  {
    int i = 0;
    if (_transfer->state!=TSWaterHeights)
    {
      _transfer->xCoord=_transfer->zCoord=0;
      _waterscape.Dim(toInt(heights[0]),toInt(heights[1]), heights[2]);
      i=3;
      _transfer->landWater=_waterscape.GetXRange() * _waterscape.GetYRange();
    }
    UpdateProgress(_transfer->progress,_transfer->state,TSWaterHeights,_transfer->landWater,"Pouring out the water");

    int yRange = _waterscape.GetYRange();

    for(; i < heights.Size(); i++)
    {                 
      _waterscape.Set(_transfer->xCoord, _transfer->zCoord) =  heights[i];

      _transfer->zCoord++;
      if (_transfer->zCoord >= yRange)
      {
        _transfer->zCoord = 0;
        _transfer->xCoord++;
      }
    }
    ProgressAdvance(heights.Size()-1);
  }
}

#endif // _ENABLE_BULDOZER
