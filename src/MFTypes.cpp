#include "MFTypes.h"
#include "internal/serialization.h"
#include <chrono>
#include <iostream>
std::unique_ptr<MF_BASE_TYPE>
MF_BASE_TYPE::deserialize(const std::string_view& data)
{
  auto type = *reinterpret_cast<const MF_OBJECT_TYPE*>(data.data());
  switch (type) {
    case MF_OBJECT_FRAME:
      return MF_FRAME::deserialize(data);
    case MF_OBJECT_BUFFER:
      return MF_BUFFER::deserialize(data);
    default:
      std::cerr << "Unknown type: " << type << std::endl;
  }

  return {};
}
std::vector<uint8_t>
MF_FRAME::serialize() const
{
  size_t size =
      sizeof(MF_OBJECT_TYPE) + sizeof(time) + sizeof(av_props) + str_user_props.length() + vec_video_data.size() + vec_audio_data.size() + sizeof(size_t) * 3;

  std::vector<uint8_t> data(size);
  uint8_t* ptr = data.data();

  MF_OBJECT_TYPE type = this->type();
  copyField(ptr, &type, sizeof(type));
  copyField(ptr, &time, sizeof(time));
  copyField(ptr, &av_props, sizeof(av_props));

  copyData(ptr, str_user_props.data(), str_user_props.size());
  copyData(ptr, vec_video_data.data(), vec_video_data.size());
  copyData(ptr, vec_audio_data.data(), vec_audio_data.size());

  return data;
}

std::unique_ptr<MF_FRAME>
MF_FRAME::deserialize(const std::string_view& data)
{
  std::unique_ptr<MF_FRAME> object = std::make_unique<MF_FRAME>();
  const char* ptr = reinterpret_cast<const char*>(data.data());

  ptr += sizeof(MF_OBJECT_TYPE);

  object->time = *reinterpret_cast<const M_TIME*>(ptr);
  ptr += sizeof(M_TIME);
  object->av_props = *reinterpret_cast<const M_AV_PROPS*>(ptr);
  ptr += sizeof(M_AV_PROPS);

  object->str_user_props = parseString(ptr);
  object->vec_video_data = parseVector(ptr);
  object->vec_audio_data = parseVector(ptr);

  return object;
};

std::vector<uint8_t>
MF_BUFFER::serialize() const
{
  size_t size = sizeof(MF_OBJECT_TYPE) + sizeof(flags) + data.size() + sizeof(size_t);
  std::vector<uint8_t> data_object(size);
  uint8_t* ptr = reinterpret_cast<uint8_t*>(data_object.data());

  MF_OBJECT_TYPE type = this->type();
  copyField(ptr, &type, sizeof(type));
  copyField(ptr, &flags, sizeof(flags));
  copyData(ptr, data.data(), data.size());

  return data_object;
}

std::unique_ptr<MF_BUFFER>
MF_BUFFER::deserialize(const std::string_view& data)
{

  std::unique_ptr<MF_BUFFER> object = std::make_unique<MF_BUFFER>();
  const char* ptr = reinterpret_cast<const char*>(data.data());

  ptr += sizeof(MF_OBJECT_TYPE);

  object->flags = *reinterpret_cast<const eMFBufferFlags*>(ptr);
  ptr += sizeof(eMFBufferFlags);

  auto start = std::chrono::steady_clock::now();
  object->data = parseVector(ptr);

  return object;
}

bool
operator==(const MF_FRAME& a, const MF_FRAME& b)
{
  return a.time.rtStartTime == b.time.rtStartTime && a.time.rtEndTime == b.time.rtEndTime && a.vec_video_data == b.vec_video_data &&
         a.str_user_props == b.str_user_props && a.vec_audio_data == b.vec_audio_data;
}

bool
operator==(const MF_BUFFER& a, const MF_BUFFER& b)
{
  return a.flags == b.flags && a.data == b.data;
}

bool
operator==(const std::shared_ptr<MF_BASE_TYPE>& a, const std::shared_ptr<MF_BUFFER>& b)
{
  return a && b && a->serialize() == b->serialize();
}