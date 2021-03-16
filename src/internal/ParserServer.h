#ifndef MFPIPE_PARSERSERVER_H
#define MFPIPE_PARSERSERVER_H
#include "../MFTypes.h"
#include "UDP.h"
#include "serialization.h"
#include <atomic>
#include <mutex>
#include <queue>
#include "MFPipe.h"

enum class PacketType : std::uint8_t
{
  PacketHandShakes,
  PacketMessage,
  PacketObject,
  PacketFinalizing
};

struct PacketHeader
{
  PacketType type;
  uint16_t size;

  static int serializedSize() { return sizeof(PacketHeader); };
};

std::vector<uint8_t> static serializeHeader(PacketType type, const std::string& strChannel, std::vector<uint8_t> paload)
{
  std::vector<uint8_t> data;
  int size = PacketHeader::serializedSize() + sizeof(size_t) + strChannel.size() + paload.size();
  data.resize(size);

  uint8_t* ptr = data.data();

  PacketHeader header;
  header.type = type;
  header.size = size;
  copyField(ptr, &header, sizeof(header));

  copyData(ptr, strChannel.data(), strChannel.size());

  std::copy(paload.begin(), paload.end(), ptr);

  return data;
}

struct Message : public MF_BASE_TYPE
{
  Message(const std::string strEventName, std::string strEventParam)
      : MF_BASE_TYPE()
      , strEventName(strEventName)
      , strEventParam(strEventParam)
  {}
  Message() = default;
  Message(const Message&) = default;
  Message(Message&&) = default;

  std::vector<uint8_t> serialize() const
  {
    std::vector<uint8_t> data;
    int size = sizeof(size_t) + strEventName.size() + sizeof(size_t) + strEventParam.size();
    data.resize(size);

    uint8_t* ptr = data.data();

    copyData(ptr, strEventName.data(), strEventName.size());
    copyData(ptr, strEventParam.data(), strEventParam.size());

    return data;
  }

  static std::unique_ptr<Message> deserialize(const char* data)
  {
    auto message = std::make_unique<Message>();

    message->strEventName = parseString(data);
    message->strEventParam = parseString(data);
    return message;
  }

  std::string strEventName;
  std::string strEventParam;
};

const int MAX_MESSAGE_SIZE = MAX_BUFFER_SIZE;

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

    if (data_size >= sizeof(PacketHeader)) {
      std::cout << "Accumulator, data_size=" << data_size << " size=" << size << " type=" << int(header()->type) << std::endl;
    }
  }

  bool isMessageReady() const { return data_size && header()->size == data_size; }

  char* data() { return m_buffer.data(); }

  std::string channel()
  {
    char* ptr = data() + sizeof(PacketHeader);
    size_t size = *(size_t*)ptr;
    ptr += sizeof(size_t);

    return std::string(ptr, ptr + size);
  }

  char* payload()
  {
    char* ptr = m_buffer.data() + sizeof(PacketHeader);
    auto channel_size = *(size_t*)ptr;
    ptr += sizeof(size_t) + channel_size;
    return ptr;
  }

  void reset() { data_size = 0; }

  const PacketHeader* header() const { return reinterpret_cast<const PacketHeader*>(m_buffer.data()); }

private:
  std::vector<char> m_buffer;
  size_t data_size = 0;
};


#endif
