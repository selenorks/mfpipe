#ifndef MF_TYPES_H_
#define MF_TYPES_H_
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

typedef long long int REFERENCE_TIME;

typedef enum HRESULT
{
  S_OK = 0,
  S_FALSE = 1,
  E_NOTIMPL = 0x80004001L,
  E_OUTOFMEMORY = 0x8007000EL,
  E_INVALIDARG = 0x80070057L
} HRESULT;

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
  static std::unique_ptr<MF_BASE_TYPE> deserialize(const void* data);
} MF_BASE_TYPE;

typedef struct MF_FRAME : public MF_BASE_TYPE
{
  typedef std::shared_ptr<MF_FRAME> TPtr;

  M_TIME time = {};
  M_AV_PROPS av_props = {};
  std::string str_user_props;
  std::vector<uint8_t> vec_video_data;
  std::vector<uint8_t> vec_audio_data;

  MF_OBJECT_TYPE type() const { return MF_OBJECT_FRAME; }
  std::vector<uint8_t> serialize() const override;
  static std::unique_ptr<MF_FRAME> deserialize(const void* data);

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
  typedef std::shared_ptr<MF_BUFFER> TPtr;

  eMFBufferFlags flags;
  std::vector<uint8_t> data;

  MF_OBJECT_TYPE type() const { return MF_OBJECT_BUFFER; }

  std::vector<uint8_t> serialize() const override;
  static std::unique_ptr<MF_BUFFER> deserialize(const void* data);
} MF_BUFFER;

bool operator ==(const MF_FRAME &a, const MF_FRAME& b);
bool operator ==(const MF_BUFFER &a, const MF_BUFFER& b);

#endif
