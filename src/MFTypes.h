#ifndef UUID_3B5A0D9B_9C86_448D_94BA_EEE6F42E7ACE
#define UUID_3B5A0D9B_9C86_448D_94BA_EEE6F42E7ACE
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "internal/serialization.h"
#ifdef WIN32
#include <winerror.h>
#endif

typedef long long int REFERENCE_TIME;

#ifndef WIN32
typedef enum HRESULT
{
  S_OK = 0,
  S_FALSE = 1,
  E_NOTIMPL = 0x80004001L,
  E_OUTOFMEMORY = 0x8007000EL,
  E_INVALIDARG = 0x80070057L
} HRESULT;
#endif
typedef struct M_TIME
{
  REFERENCE_TIME rtStartTime;
  REFERENCE_TIME rtEndTime;
} M_TIME;

typedef enum eMFCC
{
  eMFCC_Default = 0,
  eMFCC_I420 = 0x30323449,
  eMFCC_YV12 = 0x32315659,
  eMFCC_NV12 = 0x3231564e,
  eMFCC_YUY2 = 0x32595559,
  eMFCC_YVYU = 0x55595659,
  eMFCC_UYVY = 0x59565955,
  eMFCC_RGB24 = 0xe436eb7d,
  eMFCC_RGB32 = 0xe436eb7e,
} eMFCC;

typedef struct M_VID_PROPS
{
  eMFCC fccType;
  int nWidth;
  int nHeight;
  int nRowBytes;
  short nAspectX;
  short nAspectY;
  double dblRate;
} M_VID_PROPS;

typedef struct M_AUD_PROPS
{
  int nChannels;
  int nSamplesPerSec;
  int nBitsPerSample;
  int nTrackSplitBits;
} M_AUD_PROPS;

typedef struct M_AV_PROPS
{
  M_VID_PROPS vidProps;
  M_AUD_PROPS audProps;
} M_AV_PROPS;

enum MF_OBJECT_TYPE
{
  MF_OBJECT_FRAME,
  MF_OBJECT_BUFFER
};

enum MF_PIPE_MODE : uint8_t
{
  PIPE_WRITER,
  PIPE_READER,
  PIPE_UNSPECIFIED
};

typedef struct MF_BASE_TYPE
{
  virtual ~MF_BASE_TYPE() {}
  virtual std::vector<uint8_t> serialize() const = 0;
  static std::unique_ptr<MF_BASE_TYPE> deserialize(const std::string_view& data);
} MF_BASE_TYPE;

typedef struct MF_FRAME : public MF_BASE_TYPE
{
  M_TIME time = {};
  M_AV_PROPS av_props = {};
  std::string str_user_props;
  std::vector<uint8_t> vec_video_data;
  std::vector<uint8_t> vec_audio_data;

  MF_OBJECT_TYPE type() const { return MF_OBJECT_FRAME; }
  std::vector<uint8_t> serialize() const override;
  static std::unique_ptr<MF_FRAME> deserialize(const std::string_view& data);

} MF_FRAME;

typedef enum eMFBufferFlags
{
  eMFBF_Empty = 0,
  eMFBF_Buffer = 0x1,
  eMFBF_Packet = 0x2,
  eMFBF_Frame = 0x3,
  eMFBF_Stream = 0x4,
  eMFBF_SideData = 0x10,
  eMFBF_VideoData = 0x20,
  eMFBF_AudioData = 0x40,
} eMFBufferFlags;

typedef struct MF_BUFFER : public MF_BASE_TYPE
{
  eMFBufferFlags flags;
  std::vector<uint8_t> data;

  MF_OBJECT_TYPE type() const { return MF_OBJECT_BUFFER; }

  std::vector<uint8_t> serialize() const override;
  static std::unique_ptr<MF_BUFFER> deserialize(const std::string_view& data);
} MF_BUFFER;

bool
operator==(const MF_FRAME& a, const MF_FRAME& b);
bool
operator==(const MF_BUFFER& a, const MF_BUFFER& b);

bool
operator==(const std::shared_ptr<MF_BASE_TYPE>& a, const std::shared_ptr<MF_BUFFER>& b);

struct Message : public MF_BASE_TYPE
{
  Message(const std::string& strEventName, const std::string& strEventParam)
      : strEventName(strEventName)
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

  static std::unique_ptr<Message> deserialize(const std::string_view& data)
  {
    auto message = std::make_unique<Message>();

    const char* ptr = data.data();
    message->strEventName = parseString(ptr);
    message->strEventParam = parseString(ptr);
    return message;
  }

  std::string strEventName;
  std::string strEventParam;
};
#endif
