#include <El/elementpch.hpp>

#include "fileCompress.hpp"

FileBufferUncompressed::FileBufferUncompressed
(
	int outSize, QIStream &in
)
{
	_data.Init(outSize); // uncompressed data
	SSCompress ss;
	if( !ss.Decode(_data.Data(),_data.Size(),in) )
	{
		_data.Delete();
	}
}

