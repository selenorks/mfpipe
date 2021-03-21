#ifndef UUID_C21A144D_D981_4FBC_8AA4_BF301E72F198
#define UUID_C21A144D_D981_4FBC_8AA4_BF301E72F198
#include "MFTypes.h"
#include <cstdlib>
#include <limits>
template<typename TYPE>
std::vector<TYPE>
generateRandomVector(size_t size = 128)
{
  std::vector<TYPE> data;
  data.resize(size);
  for (int i = 0; i < size; i++)
    data[i] = std::rand() / ((RAND_MAX + 1u) / (std::numeric_limits<TYPE>::max)());

  return data;
}
std::unique_ptr<MF_BUFFER>
GenerateMfBuffer(size_t vector_size = 128)
{
  std::unique_ptr<MF_BUFFER> buffer = std::make_unique<MF_BUFFER>();
  buffer->data = generateRandomVector<uint8_t>(vector_size);

  eMFBufferFlags flags[] = { eMFBF_Empty, eMFBF_Buffer, eMFBF_Packet, eMFBF_Frame, eMFBF_Stream, eMFBF_SideData, eMFBF_VideoData, eMFBF_AudioData };

  int flags_count = sizeof(flags) / sizeof(flags[0]);
  int flags_num = std::rand() / ((RAND_MAX + 1u) / flags_count);

  buffer->flags = flags[flags_num];
  return buffer;
}

std::unique_ptr<MF_FRAME>
GenerateMfFrame()
{
  std::unique_ptr<MF_FRAME> buffer = std::make_unique<MF_FRAME>();

  buffer->time.rtStartTime = std::rand();
  buffer->time.rtEndTime = std::rand();

  buffer->av_props.audProps.nChannels = std::rand();
  buffer->av_props.audProps.nSamplesPerSec = std::rand();
  buffer->av_props.audProps.nBitsPerSample = std::rand();
  buffer->av_props.audProps.nTrackSplitBits = std::rand();

  eMFCC flags[] = { eMFCC_Default, eMFCC_I420, eMFCC_YV12, eMFCC_NV12, eMFCC_YUY2, eMFCC_YVYU, eMFCC_UYVY, eMFCC_RGB24, eMFCC_RGB32 };

  int flags_count = sizeof(flags) / sizeof(flags[0]);
  int flags_num = std::rand() / ((RAND_MAX + 1u) / flags_count);

  buffer->av_props.vidProps.fccType = flags[flags_num];
  buffer->av_props.vidProps.nWidth = std::rand();
  buffer->av_props.vidProps.nHeight = std::rand();
  buffer->av_props.vidProps.nRowBytes = std::rand();
  buffer->av_props.vidProps.nAspectX = std::rand() / ((RAND_MAX + 1u) / (std::numeric_limits<short>::max)());
  buffer->av_props.vidProps.nAspectY = std::rand() / ((RAND_MAX + 1u) / (std::numeric_limits<short>::max)());
  buffer->av_props.vidProps.dblRate = std::rand();

  auto vec = generateRandomVector<char>();
  buffer->str_user_props = { vec.begin(), vec.end() };
  buffer->vec_video_data = generateRandomVector<uint8_t>();
  buffer->vec_audio_data = generateRandomVector<uint8_t>();

  return buffer;
}
#endif
