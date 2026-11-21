#include "../include/dayz_pbo_codec.h"

int main()
{
  const uint8_t compressed[] = {0x07, 'A', 'B', 'C', 0xC6, 0x00, 0x00, 0x00};
  uint8_t output[] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC};

  if (dayz_pbo_abi_version() != 1) return 1;
  if (dayz_pbo_decode_v1(compressed, sizeof(compressed), output + 1, 3) != 0) return 2;
  if (output[0] != 0xCC || output[1] != 'A' || output[2] != 'B' ||
      output[3] != 'C' || output[4] != 0xCC) return 3;
  if (dayz_pbo_decode_v1(compressed, 1, output + 1, 3) != 2) return 4;
  return 0;
}
