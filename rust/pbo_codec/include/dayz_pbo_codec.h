#ifndef DAYZ_PBO_CODEC_H
#define DAYZ_PBO_CODEC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t dayz_pbo_abi_version(void);

/*
 * Return values:
 * 0 success, 1 invalid arguments, 2 truncated input,
 * 3 output overflow, 4 checksum mismatch, 5 internal panic.
 * Buffers must not overlap. After any nonzero result, discard dst.
 */
int32_t dayz_pbo_decode_v1(
  const uint8_t *src,
  size_t src_len,
  uint8_t *dst,
  size_t dst_len
);

#ifdef __cplusplus
}
#endif

#endif
