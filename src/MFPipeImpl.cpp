#include "MFPipeImpl.h"

struct AddressInfo
{
  static bool fromAddress(const std::string& strPipeID, AddressInfo& info)
  {
    const std::string udp{ "udp" };

    const static std::regex rgx{ "([A-z]+):\\/\\/([^:]+)(:([A-z0-9]+))?" };
    std::smatch match;

    if (!std::regex_search(strPipeID.begin(), strPipeID.end(), match, rgx))
      return false;

    info.pipe_protocol = match[1];

    std::transform(info.pipe_protocol.begin(), info.pipe_protocol.end(), info.pipe_protocol.begin(), [](char c) { return std::tolower(c); });
    info.address = match[2];
    info.port = match[4];
    return true;
  }

  std::string pipe_protocol;
  std::string address;
  // can also be a name of the service
  std::string port;
};

HRESULT
MFPipeImpl::CreatePipe(/*[in]*/ const std::string& strPipeID, /*[in]*/ const std::string& strHints, MF_PIPE_MODE mode, int _nMaxBuffers)
{
  if (strPipeID.empty() || m_is_running) {
    return E_INVALIDARG;
  }

  AddressInfo addressInfo;
  if (AddressInfo::fromAddress(strPipeID, addressInfo)) {

    if ("udp" == addressInfo.pipe_protocol) {
      if (MF_PIPE_MODE::PIPE_WRITER == mode) {
        m_pipe = std::make_unique<UDPPipeServer>(addressInfo.address, addressInfo.port);
      } else {
        m_pipe = std::make_unique<UDPPipeClient>(addressInfo.address, addressInfo.port);
      }
    } else if ("pipe" == addressInfo.pipe_protocol) {
      //        if (MF_PIPE_MODE::PIPE_WRITER == mode) {
      //          m_pipe = std::make_unique<ServerNamedPipe>(addressInfo.m_address);
      //        } else {
      //          m_pipe = std::make_unique<ClientNamedPipe>(addressInfo.m_address);
      //        }
    } else {
      std::cerr << "Unknown protocol: " << addressInfo.pipe_protocol << std::endl;
      return E_INVALIDARG;
    }

    m_pipe_mode = mode;
    if (!Init()) {
      m_pipe_mode = MF_PIPE_MODE::PIPE_UNSPECIFIED;
      return S_FALSE;
    }

    m_nMaxBuffers = _nMaxBuffers;
    m_pipe_id = strPipeID;

    return S_OK;
  }

  return S_FALSE;
}

HRESULT
MFPipeImpl::PipeGet(/*[in]*/ const std::string& strChannel,
                    /*[out]*/ std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
                    /*[in]*/ int _nMaxWaitMs,
                    /*[in]*/ const std::string& strHints)
{

  if (MF_PIPE_MODE::PIPE_READER != m_pipe_mode)
    return S_FALSE;

  auto local_yield = [](std::unique_lock<std::timed_mutex>& lock) {
    lock.unlock();
    std::this_thread::yield();
  };

  auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(_nMaxWaitMs);
  do {
    std::unique_lock lock(m_data.m_channels_data_mutex, std::defer_lock);

    if (!lock.try_lock_until(timeout)) {
      local_yield(lock);
      return S_FALSE;
    }

    if (m_data.m_channels_data.empty()) {
      continue;
    }

    auto iter = m_data.m_channels_data.find(strChannel);
    if (iter == m_data.m_channels_data.end()) {
      local_yield(lock);
      continue;
    }

    if (iter->second.objects.empty()) {
      local_yield(lock);
      continue;
    }
    pBufferOrFrame = std::move(iter->second.objects.front());
    iter->second.objects.pop();
    m_current_count_objects--;
    return S_OK;
  } while (std::chrono::steady_clock::now() < timeout);

  return S_FALSE;
}

HRESULT
MFPipeImpl::PipeMessageGet(
    /*[in]*/ const std::string& strChannel,
    /*[out]*/ std::string* pStrEventName,
    /*[out]*/ std::string* pStrEventParam,
    /*[in]*/ int _nMaxWaitMs)
{
  if (!pStrEventName && !pStrEventParam)
    return E_INVALIDARG;

  if (MF_PIPE_MODE::PIPE_READER != m_pipe_mode)
    return S_FALSE;

  auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(_nMaxWaitMs);

  auto local_yield = [](std::unique_lock<std::timed_mutex>& lock) {
    lock.unlock();
    std::this_thread::yield();
  };
  do {
    std::unique_lock lock(m_data.m_channels_data_mutex, std::defer_lock);

    if (!lock.try_lock_until(timeout)) {
      //      std::cout << "PipeMessageGet} 1" << std::endl;
      return S_FALSE;
    }

    if (m_data.m_channels_data.empty()) {
      local_yield(lock);
      continue;
    }

    auto iter = m_data.m_channels_data.find(strChannel);
    if (iter == m_data.m_channels_data.end()) {
      local_yield(lock);
      continue;
    }

    if (iter->second.messages.empty()) {
      local_yield(lock);
      continue;
    }
    auto* m = reinterpret_cast<Message*>(iter->second.messages.front().get());
    if (pStrEventName)
      *pStrEventName = std::move(m->strEventName);
    if (pStrEventParam)
      *pStrEventParam = std::move(m->strEventParam);
    iter->second.messages.pop();
    m_current_count_objects--;
    //      if (iter->second.messages.empty()) {
    //        m_channels_data.erase(iter);
    //      }

    return S_OK;
  } while (std::chrono::steady_clock::now() <= timeout);

  return S_FALSE;
}

HRESULT
MFPipeImpl::PipePut(/*[in]*/ const std::string& strChannel,
                    /*[in]*/ const std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
                    /*[in]*/ int _nMaxWaitMs,
                    /*[in]*/ const std::string& strHints)
{
  if (MF_PIPE_MODE::PIPE_WRITER != m_pipe_mode)
    return S_FALSE;

  std::vector<uint8_t> data = pBufferOrFrame->serialize();

  auto header_data = serializeHeader(PacketType::PacketObject, strChannel, data);
  auto hr = WriteToPipe(header_data.data(), header_data.size(), _nMaxWaitMs);
  return hr;
}

HRESULT
MFPipeImpl::PipeMessagePut(
    /*[in]*/ const std::string& strChannel,
    /*[in]*/ const std::string& strEventName,
    /*[in]*/ const std::string& strEventParam,
    /*[in]*/ int _nMaxWaitMs)
{
  if (!m_is_running)
    return S_FALSE;

  if (MF_PIPE_MODE::PIPE_WRITER != m_pipe_mode)
    return S_FALSE;

  Message message;
  message.strEventName = strEventName;
  message.strEventParam = strEventParam;
  auto data = message.serialize();

  auto header_data = serializeHeader(PacketType::PacketMessage, strChannel, data);
  auto hr = WriteToPipe(header_data.data(), header_data.size(), _nMaxWaitMs);
  return hr;
}

bool
MFPipeImpl::Init()
{
  if (!m_pipe->create())
    return false;

  // what when old thread is finished in case of reopening pipe
  if (!m_is_running && m_thread)
    m_thread->join();

  m_is_running = true;
  m_thread = std::make_unique<std::thread>(&MFPipeImpl::WorkerThread, this);

  if (MF_PIPE_MODE::PIPE_READER == m_pipe_mode) {
    if (SendHandshake()) {
      std::unique_lock lock(write_mutex);
      m_is_connected = true;
      return true;
    }
    return false;
  }
  return true;
}

void
MFPipeImpl::WorkerThread()
{
  Accumulator parser(128 * 1024);

  while (m_is_running) {
    size_t read_len = m_pipe->read(parser.dataEnd(), parser.availableSize());
    if (!read_len) {
      continue;
    }
    parser.add(read_len);
    if (parser.isMessageReady()) {
      Parse(parser);
      parser.reset();
    }
  }
}

void
MFPipeImpl::Parse(Accumulator& parser)
{
  switch (parser.type()) {
    case PacketType::PacketHandShakes: {
      std::unique_lock lock(write_mutex);
      m_is_connected = true;
      break;
    }
    case PacketType::PacketMessage: {
      const std::string& channel = parser.channel();
      int object_count = m_current_count_objects++;
      if (m_nMaxBuffers > 0 && object_count >= m_nMaxBuffers) {
        info.nMessagesDropped++;
        m_current_count_objects--;
        break;
      }

      std::unique_ptr<MF_BASE_TYPE> object = Message::deserialize(parser.payload());

      {
        std::unique_lock lock(m_data.m_channels_data_mutex);
        auto iter = m_data.m_channels_data.find(channel);
        if (iter == m_data.m_channels_data.end()) {
          ChannelData data;
          data.messages.emplace(std::move(object));
          m_data.m_channels_data.try_emplace(channel, std::move(data));
        } else {
          iter->second.messages.emplace(std::move(object));
        }
        info.nMessagesHave++;
      }
      break;
    }
    case PacketType::PacketObject: {
      const std::string& channel = parser.channel();

      int object_count = m_current_count_objects++;
      if (m_nMaxBuffers > 0 && object_count >= m_nMaxBuffers) {
        info.nObjectsDropped++;
        m_current_count_objects--;
        break;
      }

      auto start = std::chrono::steady_clock::now();
      std::shared_ptr<MF_BASE_TYPE> object = MF_BASE_TYPE::deserialize(parser.payload());

      {
        std::unique_lock lock(m_data.m_channels_data_mutex);
        auto iter = m_data.m_channels_data.find(channel);
        if (iter == m_data.m_channels_data.end()) {
          ChannelData data;
          data.objects.emplace(std::move(object));
          m_data.m_channels_data.try_emplace(channel, std::move(data));
        } else {
          iter->second.objects.emplace(std::move(object));
        }
        info.nObjectsHave++;
      }
      break;
    }

    case PacketType::PacketFinalizing:
      Finish();
      break;
    default:
      std::cerr << "Got unknown object, type: " << int(parser.type()) << std::endl;
      break;
  }
}

HRESULT
MFPipeImpl::PipeClose()
{
  if (!m_is_running)
    return S_OK;

  PacketHeader header{};
  header.size = sizeof(PacketHeader);
  header.type = PacketType::PacketFinalizing;

  auto r = WriteToPipe(&header, sizeof(header), -1);
  if (r != S_OK)
    return r;

  Finish();
  return S_OK;
}

HRESULT
MFPipeImpl::PipeInfoGet(/*[out]*/ std::string* pStrPipeName, /*[in]*/ const std::string& strChannel, MF_PIPE_INFO* _pPipeInfo)
{
  HRESULT r = S_FALSE;
  if (_pPipeInfo) {
    r = S_OK;
    *_pPipeInfo = {};
    _pPipeInfo->nPipeMode = m_pipe_mode;
    {
      std::unique_lock lock(m_data.m_channels_data_mutex);
      _pPipeInfo->nChannels = m_data.m_channels_data.size();
    }
  }
  if (pStrPipeName) {
    r = S_OK;
    *pStrPipeName = m_pipe_id;
  }
  return r;
}

MFPipeImpl::~MFPipeImpl()
{
  if (!m_thread)
    return;

  m_is_running = false;
  m_thread->join();
}

HRESULT
MFPipeImpl::PipeCreate(/*[in]*/ const std::string& strPipeID, /*[in]*/ const std::string& strHints)
{
  return CreatePipe(strPipeID, strHints, MF_PIPE_MODE::PIPE_WRITER);
}

HRESULT
MFPipeImpl::PipeOpen(/*[in]*/ const std::string& strPipeID, /*[in]*/ int _nMaxBuffers, /*[in]*/ const std::string& strHints)
{
  return CreatePipe(strPipeID, strHints, MF_PIPE_MODE::PIPE_READER, _nMaxBuffers);
}

HRESULT MFPipeImpl::WriteToPipe(const void* data, size_t size, int _nMaxWaitMs)
{
  while (!m_is_connected) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
  }
  std::unique_lock<std::timed_mutex> lock(write_mutex, std::defer_lock);
  if (_nMaxWaitMs > 0) {
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(_nMaxWaitMs);
    if (!lock.try_lock_until(timeout))
      return S_FALSE;
  } else {
    lock.lock();
  }

  // in case for first message
  if (!m_connect_event.wait_for(lock, std::chrono::milliseconds(_nMaxWaitMs), [&] { return m_is_connected; }))
    return S_FALSE;

  if (m_pipe->write(data, size) < 0)
    return S_FALSE;

  return S_OK;
}

bool MFPipeImpl::SendHandshake()
{
  PacketHeader header{};
  header.size = sizeof(PacketHeader);
  header.type = PacketType::PacketHandShakes;

  if (m_pipe->write(&header, sizeof(header)) < 0)
    return false;
  return true;
}