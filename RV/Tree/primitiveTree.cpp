#include "RVTreeCommon.h"
#include "auxiliary.h"
#include "primitiveTree.h"


namespace TreeEngine
{
    void CPTree::SetTexture(RString name, int index) {
    _textureIndex = index;
  }

  void CPTree::SetTreeParams(float width,
                            float height,
                            float balance,
                            float stiffness,
                            float twist,
                            float spinA,
                            float spinB,
                            float spread,
                            int segmentCount,
                            int drawMask,
                            float textureVOffset,
                            float geometrySpin,
                            float textureUTiling,
                            float textureVTiling,
                            bool forceSegmentCount) {
    _width = width;
    _height = height;
    _balance = balance;
    _stiffness = stiffness;
    _twist = twist;
    _spinA = spinA;
    _spinB = spinB;
    _spread = spread;
    _segmentCount = segmentCount;
    _drawMask = drawMask;
    _textureVOffset = textureVOffset;
    _geometrySpin = geometrySpin;
    _textureUTiling = textureUTiling;
    _textureVTiling = textureVTiling;
    _forceSegmentCount = forceSegmentCount;
  }

  SPrimitiveChild CPTree::GetChildA() {

    float affectedABalance;
    // If A is more important
    if (_balance < 0.5f) {
      affectedABalance = _balance * (1.0 - _stiffness);
    }
    else {
      affectedABalance = _balance;
    }
    
    // Precounted coeficient
    float BSP = (affectedABalance * _spread * (H_PI * 0.5f)) / ((float)(_segmentCount));

    // Create matrix
    Matrix4 ChildOrientation;
    ChildOrientation.SetRotationX(BSP);

    // Child origin
    Matrix4 rotZ;
    rotZ.SetRotationZ(_twist);
    Matrix4 ChildOrigin;
    ChildOrigin = _origin * rotZ;

    // Distance between segments
    float segmentShift = (_height * (1.0f - affectedABalance)) / ((float)(_segmentCount));

    // Accumulate segments
    for (int i = 0; i < _segmentCount; i++) {

      // Rotate child origin a bit
      ChildOrigin = ChildOrigin * ChildOrientation;

      // Shift it forward a bit
      ChildOrigin.SetPosition(ChildOrigin.Position() + ChildOrigin.Direction() * segmentShift);
    }

    // Add spin
    Matrix4 childSpin;
    childSpin.SetRotationZ(_spinA);

    // Fill out the structure
    SPrimitiveChild PC;
    PC._origin = ChildOrigin * childSpin;
    PC._slotIndex = 0;
    //PC._textureVOffset = _textureVOffset + _textureVTiling;
    PC._textureVOffset = _textureVOffset + _textureVTiling * _height * (1.0f - affectedABalance);
    PC._geometrySpin = _geometrySpin - _twist;
    return PC;
  }

  SPrimitiveChild CPTree::GetSideChildA(float directionPos,
                                        float sideShift,
                                        float slopeAngle,
                                        float twistAngle,
                                        float spinAngle) {
  /*
    Matrix3 r;
    r.SetRotationZ(H_PI * 0.01);
    Matrix3 rA;
    rA.SetRotationAxis(Vector3(0, 0, 1).Normalized(), H_PI * 0.01);

  */

    float affectedABalance;
    // If A is more important
    if (_balance < 0.5f) {
      affectedABalance = _balance * (1.0 - _stiffness);
    }
    else {
      affectedABalance = _balance;
    }
    
    // Precounted coeficient
    float BSP = (affectedABalance * _spread * (H_PI * 0.5f)) / ((float)(_segmentCount));

    // Create matrix
    Matrix4 ChildOrientation;
    ChildOrientation.SetRotationX(BSP);

    // Child origin
    Matrix4 rotZ;
    rotZ.SetRotationZ(_twist);
    Matrix4 ChildOrigin;
    ChildOrigin = _origin * rotZ;

    // Length of the stem
    float length = (_height * (1.0f - affectedABalance));

    // Distance between segments
    float segmentShift =  length / ((float)(_segmentCount));

    // Accumulate segment
    float lengthTreshold = length * directionPos;
    float sumSegmentShift = 0.0f;
    for (; sumSegmentShift + segmentShift < lengthTreshold; sumSegmentShift += segmentShift) {

      // Rotate child origin a bit
      ChildOrigin = ChildOrigin * ChildOrientation;

      // Shift it forward a bit
      ChildOrigin.SetPosition(ChildOrigin.Position() + ChildOrigin.Direction() * segmentShift);
    }
    ChildOrigin = ChildOrigin * ChildOrientation;

    Matrix3 RotSpin;
    RotSpin.SetRotationZ(spinAngle);
    Matrix3 RotAside;
    RotAside.SetRotationY(slopeAngle);
    Matrix3 RotDirection;
    RotDirection.SetRotationZ(twistAngle);
    Matrix3 SideChildOriginOrientation = ChildOrigin.Orientation() * RotDirection * RotAside * RotSpin;

    // Fill out the structure
    SPrimitiveChild pc;
    pc._origin.SetPosition(
      ChildOrigin.Position() + ChildOrigin.Direction() * (lengthTreshold - sumSegmentShift)
      + ChildOrigin.DirectionAside() * sideShift * cos(twistAngle)
      + ChildOrigin.DirectionUp() * sideShift * sin(twistAngle));
    pc._origin.SetOrientation(SideChildOriginOrientation);
    pc._slotIndex = -1;
    pc._textureVOffset = 0.0f;
    pc._geometrySpin = 0.0f;
    return pc;
  }

  SPrimitiveChild CPTree::GetChildB() {

    float affectedBBalance;
    // If A is more important
    if (_balance < 0.5f) {
      affectedBBalance = _balance;
    }
    else {
      affectedBBalance = 1.0 - (1.0f - _balance) * (1.0 - _stiffness);
    }

    // Precounted coeficient
    float BSP = ((1.0f - affectedBBalance) * _spread * (H_PI * 0.5f)) / ((float)(_segmentCount));

    // Create matrix
    Matrix4 ChildOrientation;
    ChildOrientation.SetRotationX(BSP);

    // Child origin
    Matrix4 rotZ;
    rotZ.SetRotationZ(_twist + H_PI);
    Matrix4 ChildOrigin;
    ChildOrigin = _origin * rotZ;

    // Distance between segments
    float segmentShift = (_height * affectedBBalance) / ((float)(_segmentCount));

    // Accumulate segments
    for (int i = 0; i < _segmentCount; i++) {

      // Rotate child origin a bit
      ChildOrigin = ChildOrigin * ChildOrientation;

      // Shift it forward a bit
      ChildOrigin.SetPosition(ChildOrigin.Position() + ChildOrigin.Direction() * segmentShift);
    }

    // Add spin
    Matrix4 childSpin;
    childSpin.SetRotationZ(_spinB);

    // Fill out the structure
    SPrimitiveChild PC;
    PC._origin = ChildOrigin * childSpin;
    PC._slotIndex = 1;
    //PC._textureVOffset = _textureVOffset + _textureVTiling;
    PC._textureVOffset = _textureVOffset + _textureVTiling * _height * affectedBBalance;
    PC._geometrySpin = _geometrySpin - (_twist + H_PI);
    return PC;
  }

  SPrimitiveChild CPTree::GetSideChildB(float directionPos,
                                        float sideShift,
                                        float slopeAngle,
                                        float twistAngle,
                                        float spinAngle) {

    float affectedBBalance;
    // If A is more important
    if (_balance < 0.5f) {
      affectedBBalance = _balance;
    }
    else {
      affectedBBalance = 1.0 - (1.0f - _balance) * (1.0 - _stiffness);
    }
    
    // Precounted coeficient
    float BSP = ((1.0f - affectedBBalance) * _spread * (H_PI * 0.5f)) / ((float)(_segmentCount));

    // Create matrix
    Matrix4 ChildOrientation;
    ChildOrientation.SetRotationX(BSP);

    // Child origin
    Matrix4 rotZ;
    rotZ.SetRotationZ(_twist + H_PI);
    Matrix4 ChildOrigin;
    ChildOrigin = _origin * rotZ;

    // Length of the stem
    float length = (_height * affectedBBalance);

    // Distance between segments
    float segmentShift =  length / ((float)(_segmentCount));

    // Accumulate segment
    float lengthTreshold = length * directionPos;
    float sumSegmentShift = 0.0f;
    for (; sumSegmentShift + segmentShift < lengthTreshold; sumSegmentShift += segmentShift) {

      // Rotate child origin a bit
      ChildOrigin = ChildOrigin * ChildOrientation;

      // Shift it forward a bit
      ChildOrigin.SetPosition(ChildOrigin.Position() + ChildOrigin.Direction() * segmentShift);
    }
    ChildOrigin = ChildOrigin * ChildOrientation;

    Matrix3 RotSpin;
    RotSpin.SetRotationZ(spinAngle);
    Matrix3 RotAside;
    RotAside.SetRotationY(slopeAngle);
    Matrix3 RotDirection;
    RotDirection.SetRotationZ(twistAngle);
    Matrix3 SideChildOriginOrientation = ChildOrigin.Orientation() * RotDirection * RotAside * RotSpin;

    // Fill out the structure
    SPrimitiveChild pc;
    pc._origin.SetPosition(
      ChildOrigin.Position() + ChildOrigin.Direction() * (lengthTreshold - sumSegmentShift)
      + ChildOrigin.DirectionAside() * sideShift * cos(twistAngle)
      + ChildOrigin.DirectionUp() * sideShift * sin(twistAngle));
    pc._origin.SetOrientation(SideChildOriginOrientation);
    pc._slotIndex = -1;
    pc._textureVOffset = 0.0f;
    pc._geometrySpin = 0.0f;
    return pc;
  }

  int CPTree::Draw(CPrimitiveStream &ps,
                  int sharedVertexIndex,
                  int sharedVertexCount,
                  bool isRoot,
                  float detail,
                  SSlot *pSlot) {

    int tAccu = 0;
    float BSP;
    Matrix4 childOrientation;
    Matrix4 childOrigin;
    float segmentShift;
    float width;
    float deltaWidth;
    Matrix4 newChildOrigin;
    float newWidth;
    float textureVOffset;
    ///float deltaTextureVTiling = _textureVTiling / ((float)_segmentCount);
    float deltaTextureVTiling;
    int newSharedVertexIndex;
    int newSharedSidesCount;
    Matrix4 childSpin;
    int stepNumber = _forceSegmentCount?
            _segmentCount:
            min(_segmentCount, max(1, toIntCeil(((float)_segmentCount) * detail)));

    float skipSegments = ((float)_segmentCount) / stepNumber;
    float sumSkipSegments;
    Matrix4 newSpinnedChildOrigin;
    float gSpin;

    float affectedABalance;
    float affectedBBalance;
    // If A is more important
    if (_balance < 0.5f) {
      affectedABalance = _balance * (1.0 - _stiffness);
      affectedBBalance = _balance;
    }
    else {
      affectedABalance = _balance;
      affectedBBalance = 1.0 - (1.0f - _balance) * (1.0 - _stiffness);
    }

    // A child ----------------------------------------------------------

    if (!(_drawMask & DM_DRAWCHILDA)) {
      pSlot[0]._sharedVertexIndex = -1;
      pSlot[0]._sharedVertexCount = 0;
    }
    else {

      // Precounted coeficient
      BSP = (affectedABalance * _spread * (H_PI * 0.5f)) / ((float)(_segmentCount));

      // Create matrix
      childOrientation.SetRotationX(BSP);

      // Child origin
      Matrix4 rotZ;
      rotZ.SetRotationZ(_twist);
      childOrigin = _origin * rotZ;

      // Distance between segments
      segmentShift = (_height * ((1.0f - affectedABalance)/* + 1.0f*/)) / ((float)(_segmentCount));

      // Delta of V tiling
      deltaTextureVTiling = _textureVTiling * segmentShift;

      // Width coeficients
      width = _width;
      deltaWidth = (_width - sqrt(1.0f - _balance/*affectedABalance*/) * _width) / ((float)_segmentCount);

      // V tiling coefs
      textureVOffset = _textureVOffset;

      // Accumulate attributes of skipped segments
      newWidth = width;
      newChildOrigin = childOrigin;
      for (int i = 0; i < toInt(skipSegments); i++) {
        newChildOrigin = newChildOrigin * childOrientation;
        newChildOrigin.SetPosition(newChildOrigin.Position() + newChildOrigin.Direction() * segmentShift);
        newWidth = newWidth - deltaWidth;
        textureVOffset += deltaTextureVTiling;
      }
      sumSkipSegments = skipSegments;

      // Geometry spin
      gSpin = _geometrySpin - _twist;

      // New spinned origin
      childSpin.SetRotationZ(_spinA * sumSkipSegments / _segmentCount + gSpin);
      newSpinnedChildOrigin = newChildOrigin * childSpin;

      // Draw first cone
      newSharedSidesCount = CONE_SIDES_NUM(detail, newWidth);
      if (sharedVertexCount > 0) {
        newSharedVertexIndex = DrawConeShared(
          ps,
          _textureIndex,
          _textureUTiling,
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
          _textureUTiling,
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
          newChildOrigin = newChildOrigin * childOrientation;
          newChildOrigin.SetPosition(newChildOrigin.Position() + newChildOrigin.Direction() * segmentShift);
          newWidth = newWidth - deltaWidth;
          textureVOffset += deltaTextureVTiling;
        }
        sumSkipSegments += skipSegments;

        // New spinned origin
        childSpin.SetRotationZ(_spinA * sumSkipSegments / _segmentCount + gSpin);
        newSpinnedChildOrigin = newChildOrigin * childSpin;

        // Draw the cone
        int oldSharedSidesCount = newSharedSidesCount;
        newSharedSidesCount = CONE_SIDES_NUM(detail, newWidth);
        newSharedVertexIndex = DrawConeShared(
          ps,
          _textureIndex,
          _textureUTiling,
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

      pSlot[0]._sharedVertexIndex = newSharedVertexIndex;
      pSlot[0]._sharedVertexCount = newSharedSidesCount + 1;
    }
    
    // B child ----------------------------------------------------------

    if (!(_drawMask & DM_DRAWCHILDB)) {
      pSlot[1]._sharedVertexIndex = -1;
      pSlot[1]._sharedVertexCount = 0;
    }
    else {

      // Precounted coeficient
      BSP = ((1.0f - affectedBBalance) * _spread * (H_PI * 0.5f)) / ((float)(_segmentCount));

      // Create matrix
      childOrientation.SetRotationX(BSP);

      // Child origin
      Matrix4 rotZ;
      rotZ.SetRotationZ(_twist + H_PI);
      childOrigin = _origin * rotZ;

      // Distance between segments
      segmentShift = (_height * (affectedBBalance/* + 1.0f*/)) / ((float)(_segmentCount));

      // Delta of V tiling
      deltaTextureVTiling = _textureVTiling * segmentShift;

      // Width coefficients
      width = _width;
      deltaWidth = (_width - sqrt(/*affectedBBalance*/_balance) * _width) / ((float)_segmentCount);

      // V tiling coefficients
      textureVOffset = _textureVOffset;

      // Accumulate attributes of skipped segments
      newWidth = width;
      newChildOrigin = childOrigin;
      for (int i = 0; i < toInt(skipSegments); i++) {
        newChildOrigin = newChildOrigin * childOrientation;
        newChildOrigin.SetPosition(newChildOrigin.Position() + newChildOrigin.Direction() * segmentShift);
        newWidth = newWidth - deltaWidth;
        textureVOffset += deltaTextureVTiling;
      }
      sumSkipSegments = skipSegments;

      // Geometry spin
      gSpin = _geometrySpin - (_twist + H_PI);

      // New spinned origin
      childSpin.SetRotationZ(_spinB * sumSkipSegments / _segmentCount + gSpin);
      newSpinnedChildOrigin = newChildOrigin * childSpin;

      // Draw first cone
      newSharedSidesCount = CONE_SIDES_NUM(detail, newWidth);
      if (sharedVertexCount > 0) {
        newSharedVertexIndex = DrawConeShared(
          ps,
          _textureIndex,
          _textureUTiling,
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
          _textureUTiling,
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
          newChildOrigin = newChildOrigin * childOrientation;
          newChildOrigin.SetPosition(newChildOrigin.Position() + newChildOrigin.Direction() * segmentShift);
          newWidth = newWidth - deltaWidth;
          textureVOffset += deltaTextureVTiling;
        }
        sumSkipSegments += skipSegments;

        // New spinned origin
        childSpin.SetRotationZ(_spinB * sumSkipSegments / _segmentCount + gSpin);
        newSpinnedChildOrigin = newChildOrigin * childSpin;

        // Draw the cone
        int oldSharedSidesCount = newSharedSidesCount;
        newSharedSidesCount = CONE_SIDES_NUM(detail, newWidth);
        newSharedVertexIndex = DrawConeShared(
          ps,
          _textureIndex,
          _textureUTiling,
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

      pSlot[1]._sharedVertexIndex = newSharedVertexIndex;
      pSlot[1]._sharedVertexCount = newSharedSidesCount + 1;
    }

    return tAccu;
  }

  void CPTree::UpdatePointList(CPointList &pointList) {
    pointList.AddPoint(_origin.Position() + _origin.DirectionAside() * _width * SquareCoef);
    pointList.AddPoint(_origin.Position() - _origin.DirectionAside() * _width * SquareCoef);
    pointList.AddPoint(_origin.Position() + _origin.DirectionUp() * _width * SquareCoef);
    pointList.AddPoint(_origin.Position() - _origin.DirectionUp() * _width * SquareCoef);

    SPrimitiveChild pc;
    pc = GetChildA();
    //BBoxCounter.AddVertex(pc._origin.Position());
    pointList.AddPoint(pc._origin.Position() + pc._origin.DirectionAside() * _width * SquareCoef);
    pointList.AddPoint(pc._origin.Position() - pc._origin.DirectionAside() * _width * SquareCoef);
    pointList.AddPoint(pc._origin.Position() + pc._origin.DirectionUp() * _width * SquareCoef);
    pointList.AddPoint(pc._origin.Position() - pc._origin.DirectionUp() * _width * SquareCoef);
    pc = GetChildB();
    //BBoxCounter.AddVertex(pc._origin.Position());
    pointList.AddPoint(pc._origin.Position() + pc._origin.DirectionAside() * _width * SquareCoef);
    pointList.AddPoint(pc._origin.Position() - pc._origin.DirectionAside() * _width * SquareCoef);
    pointList.AddPoint(pc._origin.Position() + pc._origin.DirectionUp() * _width * SquareCoef);
    pointList.AddPoint(pc._origin.Position() - pc._origin.DirectionUp() * _width * SquareCoef);
  }

  // ------------------------------------------
  // Following code is here for script purposes

  GameValue PrimitiveTree_SetTexture(const GameState *state,
                                    GameValuePar oper1,
                                    GameValuePar oper2) {

    CPTree *primitiveTree = dynamic_cast<CPTree*>(GetPrimitive(oper1));

    if (!primitiveTree) {
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

    primitiveTree->SetTexture(array[0], toInt(array[1]));

    return GameValue();
  }

  GameValue PrimitiveTree_SetTreeParams(const GameState *state,
                                        GameValuePar oper1,
                                        GameValuePar oper2) {

    CPTree *primitiveTree = dynamic_cast<CPTree*>(GetPrimitive(oper1));

    if (!primitiveTree) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 14 && array.Size() != 15 ) {
		  if (state) state->SetError(EvalDim, array.Size(), 14);
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

	  if (array[9].GetType() != GameScalar) {
		  if (state) state->TypeError(GameScalar, array[9].GetType());
		  return GameValue();
    }

	  if (array[10].GetType() != GameScalar) {
		  if (state) state->TypeError(GameScalar, array[10].GetType());
		  return GameValue();
    }

	  if (array[11].GetType() != GameScalar) {
		  if (state) state->TypeError(GameScalar, array[11].GetType());
		  return GameValue();
    }

	  if (array[12].GetType() != GameScalar) {
		  if (state) state->TypeError(GameScalar, array[12].GetType());
		  return GameValue();
    }

	  if (array[13].GetType() != GameScalar) {
		  if (state) state->TypeError(GameScalar, array[13].GetType());
		  return GameValue();
    }

    if (toInt(array[13]) < 1) {
        LogF("Segment count cannot be negative (%d)", toInt(array[13]));
        if (state) state->SetError(EvalNum);
        return GameValue();
    }

    bool forceSegmentCount = false;
    if (array.Size() >14) 
        if (array[14].GetType() == GameBool) 
            forceSegmentCount = array[14];
        else {
          if (state) state->TypeError(GameScalar, array[14].GetType());
          return GameValue();
        }

    primitiveTree->SetTreeParams(
      array[0],
      array[1],
      array[2],
      array[3],
      array[4],
      array[5],
      array[6],
      array[7],
      toInt(array[8]),
      toInt(array[9]),
      array[10],
      array[11],
      array[12],
      array[13],
      forceSegmentCount);

    return GameValue();
  }

  GameValue PrimitiveTree_GetChildA(const GameState *state,
                                    GameValuePar oper1) {

    CPTree *primitiveTree = dynamic_cast<CPTree*>(GetPrimitive(oper1));

    if (!primitiveTree) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

    SPrimitiveChild child = primitiveTree->GetChildA();

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  GameValue PrimitiveTree_GetSideChildA(const GameState *state,
                                        GameValuePar oper1,
                                        GameValuePar oper2) {

    CPTree *primitiveTree = dynamic_cast<CPTree*>(GetPrimitive(oper1));

    if (!primitiveTree) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 5) {
		  if (state) state->SetError(EvalDim, array.Size(), 5);
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

    SPrimitiveChild child = primitiveTree->GetSideChildA(array[0], array[1], array[2], array[3], array[4]);

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  GameValue PrimitiveTree_GetChildB(const GameState *state,
                                    GameValuePar oper1) {

    CPTree *primitiveTree = dynamic_cast<CPTree*>(GetPrimitive(oper1));

    if (!primitiveTree) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

    SPrimitiveChild child = primitiveTree->GetChildB();

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  GameValue PrimitiveTree_GetSideChildB(const GameState *state,
                                        GameValuePar oper1,
                                        GameValuePar oper2) {

    CPTree *primitiveTree = dynamic_cast<CPTree*>(GetPrimitive(oper1));

    if (!primitiveTree) {
		  if (state) state->SetError(EvalGen);
      return GameValue();
    }

	  const GameArrayType &array = oper2;
	  if (array.Size() != 5) {
		  if (state) state->SetError(EvalDim, array.Size(), 5);
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

    SPrimitiveChild child = primitiveTree->GetSideChildB(array[0], array[1], array[2], array[3], array[4]);

    GameArrayType returnValue; returnValue.Realloc(4); returnValue.Resize(4);
    returnValue[0] = ConvertMatrix4ToGameValue(child._origin);
    returnValue[1] = (float)child._slotIndex;
    returnValue[2] = child._textureVOffset;
    returnValue[3] = child._geometrySpin;

    return returnValue;
  }

  void CPTree::RegisterCommands(GameState &gState)
  {
    gState.NewOperator(GameOperator(GameNothing, "setTreeTexture", function, PrimitiveTree_SetTexture,    GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameNothing, "setTreeParams",  function, PrimitiveTree_SetTreeParams, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameArray,   "getSideChildA",  function, PrimitiveTree_GetSideChildA, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewOperator(GameOperator(GameArray,   "getSideChildB",  function, PrimitiveTree_GetSideChildB, GamePrimitive, GameArray TODO_OPERATOR_DOCUMENTATION));
    gState.NewFunction(GameFunction(GameArray,   "getChildA", PrimitiveTree_GetChildA, GamePrimitive TODO_FUNCTION_DOCUMENTATION));
    gState.NewFunction(GameFunction(GameArray,   "getChildB", PrimitiveTree_GetChildB, GamePrimitive TODO_FUNCTION_DOCUMENTATION));
  };

  // ------------------------------------------

}