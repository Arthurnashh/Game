#include "wpch.hpp"
#include "serializeBinExt.hpp"

void operator << ( SerializeBinStream &s, Vector3 &data )
{
	if (s.IsLoading())
	{
		Vector3P t;
		s.LoadLittleEndian(&t,3,sizeof(Coord));
		data=Vector3(t.X(),t.Y(),t.Z());
	}
	else
	{
		Vector3P t(data.X(),data.Y(),data.Z());
		s.TransferBinary(t);
	}
}

void operator << ( SerializeBinStream &s, Color &data )
{
  if (s.IsLoading())
  {
    s.LoadLittleEndian(&data,4,sizeof(float));
    Assert(sizeof(data)==4*sizeof(float));
  }
  else
  {
	  s.TransferBinary(data);
	}
}

void operator << ( SerializeBinStream &s, PackedColor &data )
{
  s.TransferLittleEndian(data);
}

void operator << ( SerializeBinStream &s, Matrix4 &data )
{
	if (s.IsLoading())
	{
		Matrix4P t = M4IdentityP;
    s.LoadLittleEndian(&t,12,sizeof(Coord));
		data = ConvertToM(t);
	}
	else
	{
		Matrix4P t = ConvertToP(data);
		s.TransferBinary(t);
	}
}

void operator << ( SerializeBinStream &s, Matrix3 &data )
{
	if (s.IsLoading())
	{
		Matrix3P t = M3IdentityP;
    s.LoadLittleEndian(&t,9,sizeof(Coord));
		data = ConvertToM(t);
	}
	else
	{
		Matrix3P t = ConvertToP(data);
		s.TransferBinary(t);
	}
}

void operator << ( SerializeBinStream &s, short &data )
{
  if (s.EndianSwapNeeded())
    s.TransferLittleEndian16b(&data);
  else
    s.TransferBinary(&data, sizeof(data));
}


#if defined Vector3KPar // TODO: introduce a new macro to switch in cases like this

void SerializeAs<Vector3K>::ConvertOnLoad(Type *dst, As *src, int nElems, bool endianSwap)
{
  // loaded as P, perform in-place conversion to K
  if (endianSwap) SerializeBinStream::ProcessLittleEndianData(src,sizeof(As)*nElems,EndianTraits<As>::BlockSize);
  while (--nElems>=0)
  {
    dst[nElems] = ConvertPToK(src[nElems]);
  }
}

void SerializeAs<Matrix3K>::ConvertOnLoad(Type *dst, As *src, int nElems, bool endianSwap)
{
  if (endianSwap) SerializeBinStream::ProcessLittleEndianData(src,sizeof(As)*nElems,EndianTraits<As>::BlockSize);
  // loaded as P, perform in-place conversion to K
  while (--nElems>=0)
  {
    dst[nElems] = ConvertPToK(src[nElems]);
  }
}

void SerializeAs<Matrix4K>::ConvertOnLoad(Type *dst, As *src, int nElems, bool endianSwap)
{
  if (endianSwap) SerializeBinStream::ProcessLittleEndianData(src,sizeof(As)*nElems,EndianTraits<As>::BlockSize);
  // loaded as P, perform in-place conversion to K
  while (--nElems>=0)
  {
    dst[nElems] = ConvertPToK(src[nElems]);
  }
}

void SerializeAs<STPair>::ConvertOnLoad(Type *dst, As *src, int nElems, bool endianSwap)
{
  if (endianSwap) SerializeBinStream::ProcessLittleEndianData(src,sizeof(As)*nElems,EndianTraits<As>::BlockSize);
  // loaded as P, perform in-place conversion to K
  while (--nElems>=0)
  {
    dst[nElems].s = ConvertPToK(src[nElems].s);
    dst[nElems].t = ConvertPToK(src[nElems].t);
  }
}

#endif
