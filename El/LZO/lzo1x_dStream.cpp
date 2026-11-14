/* lzo1x_dStream.c -- LZO1X decompression

Modification of LZO decompression, using QIStream on the input

*/

#include "El/elementpch.hpp"
#include "El/QStream/qStream.hpp"
#include <El/Credits/credits.hpp>

// modified content of "lzo1x_d1.c"

#include "config1x.h"
#undef LZO_TEST_OVERRUN
#include <El/LZO/lzo/lzopro/lzo1x.h>
#include <El/LZO/lzo1_d.ch>
#include <El/LZO/plzo1_d_fc.h>

// modified content of "lzo1x_d.ch"

// LZO_UNALIGNED_OK_4 is expected
#undef __COPY4
#define __COPY4(dst,src)    * (lzo_uint32p)(dst) = * (const lzo_uint32p)(src)

#undef COPY4
#define COPY4(dst,src)    __COPY4(dst,src)

#undef READ4
#define READ4(dst, in) in.read(dst, 4)

#if CONFIG_COMPACT_STATES
#  define STATE_M1      0x000
#  define STATE_LR      0x100
#  define STATE_MX      0x110
#  define CASE(n) case STATE_M1 + n
#else
#  define STATE_M1      0x000
#  define STATE_LR      0x100
#  define STATE_MX      0x200
#  define CASE(n) case STATE_M1 + n: case STATE_LR + n: case STATE_MX + n
#endif

#if defined(LZO1Z)
#  define UPDATE_LM     last_m_off = pd(op, m_pos)
#else
#  define UPDATE_LM     /*empty*/
#endif

#if defined(LZO1Z)
#  define GET_B14(p)      ((*(p) << 6) + (*((p)+1) >> 2))
#elif 1 && defined(acc_UA_GET16) && (acc_ABI_LITTLE_ENDIAN)
#  define GET_B14(p)        (acc_UA_GET16(p) >> 2)
#else
#  define GET_B14(p)        ((*(p) >> 2) + (*((p)+1) << 6))
#endif

/***********************************************************************
// decompress a block of data.
************************************************************************/

#pragma optimize("t",on)

LZO_PUBLIC(int)
  lzopro_lzo1x_decompress_from_stream(QIStream &in,
  lzo_bytep out, lzo_uintp out_len,
  lzo_voidp wrkmem)
{

// Custom macros overriding the LZO (pro) magic for working on QiStream rather than lzo_bytep* as source data
// All macros starting with X override a proprietary LZO macro without the 'X' prefix. These macros are platform-specific and optimized FOR PERFORMANCE.
// Change of platform might require rewriting them.

// If this code is called, just read and disable the second part
#define SEEKIN(offset) in.seekg(offset,QIOS::cur); WarningThisIsClusterFuckForPerformance()

// Relative seek in QiStream for cases when previous bytes must be read
#if POFF==0
#define SEEKINPOFF
#define SEEKINMINUSPOFF
#else
#define SEEKINPOFF SEEKIN(POFF)
#define SEEKINMINUSPOFF SEEKIN(-POFF)
#endif

// Reading current byte and advancing "pointer"
#define XPINC (in.get())
#define XPIP ((lzo_uint) XPINC)
#define XPIP_BYTE XPINC

// Overriding of some hardcore bitwise operation
#if POFF==0
#define GET_B14_SEEK_2 ((acc_uint16e_t)((minus2posOptimized = XPINC) | (((acc_uint16e_t)XPINC) << 8)) >> 2)
#else
#define NotImplementedForNonZeroPOFF() /* The macro above returns a value. If POFF is nonzero, SEEKINPOFF must be called BEFORE this, SEEKINMINUSPOFF afterwards */
#endif

// Read n bytes from the source data
#define LOADBYTES_NO_BUFFER(n,d,x) in.read(d+x,n)
#define X__FC_L_FAST(n)   SEEKINPOFF; LOADBYTES_NO_BUFFER(n,op,POFF); SEEKINMINUSPOFF

// Low level operations working on stream rather than byte array
#define X_FC_L(n,f)      __FC_CTA_NF(n,f); X__FC_L_FAST(n); op += n;
#define X_FC_T1_L(n,f,t) \
  X_FC_L(n,f); \
  if (t >= 4) do { \
  X__FC_L_FAST(4); op += 4; t -= 4; \
  } while (t >= 4); \
  if (t) do POP = XPIP_BYTE; while (--t > 0);
#define XFC_T1_L(n,f,t)  FC_L_CHECK(n,f,t,1);   X_FC_T1_L(n,f,t)
#define XFC_L(n,f) FC_L_CHECK(n,f,0,0); X_FC_L(n,f);
#define XP_LOOP_255(t,add) \
  do { t += 255; NEED_IP(1+3); } while ((minus1loopOptimized = XPINC) == 0); t += add - 255 + minus1loopOptimized;

  // End of custom macros

  lzo_bytep op;
  unsigned state;

#if defined(HAVE_ANY_OP)
  lzo_bytep const op_end = out + *out_len - POFF;
#endif
#if defined(LZO1Z)
  lzo_uint last_m_off = 0;
#endif

  ACC_UNUSED(wrkmem);

  *out_len = 0;

  op = out - POFF;

  lzo_byte minus2posOptimized;
  lzo_byte minus1loopOptimized;

  SEEKINPOFF;

  state = STATE_LR;

  NEED_IP(3);
  {
    SEEKINPOFF;
    lzo_uint t = in.get();
    SEEKINMINUSPOFF;
    in.unget();

    if (t > 17)
    {
      in.get();
      t -= 17;
      
      state = t >= 4 ? STATE_MX : STATE_M1;
#if 1
      XFC_T1_L(0,0,t);
#else
      NEED_OP(t); NEED_IP(t);
      do POP = PIP_BYTE; while (--t > 0);
#endif
    }
  }

  for (;;) {
#if 1 && (LZO_CC_GNUC && (LZO_CC_GNUC == 0x025b00ul))
    LZO_DEFINE_UNINITIALIZED_VAR(lzo_uint, t, 0);
#else
    lzo_uint t;
#endif
    const lzo_bytep m_pos;

    /* prefetch write cache for writing at "op" */
#if 0
    __builtin_prefetch(op, 1);  /* gcc */
#endif
#if 0 && (LZO_ARCH_ARM)
    /* !!! WARNING: this does overrun "op" by one byte !!! */
    (void) * (volatile char *) op;
#endif

    NEED_IP(3);
    t = XPIP;

#if CONFIG_COMPACT_STATES
    if (t >= 16)
      state = STATE_M1;
#endif
    state += (unsigned) t;
    
    switch (state)
    {

      /* .LR long literal */
    case STATE_LR + 0x00:
      /* note: t == 0 */
#if 0 && (ACC_ARCH_SPU && ACC_CC_GNUC)
      /* FIXME: avoid bug in spu-toolchain-2.3-bsc2.0:
      * "Error: Relocation doesn't fit." */
      t = 0; P_LOOP_255(ip, t, 0);
#else
      assert(t == 0); XP_LOOP_255(t, 0);
#endif
      XFC_T1_L(18,20,t);
      state = STATE_MX; continue;

    case STATE_LR+0x01: XFC_L(4,4); state=STATE_MX; continue;
    case STATE_LR+0x02: XFC_L(5,8); state=STATE_MX; continue;
    case STATE_LR+0x03: XFC_L(6,8); state=STATE_MX; continue;
    case STATE_LR+0x04: XFC_L(7,8); state=STATE_MX; continue;
    case STATE_LR+0x05: XFC_L(8,8); state=STATE_MX; continue;
    case STATE_LR+0x06: XFC_L(9,12); state=STATE_MX; continue;
    case STATE_LR+0x07: XFC_L(10,12); state=STATE_MX; continue;
    case STATE_LR+0x08: XFC_L(11,12); state=STATE_MX; continue;
    case STATE_LR+0x09: XFC_L(12,12); state=STATE_MX; continue;
    case STATE_LR+0x0a: XFC_L(13,16); state=STATE_MX; continue;
    case STATE_LR+0x0b: XFC_L(14,16); state=STATE_MX; continue;
    case STATE_LR+0x0c: XFC_L(15,16); state=STATE_MX; continue;
    case STATE_LR+0x0d: XFC_L(16,16); state=STATE_MX; continue;
    case STATE_LR+0x0e: XFC_L(17,20); state=STATE_MX; continue;
    case STATE_LR+0x0f: XFC_L(18,20); state=STATE_MX; continue;


      /* .MX match */
    case STATE_MX+0x00: m_pos = op-2049U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_0;
    case STATE_MX+0x01: m_pos = op-2049U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_1;
    case STATE_MX+0x02: m_pos = op-2049U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_2;
    case STATE_MX+0x03: m_pos = op-2049U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_3;
    case STATE_MX+0x04: m_pos = op-2050U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_0;
    case STATE_MX+0x05: m_pos = op-2050U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_1;
    case STATE_MX+0x06: m_pos = op-2050U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_2;
    case STATE_MX+0x07: m_pos = op-2050U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_3;
    case STATE_MX+0x08: m_pos = op-2051U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_0;
    case STATE_MX+0x09: m_pos = op-2051U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_1;
    case STATE_MX+0x0a: m_pos = op-2051U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_2;
    case STATE_MX+0x0b: m_pos = op-2051U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_3;
    case STATE_MX+0x0c: m_pos = op-2052U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_0;
    case STATE_MX+0x0d: m_pos = op-2052U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_1;
    case STATE_MX+0x0e: m_pos = op-2052U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_2;
    case STATE_MX+0x0f: m_pos = op-2052U-(XPIP<<2); UPDATE_LM; FC_M4(3,4); goto next_state_3;



      /* .M1 match */
    case STATE_M1+0x00: m_pos = op-1U-(XPIP<<2); UPDATE_LM; FC_M1(2,4); goto next_state_0;
    case STATE_M1+0x01: m_pos = op-1U-(XPIP<<2); UPDATE_LM; FC_M1(2,4); goto next_state_1;
    case STATE_M1+0x02: m_pos = op-1U-(XPIP<<2); UPDATE_LM; FC_M1(2,4); goto next_state_2;
    case STATE_M1+0x03: m_pos = op-1U-(XPIP<<2); UPDATE_LM; FC_M1(2,4); goto next_state_3;
    case STATE_M1+0x04: m_pos = op-2U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_0;
    case STATE_M1+0x05: m_pos = op-2U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_1;
    case STATE_M1+0x06: m_pos = op-2U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_2;
    case STATE_M1+0x07: m_pos = op-2U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_3;
    case STATE_M1+0x08: m_pos = op-3U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_0;
    case STATE_M1+0x09: m_pos = op-3U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_1;
    case STATE_M1+0x0a: m_pos = op-3U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_2;
    case STATE_M1+0x0b: m_pos = op-3U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_3;
    case STATE_M1+0x0c: m_pos = op-4U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_0;
    case STATE_M1+0x0d: m_pos = op-4U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_1;
    case STATE_M1+0x0e: m_pos = op-4U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_2;
    case STATE_M1+0x0f: m_pos = op-4U-(XPIP<<2); UPDATE_LM; FC_M4(2,4); goto next_state_3;


      /* .M4a match */

      CASE(0x10): t=0; XP_LOOP_255(t,0); m_pos = op-16384U-GET_B14_SEEK_2; UPDATE_LM; FC_T1_M8(9,12,t); goto next_state;
      CASE(0x18): t=0; XP_LOOP_255(t,0); m_pos = op-32768U-GET_B14_SEEK_2; UPDATE_LM; FC_T1_M8(9,12,t); goto next_state;

      CASE(0x11):
      m_pos = op - GET_B14_SEEK_2;
      if (m_pos == op) goto eof_found;
      m_pos -= 0x4000;
      UPDATE_LM; FC_M4(3,4);
      goto next_state;

      /* .M4b match */

      CASE(0x12): m_pos = op-16384U-GET_B14_SEEK_2; UPDATE_LM; FC_M4(4,4); goto next_state;
      CASE(0x13): m_pos = op-16384U-GET_B14_SEEK_2; UPDATE_LM; FC_M8(5,8); goto next_state;
      CASE(0x14): m_pos = op-16384U-GET_B14_SEEK_2; UPDATE_LM; FC_M8(6,8); goto next_state;
      CASE(0x15): m_pos = op-16384U-GET_B14_SEEK_2; UPDATE_LM; FC_M8(7,8); goto next_state;
      CASE(0x16): m_pos = op-16384U-GET_B14_SEEK_2; UPDATE_LM; FC_M8(8,8); goto next_state;
      CASE(0x17): m_pos = op-16384U-GET_B14_SEEK_2; UPDATE_LM; FC_M8(9,12); goto next_state;
      CASE(0x19): m_pos = op-32768U-GET_B14_SEEK_2; UPDATE_LM; FC_M4(3,4); goto next_state;
      CASE(0x1a): m_pos = op-32768U-GET_B14_SEEK_2; UPDATE_LM; FC_M4(4,4); goto next_state;
      CASE(0x1b): m_pos = op-32768U-GET_B14_SEEK_2; UPDATE_LM; FC_M8(5,8); goto next_state;
      CASE(0x1c): m_pos = op-32768U-GET_B14_SEEK_2; UPDATE_LM; FC_M8(6,8); goto next_state;
      CASE(0x1d): m_pos = op-32768U-GET_B14_SEEK_2; UPDATE_LM; FC_M8(7,8); goto next_state;
      CASE(0x1e): m_pos = op-32768U-GET_B14_SEEK_2; UPDATE_LM; FC_M8(8,8); goto next_state;
      CASE(0x1f): m_pos = op-32768U-GET_B14_SEEK_2; UPDATE_LM; FC_M8(9,12); goto next_state;


      /* .M3 match */
      CASE(0x20):
      t = 0; XP_LOOP_255(t, 0);
      m_pos = op - 1 - GET_B14_SEEK_2;
      UPDATE_LM; FC_T1_M1(33,36,t);
      goto next_state;

      CASE(0x21): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(3,4); goto next_state;
      CASE(0x22): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(4,4); goto next_state;
      CASE(0x23): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(5,8); goto next_state;
      CASE(0x24): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(6,8); goto next_state;
      CASE(0x25): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(7,8); goto next_state;
      CASE(0x26): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(8,8); goto next_state;
      CASE(0x27): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(9,12); goto next_state;
      CASE(0x28): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(10,12); goto next_state;
      CASE(0x29): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(11,12); goto next_state;
      CASE(0x2a): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(12,12); goto next_state;
      CASE(0x2b): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(13,16); goto next_state;
      CASE(0x2c): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(14,16); goto next_state;
      CASE(0x2d): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(15,16); goto next_state;
      CASE(0x2e): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(16,16); goto next_state;
      CASE(0x2f): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(17,20); goto next_state;
      CASE(0x30): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(18,20); goto next_state;
      CASE(0x31): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(19,20); goto next_state;
      CASE(0x32): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(20,20); goto next_state;
      CASE(0x33): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(21,24); goto next_state;
      CASE(0x34): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(22,24); goto next_state;
      CASE(0x35): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(23,24); goto next_state;
      CASE(0x36): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(24,24); goto next_state;
      CASE(0x37): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(25,28); goto next_state;
      CASE(0x38): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(26,28); goto next_state;
      CASE(0x39): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(27,28); goto next_state;
      CASE(0x3a): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(28,28); goto next_state;
      CASE(0x3b): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(29,32); goto next_state;
      CASE(0x3c): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(30,32); goto next_state;
      CASE(0x3d): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(31,32); goto next_state;
      CASE(0x3e): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(32,32); goto next_state;
      CASE(0x3f): m_pos = op-1U-GET_B14_SEEK_2; UPDATE_LM; FC_M1(33,36); goto next_state;


      /* .M2 match */

      CASE(0x40): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(3,4); goto next_state_0;
      CASE(0x41): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(3,4); goto next_state_1;
      CASE(0x42): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(3,4); goto next_state_2;
      CASE(0x43): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(3,4); goto next_state_3;
      CASE(0x44): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(3,4); goto next_state_0;
      CASE(0x45): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(3,4); goto next_state_1;
      CASE(0x46): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(3,4); goto next_state_2;
      CASE(0x47): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(3,4); goto next_state_3;
      CASE(0x48): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_0;
      CASE(0x49): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_1;
      CASE(0x4a): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_2;
      CASE(0x4b): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_3;
      CASE(0x4c): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_0;
      CASE(0x4d): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_1;
      CASE(0x4e): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_2;
      CASE(0x4f): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_3;
      CASE(0x50): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_0;
      CASE(0x51): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_1;
      CASE(0x52): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_2;
      CASE(0x53): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_3;
      CASE(0x54): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_0;
      CASE(0x55): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_1;
      CASE(0x56): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_2;
      CASE(0x57): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_3;
      CASE(0x58): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_0;
      CASE(0x59): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_1;
      CASE(0x5a): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_2;
      CASE(0x5b): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_3;
      CASE(0x5c): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_0;
      CASE(0x5d): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_1;
      CASE(0x5e): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_2;
      CASE(0x5f): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M4(3,4); goto next_state_3;
      CASE(0x60): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(4,4); goto next_state_0;
      CASE(0x61): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(4,4); goto next_state_1;
      CASE(0x62): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(4,4); goto next_state_2;
      CASE(0x63): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(4,4); goto next_state_3;
      CASE(0x64): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(4,4); goto next_state_0;
      CASE(0x65): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(4,4); goto next_state_1;
      CASE(0x66): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(4,4); goto next_state_2;
      CASE(0x67): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(4,4); goto next_state_3;
      CASE(0x68): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(4,4); goto next_state_0;
      CASE(0x69): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(4,4); goto next_state_1;
      CASE(0x6a): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(4,4); goto next_state_2;
      CASE(0x6b): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(4,4); goto next_state_3;
      CASE(0x6c): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_0;
      CASE(0x6d): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_1;
      CASE(0x6e): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_2;
      CASE(0x6f): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_3;
      CASE(0x70): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_0;
      CASE(0x71): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_1;
      CASE(0x72): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_2;
      CASE(0x73): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_3;
      CASE(0x74): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_0;
      CASE(0x75): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_1;
      CASE(0x76): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_2;
      CASE(0x77): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_3;
      CASE(0x78): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_0;
      CASE(0x79): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_1;
      CASE(0x7a): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_2;
      CASE(0x7b): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_3;
      CASE(0x7c): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_0;
      CASE(0x7d): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_1;
      CASE(0x7e): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_2;
      CASE(0x7f): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M4(4,4); goto next_state_3;
      CASE(0x80): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(5,8); goto next_state_0;
      CASE(0x81): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(5,8); goto next_state_1;
      CASE(0x82): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(5,8); goto next_state_2;
      CASE(0x83): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(5,8); goto next_state_3;
      CASE(0x84): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(5,8); goto next_state_0;
      CASE(0x85): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(5,8); goto next_state_1;
      CASE(0x86): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(5,8); goto next_state_2;
      CASE(0x87): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(5,8); goto next_state_3;
      CASE(0x88): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(5,8); goto next_state_0;
      CASE(0x89): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(5,8); goto next_state_1;
      CASE(0x8a): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(5,8); goto next_state_2;
      CASE(0x8b): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(5,8); goto next_state_3;
      CASE(0x8c): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(5,8); goto next_state_0;
      CASE(0x8d): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(5,8); goto next_state_1;
      CASE(0x8e): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(5,8); goto next_state_2;
      CASE(0x8f): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(5,8); goto next_state_3;
      CASE(0x90): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_0;
      CASE(0x91): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_1;
      CASE(0x92): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_2;
      CASE(0x93): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_3;
      CASE(0x94): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_0;
      CASE(0x95): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_1;
      CASE(0x96): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_2;
      CASE(0x97): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_3;
      CASE(0x98): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_0;
      CASE(0x99): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_1;
      CASE(0x9a): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_2;
      CASE(0x9b): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_3;
      CASE(0x9c): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_0;
      CASE(0x9d): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_1;
      CASE(0x9e): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_2;
      CASE(0x9f): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(5,8); goto next_state_3;
      CASE(0xa0): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(6,8); goto next_state_0;
      CASE(0xa1): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(6,8); goto next_state_1;
      CASE(0xa2): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(6,8); goto next_state_2;
      CASE(0xa3): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(6,8); goto next_state_3;
      CASE(0xa4): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(6,8); goto next_state_0;
      CASE(0xa5): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(6,8); goto next_state_1;
      CASE(0xa6): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(6,8); goto next_state_2;
      CASE(0xa7): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(6,8); goto next_state_3;
      CASE(0xa8): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(6,8); goto next_state_0;
      CASE(0xa9): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(6,8); goto next_state_1;
      CASE(0xaa): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(6,8); goto next_state_2;
      CASE(0xab): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(6,8); goto next_state_3;
      CASE(0xac): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(6,8); goto next_state_0;
      CASE(0xad): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(6,8); goto next_state_1;
      CASE(0xae): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(6,8); goto next_state_2;
      CASE(0xaf): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(6,8); goto next_state_3;
      CASE(0xb0): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(6,8); goto next_state_0;
      CASE(0xb1): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(6,8); goto next_state_1;
      CASE(0xb2): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(6,8); goto next_state_2;
      CASE(0xb3): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(6,8); goto next_state_3;
      CASE(0xb4): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_0;
      CASE(0xb5): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_1;
      CASE(0xb6): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_2;
      CASE(0xb7): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_3;
      CASE(0xb8): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_0;
      CASE(0xb9): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_1;
      CASE(0xba): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_2;
      CASE(0xbb): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_3;
      CASE(0xbc): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_0;
      CASE(0xbd): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_1;
      CASE(0xbe): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_2;
      CASE(0xbf): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(6,8); goto next_state_3;
      CASE(0xc0): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(7,8); goto next_state_0;
      CASE(0xc1): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(7,8); goto next_state_1;
      CASE(0xc2): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(7,8); goto next_state_2;
      CASE(0xc3): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(7,8); goto next_state_3;
      CASE(0xc4): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(7,8); goto next_state_0;
      CASE(0xc5): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(7,8); goto next_state_1;
      CASE(0xc6): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(7,8); goto next_state_2;
      CASE(0xc7): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(7,8); goto next_state_3;
      CASE(0xc8): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(7,8); goto next_state_0;
      CASE(0xc9): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(7,8); goto next_state_1;
      CASE(0xca): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(7,8); goto next_state_2;
      CASE(0xcb): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(7,8); goto next_state_3;
      CASE(0xcc): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_0;
      CASE(0xcd): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_1;
      CASE(0xce): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_2;
      CASE(0xcf): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_3;
      CASE(0xd0): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_0;
      CASE(0xd1): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_1;
      CASE(0xd2): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_2;
      CASE(0xd3): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_3;
      CASE(0xd4): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_0;
      CASE(0xd5): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_1;
      CASE(0xd6): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_2;
      CASE(0xd7): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(7,8); goto next_state_3;
      CASE(0xd8): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(7,8); goto next_state_0;
      CASE(0xd9): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(7,8); goto next_state_1;
      CASE(0xda): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(7,8); goto next_state_2;
      CASE(0xdb): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M8(7,8); goto next_state_3;
      CASE(0xdc): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(7,8); goto next_state_0;
      CASE(0xdd): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(7,8); goto next_state_1;
      CASE(0xde): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(7,8); goto next_state_2;
      CASE(0xdf): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(7,8); goto next_state_3;
      CASE(0xe0): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(8,8); goto next_state_0;
      CASE(0xe1): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(8,8); goto next_state_1;
      CASE(0xe2): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(8,8); goto next_state_2;
      CASE(0xe3): m_pos = op-1U-(XPIP<<3); UPDATE_LM; FC_M1(8,8); goto next_state_3;
      CASE(0xe4): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(8,8); goto next_state_0;
      CASE(0xe5): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(8,8); goto next_state_1;
      CASE(0xe6): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(8,8); goto next_state_2;
      CASE(0xe7): m_pos = op-2U-(XPIP<<3); UPDATE_LM; FC_M2(8,8); goto next_state_3;
      CASE(0xe8): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(8,8); goto next_state_0;
      CASE(0xe9): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(8,8); goto next_state_1;
      CASE(0xea): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(8,8); goto next_state_2;
      CASE(0xeb): m_pos = op-3U-(XPIP<<3); UPDATE_LM; FC_M2(8,8); goto next_state_3;
      CASE(0xec): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_0;
      CASE(0xed): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_1;
      CASE(0xee): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_2;
      CASE(0xef): m_pos = op-4U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_3;
      CASE(0xf0): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_0;
      CASE(0xf1): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_1;
      CASE(0xf2): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_2;
      CASE(0xf3): m_pos = op-5U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_3;
      CASE(0xf4): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_0;
      CASE(0xf5): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_1;
      CASE(0xf6): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_2;
      CASE(0xf7): m_pos = op-6U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_3;
      CASE(0xf8): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_0;
      CASE(0xf9): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_1;
      CASE(0xfa): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_2;
      CASE(0xfb): m_pos = op-7U-(XPIP<<3); UPDATE_LM; FC_M4(8,8); goto next_state_3;
      CASE(0xfc): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(8,8); goto next_state_0;
      CASE(0xfd): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(8,8); goto next_state_1;
      CASE(0xfe): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(8,8); goto next_state_2;
      CASE(0xff): m_pos = op-8U-(XPIP<<3); UPDATE_LM; FC_M8(8,8); goto next_state_3;


#if 0 && !defined(NDEBUG)
    default:
      fprintf(stderr, "error %d %d\n", (int)state, (int)t); exit(1);
#endif


    }

next_state:
    state = STATE_LR;
#if defined(LZO1Z)
    t = ip[POFF-1] & 3;
#else

    t = minus2posOptimized & 3;
    /*
    SEEKIN(POFF-2);
    t = ((lzo_uint) in.get()) & 3;
    SEEKIN(-(1 + POFF - 2)); 
    */
#endif
    if (t) {
#if defined(CONFIG_FAST)
      /* See FC_L_CHECK() about NEED_OP() semantics */
      /* WARNING: this uses __FC_L() directly */
      NEED_OP(t); NEED_IP(4);
      __FC_L(4,0,0); op += t; ip += t;
#else
      NEED_OP(t); NEED_IP(4);
      do POP = XPIP_BYTE; while (--t > 0);
#endif
      state = STATE_M1;
    }
    continue;


next_state_0:
    state = STATE_LR;
    continue;
next_state_1:
    XFC_L(1,4);
    state = STATE_M1;
    continue;
next_state_2:
    XFC_L(2,4);
    state = STATE_M1;
    continue;
next_state_3:
    XFC_L(3,4);
    state = STATE_M1;
    continue;
  }


eof_found:
  assert(ip <= ip_end);
#if defined(HAVE_ANY_OP)
  assert(op <= op_end);
#endif
  *out_len = pd(op + POFF, out);
  return LZO_E_OK;

#if defined(HAVE_NEED_IP)
input_overrun:
  assert(ip <= ip_end); assert(op <= op_end);
  *out_len = pd(op + POFF, out);
  return LZO_E_INPUT_OVERRUN;
#endif

#if defined(HAVE_NEED_OP)
output_overrun:
  assert(ip <= ip_end); assert(op <= op_end);
  *out_len = pd(op + POFF, out);
  return LZO_E_OUTPUT_OVERRUN;
#endif

#if defined(LZO_TEST_OVERRUN_LOOKBEHIND)
lookbehind_overrun:
  assert(ip <= ip_end); assert(op <= op_end);
  *out_len = pd(op + POFF, out);
  return LZO_E_LOOKBEHIND_OVERRUN;
#endif
}

#pragma optimize("t",on)
LZO_PUBLIC(int)
  lzo1x_decompress_from_stream_old(QIStream &in,
  lzo_bytep out, lzo_uintp out_len,
  lzo_voidp wrkmem)

{
  register lzo_bytep op;
  register lzo_uint t;
  register lzo_uint temp, temp2;
  register lzo_uint matching;
#if defined(COPY_DICT)
  lzo_uint m_off;
  const lzo_bytep dict_end;
#else
  register const lzo_bytep m_pos;
#endif

#if defined(HAVE_ANY_OP)
  lzo_bytep const op_end = out + *out_len;
#endif
#if defined(LZO1Z)
  lzo_uint last_m_off = 0;
#endif

  LZO_UNUSED(wrkmem);

#if defined(COPY_DICT)
  if (dict)
  {
    if (dict_len > M4_MAX_OFFSET)
    {
      dict += dict_len - M4_MAX_OFFSET;
      dict_len = M4_MAX_OFFSET;
    }
    dict_end = dict + dict_len;
  }
  else
  {
    dict_len = 0;
    dict_end = NULL;
  }
#endif /* COPY_DICT */

  *out_len = 0;

  op = out;
  t = in.get();

  if (t > 17)
  {
    t -= 17;
    if (t < 4)
      goto match_next;
    assert(t > 0); NEED_OP(t); NEED_IP(t+1);
    do *op++ = in.get(); while (--t > 0);
    goto first_literal_run;
  }

  in.unget();

  while (TEST_IP && TEST_OP)
  {
    t = in.get();
    if (t >= 16)
      goto match;
    /* a literal run */
    if (t == 0)
    {
      NEED_IP(1);
      temp = in.get();
      while (temp == 0)
      {
        t += 255;
        NEED_IP(1);
        temp = in.get();
      }
      t += 15 + temp;
    }
    /* copy literals */
    assert(t > 0); NEED_OP(t+3); NEED_IP(t+4);
    READ4(op, in);
    op += 4;
    if (--t > 0)
    {
      if (t >= 4)
      {
        do {
          READ4(op, in);
          op += 4; t -= 4;
        } while (t >= 4);
        if (t > 0) do *op++ = in.get(); while (--t > 0);
      }
      else
        do *op++ = in.get(); while (--t > 0);
    }

first_literal_run:


    t = in.get();
    if (t >= 16)
      goto match;
    matching = t;
#if defined(COPY_DICT)
#if defined(LZO1Z)
    m_off = (1 + M2_MAX_OFFSET) + (t << 6) + (in.get() >> 2);
    last_m_off = m_off;
#else
    m_off = (1 + M2_MAX_OFFSET) + (t >> 2) + (in.get() << 2);
#endif
    NEED_OP(3);
    t = 3; COPY_DICT(t,m_off)
#else /* !COPY_DICT */
#if defined(LZO1Z)
    t = (1 + M2_MAX_OFFSET) + (t << 6) + (in.get() >> 2);
    m_pos = op - t;
    last_m_off = t;
#else
    m_pos = op - (1 + M2_MAX_OFFSET);
    m_pos -= t >> 2;
    m_pos -= in.get() << 2;
#endif
    TEST_LB(m_pos); NEED_OP(3);
    *op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos;
#endif /* COPY_DICT */
    goto match_done;


    /* handle matches */
    do {
match:
      if (t >= 64)                /* a M2 match */
      {
        matching = t;
#if defined(COPY_DICT)
#if defined(LZO1X)
        m_off = 1 + ((t >> 2) & 7) + (in.get() << 3);
        t = (t >> 5) - 1;
#elif defined(LZO1Y)
        m_off = 1 + ((t >> 2) & 3) + (in.get() << 2);
        t = (t >> 4) - 3;
#elif defined(LZO1Z)
        m_off = t & 0x1f;
        if (m_off >= 0x1c)
          m_off = last_m_off;
        else
        {
          m_off = 1 + (m_off << 6) + (in.get() >> 2);
          last_m_off = m_off;
        }
        t = (t >> 5) - 1;
#endif
#else /* !COPY_DICT */
#if defined(LZO1X)
        m_pos = op - 1;
        m_pos -= (t >> 2) & 7;
        m_pos -= in.get() << 3;
        t = (t >> 5) - 1;
#elif defined(LZO1Y)
        m_pos = op - 1;
        m_pos -= (t >> 2) & 3;
        m_pos -= in.get() << 2;
        t = (t >> 4) - 3;
#elif defined(LZO1Z)
        {
          lzo_uint off = t & 0x1f;
          m_pos = op;
          if (off >= 0x1c)
          {
            assert(last_m_off > 0);
            m_pos -= last_m_off;
          }
          else
          {
            off = 1 + (off << 6) + (in.get() >> 2);
            m_pos -= off;
            last_m_off = off;
          }
        }
        t = (t >> 5) - 1;
#endif
        TEST_LB(m_pos); assert(t > 0); NEED_OP(t+3-1);
        goto copy_match;
#endif /* COPY_DICT */
      }
      else if (t >= 32)           /* a M3 match */
      {
        t &= 31;
        if (t == 0)
        {
          NEED_IP(1);
          temp = in.get();
          while (temp == 0)
          {
            t += 255;
            NEED_IP(1);
            temp = in.get();
          }
          t += 31 + temp;
        }

        NEED_IP(2);
        temp = in.get();
        temp2 = in.get();
        matching = temp;

#if defined(COPY_DICT)
#if defined(LZO1Z)
        m_off = 1 + (temp << 6) + (temp2 >> 2);
        last_m_off = m_off;
#else
        m_off = 1 + (temp >> 2) + (temp2 << 6);
#endif
#else /* !COPY_DICT */
#if defined(LZO1Z)
        {
          lzo_uint off = 1 + (temp << 6) + (temp2 >> 2);
          m_pos = op - off;
          last_m_off = off;
        }
#else
        m_pos = op - 1;
        m_pos -= (temp >> 2) + (temp2 << 6);
#endif
#endif /* COPY_DICT */
      }
      else if (t >= 16)           /* a M4 match */
      {
#if defined(COPY_DICT)
        m_off = (t & 8) << 11;
#else /* !COPY_DICT */
        m_pos = op;
        m_pos -= (t & 8) << 11;
#endif /* COPY_DICT */
        t &= 7;
        if (t == 0)
        {
          NEED_IP(1);
          temp = in.get();
          while (temp == 0)
          {
            t += 255;
            NEED_IP(1);
            temp = in.get();
          }
          t += 7 + temp;
        }
        temp = in.get();
        temp2 = in.get();
        matching = temp;
#if defined(COPY_DICT)
#if defined(LZO1Z)
        m_off += (temp << 6) + (temp2 >> 2);
#else
        m_off += (temp >> 2) + (temp2 << 6);
#endif
        if (m_off == 0)
          goto eof_found;
        m_off += 0x4000;
#if defined(LZO1Z)
        last_m_off = m_off;
#endif
#else /* !COPY_DICT */
#if defined(LZO1Z)
        m_pos -= (temp << 6) + (temp2 >> 2);
#else
        m_pos -= (temp >> 2) + (temp2 << 6);
#endif
        if (m_pos == op)
          goto eof_found;
        m_pos -= 0x4000;
#if defined(LZO1Z)
        last_m_off = pd((const lzo_bytep)op, m_pos);
#endif
#endif /* COPY_DICT */
      }
      else                            /* a M1 match */
      {
        matching = t;
#if defined(COPY_DICT)
#if defined(LZO1Z)
        m_off = 1 + (t << 6) + (in.get() >> 2);
        last_m_off = m_off;
#else
        m_off = 1 + (t >> 2) + (in.get() << 2);
#endif
        NEED_OP(2);
        t = 2; COPY_DICT(t,m_off)
#else /* !COPY_DICT */
#if defined(LZO1Z)
        t = 1 + (t << 6) + (in.get() >> 2);
        m_pos = op - t;
        last_m_off = t;
#else
        m_pos = op - 1;
        m_pos -= t >> 2;
        m_pos -= in.get() << 2;
#endif
        TEST_LB(m_pos); NEED_OP(2);
        *op++ = *m_pos++; *op++ = *m_pos;
#endif /* COPY_DICT */
        goto match_done;
      }

      /* copy match */
#if defined(COPY_DICT)

      NEED_OP(t+3-1);
      t += 3-1; COPY_DICT(t,m_off)

#else /* !COPY_DICT */

      TEST_LB(m_pos); assert(t > 0); NEED_OP(t+3-1);
#if defined(LZO_UNALIGNED_OK_4) || defined(LZO_ALIGNED_OK_4)
#if !defined(LZO_UNALIGNED_OK_4)
      if (t >= 2 * 4 - (3 - 1) && PTR_ALIGNED2_4(op,m_pos))
      {
        assert((op - m_pos) >= 4);  /* both pointers are aligned */
#else
      if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4)
      {
#endif
        COPY4(op,m_pos);
        op += 4; m_pos += 4; t -= 4 - (3 - 1);
        do {
          COPY4(op,m_pos);
          op += 4; m_pos += 4; t -= 4;
        } while (t >= 4);
        if (t > 0) do *op++ = *m_pos++; while (--t > 0);
      }
      else
#endif
      {
copy_match:
        *op++ = *m_pos++; *op++ = *m_pos++;
        do *op++ = *m_pos++; while (--t > 0);
      }

#endif /* COPY_DICT */

match_done:
#if defined(LZO1Z)
      // t = ip[-1] & 3;
      in.unget();
      t = in.get() & 3;
#else
      // t = ip[-2] & 3;
      // wanted value stored to "matching" in all branches
      t = matching & 3;
#endif
      if (t == 0)
        break;

      /* copy literals */
match_next:
      assert(t > 0); assert(t < 4); NEED_OP(t); NEED_IP(t+1);
#if 0
      do *op++ = in.get(); while (--t > 0);
#else
      *op++ = in.get();
      if (t > 1) { *op++ = in.get(); if (t > 2) { *op++ = in.get(); } }
#endif
      t = in.get();
    } while (TEST_IP && TEST_OP);
  }

#if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
  /* no EOF code was found */
  *out_len = pd(op, out);
  return LZO_E_EOF_NOT_FOUND;
#endif

eof_found:
  assert(t == 1);
  *out_len = pd(op, out);
  return LZO_E_OK;


#if defined(HAVE_NEED_IP)
input_overrun:
  *out_len = pd(op, out);
  return LZO_E_INPUT_OVERRUN;
#endif

#if defined(HAVE_NEED_OP)
output_overrun:
  *out_len = pd(op, out);
  return LZO_E_OUTPUT_OVERRUN;
#endif

#if defined(LZO_TEST_OVERRUN_LOOKBEHIND)
lookbehind_overrun:
  *out_len = pd(op, out);
  return LZO_E_LOOKBEHIND_OVERRUN;
#endif
}
/* vim:set ts=4 sw=4 et: */

REGISTER_CREDITS("LZO Professional", "1996-2011 Markus Franz Xaver Johannes Oberhumer","")