#pragma once
#include <el\evaluator\express.hpp>
#include "O2TypeID.h"
#include <El\Pathname\Pathname.h>
#include <el/math/math3d.hpp>

#define TYPES_MATRIX(XX, Category) \
  XX("Matrix",EvalType_Matrix,CreateMatrix,"@Matrix","Matrix","This type represents general transformation matrix 4x3 (for 3D transforms)",Category)\

TYPES_MATRIX(DECLARE_TYPE, "Matrix")

class GdMatrix :  public GameData, public Matrix4P
{
public:


  GdMatrix() {}
  GdMatrix(const Matrix4P &mx):Matrix4P(mx) {}
  

  const GameType &GetType() const {return EvalType_Matrix;}
  RString GetText() const;

  bool IsEqualTo(const GameData *data) const;
  const char *GetTypeName() const {return "Matrix";}
  GameData *Clone() const {return new GdMatrix (*this);}

  USE_FAST_ALLOCATOR;

  static void RegisterToGameState(GameState *gState);
};
