#ifdef _MSC_VER
#  pragma once
#endif

/**
    @file   common/crc32.h
    @brief  CRC-32 checksum.

    Copyright &copy; 1995-1996 by Mark Adler, adopted 2001 by J.Pelikan
    @author PE
    @since  3.1.2002
    @date   30.9.2002
*/

#ifndef _CRC32_H
#define _CRC32_H

#include "Es/Common/global.hpp"

/**
    CRC32 checksum.
    @param  crc Initial accumulator value.
    @param  buf Data buffer.
    @param  len Data buffer length.
*/
extern unsigned32 crc32 ( unsigned32 crc, const unsigned8 *buf, int64 len );

#endif
