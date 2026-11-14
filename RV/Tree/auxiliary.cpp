#include "RVTreeCommon.h"
#include "auxiliary.h"

namespace TreeEngine
{

  void ConvertMatrix(D3DMATRIX &mat, Matrix4Par src)
  {
    mat._11 = src.DirectionAside()[0];
    mat._12 = src.DirectionAside()[1];
    mat._13 = src.DirectionAside()[2];

    mat._21 = src.DirectionUp()[0];
    mat._22 = src.DirectionUp()[1];
    mat._23 = src.DirectionUp()[2];

    mat._31 = src.Direction()[0];
    mat._32 = src.Direction()[1];
    mat._33 = src.Direction()[2];

    mat._41 = src.Position()[0];
    mat._42 = src.Position()[1];
    mat._43 = src.Position()[2];

    mat._14 = 0;
    mat._24 = 0;
    mat._34 = 0;
    mat._44 = 1;
  }

  GameValue ConvertVector3ToGameValue(Vector3Par vector) {
    GameArrayType GATVector; GATVector.Realloc(3); GATVector.Resize(3);
    GATVector[0] = vector.X();
    GATVector[1] = vector.Y();
    GATVector[2] = vector.Z();
    return GATVector;
  }

  GameValue ConvertMatrix4ToGameValue(Matrix4Par matrix) {
    GameArrayType GATMatrix; GATMatrix.Realloc(4); GATMatrix.Resize(4);
    GATMatrix[0] = ConvertVector3ToGameValue(matrix.DirectionAside());
    GATMatrix[1] = ConvertVector3ToGameValue(matrix.DirectionUp());
    GATMatrix[2] = ConvertVector3ToGameValue(matrix.Direction());
    GATMatrix[3] = ConvertVector3ToGameValue(matrix.Position());
    return GATMatrix;
  }

  int IsTypeVector3(GameValuePar data) {
    if (data.GetType() != GameArray) return 0;
    const GameArrayType &array = data;
    if (array.Size() != 3) return 0;
    if (array[0].GetType() != GameScalar) return 0;
    if (array[1].GetType() != GameScalar) return 0;
    if (array[2].GetType() != GameScalar) return 0;
    return 1;
  }

  int IsTypeMatrix4(GameValuePar data) {
    if (data.GetType() != GameArray) return 0;
    const GameArrayType &array = data;
    if (array.Size() != 4) return 0;
    if (!IsTypeVector3(array[0])) return 0;
    if (!IsTypeVector3(array[1])) return 0;
    if (!IsTypeVector3(array[2])) return 0;
    if (!IsTypeVector3(array[3])) return 0;
    return 1;
  }

  Vector3 ConvertGameValueToVector3(GameValue vector) {
    const GameArrayType &array = vector;
    return Vector3(array[0], array[1], array[2]);
  }

  Matrix4 ConvertGameValueToMatrix4(GameValue matrix) {
    const GameArrayType &array = matrix;
    Matrix4 m;
    m.SetDirectionAside(ConvertGameValueToVector3(array[0]));
    m.SetDirectionUp(ConvertGameValueToVector3(array[1]));
    m.SetDirection(ConvertGameValueToVector3(array[2]));
    m.SetPosition(ConvertGameValueToVector3(array[3]));
    return m;
  }

  RString ConvertScript2Text (RString &in)
  {
    int n = in.GetLength();
    int j = 0;
    for (int i = 0; i < n;)
    {
      if (in[i] != '\\')
      {
        j++;
        i++;
      }
      else if (++i < n)
      {
        switch (in[i++])
        {
        case '\\':
        case 'n':
        case 'r':
        case 't':
          j++;
          break;
        }
      }
    }
    j++;

    char *buf = new char[j];

    j = 0;
    for (int i = 0; i < n;)
    {
      if (in[i] != '\\')
      {
        buf[j++] = in[i++];
      }
      else if (++i < n)
      {
        switch (in[i++])
        {
        case '\\':
          buf[j++] = '\\';
          break;

        case 'n':
          buf[j++] = '\n';
          break;

        case 'r':
          buf[j++] = '\r';
          break;

        case 't':
          buf[j++] = '\t';
          break;
        }
      }
    }
    buf[j] = '\0';

    RString result = buf;
    delete [] buf;

    return result;
  }

  RString ConvertText2Script (RString &in)
  {
    char buf[4096];
    int n = in.GetLength();
    int j = 0;
    for (int i = 0; i < n; ++i)
    {
      switch (in[i])
      {
      case '\\':
        buf[j++] = '\\';
        buf[j++] = '\\';
        break;

      case '\n':
        buf[j++] = '\\';
        buf[j++] = 'n';
        break;

      case '\r':
        buf[j++] = '\\';
        buf[j++] = 'r';
        break;

      case '\t':
        buf[j++] = '\\';
        buf[j++] = 't';
        break;

      default:
        buf[j++] = in[i];
        break;
      }
    }
    buf[j] = '\0';

    return RString (buf);
  }

  RString defLoadRStringFromFile(RString &fileName) {
    QIFStream stream;
    stream.open(fileName);
    if (!stream.fail())
    {
      AutoArray<char> data;
      data.Realloc(stream.rest());
      data.Resize(stream.rest());
      stream.read(data.Data(),stream.rest());
      return RString(data.Data(),data.Size());
    }
    return RString();
  }

  RString defLoadAndPreprocessRStringFromFile(RString &fileName) {
    FilePreprocessor fp;
    QOStrStream stream;
    if (fp.Process(&stream, fileName)) {
      return RString(RString("scopeName \"")+fileName+"\";\r\n",RString(stream.str(), stream.pcount()));
    }
    else {
      return RString();
    }
  }

fntype_LoadRStringFromFile LoadRStringFromFile = &defLoadRStringFromFile;
fntype_LoadAndPreprocessRStringFromFile LoadAndPreprocessRStringFromFile = &defLoadAndPreprocessRStringFromFile;


};