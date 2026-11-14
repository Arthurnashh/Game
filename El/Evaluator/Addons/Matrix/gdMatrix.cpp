#include <fstream>
#include <strstream>
using namespace std;
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <Es/essencePch.hpp>
#include <Es/Types/pointers.hpp>
#include <Es/Containers/array.hpp>
#include <El/Pathname/Pathname.h>
#include <malloc.h>
#include <El/Btree/Btree.h>


#include <el/math/math3d.hpp>


#include <Es/Common/fltOpts.hpp>
#include <ES/Strings/rString.hpp>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include ".\gdmatrix.h"

#define Category "Matrix"

DEFINE_FAST_ALLOCATOR(GdMatrix)


RString GdMatrix::GetText() const
{
  char buff[500];
  ostrstream str(buff,sizeof(buff));
  str.precision(1);
  str<<fixed;
  for (int row=0;row<4;row++)
  {
    str<<"[";
    for (int col=0;col<3;col++)
    {
      if (col) str<<",";
      str<<(*this)(col,row);
    }
    str<<"]";
  }
  str.put(0);
  return RString(buff);
}


bool GdMatrix::IsEqualTo(const GameData *data) const
{
  const GdMatrix *other=dynamic_cast<const GdMatrix *>(data);
  return (other && (Matrix4P&)*other==(Matrix4P&)*this);
}

#define GET_MATRIX(oper) (static_cast<GdMatrix &>(*oper.GetData()))

static GameValue mxMultiply(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  GdMatrix &mx1=GET_MATRIX(oper1);
  GdMatrix &mx2=GET_MATRIX(oper2);
  GdMatrix *sum=new GdMatrix;
  sum->SetMultiply(mx2,mx1);
  return GameValue(sum);
}

static GameValue mxPush(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  if (oper1.GetReadOnly())
  {
    gs->SetError(EvalForeignError,"Readonly stack");
  }
  GameArrayType &arr=const_cast<GameArrayType &>((const GameArrayType &)oper1);
  arr.Append()=oper2;
  return oper1;
}

static GameValue mxPivotScale(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType &vector=oper1;
  if (vector.Size()<3) 
  {
    gs->SetError(EvalType);
    return GameValue();
  }
  Vector3 pivot(vector[0],vector[1],vector[2]);
  Vector3 scale;
  if (oper2.GetType()==GameScalar) scale=Vector3(oper2,oper2,oper2);
  else 
  {
    const GameArrayType &arr=oper2;
    if (arr.Size()<3) 
    {
      gs->SetError(EvalType);
      return GameValue();
    }
    scale=Vector3(arr[0],arr[1],arr[2]);
  }
      
  GdMatrix *ret=new GdMatrix;
  ret->SetMultiply(Matrix4P(MTranslation,-pivot),Matrix4P(MMultiply,Matrix4P(MScale,scale[0],scale[1],scale[2]),Matrix4P(MTranslation,pivot)));
  return GameValue(ret);
}

static GameValue mxPivotRotate(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType &vector=oper1;
  if (vector.Size()<3) 
  {
    gs->SetError(EvalType);
    return GameValue();
  }
  Vector3 pivot(vector[0],vector[1],vector[2]);
  Vector3 axis;
  float angle; 
  const GameArrayType &arr=oper2;
  if (arr.Size()==2 && arr[0].GetType()==GameArray && arr[1].GetType()==GameScalar)
  {
    const GameArrayType &vc=arr[0];
    if (vc.Size()<3)
      {
        gs->SetError(EvalType);
        return GameValue();
      }
    axis=Vector3(vc[0],vc[1],vc[2]);
    angle=arr[1];
  }
  else if (arr.Size()==4)
  {
    axis=Vector3(arr[0],arr[1],arr[2]);
    angle=arr[3];
  }
  else
  {
    gs->SetError(EvalType);
    return GameValue();
  }

  GdMatrix *ret=new GdMatrix;
  ret->SetMultiply(Matrix4P(MTranslation,-pivot),Matrix4P(MMultiply,Matrix4P(MRotationAxis,axis,angle*3.1415926535f/180.0f),Matrix4P(MTranslation,pivot)));
  return GameValue(ret);
}

static GameValue mxInterpolate(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType &stack=oper1;
  if (stack.Size()<2)
    {
      gs->SetError(EvalForeignError,"stack is too small");
      return GameValue();
    }
  GameValuePar suboper1=stack[stack.Size()-1];
  GameValuePar suboper2=stack[stack.Size()-2];
  if (suboper1.GetType()!=EvalType_Matrix || suboper2.GetType()!=EvalType_Matrix)
    {
      gs->SetError(EvalType);
      return GameValue();
    }
  GdMatrix &mx1=GET_MATRIX(suboper1);
  GdMatrix &mx2=GET_MATRIX(suboper2);
  GdMatrix *ret=new GdMatrix;
  float factor=oper2;
  bool norm=factor<0.0f;
  factor=(float)fabs(factor);
  for (int i=0;i<4;i++)
    for (int j=0;j<3;j++)
    {
      float a=mx1(j,i);
      float b=mx2(j,i);
      (*ret)(j,i)=a+(b-a)*factor;
    }
  if (norm)
  {
    Vector3 a;
    a=ret->Direction();ret->SetDirection(a.Normalized());
    a=ret->DirectionUp();ret->SetDirectionUp(a.Normalized());
    a=ret->DirectionAside();ret->SetDirectionAside(a.Normalized());
  }
  return GameValue(ret);
}

static GameValue mxCalculateHierarchy(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType hierarchy=oper1;
  const GameArrayType stack=oper2;
  GameArrayType out;
  for (int i=0;i<stack.Size();i++)
  {
    int parent=-1;
    if (i<hierarchy.Size() && hierarchy[i].GetType()==GameScalar) parent=toInt(hierarchy[i]);
    GdMatrix *resmx=new GdMatrix;    
    out.Append()=GameValue(resmx);
    if (stack[i].GetType()==EvalType_Matrix)
    {
      if (parent<-1 || parent>=stack.Size()) parent=-1;
      if (parent==-1) *resmx=GET_MATRIX(stack[i]);
      else      
        resmx->SetMultiply(GET_MATRIX(stack[i]),GET_MATRIX(stack[parent]));    
    }
  }
  return out;
}

static GameValue mxMove(const GameState *gs, GameValuePar oper1)
{
  const GameArrayType vector=oper1;
  GdMatrix *resmx=new GdMatrix;
  Vector3 vx(vector[0],vector[1],vector[2]);
  resmx->SetTranslation(vx);
  return GameValue(resmx);
}

static GameValue mxPivot(const GameState *gs, GameValuePar oper1)
{
  const GameArrayType vector=oper1;
  GdMatrix *resmx=new GdMatrix;
  Vector3 vx(-(float)vector[0],-(float)vector[1],-(float)vector[2]);
  resmx->SetTranslation(vx);
  return GameValue(resmx);
}

static GameValue mxScale(const GameState *gs, GameValuePar oper1)
{
  Vector3 vx;
  if (oper1.GetType()==GameScalar)  
    vx=Vector3(oper1,oper1,oper1);
  else
  {
    const GameArrayType vector=oper1;
    if (vector.Size()<3)
    {
      gs->SetError(EvalType);
      return GameValue();
    }
    vx=Vector3((float)vector[0],(float)vector[1],(float)vector[2]);
  }
  GdMatrix *resmx=new GdMatrix;
  resmx->SetScale(vx[0],vx[1],vx[2]);
  return GameValue(resmx);
}

static GameValue mxRotate(const GameState *gs, GameValuePar oper1)
{
  Vector3 axis;
  float angle; 
  const GameArrayType &arr=oper1;
  if (arr.Size()==2 && arr[0].GetType()==GameArray && arr[1].GetType()==GameScalar)
  {
    const GameArrayType &vc=arr[0];
    if (vc.Size()<3)
      {
        gs->SetError(EvalType);
        return GameValue();
      }
    axis=Vector3(vc[0],vc[1],vc[2]);
    angle=arr[1];
  }
  else if (arr.Size()==4)
  {
    axis=Vector3(arr[0],arr[1],arr[2]);
    angle=arr[3];
  }
  else
  {
    gs->SetError(EvalType);
    return GameValue();
  }
  GdMatrix *resmx=new GdMatrix;
  resmx->SetRotationAxis(axis,angle*3.1415926535f/180.0f);
  return GameValue(resmx);
}

static GameValue mxLoad(const GameState *gs, GameValuePar oper1)
{
  const GameArrayType &arr=oper1;
  GdMatrix *resmx=new GdMatrix;
  if (arr.Size()<4)
    {
      gs->SetError(EvalType);
      return GameValue(resmx);
    }
  for (int i=0;i<4;i++)
  {
    if (arr[i].GetType()!=GameArray)
    {
      gs->SetError(EvalType);
      return GameValue(resmx);
    }
    const GameArrayType &line=arr[i];
    if (line.Size()<3)
    {
      gs->SetError(EvalType);
      return GameValue(resmx);
    }
    for (int j=0;j<3;j++)
      (*resmx)(j,i)=line[j];
  }
 return GameValue(resmx);
}

static GameValue mxGet(const GameState *gs, GameValuePar oper1)
{
  GdMatrix &mx=GET_MATRIX(oper1);
  GameArrayType out;
  for (int i=0;i<4;i++)
  {
    GameArrayType line;
    line.Access(2);
    for (int j=0;j<3;j++)    
      line[j]=mx(j,i);
    out.Append()=GameValue(line);
  }
  return out;
}

static GameValue mxTop(const GameState *gs, GameValuePar oper1)
{
  const GameArrayType stack=oper1;
  if (stack.Size() && stack[stack.Size()-1].GetType()==EvalType_Matrix)
  {
    return stack[stack.Size()-1];
  }
  else
  {
  GdMatrix *resmx=new GdMatrix;
  resmx->SetScale(1,1,1);
  return GameValue(resmx);
  }
}

static GameValue mxPop(const GameState *gs, GameValuePar oper1)
{
  if (oper1.GetReadOnly())
  {
    gs->SetError(EvalForeignError,"Readonly stack");
  }
  GameArrayType &stack=const_cast<GameArrayType &>((const GameArrayType &)oper1);
  GameValue ret=mxTop(gs,oper1);
  if (stack.Size())  
    stack.Delete(stack.Size()-1,1);   
  return ret;
}

static GameValue mxInverse(const GameState *gs, GameValuePar oper1)
{
  GdMatrix &mx=GET_MATRIX(oper1);
  GdMatrix *retmx=new GdMatrix();
  retmx->SetInvertGeneral(mx);
  return GameValue(retmx);
}

static GameValue transformVect(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType &arr=oper2;
  GdMatrix &mx1=GET_MATRIX(oper1);
  if (arr.Size()==3 && arr[0].GetType()==GameScalar)
  {
    Vector3 vc(arr[0],arr[1],arr[2]);
    Vector3 res;res.SetFastTransform(mx1,vc);
    GameArrayType out;
    out.Append()=res[0];
    out.Append()=res[1];
    out.Append()=res[2];
    return out;
  }
  else
  {
    GameArrayType bigout;
    for (int i=0;i<arr.Size();i++) if (arr[i].GetType()==GameArray)
    {
      const GameArrayType &vecarr=arr[i];
      if (vecarr.Size()==3)
      {
        Vector3 vc(vecarr[0],vecarr[1],vecarr[2]);
        Vector3 res;res.SetFastTransform(mx1,vc);
        GameArrayType out;
        out.Append()=res[0];
        out.Append()=res[1];
        out.Append()=res[2];
        bigout.Append()=out;
      }
    }
    return bigout;
  }
}

static GameValue transformFast(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  GameArrayType &arr=const_cast<GameArrayType &>((const GameArrayType &)oper2);
  if (oper2.GetReadOnly()) 
  {
    gs->SetError(EvalBadVar);
    return GameValue();
  }
  GdMatrix &mx1=GET_MATRIX(oper1);
  if (arr.Size()==3 && arr[0].GetType()==GameScalar)
  {
    Vector3 vc(arr[0],arr[1],arr[2]);
    Vector3 res;res.SetFastTransform(mx1,vc);
    arr[0]=res[0];
    arr[1]=res[1];
    arr[2]=res[2];
    return oper2;
  }
  else
  {
    for (int i=0;i<arr.Size();i++) if (arr[i].GetType()==GameArray && !arr[i].GetReadOnly())
    {
      GameArrayType &vecarr=const_cast<GameArrayType &>((const GameArrayType &)arr[i]);
      if (vecarr.Size()==3)
      {
        Vector3 vc(vecarr[0],vecarr[1],vecarr[2]);
        Vector3 res;res.SetFastTransform(mx1,vc);
        vecarr[0]=res[0];
        vecarr[1]=res[1];
        vecarr[2]=res[2];
      }
    }
    return oper2;
  }
}


static GameValue vectorAdd(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType &o1=oper1;
  const GameArrayType &o2=oper2;
  GameArrayType res;
  for (int i=0,cnt=__max(o2.Size(),o1.Size());i<cnt;i++) 
    if (i>=o1.Size()) res.Append()=o2[i];
    else if (i>=o2.Size()) res.Append()=o1[i];
    else res.Append()=(float)o1[i]+(float)o2[i];
  return res;
}

static GameValue vectorDiff(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType &o1=oper1;
  const GameArrayType &o2=oper2;
  GameArrayType res;
  for (int i=0,cnt=__max(o2.Size(),o1.Size());i<cnt;i++) 
    if (i>=o1.Size()) res.Append()=-(float)o2[i];
    else if (i>=o2.Size()) res.Append()=o1[i];
    else res.Append()=(float)o1[i]-(float)o2[i];
  return res;
}

static GameValue vectorMult(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType &o1=oper1;
  float o2=oper2;
  GameArrayType res;
  for (int i=0,cnt=o1.Size();i<cnt;i++) 
    res.Append()=(float)o1[i]*o2;
  return res;
}

static GameValue vectorMult2(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  return vectorMult(gs,oper2,oper1);
}

static GameValue vectorCross(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType &o1=oper1;
  const GameArrayType &o2=oper2;
  if (o2.Size()<3 || o1.Size()<3) 
  {
    gs->SetError(EvalType);
    return GameValue();
  }
  Vector3 v1(o1[0],o1[1],o1[2]);
  Vector3 v2(o2[0],o2[1],o2[2]);
  Vector3 res=v1.CrossProduct(v2);
  
  GameArrayType out;
  out.Append()=res[0];
  out.Append()=res[1];
  out.Append()=res[2];
  return out;
}

static GameValue vectorDot(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType &o1=oper1;
  const GameArrayType &o2=oper2;
  float res=0;
  for (int i=0,cnt=__max(o2.Size(),o1.Size());i<cnt;i++) 
    if (i<o1.Size() && i<o2.Size()) res+=(float)o1[i]*(float)o2[i];
  return res;
}

static GameValue vectorCosAngle(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType &o1=oper1;
  const GameArrayType &o2=oper2;
  float sqr1=0;
  float sqr2=0;
  for (int i=0,cnt=o1.Size();i<cnt;i++) sqr1+=(float)o1[i]*(float)o1[i];
  for (int i=0,cnt=o2.Size();i<cnt;i++) sqr2+=(float)o2[i]*(float)o2[i];
  float dot=0;
  for (int i=0,cnt=__max(o2.Size(),o1.Size());i<cnt;i++) 
    if (i<o1.Size() && i<o2.Size()) dot+=(float)o1[i]*(float)o2[i];
  return (float)(dot/(sqrt(sqr1*sqr2)));
}

static GameValue vectorAngle(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  return acos((float)vectorCosAngle(gs,oper1,oper2))*180.0f/3.1415926535f;
}

static GameValue distanceFrom(const GameState *gs, GameValuePar oper1, GameValuePar oper2)
{
  const GameArrayType &o1=oper1;
  const GameArrayType &o2=oper2;
  float res=0;
  for (int i=0,cnt=__max(o2.Size(),o1.Size());i<cnt;i++) 
    if (i>=o1.Size()) res+=(float)o2[i]*(float)o2[i];
    else if (i>=o2.Size()) res+=(float)o1[i]*(float)o1[i];
    else res+=((float)o1[i]-(float)o2[i])*((float)o1[i]-(float)o2[i]);
  return (float)sqrt(res);
}

static GameValue vectorSize(const GameState *gs, GameValuePar oper1)
{
  const GameArrayType &o1=oper1;
  float res=0;
  for (int i=0,cnt=o1.Size();i<cnt;i++) res+=(float)o1[i]*(float)o1[i];
  return (float)sqrt(res);
}

#include "../determinant.h"
static GameValue determinant(const GameState *gs, GameValuePar oper1)
{
  using namespace ObjektivLib;
  const GameArrayType &o1=oper1;
  int cnt=o1.Size();  
  if (cnt>256) 
  {
    gs->SetError(EvalDim);
    return GameValue();
  }
#ifdef _DEBUG
     cacheHit=cacheMiss=0;;
#endif

  float *mx=new float[cnt*cnt];

  for (int i=0;i<cnt;i++)
  {
    const GameArrayType &line=o1[i];
    for (int j=0;j<cnt;j++) if (line.Size()>j) mx[i*cnt+j]=line[j];else mx[i*cnt+j]=0;
  }

  unsigned int save=_control87( 0, 0 );
  _control87(_PC_64,_MCW_PC);

  float res=(float)ObjektivLib::Determinant(mx,cnt);

  _control87(save,_MCW_PC);

  delete mx;

#ifdef _DEBUG
  LogF("Determinant Miss: %I64d, Hit: %I64d, Ratio: %I64d%%",cacheMiss,cacheHit,(cacheHit/((cacheMiss+cacheHit)/100)));
#endif

  return res;

}


static GameValue solveEquation(const GameState *gs, GameValuePar oper1,GameValuePar oper2)
{
 using namespace ObjektivLib;
  const GameArrayType &o1=oper1;
  const GameArrayType &o2=oper2;
  int cnt=o1.Size();  
  if (cnt>256 || o2.Size()<cnt) 
  {
    gs->SetError(EvalDim);
    return GameValue();
  }
#ifdef _DEBUG
          cacheHit=cacheMiss=0;;
#endif

  GameArrayType resArr;

  float *mx=new float[cnt*(cnt+1)];

  for (int i=0;i<cnt;i++)
  {
    const GameArrayType &line=o1[i];
    for (int j=0;j<cnt;j++) if (line.Size()>j) mx[i*cnt+j]=line[j];else mx[i*cnt+j]=0;
  }

  unsigned int save=_control87( 0, 0 );
  _control87(_PC_64,_MCW_PC);

  double det=Determinant(mx,cnt);
  if (fabs(det)==0) return resArr;
  
  for (int i=0;i<cnt;i++)
  {
    for (int j=0;j<cnt;j++)
    {
      mx[cnt*cnt+j]=mx[j*cnt+i];
      mx[j*cnt+i]=(float)(o2[j]);
    }
    double res=Determinant(mx,cnt);
    resArr.Append()=(float)(res/det);      
    for (int j=0;j<cnt;j++)
    {
      mx[j*cnt+i]=mx[cnt*cnt+j];            
    }
  }

  _control87(save,_MCW_PC);

  delete mx;

#ifdef _DEBUG
  LogF("SolveEq Miss: %I64d, Hit: %I64d, Ratio: %I64d%%",cacheMiss,cacheHit,(cacheHit/((cacheMiss+cacheHit)/100)));
#endif

  return resArr;

}

static GameValue LoadMatrixSpecial(const GameState *state, GameValuePar oper1, void (GdMatrix::*funct)(Vector3Par,Vector3Par))
{
  const GameArrayType &arr=oper1;
  if (arr.Size()==2)
  {
    if (arr[0].GetType()==GameArray)
    {
      if (arr[1].GetType()==GameArray)
      {
        const GameArrayType &vx1=arr[0];
        const GameArrayType &vx2=arr[1];
        if (vx1.Size()==3)
        {
          if (vx2.Size()==3)
          {
            GdMatrix *resmx=new GdMatrix;
            (resmx->*funct)(Vector3(vx1[0],vx1[1],vx1[2]),Vector3(vx2[0],vx2[1],vx2[2]));
            resmx->SetPosition(Vector3(0,0,0));
            return GameValue(resmx);
          }
          else
            state->SetError(EvalForeignError,"Excepted 3-dim vector as parameter 2");
        }
        else
          state->SetError(EvalForeignError,"Excepted 3-dim vector as parameter 1");
      }
      else
        state->TypeError(GameArray,arr[1].GetType(),"Parameter 2");
    }
    else
      state->TypeError(GameArray,arr[0].GetType(),"Parameter 1");
  }
  else
    state->SetError(EvalForeignError,"Excepted 2 parameters (vectors)");
  return GameValue();
}

static GameValue mxLoadDirAside(const GameState *state, GameValuePar oper1)
{
  return LoadMatrixSpecial(state,oper1,&GdMatrix::SetDirectionAndAside);
}

static GameValue mxLoadDirUp(const GameState *state, GameValuePar oper1)
{
  return LoadMatrixSpecial(state,oper1,&GdMatrix::SetDirectionAndUp);
}

static GameValue mxLoadUpAside(const GameState *state, GameValuePar oper1)
{
  return LoadMatrixSpecial(state,oper1,&GdMatrix::SetUpAndAside);
}

static GameValue mxLoadUpDir(const GameState *state, GameValuePar oper1)
{
  return LoadMatrixSpecial(state,oper1,&GdMatrix::SetUpAndDirection);
}

#define OPERATORS_DEFAULT(XX, Category) \
  XX(EvalType_Matrix, "*",function, mxMultiply,EvalType_Matrix, EvalType_Matrix, "a","b", "Multiplies two matrices. It means, that two transformations is combined into one matrix in order. Note: a*b is not same as b*a, both creates different transformations", "_t=_a*_b;", "", "", "", Category) \
  XX(GameArray, "mxPush",function, mxPush,GameArray, EvalType_Matrix, "a","b", "Pushes matrix into stack. Stack is represented as array. New matrix is inserted at the end of array", "_stack _mxPush (_mxTop _stack * _mxScale 5);", "", "", "", Category) \
  XX(EvalType_Matrix, "mxPivotScale",function, mxPivotScale,GameArray, GameArray, "pivot","vector", "Creates matrix that represents scaling object relative to pin", "_mx=[1,2,3] mxPivotScale [2,2,2]", "... resizition twice relative to [1,2,3]", "", "", Category) \
  XX(EvalType_Matrix, "mxPivotScale",function, mxPivotScale,GameArray, GameScalar, "pivot","factor", "Creates matrix that represents scaling object relative to pin", "_mx=[1,2,3] mxPivotScale [2,2,2]", "... resizition twice relative to [1,2,3]", "", "", Category) \
  XX(EvalType_Matrix, "mxPivotRotate",function, mxPivotRotate,GameArray, GameArray, "pivot","array", "Creates matrix that represents rotation about axis relative to pin. Two forms are possible: [[axis_x,axis_y,axis_z],angle] or [axis_x,axis_y,axis_z,angle]", "_mx=[1,2,3] mxPivotRotate [0,1,0,90]", "... rotation relative to [1,2,3]", "", "", Category) \
  XX(EvalType_Matrix, "mxInterpolate",function, mxInterpolate,GameArray, GameScalar, "stack","factor", "Creates interpolation between two matrices (most recent in stack). factor is value in range &lt;0,1&gt; or &lt;-1,0&gt;. If factor is negative, absolute value is get, and interpolation will normalize rotation vectors in matrix", "_stack mxInterpolate -0.5", "", "", "", Category) \
  XX(GameArray, "mxCalculateHierarchy",function, mxCalculateHierarchy,GameArray, GameArray, "parents","matrices", "Calculates hierarchy. parents is array of scalar values contains indicies to parent matrices. If -1 is used, matrix has no parent. It recomended use it on root parent, it enables you transform whole hierarchy by modifiing one matrix", "_matrices = [-1,0,0,2,3] mxInterpolate _relmx;//hierarchy (0,(1,2,(3,(4)))", "array _matrices contains transforms in world space. ", "", "", Category) \
  XX(GameArray, "transform",function,transformVect,EvalType_Matrix,GameArray,"matrix","vec/arr","Transforms vector (s) specified in array. Array can contain 3 scalars, or any number of vectors of 3 floats","","Array of transformed vectors","","",Category)\
  XX(GameArray, "transformFast",function,transformFast,EvalType_Matrix,GameArray,"matrix","vec/arr","Transforms vector (s) specified in array. Array can contain 3 scalars, or any number of vectors of 3 floats. Function rewrites values in array. This is little faster, because reduces overhead building the resulting array. Note: Array must be R/W. Using R/O causes an error","","second operand","","",Category)\
  XX(GameArray, "++",function,vectorAdd,GameArray,GameArray,"v1","v2","Makes v1 + v2 where v1 and v2 are vectors","","","","",Category)\
  XX(GameArray, "--",function,vectorDiff,GameArray,GameArray,"v1","v2","Makes v1 - v2 where v1 and v2 are vectors","","","","",Category)\
  XX(GameArray, "*",function,vectorMult,GameArray,GameScalar,"v","val","Makes v1 * val where v1 is vector and val is scalar","","","","",Category)\
  XX(GameArray, "*",function,vectorMult2,GameScalar,GameArray,"val","v","Makes v1 * val where v1 is vector and val is scalar","","","","",Category)\
  XX(GameArray, "**",function,vectorCross,GameArray,GameArray,"v1","v2","Makes cross product of two vectors in order. Current implementation works with 3 dimension vectors only","","","","",Category)\
  XX(GameScalar, ".",function,vectorDot,GameArray,GameArray,"v1","v2","Makes dot product of two vectors","","","","",Category)\
  XX(GameScalar, "angle",function,vectorAngle,GameArray,GameArray,"v1","v2","Calculates angle between two vectors","","","","",Category)\
  XX(GameScalar, "cosAngle",function,vectorCosAngle,GameArray,GameArray,"v1","v2","Calculates cosinus angle between two vectors. Note: It is faster than angle itself","","","","",Category)\
  XX(GameScalar, "distanceFrom",function,distanceFrom,GameArray,GameArray,"v1","v2","Calculates distance between two points","","","","",Category)\
  XX(GameArray, "solveEquation",function,solveEquation,GameArray,GameArray,"matrix","right_side","Solves Equation using the Cramer Rule. Function returns array of results. If equation has no solution, returns empty array","","","","",Category)\


#define FUNCTIONS_DEFAULT(XX, Category) \
  XX(EvalType_Matrix, "mxMove", mxMove, GameArray, "vector", "Creates matrix represents trasnlation by offset [x,y,z]", "_mx=mxMove [0.5,-1.2,0.2]", "[0,0,0][0,0,0][0,0,0][0.5,-1.2,0.2]", "", "", Category) \
  XX(EvalType_Matrix, "mxPivot", mxPivot, GameArray, "pivot", "Creates matrix that enables you to use all other trasnforms about pivot", "see mxUnpivot example", "", "","",  Category) \
  XX(EvalType_Matrix, "mxUnpivot", mxMove, GameArray, "vector", "Creates matrix that closes all pivot transforms. mxPivot and mxUnpivot is useful if used in transform stream or transform stack", "_mx=mxPivot _pivot * mxScale _scale * mxRotate _rotate * mxUnpivot _pivot", "... create scaling and rotation about pivot", "", "", Category) \
  XX(EvalType_Matrix, "mxScale", mxScale, GameArray, "vector", "Creates matrix represents scale by vector [x,y,z]", "_mx=mxScale [2,-2,1]", "[2,0,0][0,-2,0][0,0,1][0,0,0]", "", "", Category) \
  XX(EvalType_Matrix, "mxScale", mxScale, GameScalar, "scale", "Creates matrix represents uniform scale", "_mx=mxScale 2", "[2,0,0][0,2,0][0,0,2][0,0,0]", "", "", Category) \
  XX(EvalType_Matrix, "mxLoad", mxLoad, GameArray, "matrix", "Loads matrix from array", "_mx=mxLoad [[2,0,0],[0,1,0],[0,0,2]]", "", "", "", Category) \
  XX(EvalType_Matrix, "mxLoadDirAside", mxLoadDirAside, GameArray, "[dir,aside]", "Creates matrix at zero position from direction and aside", "", "", "", "", Category) \
  XX(EvalType_Matrix, "mxLoadDirUp", mxLoadDirUp, GameArray, "[dir,up]", "Creates matrix at zero position from direction and up", "", "", "", "", Category) \
  XX(EvalType_Matrix, "mxLoadUpAside", mxLoadDirUp, GameArray, "[up,aside]", "Creates matrix at zero position from up and aside", "", "", "", "", Category) \
  XX(EvalType_Matrix, "mxLoadUpDir", mxLoadUpDir, GameArray, "[up,dir]", "Creates matrix at zero position from up and direction", "", "", "", "", Category) \
  XX(GameArray, "mxGet", mxGet, EvalType_Matrix, "matrix", "Creates array from matrix", "_array=mxGet _mx", "", "", "", Category) \
  XX(EvalType_Matrix, "mxRotate", mxRotate, GameArray, "array", "Creates rotation matrix. Rotation is represented as axis and angle. Two forms are possible: [[axis_x,axis_y,axis_z],angle] or [axis_x,axis_y,axis_z,angle]", "_mx=mxRotate [0,1,0,90]", "rotation about Y using angle 90*", "", "", Category) \
  XX(EvalType_Matrix, "mxTop", mxTop, GameArray, "stack", "Returns most recent matrix in stack. If stack is empty, do nothing and returns identity. You can use it to get identity matrix _ident=mxTop []", "_mx=mxTop _stack", "", "", "", Category) \
  XX(EvalType_Matrix, "mxPop", mxPop, GameArray, "stack", "Removes most recent matrix from stack. If stack is empty, do nothing and returns identity. You can use it to get identity matrix _ident=mxPop []", "_mx=mxPop _stack", "", "", "", Category) \
  XX(EvalType_Matrix, "mxInverse", mxInverse, EvalType_Matrix, "mx", "Calculates inversion for matrix", "_inv=mxInverse _mx", "", "", "", Category) \
  XX(GameScalar, "vectorSize", vectorSize, GameArray, "vector", "Calculates size of vector", "", "", "", "", Category) \
  XX(GameScalar, "determinant", determinant, GameArray, "matrix", "Calculates determinant of the matrix. Function is not optimized, but still faster than script", "", "", "", "", Category) \



static GameOperator BinaryFuncts[]=
{
  OPERATORS_DEFAULT(REGISTER_OPERATOR, Category)
};

static GameFunction UnaryFuncts[]=
{
  FUNCTIONS_DEFAULT(REGISTER_FUNCTION, Category)
};


static GameData *CreateMatrix(ParamArchive *ar) {return new GdMatrix;}

TYPES_MATRIX(DEFINE_TYPE, "Matrix")

#if DOCUMENT_COMREF
static ComRefFunc DefaultComRefFunc[] =
{
/*  NULARS_DEFAULT(COMREF_NULAR, Category)*/
  FUNCTIONS_DEFAULT(COMREF_FUNCTION, Category)
  OPERATORS_DEFAULT(COMREF_OPERATOR, Category)
};

static ComRefType DefaultComRefType[] =
{
  TYPES_DEFAULT(COMREF_TYPE, Category)
/*  TYPES_DEFAULT_COMB(COMREF_TYPE, "Default")*/
};
#endif


void GdMatrix::RegisterToGameState(GameState *gState)
{
  GameState &state = *gState; // helper to make macro works
  TYPES_MATRIX(REGISTER_TYPE, "Matrix")

  gState->NewOperators(BinaryFuncts,lenof(BinaryFuncts));
  gState->NewFunctions(UnaryFuncts,lenof(UnaryFuncts));
/*  gState->NewNularOps(NularyFuncts,lenof(NularyFuncts));*/
#if DOCUMENT_COMREF
  gState->AddComRefFunctions(DefaultComRefFunc,lenof(DefaultComRefFunc));
  gState->AddComRefTypes(DefaultComRefType,lenof(DefaultComRefType));
#endif
}

