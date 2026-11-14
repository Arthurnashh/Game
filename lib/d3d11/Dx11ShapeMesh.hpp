#pragma once
#ifndef DAYZ_D3D11_SHAPE_MESH_HPP
#define DAYZ_D3D11_SHAPE_MESH_HPP

#include <Es/Common/win.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include "Dx11Mesh.hpp"
#include <vector>

class Shape;

struct Dx11ShapeSectionRange
{
  UINT sectionIndex;
  UINT firstIndex;
  UINT indexCount;
};

// Converts a loaded static game Shape into D3D11 vertex/index buffers.
// Materials and section-specific textures are deliberately bound separately.
class Dx11ShapeMesh
{
public:
  Dx11ShapeMesh() {}
  Dx11ShapeMesh(const Dx11ShapeMesh &) = delete;
  Dx11ShapeMesh &operator=(const Dx11ShapeMesh &) = delete;

  HRESULT Initialize(ID3D11Device *device, const Shape &shape,
    ID3DBlob *vertexShaderBytecode);
  void Draw(ID3D11DeviceContext *context) const { _mesh.Draw(context); }
  HRESULT DrawSection(ID3D11DeviceContext *context, UINT sectionIndex) const;
  void Reset() { _mesh.Reset(); _sections.clear(); }
  UINT IndexCount() const { return _mesh.IndexCount(); }
  UINT SectionCount() const { return (UINT)_sections.size(); }
  const Dx11ShapeSectionRange &Section(UINT index) const { return _sections[index]; }

private:
  Dx11Mesh _mesh;
  std::vector<Dx11ShapeSectionRange> _sections;
};

#endif
