#ifndef UUID_0068BFE0_54EC_429D_B0F2_923D50F6690D
#define UUID_0068BFE0_54EC_429D_B0F2_923D50F6690D
#include <cstdio>
#include <cstdint>
#include <string>

struct IPipe
{
  virtual bool create() = 0;
  virtual size_t read(void* buffer, uint32_t buffer_len) = 0;
  virtual size_t write(const void* buffer, uint32_t buffer_len) = 0;
  virtual ~IPipe() {}
};
#endif
