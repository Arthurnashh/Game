#include "Dx11ShapeMesh.hpp"
#include "../Shape/shape.hpp"
#include <limits.h>
#include <stddef.h>
#include <vector>

namespace
{
struct ShapeVertexD3D11
{
  float position[3];
  float normal[3];
  float texcoord[2];
};

bool AppendFaceIndices(const Shape &shape, Offset begin, Offset end,
  UINT vertexCount, std::vector<UINT> &indices)
{
  for (Offset offset = begin; offset < end; shape.NextFace(offset))
  {
    const Poly &face = shape.Face(offset);
    if (face.N() < 3) continue;
    const UINT first = (UINT)face.GetVertex(0);
    if (first >= vertexCount) return false;
    for (int i = 1; i + 1 < face.N(); ++i)
    {
      if (indices.size() > UINT_MAX / sizeof(UINT) - 3) return false;
      const UINT second = (UINT)face.GetVertex(i);
      const UINT third = (UINT)face.GetVertex(i + 1);
      if (second >= vertexCount || third >= vertexCount) return false;
      indices.push_back(first);
      indices.push_back(second);
      indices.push_back(third);
    }
  }
  return true;
}
}

HRESULT Dx11ShapeMesh::Initialize(ID3D11Device *device, const Shape &shape,
  ID3DBlob *vertexShaderBytecode)
{
  if (!device || !vertexShaderBytecode || !shape.IsLoadLocked() ||
      shape.NVertex() <= 0 || shape.NFaces() <= 0)
    return E_INVALIDARG;

  const UINT vertexCount = (UINT)shape.NVertex();
  if (vertexCount > UINT_MAX / sizeof(ShapeVertexD3D11)) return E_INVALIDARG;

  std::vector<ShapeVertexD3D11> vertices(vertexCount);
  const bool hasNormals = shape.GetNormArray().Size() == (int)vertexCount;
  const bool hasUV = shape.GetUVArray().Size() == (int)vertexCount;
  for (UINT i = 0; i < vertexCount; ++i)
  {
    Vector3 position = shape.Pos((int)i);
    Vector3 normal = hasNormals ? shape.Norm((int)i).Get() : VZero;
    UVPair uv = hasUV ? shape.UV((int)i) : UVPair(0, 0);
    vertices[i].position[0] = position.X();
    vertices[i].position[1] = position.Y();
    vertices[i].position[2] = position.Z();
    vertices[i].normal[0] = normal.X();
    vertices[i].normal[1] = normal.Y();
    vertices[i].normal[2] = normal.Z();
    vertices[i].texcoord[0] = uv.u;
    vertices[i].texcoord[1] = uv.v;
  }

  size_t indexCount = 0;
  Offset faceOffset = shape.BeginFaces();
  const Offset faceEnd = shape.EndFaces();
  while (faceOffset < faceEnd)
  {
    const Poly &face = shape.Face(faceOffset);
    if (face.N() >= 3)
    {
      const size_t triangles = (size_t)face.N() - 2;
      if (triangles > (UINT_MAX / sizeof(UINT) - indexCount) / 3)
        return E_INVALIDARG;
      indexCount += triangles * 3;
    }
    shape.NextFace(faceOffset);
  }
  if (!indexCount) return E_INVALIDARG;

  std::vector<UINT> indices;
  indices.reserve(indexCount);
  std::vector<Dx11ShapeSectionRange> sections;
  if (shape.NSections() > 0)
  {
    sections.reserve(shape.NSections());
    for (int s = 0; s < shape.NSections(); ++s)
    {
      const ShapeSection &section = shape.GetSection(s);
      Dx11ShapeSectionRange range;
      range.sectionIndex = (UINT)s;
      range.firstIndex = (UINT)indices.size();
      if (!AppendFaceIndices(shape, section.beg, section.end, vertexCount, indices))
        return E_INVALIDARG;
      range.indexCount = (UINT)indices.size() - range.firstIndex;
      sections.push_back(range);
    }
  }
  else
  {
    Dx11ShapeSectionRange range = {};
    range.sectionIndex = 0;
    if (!AppendFaceIndices(shape, shape.BeginFaces(), faceEnd, vertexCount, indices))
      return E_INVALIDARG;
    range.firstIndex = 0;
    range.indexCount = (UINT)indices.size();
    if (range.indexCount) sections.push_back(range);
  }
  if (indices.empty() || indices.size() > UINT_MAX / sizeof(UINT)) return E_INVALIDARG;

  const D3D11_INPUT_ELEMENT_DESC input[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
      (UINT)offsetof(ShapeVertexD3D11, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
      (UINT)offsetof(ShapeVertexD3D11, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
      (UINT)offsetof(ShapeVertexD3D11, texcoord), D3D11_INPUT_PER_VERTEX_DATA, 0 }
  };
  HRESULT hr = _mesh.Initialize(device, &vertices[0], vertexCount,
    sizeof(ShapeVertexD3D11), &indices[0], (UINT)indices.size(),
    DXGI_FORMAT_R32_UINT, input, ARRAYSIZE(input), vertexShaderBytecode);
  if (SUCCEEDED(hr)) _sections.swap(sections);
  return hr;
}

HRESULT Dx11ShapeMesh::DrawSection(ID3D11DeviceContext *context,
  UINT sectionIndex) const
{
  if (sectionIndex >= _sections.size()) return E_INVALIDARG;
  const Dx11ShapeSectionRange &range = _sections[sectionIndex];
  if (!range.indexCount) return S_FALSE;
  return _mesh.DrawIndexedRange(context, range.firstIndex, range.indexCount);
}
