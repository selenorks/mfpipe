#ifndef MF_PIPEIMPL_H_
#define MF_PIPEIMPL_H_

#include <condition_variable>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include "MFPipe.h"
#include "MFTypes.h"

#include "internal/ParserServer.h"
#include "internal/protocols/ipipe.h"
#include <udp.h>

struct ChannelData
{
  std::queue<std::shared_ptr<MF_BASE_TYPE>> messages;
  std::queue<std::shared_ptr<MF_BASE_TYPE>> objects;
};

struct FullChannelsData
{
  std::map<std::string, ChannelData> m_channels_data;
  std::timed_mutex m_channels_data_mutex;
};

class MFPipeImpl : public MFPipe
{
public:
  explicit MFPipeImpl() = default;

  MFPipeImpl(const MFPipeImpl& other) = delete;

  HRESULT PipeGet(/*[in]*/ const std::string& strChannel,
                  /*[out]*/ std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
                  /*[in]*/ int _nMaxWaitMs,
                  /*[in]*/ const std::string& strHints) override;
  HRESULT PipeMessageGet(
      /*[in]*/ const std::string& strChannel,
      /*[out]*/ std::string* pStrEventName,
      /*[out]*/ std::string* pStrEventParam,
      /*[in]*/ int _nMaxWaitMs) override;

  HRESULT PipePut(/*[in]*/ const std::string& strChannel,
                  /*[in]*/ const std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
                  /*[in]*/ int _nMaxWaitMs,
                  /*[in]*/ const std::string& strHints) override;

  HRESULT PipeMessagePut(
      /*[in]*/ const std::string& strChannel,
      /*[in]*/ const std::string& strEventName,
      /*[in]*/ const std::string& strEventParam,
      /*[in]*/ int _nMaxWaitMs) override;

  ~MFPipeImpl() override;

  HRESULT PipeClose() override;
  HRESULT PipeInfoGet(/*[out]*/ std::string* pStrPipeName, /*[in]*/ const std::string& strChannel, MF_PIPE_INFO* _pPipeInfo) override;

  HRESULT PipeCreate(/*[in]*/ const std::string& strPipeID, /*[in]*/ const std::string& strHints) override;

  HRESULT PipeOpen(/*[in]*/ const std::string& strPipeID, /*[in]*/ int _nMaxBuffers, /*[in]*/ const std::string& strHints) override;

  HRESULT PipeFlush(/*[in]*/ const std::string& strChannel, /*[in]*/ eMFFlashFlags _eFlashFlags) override { return E_NOTIMPL; }
  HRESULT PipePeek(/*[in]*/ const std::string& strChannel,
                   /*[in]*/ int _nIndex,
                   /*[out]*/ std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
                   /*[in]*/ int _nMaxWaitMs,
                   /*[in]*/ const std::string& strHints) override
  {
    return E_NOTIMPL;
  }

protected:
  HRESULT WriteToPipe(const void* data, size_t size, int _nMaxWaitMs);
  virtual bool Init();
  virtual void WorkerThread();
  void Finish() { m_is_running = false; }
  void Parse(Accumulator& parser);
  bool SendHandshake();

  HRESULT CreatePipe(/*[in]*/ const std::string& strPipeID, /*[in]*/ const std::string& strHints, MF_PIPE_MODE mode, int _nMaxBuffers = -1);

  FullChannelsData m_data;

  std::unique_ptr<IPipe> m_pipe;
  std::unique_ptr<std::thread> m_thread;

  std::atomic<bool> m_is_connected{ false };
  std::condition_variable_any m_connect_event;

  std::timed_mutex write_mutex;
  int m_nMaxBuffers = -1;
  std::atomic<int> m_current_count_objects{ 0 };
  MF_PIPE_MODE m_pipe_mode = PIPE_UNSPECIFIED;

  MF_PIPE_INFO info{};

  std::string m_pipe_id;

  bool m_is_running = false;
};

#endif