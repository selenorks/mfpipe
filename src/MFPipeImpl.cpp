#ifndef MF_PIPEIMPL_H_
#define MF_PIPEIMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "MFPipe.h"
#include "MFTypes.h"

#include "internal/ParserServer.h"
#include "internal/UDP.h"
#include <regex>

struct AddressInfo
{
  std::string pipe_protocol;
  std::string address;
  std::string port; // can also be a name of the service
  static bool fromAddress(const std::string& strPipeID, AddressInfo& info)
  {
    const std::string udp{ "udp" };

    std::regex rgx("([A-z]+):\\/\\/([^:]+):([A-z0-9]+)");

    std::smatch match;

    std::string protocol;
    std::string address;
    std::string port;
    if (std::regex_search(strPipeID.begin(), strPipeID.end(), match, rgx)) {
      info.pipe_protocol = match[1];
      std::transform(info.pipe_protocol.begin(), info.pipe_protocol.end(), info.pipe_protocol.begin(), [](char c) { return std::tolower(c); });
      info.address = match[2];
      info.port = match[3];
      return true;
    }

    return false;
  }
};

class MFPipeImpl : public MFPipe
{
public:
  explicit MFPipeImpl()=default;

  MFPipeImpl(const MFPipeImpl& other) = delete;

  virtual bool init()
  {
    if (!m_pipe->create())
      return false;

    // what when old thread is finished in case of reopening pipe
    if (!is_running && m_thread)
      m_thread->join();

    is_running = true;
    m_thread = std::make_unique<std::thread>([&]() { this->work_thread(); });

    if (MF_PIPE_MODE::PIPE_READER == m_pipe_mode)
      return sendHandshake();
    return true;
  }

  virtual void work_thread()
  {
    Accumulator parser(MAX_MESSAGE_SIZE);

    while (is_running) {
      size_t read_len = m_pipe->read(parser.dataEnd(), parser.availableSize());
      if (!read_len) {
        std::this_thread::yield();
        continue;
      }
      parser.add(read_len);
      if (parser.isMessageReady()) {
        parse(parser);
        parser.reset();
      }
    }
  }

  void finish() { is_running = false; }

  void parse(Accumulator& parser)
  {
    switch (parser.header()->type) {
      case PacketType::PacketHandShakes:
        if (!is_connected)
          sendHandshake();
        is_connected = true;
        break;
      case PacketType::PacketMessage: {
        std::string channel = parser.channel();

        int object_count = object_counter++;
        if (m_nMaxBuffers > 0 && object_count >= m_nMaxBuffers) {
          info.nMessagesDropped++;
          object_counter--;
          break;
        }

        std::unique_ptr<MF_BASE_TYPE> object = Message::deserialize(parser.payload());

        std::unique_lock lock(m_channels_data_mutex);
        auto iter = m_channels_data.find(channel);
        if (iter == m_channels_data.end()) {
          ChannelData data;
          data.messages.emplace(std::move(object));
          m_channels_data.try_emplace(channel, std::move(data));
        } else {
          iter->second.messages.emplace(std::move(object));
        }
        lock.unlock();
        break;
      }
      case PacketType::PacketObject: {
        std::string channel = parser.channel();

        int object_count = object_counter++;
        if (m_nMaxBuffers > 0 && object_count >= m_nMaxBuffers) {
          info.nObjectsDropped++;
          object_counter--;
          break;
        }

        std::unique_ptr<MF_BASE_TYPE> object = MF_BASE_TYPE::deserialize(parser.payload());

        std::unique_lock lock(m_channels_data_mutex);
        auto iter = m_channels_data.find(channel);
        if (iter == m_channels_data.end()) {
          ChannelData data;
          data.objects.emplace(std::move(object));
          m_channels_data.try_emplace(channel, std::move(data));
        } else {
          iter->second.objects.emplace(std::move(object));
        }
        lock.unlock();
        break;
      }

      case PacketType::PacketFinalizing:
        finish();
        break;
      default:
        std::cerr << "Got unknown object, type: " << int(parser.header()->type) << std::endl;
        break;
    }
  }

  HRESULT PipeGet(/*[in]*/ const std::string& strChannel,
                  /*[out]*/ std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
                  /*[in]*/ int _nMaxWaitMs,
                  /*[in]*/ const std::string& strHints) override
  {

    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(_nMaxWaitMs);
    while (std::chrono::steady_clock::now() < timeout) {
      std::unique_lock lock(m_channels_data_mutex, std::defer_lock);

      if (!lock.try_lock_until(timeout))
        return S_FALSE;

      if (m_channels_data.empty())
        continue;

      auto iter = m_channels_data.find(strChannel);
      if (iter == m_channels_data.end())
        continue;

      if (iter->second.objects.empty())
        continue;
      pBufferOrFrame = std::move(iter->second.objects.front());
      iter->second.objects.pop();
      object_counter--;
      //      if (iter->second.objects.empty()) {
      //        m_channels_data.erase(iter);
      //      }

      return S_OK;
    }

    return S_FALSE;
  }

  HRESULT PipeMessageGet(
      /*[in]*/ const std::string& strChannel,
      /*[out]*/ std::string* pStrEventName,
      /*[out]*/ std::string* pStrEventParam,
      /*[in]*/ int _nMaxWaitMs)
  {
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(_nMaxWaitMs);
    while (std::chrono::steady_clock::now() < timeout) {
      std::unique_lock lock(m_channels_data_mutex, std::defer_lock);

      if (!lock.try_lock_until(timeout))
        return S_FALSE;

      if (m_channels_data.empty())
        continue;

      auto iter = m_channels_data.find(strChannel);
      if (iter == m_channels_data.end())
        continue;

      if (iter->second.messages.empty())
        continue;
      auto* m = reinterpret_cast<Message*>(iter->second.messages.front().get());
      *pStrEventName = std::move(m->strEventName);
      *pStrEventParam = std::move(m->strEventParam);
      iter->second.messages.pop();
      object_counter--;
      //      if (iter->second.messages.empty()) {
      //        m_channels_data.erase(iter);
      //      }

      return S_OK;
    }

    return S_FALSE;
  }

  template<class T>
  HRESULT get(T& objects, std::timed_mutex& mutex, const std::string& strChannel, int _nMaxWaitMs, std::unique_ptr<MF_BASE_TYPE>& data)
  {
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(_nMaxWaitMs);
    while (std::chrono::steady_clock::now() < timeout) {
      std::unique_lock lock(mutex, std::defer_lock);

      if (!lock.try_lock_until(timeout))
        return S_FALSE;

      if (objects.empty())
        continue;

      auto iter = objects.find(strChannel);
      if (iter == objects.end())
        continue;
      data = std::move(iter->second.front());
      iter->second.pop();
      object_counter--;
      if (iter->second.empty()) {
        objects.erase(iter);
      }

      return S_OK;
    }

    return S_FALSE;
  }

  HRESULT PipePut(/*[in]*/ const std::string& strChannel,
                  /*[in]*/ const std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
                  /*[in]*/ int _nMaxWaitMs,
                  /*[in]*/ const std::string& strHints) override
  {
    while (!is_connected) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(10ms);
    }
    std::vector<uint8_t> data = pBufferOrFrame->serialize();

    auto header_data = serializeHeader(PacketType::PacketObject, strChannel, data);
    return writeToPipe(header_data.data(), header_data.size());
  }

  HRESULT PipeMessagePut(
      /*[in]*/ const std::string& strChannel,
      /*[in]*/ const std::string& strEventName,
      /*[in]*/ const std::string& strEventParam,
      /*[in]*/ int _nMaxWaitMs) override
  {
    if (!is_running)
      return S_FALSE;
    while (!is_connected) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(10ms);
    }

    Message message;
    message.strEventName = strEventName;
    message.strEventParam = strEventParam;
    auto data = message.serialize();

    auto header_data = serializeHeader(PacketType::PacketMessage, strChannel, data);
    return writeToPipe(header_data.data(), header_data.size());
  }

  HRESULT writeToPipe(void* data, size_t size, int _nMaxWaitMs = -1)
  {
    std::unique_lock lock(write_mutex, std::defer_lock);
    if (_nMaxWaitMs > 0) {
      auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(_nMaxWaitMs);
      if (!lock.try_lock_until(timeout))
        return S_FALSE;
    } else {
      lock.lock();
    }

    if (m_pipe->write(data, size) < 0)
      return S_FALSE;

    return S_OK;
  }

  virtual ~MFPipeImpl()
  {
    if (!m_thread)
      return;

    is_running = false;
    m_thread->join();
  }

  HRESULT PipeClose() override
  {
    if (!is_running)
      return S_OK;

    PacketHeader header;
    header.size = sizeof(PacketHeader);
    header.type = PacketType::PacketFinalizing;

    auto r = writeToPipe(&header, sizeof(header));
    if (r != S_OK)
      return r;

    finish();
    return S_OK;
  }

  HRESULT PipeInfoGet(/*[out]*/ std::string* pStrPipeName, /*[in]*/ const std::string& strChannel, MF_PIPE_INFO* _pPipeInfo) override
  {

    *_pPipeInfo = {};
    _pPipeInfo->nPipeMode = m_pipe_mode;
    {
      std::unique_lock lock(m_channels_data_mutex);
      _pPipeInfo->nChannels = m_channels_data.size();
    }
    *pStrPipeName = m_pipe_id;
    return S_OK;
  }
  HRESULT PipeFlush(/*[in]*/ const std::string& strChannel, /*[in]*/ eMFFlashFlags _eFlashFlags) override { return E_NOTIMPL; }

  HRESULT PipeCreate(/*[in]*/ const std::string& strPipeID, /*[in]*/ const std::string& strHints) override
  {
    return createPipe(strPipeID, strHints, MF_PIPE_MODE::PIPE_WRITER);
  }

  HRESULT PipeOpen(/*[in]*/ const std::string& strPipeID, /*[in]*/ int _nMaxBuffers, /*[in]*/ const std::string& strHints) override
  {
    return createPipe(strPipeID, strHints, MF_PIPE_MODE::PIPE_READER, _nMaxBuffers);
  }

  HRESULT PipePeek(/*[in]*/ const std::string& strChannel,
                   /*[in]*/ int _nIndex,
                   /*[out]*/ std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
                   /*[in]*/ int _nMaxWaitMs,
                   /*[in]*/ const std::string& strHints) override
  {
    return E_NOTIMPL;
  }

protected:
  bool sendHandshake()
  {
    PacketHeader header;
    header.size = sizeof(PacketHeader);
    header.type = PacketType::PacketHandShakes;

    if (m_pipe->write(&header, sizeof(header)) < 0)
      return false;
    return true;
  }

  HRESULT createPipe(/*[in]*/ const std::string& strPipeID, /*[in]*/ const std::string& strHints, MF_PIPE_MODE mode, int _nMaxBuffers = -1)
  {
    if (strPipeID.empty() || is_running) {
      return E_INVALIDARG;
    }

    AddressInfo addressInfo;
    if (AddressInfo::fromAddress(strPipeID, addressInfo)) {
      if (addressInfo.pipe_protocol == "udp") {

        if (MF_PIPE_MODE::PIPE_WRITER == mode) {
          m_pipe = std::make_unique<UDPPipeServer>(addressInfo.address, addressInfo.port);
        } else {
          m_pipe = std::make_unique<UDPPipeClient>(addressInfo.address, addressInfo.port);
        }

        m_pipe_mode = mode;
        if (!init()) {
          m_pipe_mode = MF_PIPE_MODE::PIPE_UNSPECIFIED;
          return S_FALSE;
        }

        m_nMaxBuffers = _nMaxBuffers;
        m_pipe_id = strPipeID;

        return S_OK;
      }

      return E_INVALIDARG;
    }

    return E_NOTIMPL;
  }

  bool is_running = false;

  struct ChannelData
  {
    std::queue<std::unique_ptr<MF_BASE_TYPE>> messages;
    std::queue<std::unique_ptr<MF_BASE_TYPE>> objects;
  };
  std::map<std::string, ChannelData> m_channels_data;

  std::timed_mutex m_channels_data_mutex;

  std::unique_ptr<IPipe> m_pipe;
  std::unique_ptr<std::thread> m_thread;
  std::atomic<bool> is_connected{ false };

  std::timed_mutex write_mutex;
  int m_nMaxBuffers = -1;
  std::atomic<int> object_counter{ 0 };
  MF_PIPE_MODE m_pipe_mode = PIPE_UNSPECIFIED;

  typedef struct MF_PIPE_INFO
  {
    int nPipeMode;
    int nPipesConnected;
    int nChannels;
    int nObjectsHave;
    int nObjectsMax;
    int nObjectsDropped;
    int nObjectsFlushed;
    int nMessagesHave;
    int nMessagesMax;
    int nMessagesDropped;
    int nMessagesFlushed;
  } MF_PIPE_INFO;

  MF_PIPE_INFO info{};

  std::string m_pipe_id;
};

#endif