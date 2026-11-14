#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FILE_COMPRESS_HPP
#define _FILE_COMPRESS_HPP

#include "qStream.hpp"


class FileBufferUncompressed: public FileBufferMemory
{

	public:
	FileBufferUncompressed
	(
		int outSize, QIStream &in
	);

};

#endif
