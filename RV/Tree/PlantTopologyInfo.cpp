#include "RVTreeCommon.h"
#include ".\planttopologyinfo.h"
#include "component.h"
#include "IPlantDB.h"
#include "PointList.h"

///Adds a root node and returns its index
namespace TreeEngine
{

  unsigned int PlantTopologyInfo::AddRootNode()
  {
    unsigned int idx=_nodeList.Size();
    _nodeList.Add();
    return idx;
  }

  void PlantTopologyInfo::SetNodeInfo(unsigned int nodeIndex, const SComponentChild &node)
  {
    Assert(nodeIndex<(unsigned)_nodeList.Size()); //invalid nodeIndex
    NodeInfo &snode=_nodeList[nodeIndex];

    snode.age=node._actualParams._age;
    snode.componentIndex=node._componentIndex;
    snode.importancy=node._importancy;    
    snode.position=node._actualParams._origin.Position();
    snode.branchTypeId=node._actualParams._branchTypeId;
    snode.direction=node._actualParams._origin.Direction();
  }

  bool PlantTopologyInfo::AllocateNodeChildren(unsigned int nodeIndex, unsigned int childCount)
  {
    Assert(nodeIndex<(unsigned)_nodeList.Size()); //invalid nodeIndex
    NodeInfo &snode=_nodeList[nodeIndex];

    if (snode.numChilds!=0) return false;
    if (childCount==0) return true;
    snode.firstChild=_nodeList.Size();
    snode.numChilds=childCount;
    _nodeList.Resize(snode.firstChild+childCount);
    return true;
  }

  unsigned int PlantTopologyInfo::GetNodeChildId(unsigned int nodeIndex, unsigned int childIndex) const
  {
    Assert(nodeIndex<(unsigned)_nodeList.Size()); //invalid nodeIndex
    const NodeInfo &snode=_nodeList[nodeIndex];
    Assert(childIndex<snode.numChilds);
    return snode.firstChild+childIndex;
  }

  unsigned int PlantTopologyInfo::GetNodeChildrenCount(unsigned int nodeIndex) const
  {
    Assert(nodeIndex<(unsigned)_nodeList.Size()); //invalid nodeIndex
    const NodeInfo &snode=_nodeList[nodeIndex];
    return snode.numChilds;
  }


  inline static float pow2(float val)
  {
    return val*val;
  }
  float PlantTopologyInfo::GetDifferenceFactor(const PlantTopologyInfo &other,const TopologyWeights &weights,float ceilHint) const
  {
    if (other.GetCountOfNodes()==0 && GetCountOfNodes()==0) return 0;
    if (other.GetCountOfNodes()==0 || GetCountOfNodes()==0) return ceilHint;
    if (_nodeList[0].branchTypeId!=other._nodeList[0].branchTypeId) return 1e10f;
    float normSize1=GetSizeCoeficient();
    float normSize2=other.GetSizeCoeficient();
    if ((normSize1 < 0.00001f) != (normSize2 <  0.00001f))
        return 1e10f;
    float sumDiff=GetDifferenceFactor(other,weights,0,0,ceilHint,normSize1,normSize2);    
    return (sumDiff+(__max(normSize1,normSize2)/__min(normSize1,normSize2))*weights.sizecoef_diffFact*100)/100;   
  }


  float PlantTopologyInfo::GetDifferenceFactor(const PlantTopologyInfo &other,const TopologyWeights &weights, int thisNodeIndex, int otherNodeIndex, float ceilHint, float normSize1, float normSize2) const
  {
    const NodeInfo &thisNode=_nodeList[thisNodeIndex];
    const NodeInfo &otherNode=other._nodeList[otherNodeIndex];


    float ageDiff=pow2(thisNode.age-otherNode.age)*weights.age_diffFact;
    float impDiff=pow2(thisNode.importancy-otherNode.importancy)*weights.importancy_diffFact;
    float compDiff=thisNode.componentIndex==otherNode.componentIndex?0:weights.component_diffFact;

    float sumDiff=ageDiff+impDiff+compDiff;
    if (sumDiff>ceilHint) return sumDiff;


    if (normSize1 != 0 && normSize2 != 0)
        sumDiff+=((thisNode.position)/normSize1).Distance2(otherNode.position/normSize2)*weights.position_diffFact;
    sumDiff+=thisNode.direction.Distance2(otherNode.direction)*weights.direction_diffFact;
    if (sumDiff>ceilHint) return sumDiff;

    if (thisNode.numChilds || otherNode.numChilds)
    {
      if (thisNode.numChilds>otherNode.numChilds)
      {
        for (unsigned int i=0;i<thisNode.numChilds;i++)
        {
          float bestDiff=FLT_MAX;
          int bestChild=-1;
          for (unsigned int j=0;j<otherNode.numChilds;j++) 
          {
            float diff=GetDifferenceFactor(other,weights,thisNode.firstChild+i,otherNode.firstChild+j,ceilHint-sumDiff,normSize1,normSize2);
            if (diff<bestDiff) {bestDiff=diff;bestChild=j;}
          }
          if (bestChild==-1)
          {
            bestDiff=GetDifferenceFactor(other,weights,thisNode.firstChild+i,otherNodeIndex,ceilHint,normSize1,normSize2);
          }
          sumDiff+=bestDiff;
          if (sumDiff>ceilHint) return sumDiff;
        }
      }
      else
      {
        for (unsigned int i=0;i<otherNode.numChilds;i++)
        {
          float bestDiff=FLT_MAX;
          int bestChild=-1;
          for (unsigned int j=0;j<thisNode.numChilds;j++) 
          {
            float diff=GetDifferenceFactor(other,weights,thisNode.firstChild+j,otherNode.firstChild+i,ceilHint-sumDiff,normSize1,normSize2);
            if (diff<bestDiff) {bestDiff=diff;bestChild=j;}
          }
          if (bestChild==-1)
          {
            bestDiff=GetDifferenceFactor(other,weights,thisNodeIndex,otherNode.firstChild+i,ceilHint,normSize1,normSize2);
          }
          sumDiff+=bestDiff;
          if (sumDiff>ceilHint) return sumDiff;
        }
      }
    }

 /*   if (thisNode.numChilds)
    {   
      
        bool *removedChilds=(bool *)alloca(otherNode.numChilds);
        memset(removedChilds,false,otherNode.numChilds);
        for (unsigned int i=0;i<thisNode.numChilds;i++)
        {
          float bestDiff=FLT_MAX;
          int bestChild=-1;
          for (unsigned int j=0;j<otherNode.numChilds;j++) if (!removedChilds[j])
          {
            float diff=GetDifferenceFactor(other,thisNode.firstChild+i,otherNode.firstChild+j,ceilHint-sumDiff);
            if (diff<bestDiff) {bestDiff=diff;bestChild=j;}
          }
          if (bestChild==-1)
          {
            bestDiff=GetDifferenceFactor(other,thisNode.firstChild+i,otherNodeIndex,ceilHint)*thisNode.numChilds;
          }
          else
          {
            removedChilds[bestChild]=true;
          }
          sumDiff+=bestDiff;
          if (sumDiff>ceilHint) return sumDiff;
        }
    }
    else if (otherNode.numChilds)
    {
      for (unsigned int j=0;j<otherNode.numChilds;j++) 
      {
        sumDiff+=GetDifferenceFactor(other,thisNodeIndex,otherNode.firstChild+j,ceilHint-sumDiff)*otherNode.numChilds;
      }
    }*/
    return sumDiff;
  }

  void  PlantTopologyInfo::FinalizeTopology(const Matrix4 &origin)
  {
/*    Vector3 sumFocus(0, 0, 0);
    for (int i = 0; i < _nodeList.Size(); i++) 
    {
      _nodeList[i].position-=origin;
      sumFocus += _nodeList[i].position;
    }
    float rotated=atan2(sumFocus[2],sumFocus[0]);*/
    Matrix4 backRot(MInverseGeneral,origin);
/*    backRot.SetRotationY(-rotated);
    _rotation+=rotated;*/
    _boundingMin=Vector3(0,0,0);
    _boundingMax=Vector3(0,0,0);
    for (int i = 0; i < _nodeList.Size(); i++) 
    {
      Vector3 pos=_nodeList[i].position=backRot.FastTransform(_nodeList[i].position);
      if (pos[0]>_boundingMax[0]) _boundingMax[0]=pos[0];
      if (pos[1]>_boundingMax[1]) _boundingMax[1]=pos[1];
      if (pos[2]>_boundingMax[2]) _boundingMax[2]=pos[2];
      if (pos[0]<_boundingMin[0]) _boundingMin[0]=pos[0];
      if (pos[1]<_boundingMin[1]) _boundingMin[1]=pos[1];
      if (pos[2]<_boundingMin[2]) _boundingMin[2]=pos[2];
    }
    _sizeCoef=-1;
    _length=-1;
  }

  float PlantTopologyInfo::GetTotalVolume() const
  {
    return (_boundingMax[0]-_boundingMin[0])*(_boundingMax[1]-_boundingMin[1])*(_boundingMax[2]-_boundingMin[2]);
  }

  float PlantTopologyInfo::GetBoundingSize() const
  {
    return (_boundingMax[0]-_boundingMin[0])+(_boundingMax[1]-_boundingMin[1])+(_boundingMax[2]-_boundingMin[2])+0.001f;
  }

  float PlantTopologyInfo::GetSizeCoeficient() const
  {
//    return sqrt(GetTotalVolume());
    if (_sizeCoef>0) return _sizeCoef;
    float sz=0;
    for (int i=0;i<_nodeList.Size();i++)
    {
      float p=_nodeList[i].position.Size();
      if (p>sz) sz=p;
    }
    _sizeCoef=sz;
    return sz;
  }

  float PlantTopologyInfo::GetSubBranchLength(const Vector3 &startPos, int branch) const
  {    
    const NodeInfo &thisNode=_nodeList[branch];

    float d=0;
    for (unsigned int i=0;i<thisNode.numChilds;i++)
    {
      d+=GetSubBranchLength(thisNode.position,thisNode.firstChild+i);
    }
    d=thisNode.numChilds?thisNode.numChilds:0+thisNode.position.Distance(startPos);
    return d;

  }

  float PlantTopologyInfo::GetTopologyLength() const
  {
    if (_length<0) _length=GetSubBranchLength()/_nodeList.Size();
    return _length;
  }

  static float readFloatFromDb(IPlantControlDB &db,const char *name, float defaultVal) {

      const char *str = db.GetGlobalDBItemString(name);
      if (str == 0) return defaultVal;
      float res;
      if (sscanf(str,"%f",&res) != 1) 
          res = defaultVal;
      return res;

  }

  void TopologyWeights::LoadWeightsFromDB(IPlantControlDB &db) {
    
      age_diffFact = readFloatFromDb(db,"ageDiff",0.5f);
      importancy_diffFact = readFloatFromDb(db,"importanceDiff",0.2f);
      component_diffFact = readFloatFromDb(db,"componentDiff",0.001f);
      child_diffFact = readFloatFromDb(db,"childDiff",0.2f);
      position_diffFact = readFloatFromDb(db,"positionDiff",1);
      direction_diffFact = readFloatFromDb(db,"directionDiff",1);
      sizecoef_diffFact = readFloatFromDb(db,"sizecoefDiff",1);
  }

  void PlantTopologyInfo::dumpTopology(const char *filename, int nodeIndex, bool append) {
      FILE *f = fopen(filename,append?"at":"wt");
      fprintf(f,"Node: %d\n", nodeIndex);
      fprintf(f,"\t:Size coef %f\n", GetSizeCoeficient());
     for (int i=0;i<_nodeList.Size();i++)
    {
      fprintf(f,"\t\t:Point %f,%f,%f\n", _nodeList[i].position[0],_nodeList[i].position[1],_nodeList[i].position[2]);
    }
     fclose(f);
  }

  float PlantTopologyInfo::getSizeCoef(const CPointList &pointList, const Vector3 &origin) {
     float sz=0;
     for (int i = 0; i < pointList.NPoints();i++) {
        float p = pointList.GetPoint(i).Distance(origin);
        if (p > sz) sz = p;
     }
     return sz;

  }

}