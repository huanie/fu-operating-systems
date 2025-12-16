#include "stddef.h"

extern "C" {
[[gnu::used]]
void *memcpy(void *dest, const void *src, size_t n) {
  const char *s = static_cast<const char *>(src);
  char *d = static_cast<char *>(dest);
  while (n--)
    *d++ = *s++;
  return dest;
}

[[gnu::used]]
void *memset(void *dest, int c, size_t n) {
  unsigned char *d = static_cast<unsigned char *>(dest);
  while (n--)
    *d++ = static_cast<unsigned char>(c);
  return dest;
}
}
