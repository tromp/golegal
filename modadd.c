#include <stdint.h>

void modadd(uint64_t modulus, uint64_t *a, uint64_t b) {
  uint64_t c = *a+b;
  if (c < b || c >= modulus)
    c -= modulus;
  *a = c;
}

