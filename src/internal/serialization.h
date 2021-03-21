#ifndef UUID_D4F59CFF_154C_49C7_8AAE_1BF1045B5841
#define UUID_D4F59CFF_154C_49C7_8AAE_1BF1045B5841
#include <iostream>

static inline void
copyField(uint8_t*& ptr, const void* object, const size_t size)
{
  std::memcpy(ptr, object, size);
  ptr += size;
}

static inline void
copyData(uint8_t*& ptr, const void* object, const size_t size)
{
  std::memcpy(ptr, &size, sizeof(size_t));
  ptr += sizeof(size_t);

  std::memcpy(ptr, object, size);
  ptr += size;
}

static inline std::string
parseString(char const*& ptr)
{
  size_t size = *(size_t*)ptr;
  ptr += sizeof(size_t);

  std::string s;
  s.resize(size);
  std::memcpy(s.data(), ptr, size);
  ptr += size;

  return s;
}

static inline std::vector<uint8_t>
parseVector(char const*& ptr)
{
  auto object_size = *reinterpret_cast<const size_t*>(ptr);
  ptr += sizeof(size_t);

  std::vector<uint8_t> vec;
  vec.resize(object_size);
  std::memcpy(vec.data(), ptr, object_size);

  ptr += object_size;
  return vec;
}
#endif
