#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "primitiveTreeNode.h"


namespace TreeEngine
{
    void CPTreeNode::SetTexture(RString name, int index) {
    _textureIndex = index;
  }

  void CPTreeNode::SetTreeNodeParams(float baseWidth,
                                    float textureVOffset,
                                    float geometrySpin,
                                    int childNum) {
    assert(childNum <= TREENODE_MAXCHILDCOUNT);
    _baseWidth = baseWidth;
    _textureVOffset = textureVOffset;
    _geometrySpin = geometrySpin;
    _childNum = childNum;
  }

  void CPTreeNode::SetTreeNodeChildParams(int childIndex,
                                          float width,
                                          float height,
                                          float twist,
                                          float spin,
                                          float diversion,
                                          float textureUTiling,
                                          float textureVTiling,
                                          int segmentCount,
                                          bool forceSegmentCount) {
    assert(childIndex >= 0);
    assert(childIndex < TREENODE_MAXCHILDCOUNT);
    SPTreeNodeChild *child = &_child[childIndex];
    child->_width = width;
    child->_height = height;
    child->_twist = twist;
    child->_spin = spin;
    child->_diversion = diversion;
    child->_textureUTiling = textureUTiling;
    child->_textureVTiling = textureVTiling;
    child->_segmentCount = segmentCount;
    child->_forceSegmentCount = forceSegmentCount;
  }

  SPrimitiveChild CPTreeNode::GetChild(int childIndex) {
    // Get the specified child
    assert(childIndex >= 0);
    assert(childIndex < TREENODE_MAXCHILDCOUNT);
    SPTreeNodeChild *child = &_child[childIndex];

    // Compute diversion per segment
    float diversionPerSegment = child->_diversion / ((float)child->_segmentCount);

    // Diversion matrix
    Matrix4 diversionMatrix;
    diversionMatrix.SetRotationX(diversionPerSegment);

    // Twist matrix
    Matrix4 twistMatrix;
    twistMatrix.SetRotationZ(child->_twist);

    // ChildOrigin matrix
    Matrix4 childOrigin;
    childOrigin = _origin * twistMatrix;

    // Distance between segments
    float segmentShift = child->_height / ((float)child->_segmentCount);

    // Accumulate segments
    for (int i = 0; i < child->_segmentCount; i++) {

      // Rotate child origin a bit
      childOrigin = childOrigin * diversionMatrix;

      // Shift it forward a bit
      childOrigin.SetPosition(childOrigin.Position() + childOrigin.Direction() * segmentShift);
    }

    // Add spin
    Matrix4 childSpin;
    childSpin.SetRotationZ(child->_spin);

    // Fill out the structure
    SPrimitiveChild PC;
    PC._origin = childOrigin * childSpin;
    PC._slotIndex = childIndex;
    PC._textureVOffset = _textureVOffset + child->_textureVTiling;
    PC._geometrySpin = _geometrySpin - child->_twist;
    return PC;
  }

  SPrimitiveChild CPTreeNode::GetSideChild(int childIndex,
                                          float directionPos,
                                          float sideShift,
                                          float slopeAngle,
                                          float twistAngle,
                                          float spinAngle)
  {

    // Get the specified child
    assert(childIndex >= 0);
    assert(childIndex < TREENODE_MAXCHILDCOUNT);
    SPTreeNodeChild *child = &_child[childIndex];

    // Compute diversion per segment
    float diversionPerSegment = child->_diversion / ((float)child->_segmentCount);

    // Diversion matrix
    Matrix4 diversionMatrix;
    diversionMatrix.SetRotationX(diversionPerSegment);

    // Twist matrix
    Matrix4 twistMatrix;
    twistMatrix.SetRotationZ(child->_twist);

    // ChildOrigin matrix
    Matrix4 childOrigin;
    childOrigin = _origin * twistMatrix;

    // Distance between segments
    float segmentShift = child->_height / ((float)child->_segmentCount);

    // Accumulate segment
    float lengthTreshold = child->_height * directionPos;
    float sumSegmentShift = 0.0f;
    for (; sumSegmentShift + segmentShift < lengthTreshold; sumSegmentShift += segmentShift) {

      // Rotate child origin a bit
      childOrigin = childOrigin * diversionMatrix;

      // Shift it forward a bit
      childOrigin.SetPosition(childOrigin.Position() + childOrigin.Direction() * segmentShift);
    }
    childOrigin = childOrigin * diversionMatrix;

    Matrix3 RotSpin;
    RotSpin.SetRotationZ(spinAngle);
    Matrix3 RotAside;
    RotAside.SetRotationY(slopeAngle);
    Matrix3 RotDirection;
    RotDirection.SetRotationZ(twistAngle);
    Matrix3 SideChildOriginOrientation = childOrigin.Orientation() * RotDirection * RotAside * RotSpin;

    // Fill out the structure
    SPrimitiveChild pc;
    pc._origin.SetPosition(
      childOrigin.Position() + childOrigin.Direction() * (lengthTreshold - sumSegmentShift)
      + childOrigin.DirectionAside() * sideShift * cos(twistAngle)
      + childOrigin.DirectionUp() * sideShift * sin(twistAngle));
    pc._origin.SetOrientation(SideChildOriginOrientation);
    pc._slotIndex = -1;
    pc._textureVOffset = 0.0f;
    pc._geometrySpin = 0.0f;
    return pc;
  }

  SPrimitiveChild CPTreeNode::GetSideChildFA(int childIndex,
                                            float directionPos,
                                            float forwardShift,
                                            const AxisRotation *ar,
                                            int arCount)
  {

    // Get the specified child
    assert(childIndex >= 0);
    assert(childIndex < TREENODE_MAXCHILDCOUNT);
    SPTreeNodeChild *child = &_child[childIndex];

    // Compute diversion per segment
    float diversionPerSegment = child->_diversion / ((float)child->_segmentCount);

    // Diversion matrix
    Matrix4 diversionMatrix;
    diversionMatrix.SetRotationX(diversionPerSegment);

    // Twist matrix
    Matrix4 twistMatrix;
    twistMatrix.SetRotationZ(child->_twist);

    // ChildOrigin matrix
    Matrix4 childOrigin;
    childOrigin = _origin * twistMatrix;

    // Distance between segments
    float segmentShift = child->_height / ((float)child->_segmentCount);

    // Accumulate segment
    float lengthTreshold = child->_height * directionPos;
    float sumSegmentShift = 0.0f;
    for (; sumSegmentShift + segmentShift < lengthTreshold; sumSegmentShift += segmentShift) {

      // Rotate child origin a bit
      childOrigin = childOrigin * diversionMatrix;

      // Shift it forward a bit
      childOrigin.SetPosition(childOrigin.Position() + childOrigin.Direction() * segmentShift);
    }
    childOrigin = childOrigin * diversionMatrix;

    // Add all axis rotations
    Matrix3 sideChildOriginOrientation = GetSideChildOriginOrientation(childOrigin.Orientation(),ar,arCount);

    // Fill out the structure
    SPrimitiveChild pc;
    pc._origin.SetPosition(
      childOrigin.Position() + childOrigin.Direction() * (lengthTreshold - sumSegmentShift)
      + sideChildOriginOrientation.Direction() * forwardShift);
    pc._origin.SetOrientation(sideChildOriginOrientation);
    pc._slotIndex = -1;
    pc._textureVOffset = 0.0f;
    pc._geometrySpin = 0.0f;
    return pc;
  }

  int CPTreeNode::Draw(CPrimitiveStream &ps,
                      int sharedVertexIndex,
                      int sharedVertexCount,
                      bool isRoot,
                      float detail,
                      SSlot *pSlot) {

    // Counter of triangles
    int tAccu = 0;

    // Cycle through all the children
    for (int i = 0; i < _childNum; i++) {

      // Get the specified child
      SPTreeNodeChild *child = &_child[i];

      // Init variables related to segment skipping and counting
      int stepNumber = child->_forceSegmentCount?
          child->_segmentCount:
          min(child->_segmentCount, max(1, toIntCeil(((float)child->_segmentCount) * detail)));

      float skipSegments = ((float)child->_segmentCount) / stepNumber;

      // Compute diversion per segment
      float diversionPerSegment = child->_diversion / ((float)child->_segmentCount);

      // Diversion matrix
      Matrix4 diversionMatrix;
      diversionMatrix.SetRotationX(diversionPerSegment);

      // Twist matrix
      Matrix4 twistMatrix;
      twistMatrix.SetRotationZ(child->_twist);

      // ChildOrigin matrix
      Matrix4 childOrigin;
      childOrigin = _origin * twistMatrix;

      // Distance between segments
      float segmentShift = child->_height / ((float)child->_segmentCount);

      // Offset
      float textureVOffset = _textureVOffset;
      float deltaTextureVTiling = child->_textureVTiling / ((float)child->_segmentCount);

      // Width
      float width = _baseWidth;
      float deltaWidth = (child->_width - _baseWidth) / ((float)child->_segmentCount);

      // Accumulate attributes of skipped segments
      float newWidth = width;
      Matrix4 newChildOrigin = childOrigin;
      for (int i = 0; i < toInt(skipSegments); i++) {
        newChildOrigin = newChildOrigin * diversionMatrix;
        newChildOrigin.SetPosition(newChildOrigin.Position() + newChildOrigin.Direction() * segmentShift);
        newWidth += deltaWidth;
        textureVOffset += deltaTextureVTiling;
      }
      float sumSkipSegments = skipSegments;

      // Geometry spin
      float gSpin = _geometrySpin - child->_twist;

      // New spinned origin
      Matrix4 childSpin;
      childSpin.SetRotationZ(child->_spin * sumSkipSegments / child->_segmentCount + gSpin);
      Matrix4 newSpinnedChildOrigin = newChildOrigin * childSpin;

      // Draw first cone
      int newSharedSidesCount = CONE_SIDES_NUM(detail, newWidth);
      int newSharedVertexIndex;
      if (sharedVertexCount > 0) {
        newSharedVertexIndex = DrawConeShared(
          ps,
          _textureIndex,
          child->_textureUTiling,
          D3DCOLOR_XRGB(255, 255, 255),
          sharedVertexIndex,
          sharedVertexCount - 1,
          isRoot,
          newSpinnedChildOrigin,
          newWidth,
          textureVOffset,
          newSharedSidesCount,
          &tAccu);
      }
      else {
        Matrix4 tmpSpin;
        tmpSpin.SetRotationZ(gSpin);
        newSharedVertexIndex = DrawCone(
          ps,
          _textureIndex,
          child->_textureUTiling,
          D3DCOLOR_XRGB(255, 255, 255),
          childOrigin * tmpSpin,
          width,
          _textureVOffset,
          CONE_SIDES_NUM(detail, width),
          newSpinnedChildOrigin,
          newWidth,
          textureVOffset,
          newSharedSidesCount,
          &tAccu);
      }

      // Accumulate segments
      for (int i = 1; i < stepNumber; i++) {

        // Accumulate attributes of skipped segments
        for (int j = toInt(sumSkipSegments); j < toInt(sumSkipSegments + skipSegments); j++) {
          newChildOrigin = newChildOrigin * diversionMatrix;
          newChildOrigin.SetPosition(newChildOrigin.Position() + newChildOrigin.Direction() * segmentShift);
          newWidth += deltaWidth;
          textureVOffset += deltaTextureVTiling;
        }
        sumSkipSegments += skipSegments;

        // New spinned origin
        childSpin.SetRotationZ(child->_spin * sumSkipSegments / child->_segmentCount + gSpin);
        newSpinnedChildOrigin = newChildOrigin * childSpin;

        // Draw the cone
        int oldSharedSidesCount = newSharedSidesCount;
        newSharedSidesCount = CONE_SIDES_NUM(detail, newWidth);
        newSharedVertexIndex = DrawConeShared(
          ps,
          _textureIndex,
          child->_textureUTiling,
          D3DCOLOR_XRGB(255, 255, 255),
          newSharedVertexIndex,
          oldSharedSidesCount,
          isRoot,
          newSpinnedChildOrigin,
          newWidth,
          textureVOffset,
          newSharedSidesCount,
          &tAccu);
      }

      // Fill out the slot
      pSlot[i]._sharedVertexIndex = newSharedVertexIndex;
      pSlot[i]._sharedVertexCount = newSharedSidesCount + 1;
    }
    return tAccu;
  }

  void CPTreeNode::UpdatePointList(CPointList &pointList) {
    pointList.AddPoint(_origin.Position() + _origin.DirectionAside() * _baseWidth * SquareCoef);
    pointList.AddPoint(_origin.Position() - _origin.DirectionAside() * _baseWidth * SquareCoef);
    pointList.AddPoint(_origin.Position() + _origin.DirectionUp() * _baseWidth * SquareCoef);
    pointList.AddPoint(_origin.Position() - _origin.DirectionUp() * _baseWidth * SquareCoef);

    // Cycle through all the children
    for (int i = 0; i < _childNum; i++) {
      SPrimitiveChild pc;
      pc = GetChild(i);
      float childWidth = _child[i]._width;
      pointList.AddPoint(pc._origin.Position() + pc._origin.DirectionAside() * childWidth * SquareCoef);
      pointList.AddPoint(pc._origin.Position() - pc._origin.DirectionAside() * childWidth * SquareCoef);
      pointList.AddPoint(pc._origin.Position() + pc._origin.DirectionUp() * childWidth * SquareCoef);
      pointList.AddPoint(pc._origin.Position() - pc._origin.DirectionUp() * childWidth * SquareCoef);
    }
  }

  int CPTreeNode::DrawBody(ObjectData *o, int &bodyId)
  {
    int trianglesCount = 0;
    for (int i = 0; i < _childNum; i++)
    {
      SPrimitiveChild pc;
      pc = GetChild(i);
      Vector3 childVector = pc._origin.Position() - _origin.Position();
      float childHeight = childVector.Size();
      Matrix4 blockOrigin; 
      blockOrigin.SetPosition(_origin.Position());
      blockOrigin.SetDirectionAndUp(childVector, _origin.DirectionUp());
      trianglesCount += CPrimitive::DrawBlock(o, bodyId, blockOrigin, _child[i]._width, childHeight);
    }
    return trianglesCount;
  };

  // ------------------------------------------
  // Following code is here for script purposes

  GameValue PrimitiveTreeNode_SetTexture(const GameState *state,
                                        GameValuePar oper1,
                                        GameValuePar oper2) {

    CPTreeNode *primitiveTreeNode = dynamic_cast<CPTreeNode*>(GetPrimitive(oper1));

    if (!primitiveTreeNode) {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size() != 2) {
      if (state) state->SetError(EvalDim, array.Size(), 2);
      return GameValue();
    }

    if (array[0].GetType() != GameString) {
      if (state) state->TypeError(GameString, array[0].GetType());
      return GameValue();
    }

    if (array[1].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return GameValue();
    }

    primitiveTreeNode->SetTexture(array[0], toInt(array[1]));

    return GameValue();
  }

  GameValue PrimitiveTreeNode_SetTreeNodeParams(const GameState *state,
                                                GameValuePar oper1,
                                                GameValuePar oper2) {

    CPTreeNode *primitiveTreeNode = dynamic_cast<CPTreeNode*>(GetPrimitive(oper1));

    if (!primitiveTreeNode) {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size() != 4) {
      if (state) state->SetError(EvalDim, array.Size(), 4);
      return GameValue();
    }

    if (array[0].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[0].GetType());
      return GameValue();
    }

    if (array[1].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return GameValue();
    }

    if (array[2].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[2].GetType());
      return GameValue();
    }

    if (array[3].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[3].GetType());
      return GameValue();
    }

    primitiveTreeNode->SetTreeNodeParams(
      array[0],
      array[1],
      array[2],
      toInt(array[3]));

    return GameValue();
  }

  GameValue PrimitiveTreeNode_SetTreeNodeChildParams(const GameState *state,
                                                    GameValuePar oper1,
                                                    GameValuePar oper2) {

    CPTreeNode *primitiveTreeNode = dynamic_cast<CPTreeNode*>(GetPrimitive(oper1));

    if (!primitiveTreeNode) {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size() != 9 && array.Size() != 10) {
      if (state) state->SetError(EvalDim, array.Size(), 9);
      return GameValue();
    }

    if (array[0].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[0].GetType());
      return GameValue();
    }

    if (array[1].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return GameValue();
    }

    if (array[2].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[2].GetType());
      return GameValue();
    }

    if (array[3].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[3].GetType());
      return GameValue();
    }

    if (array[4].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[4].GetType());
      return GameValue();
    }

    if (array[5].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[5].GetType());
      return GameValue();
    }

    if (array[6].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[6].GetType());
      return GameValue();
    }

    if (array[7].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[7].GetType());
      return GameValue();
    }

    if (array[8].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[8].GetType());
      return GameValue();
    }

    if (toInt(array[8]) < 1) {
        LogF("Segment count cannot be negative (%d)", toInt(array[8]));
        if (state) state->SetError(EvalNum);
        return GameValue();
    }

    bool forceSegmentCount = false;
    if (array.Size() >9) 
        if (array[9].GetType() == GameBool) 
            forceSegmentCount = array[9];
        else {
          if (state) state->TypeError(GameScalar, array[9].GetType());
          return GameValue();
        }


    primitiveTreeNode->SetTreeNodeChildParams(
      toInt(array[0]),
      array[1],
      array[2],
      array[3],
      array[4],
      array[5],
      array[6],
      array[7],
      toInt(array[8]),
      forceSegmentCount);

    return GameValue();
  }

  GameValue PrimitiveTreeNode_GetChild(const GameState *state,
                                      GameValuePar oper1,
                                      GameValuePar oper2) {

    CPTreeNode *primitiveTreeNode = dynamic_cast<CPTreeNode*>(GetPrimitive(oper1));

    if (!primitiveTreeNode) {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    GameScalarType childIndex = oper2;
    SPrimitiveChild child = primitiveTreeNode->GetChild(toInt(childIndex));
    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  GameValue PrimitiveTreeNode_GetSideChild(const GameState *state,
                                          GameValuePar oper1,
                                          GameValuePar oper2)
  {

    CPTreeNode *primitiveTreeNode = dynamic_cast<CPTreeNode*>(GetPrimitive(oper1));

    if (!primitiveTreeNode) {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size() != 6) {
      if (state) state->SetError(EvalDim, array.Size(), 6);
      return GameValue();
    }

    if (array[0].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[0].GetType());
      return GameValue();
    }

    if (array[1].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return GameValue();
    }

    if (array[2].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[2].GetType());
      return GameValue();
    }

    if (array[3].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[3].GetType());
      return GameValue();
    }

    if (array[4].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[4].GetType());
      return GameValue();
    }

    if (array[5].GetType() != GameScalar) {
      if (state) state->TypeError(GameScalar, array[5].GetType());
      return GameValue();
    }

    SPrimitiveChild child = primitiveTreeNode->GetSideChild(toInt(array[0]), array[1], array[2], array[3], array[4], array[5]);

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  GameValue PrimitiveTreeNode_GetSideChildFA(const GameState *state,
                                            GameValuePar oper1,
                                            GameValuePar oper2)
  {

    CPTreeNode *primitiveTreeNode = dynamic_cast<CPTreeNode*>(GetPrimitive(oper1));

    if (!primitiveTreeNode)
    {
      if (state) state->SetError(EvalGen);
      return GameValue();
    }

    const GameArrayType &array = oper2;
    if (array.Size() != 4)
    {
      if (state) state->SetError(EvalDim, array.Size(), 4);
      return GameValue();
    }

    if (array[0].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[0].GetType());
      return GameValue();
    }

    if (array[1].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[1].GetType());
      return GameValue();
    }

    if (array[2].GetType() != GameScalar)
    {
      if (state) state->TypeError(GameScalar, array[2].GetType());
      return GameValue();
    }

    // Array of transformations
    if (array[3].GetType() != GameArray)
    {
      if (state) state->TypeError(GameArray, array[3].GetType());
      return GameValue();
    }

    const GameArrayType &tArray = array[3];
    int tArraySize = tArray.Size();

    AxisRotation axisRotation[AXIS_ROTATION_COUNT_MAX];
    DoAssert(tArraySize <= AXIS_ROTATION_COUNT_MAX);

    for (int i = 0; i < tArraySize; i++)
    {
      if (tArray[i].GetType() != GameArray)
      {
        if (state) state->TypeError(GameArray, tArray[i].GetType());
        return GameValue();
      }

      const GameArrayType &pair = tArray[i];
      if (pair.Size() != 2)
      {
        if (state) state->SetError(EvalDim, pair.Size(), 2);
        return GameValue();
      }

      if (pair[0].GetType() != GameScalar)
      {
        if (state) state->TypeError(GameScalar, pair[0].GetType());
        return GameValue();
      }

      if (pair[1].GetType() != GameScalar)
      {
        if (state) state->TypeError(GameScalar, pair[1].GetType());
        return GameValue();
      }

      axisRotation[i]._at = (AxisType)toInt(pair[0]);
      axisRotation[i]._angle = pair[1];
    }

    SPrimitiveChild child = primitiveTreeNode->GetSideChildFA(array[0], array[1], array[2], axisRotation, tArraySize);

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  void CPTreeNode::RegisterCommands(GameState &gState)
  {
    gState.NewOperator(GameOperator(GameNothing, "setTreeNodeTexture",      function, PrimitiveTreeNode_SetTexture,               GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setTreeNodeParams",       function, PrimitiveTreeNode_SetTreeNodeParams,        GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setTreeNodeChildParams",  function, PrimitiveTreeNode_SetTreeNodeChildParams,   GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameArray,   "getTreeNodeChild",        function, PrimitiveTreeNode_GetChild,                 GamePrimitive, GameScalar TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameArray,   "getTreeNodeSideChild",    function, PrimitiveTreeNode_GetSideChild,             GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameArray,   "getTreeNodeSideChildFA",  function, PrimitiveTreeNode_GetSideChildFA,           GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
  };

}