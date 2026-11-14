#ifndef _auxiliary_h_
#define _auxiliary_h_

#include <El/Evaluator/express.hpp>
#include "TreeDXCommon.h"
#include <Es/Types/pointers.hpp>
#include <El/Math/math3d.hpp>

namespace TreeEngine
{


void ConvertMatrix(D3DMATRIX &mat, Matrix4Par src);

//! Converts a matrix to an array of vectors
GameValue ConvertVector3ToGameValue(Vector3Par vector);
//! Converts a matrix to an array of floats
GameValue ConvertMatrix4ToGameValue(Matrix4Par matrix);
//! Determines wheter it is a proper vector
int IsTypeVector3(GameValuePar data);
//! Determines wheter it is a proper matrix
int IsTypeMatrix4(GameValuePar data);
//! Convertes a GameValue to a vector - gamevalue must have a proper strucutre
Vector3 ConvertGameValueToVector3(GameValue vector);
//! Convertes a GameValue to a matrix - gamevalue must have a proper strucutre
Matrix4 ConvertGameValueToMatrix4(GameValue matrix);
//! Converts inner format to text
RString ConvertScript2Text (RString &in);
//! Converts text to inner format
RString ConvertText2Script (RString &in);
//! Loads the RString structure from specified file

typedef RString (*fntype_LoadRStringFromFile)(RString &fileName);
typedef RString (*fntype_LoadAndPreprocessRStringFromFile)(RString &fileName);

extern fntype_LoadRStringFromFile LoadRStringFromFile;
extern fntype_LoadAndPreprocessRStringFromFile LoadAndPreprocessRStringFromFile;



}
#endif
