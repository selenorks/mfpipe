#ifndef MFPIPE_SERIALIZATION_H
#define MFPIPE_SERIALIZATION_H

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
parseString(char const *& ptr)
{
  size_t size = *(size_t*)ptr;
  ptr += sizeof(size_t);
  std::string s{ ptr, size };
  ptr += size;
  return s;
}

static inline std::vector<uint8_t>
parseVector(char const *& ptr)
{
  auto object_size = *reinterpret_cast<const size_t*>(ptr);
  ptr += sizeof(size_t);
  std::vector<uint8_t> vec = { ptr, ptr + object_size };
  ptr += object_size;
  return vec;

}
#endif // MFPIPE_SERIALIZATION_H
