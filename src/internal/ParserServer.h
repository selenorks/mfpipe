#ifndef UUID_89B59833_69A0_4D36_88E6_206F32F030CD
#define UUID_89B59833_69A0_4D36_88E6_206F32F030CD
#include "../MFTypes.h"
#include "MFPipe.h"
#include "serialization.h"
#include <atomic>
#include <mutex>
#include <queue>

enum class PacketType : std::uint8_t
{
  PacketHandShakes,
  PacketFinalizing,
  // end system packet, all below should have channel name
  PacketMessage,
  PacketObject,
};

struct PacketHeader
{
  PacketType type;
  uint32_t size;

  static int serializedSize() { return sizeof(PacketHeader); };
};

std::vector<uint8_t> static serializeHeader(PacketType type, const std::string& strChannel, const std::vector<uint8_t>& payload)
{
  std::vector<uint8_t> data;
  int size = PacketHeader::serializedSize() + sizeof(size_t) + strChannel.size() + payload.size();
  data.resize(size);

  uint8_t* ptr = data.data();

  PacketHeader header;
  header.type = type;
  header.size = size;

  copyField(ptr, &header, sizeof(header));
  copyData(ptr, strChannel.data(), strChannel.size());
  std::copy(payload.begin(), payload.end(), ptr);

  return data;
}

class Accumulator
{
public:
  Accumulator(size_t buffer_size)
      : m_buffer(buffer_size)
  {}

  char* dataEnd() { return m_buffer.data() + data_size; }
  size_t availableSize() { return m_buffer.size() - data_size; }
  void add(size_t size)
  {
    data_size += size;

    if (data_size >= sizeof(PacketHeader) && header()->size > m_buffer.size()) {
      m_buffer.resize(header()->size);
    }

    //            if (data_size >= sizeof(PacketHeader)) {
    //              std::cout << "Accumulator, data_size=" << data_size << " size=" << size << " type=" << int(header()->type) << std::endl;
    //            }
  }

  bool isMessageReady() const { return data_size && header()->size == data_size; }

  std::string channel()
  {
    if (header()->type <= PacketType::PacketFinalizing)
      return {};
    char* ptr = data() + sizeof(PacketHeader);
    size_t size = *(size_t*)ptr;
    ptr += sizeof(size_t);

    return std::string(ptr, ptr + size);
  }

  std::string_view payload()
  {
    char* ptr = data() + sizeof(PacketHeader);
    auto channel_size = *(size_t*)ptr;
    ptr += sizeof(size_t) + channel_size;

    std::string_view view{ ptr, header()->size - channel_size - sizeof(PacketHeader) };
    if (header()->size > data_size)
      throw std::bad_alloc();
    return view;
  }
  PacketType type() { return header()->type; }

  void reset() { data_size = 0; }

private:
  char* data() { return m_buffer.data(); }
  const PacketHeader* header() const { return reinterpret_cast<const PacketHeader*>(m_buffer.data()); }
  std::vector<char> m_buffer;
  size_t data_size = 0;
};

#endif
