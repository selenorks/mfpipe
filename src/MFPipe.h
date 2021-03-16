#ifndef MF_PIPE_
#define MF_PIPE_

#include <memory>
#include <string>

#include "MFTypes.h"

class MFPipe
{
public:
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

  typedef enum eMFFlashFlags
  {
    eMFFL_ResetCounters = 0x2,
    eMFFL_FlushObjects = 0x20,
    eMFFL_FlushMessages = 0x40,
    eMFFL_FlushStream = 0x20,
    eMFFL_FlushAll = 0xf0,
    eMFFL_RemoveChannel = 0x100
  } eMFFlashFlags;

  virtual ~MFPipe() {}

  virtual HRESULT PipeInfoGet(/*[out]*/ std::string* pStrPipeName, /*[in]*/ const std::string& strChannel, MF_PIPE_INFO* _pPipeInfo) = 0;
  virtual HRESULT PipeCreate(/*[in]*/ const std::string& strPipeID, /*[in]*/ const std::string& strHints) = 0;
  virtual HRESULT PipeOpen(/*[in]*/ const std::string& strPipeID, /*[in]*/ int _nMaxBuffers, /*[in]*/ const std::string& strHints) = 0;
  virtual HRESULT PipePut(/*[in]*/ const std::string& strChannel,
                          /*[in]*/ const std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
                          /*[in]*/ int _nMaxWaitMs,
                          /*[in]*/ const std::string& strHints) = 0;
  virtual HRESULT PipeGet(/*[in]*/ const std::string& strChannel,
                          /*[out]*/ std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
                          /*[in]*/ int _nMaxWaitMs,
                          /*[in]*/ const std::string& strHints) = 0;
  virtual HRESULT PipePeek(/*[in]*/ const std::string& strChannel,
                           /*[in]*/ int _nIndex,
                           /*[out]*/ std::shared_ptr<MF_BASE_TYPE>& pBufferOrFrame,
                           /*[in]*/ int _nMaxWaitMs,
                           /*[in]*/ const std::string& strHints) = 0;
  virtual HRESULT PipeMessagePut(
      /*[in]*/ const std::string& strChannel,
      /*[in]*/ const std::string& strEventName,
      /*[in]*/ const std::string& strEventParam,
      /*[in]*/ int _nMaxWaitMs) = 0;
  virtual HRESULT PipeMessageGet(
      /*[in]*/ const std::string& strChannel,
      /*[out]*/ std::string* pStrEventName,
      /*[out]*/ std::string* pStrEventParam,
      /*[in]*/ int _nMaxWaitMs) = 0;
  virtual HRESULT PipeFlush(/*[in]*/ const std::string& strChannel, /*[in]*/ eMFFlashFlags _eFlashFlags) = 0;
  virtual HRESULT PipeClose() = 0;
};

#endif