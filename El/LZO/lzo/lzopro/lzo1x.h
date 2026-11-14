/* lzo1x.h -- public interface of the LZO1X compression algorithm

   LZO Professional data compression library.
   LICENSED MATERIALS - PROPERTY OF OBERHUMER.

   Copyright (C) 2011 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2010 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2009 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2008 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2007 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2006 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2005 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2004 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2003 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2002 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2001 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2000 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1999 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1998 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   CONFIDENTIAL & PROPRIETARY SOURCE CODE.

   ANY USAGE OF THIS FILE IS SUBJECT TO YOUR LICENSE AGREEMENT.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/products/lzo-professional/
 */


#ifndef __LZOPRO_LZO1X_H_INCLUDED
#define __LZOPRO_LZO1X_H_INCLUDED 1

#ifndef __LZOCONF_H_INCLUDED
#include <lzo/lzoconf.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
// fast decompressors
************************************************************************/

#define LZOPRO_LZO1X_MEM_DECOMPRESS       (0)

LZO_EXTERN(int)
lzopro_lzo1x_decompress         ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem /* NOT USED */ );

LZO_EXTERN(int)
lzopro_lzo1x_decompress_safe    ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem /* NOT USED */ );

#ifdef __cplusplus
class QIStream;

/* BI custom LZOPRO decompression from a stream */
LZO_EXTERN(int) lzopro_lzo1x_decompress_from_stream(QIStream &in, lzo_bytep out, lzo_uintp out_len, lzo_voidp wrkmem);
#endif


/* Additional versions of the decompressor that can be faster on some
 * architectures. You should use these only after thorough performance
 * testing.
 *
 * NOTE:
 *   These additional implementations are only available if you build
 *   LZO Professional using -DLZOPRO_CFG_BUILD_EXTRA_DECOMPRESSORS .
 *   Otherwise these versions are just an alias for the function above.
 */
LZO_EXTERN(int)
lzopro_lzo1x_decompress_a00     ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem /* NOT USED */ );
LZO_EXTERN(int)
lzopro_lzo1x_decompress_a01     ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem /* NOT USED */ );
LZO_EXTERN(int)
lzopro_lzo1x_decompress_a02     ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem /* NOT USED */ );
LZO_EXTERN(int)
lzopro_lzo1x_decompress_a03     ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem /* NOT USED */ );
LZO_EXTERN(int)
lzopro_lzo1x_decompress_a04     ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem /* NOT USED */ );
LZO_EXTERN(int)
lzopro_lzo1x_decompress_a05     ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem /* NOT USED */ );
LZO_EXTERN(int)
lzopro_lzo1x_decompress_a06     ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem /* NOT USED */ );
LZO_EXTERN(int)
lzopro_lzo1x_decompress_a07     ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem /* NOT USED */ );


/***********************************************************************
// good compressors
************************************************************************/

/* NOTE: you must provide nalloc/nfree callbacks via 'cb' */

LZO_EXTERN(int)
lzopro_lzo1x_99_compress       ( const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_callback_p cb,
                                       int compression_level /* 1..10 */ );


/***********************************************************************
// fast compressors
// [the new "w02" and "w03" compressors below are probably preferable]
************************************************************************/

#define LZOPRO_LZO1X_1_06_MEM_COMPRESS    (256)

LZO_EXTERN(int)
lzopro_lzo1x_1_06_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_1_07_MEM_COMPRESS    (512)

LZO_EXTERN(int)
lzopro_lzo1x_1_07_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_1_08_MEM_COMPRESS    (1 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_1_08_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_1_09_MEM_COMPRESS    (2 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_1_09_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_1_10_MEM_COMPRESS    (4 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_1_10_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_1_11_MEM_COMPRESS    (8 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_1_11_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_1_12_MEM_COMPRESS    (16 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_1_12_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_1_13_MEM_COMPRESS    (32 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_1_13_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_1_14_MEM_COMPRESS    (64 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_1_14_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_1_15_MEM_COMPRESS    (128 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_1_15_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_1_16_MEM_COMPRESS    (256 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_1_16_compress      ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


/***********************************************************************
// VERY FAST compressors "w02".
// Currently only implemented for i386 (aka "x86") and amd64 (aka "x64").
************************************************************************/

#if defined(LZO_ARCH_AMD64) || defined(LZO_ARCH_I386)

#define LZOPRO_LZO1X_W02_11_MEM_COMPRESS  (8 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_w02_11_compress    ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_W02_12_MEM_COMPRESS  (16 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_w02_12_compress    ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_W02_13_MEM_COMPRESS  (32 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_w02_13_compress    ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_W02_14_MEM_COMPRESS  (64 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_w02_14_compress    ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );

#endif /* defined(LZO_ARCH_AMD64) || defined(LZO_ARCH_I386) */


/***********************************************************************
// EXTREMELY FAST compressors "w03" - these trade a little bit
//   of compression ratio for even more compression speed.
// Currently only implemented for i386 (aka "x86") and amd64 (aka "x64").
************************************************************************/

#if defined(LZO_ARCH_AMD64) || defined(LZO_ARCH_I386)

#define LZOPRO_LZO1X_W03_11_MEM_COMPRESS  (4 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_w03_11_compress    ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_W03_12_MEM_COMPRESS  (8 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_w03_12_compress    ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_W03_13_MEM_COMPRESS  (16 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_w03_13_compress    ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_W03_14_MEM_COMPRESS  (32 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_w03_14_compress    ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );


#define LZOPRO_LZO1X_W03_15_MEM_COMPRESS  (64 * 1024UL)

LZO_EXTERN(int)
lzopro_lzo1x_w03_15_compress    ( const lzo_bytep src, lzo_uint  src_len,
                                        lzo_bytep dst, lzo_uintp dst_len,
                                        lzo_voidp wrkmem );

#endif /* defined(LZO_ARCH_AMD64) || defined(LZO_ARCH_I386) */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */


/* vim:set ts=4 et: */
