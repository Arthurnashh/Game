#include "../wpch.hpp"

#if defined(_WIN64) && _ENABLE_DX11

// Several shared headers still declare the old, global D3D9 handle type.
// In the x64 target only DX11::PostProcessHndl owns render effects; this
// compatibility serializer preserves the shared identity/type fields without
// constructing an EngineDD9 post-process object.
#include "../d3d9/engdd9.hpp"

LSError PostProcessHndl::Serialize(ParamArchive &ar)
{
  CHECK(ar.Serialize("hndl", _hndl, 0));

  int serializedType = (int)_type;
  CHECK(ar.Serialize("type", serializedType, 0));
  _type = (PostEffectType)serializedType;

  return LSOK;
}

// The D3D9 translation unit normally owns this diagnostic enum table.  It is
// excluded from the DX11 x64 executable, so provide the same names here.
DEFINE_ENUM(VertexShaderPoolID, VSP, VSP_ENUM)

#endif
