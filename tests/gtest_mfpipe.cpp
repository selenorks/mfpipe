#include <gtest/gtest.h>

#include <MFPipeImpl.cpp>

class MFPipeInvalidTest : public testing::TestWithParam<::std::tuple<std::string>>
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_P(MFPipeInvalidTest, CreateInvalid)
{
  std::string url = std::get<0>(GetParam());
  MFPipeImpl MFPipe_Write;
  EXPECT_EQ(MFPipe_Write.PipeCreate(url, ""), E_INVALIDARG);
}

INSTANTIATE_TEST_SUITE_P(CreateInvalid, MFPipeInvalidTest, ::testing::Combine(::testing::Values("", "", "tcp://127.0.0.1:12345")));

class MFPipeTest : public testing::TestWithParam<::std::tuple<std::string>>
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// TEST_P(MFPipeTest, CreateUDP)
//{
//  std::string url = std::get<0>(GetParam());
//  MFPipeImpl MFPipe_Write;
//  auto result = MFPipe_Write.PipeCreate(url, "");
//  EXPECT_EQ(result, S_OK);
//}

const int WAIT_TIMEOUT = 500;

TEST_P(MFPipeTest, SingleChannel)
{
  std::string address = std::get<0>(GetParam());
  std::string pStrEventName = "ch1";
  std::string message = "test";

  std::string messageRecived;

  MFPipeImpl MFPipe_Write;

  EXPECT_EQ(MFPipe_Write.PipeCreate(address, ""), S_OK);
  MFPipeImpl MFPipe_Read;
  EXPECT_EQ(MFPipe_Read.PipeOpen(address, 32, ""), S_OK);

  EXPECT_EQ(MFPipe_Write.PipeMessagePut("ch1", message, pStrEventName, WAIT_TIMEOUT), S_OK);

  EXPECT_EQ(MFPipe_Read.PipeMessageGet("ch2", &messageRecived, &pStrEventName, WAIT_TIMEOUT), S_FALSE);
  EXPECT_EQ(MFPipe_Read.PipeMessageGet("ch1", &messageRecived, &pStrEventName, WAIT_TIMEOUT), S_OK);

  EXPECT_EQ(message, messageRecived);
}

class MFPipeSimpleTest : public testing::TestWithParam<::std::tuple<std::string>>
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_P(MFPipeTest, MultiChannel)
{
  std::string address = std::get<0>(GetParam());
  std::string pStrEventName = "ch1";
  std::string message = "ch1-message";

  std::string messageRecived;

  MFPipeImpl server;

  EXPECT_EQ(server.PipeCreate(address, ""), S_OK);
  MFPipeImpl client;
  EXPECT_EQ(client.PipeOpen(address, 32, ""), S_OK);

  EXPECT_EQ(server.PipeMessagePut("ch1", message, pStrEventName, WAIT_TIMEOUT), S_OK);

  EXPECT_EQ(client.PipeMessageGet("ch2", &messageRecived, &pStrEventName, WAIT_TIMEOUT), S_FALSE);
  EXPECT_EQ(client.PipeMessageGet("ch1", &messageRecived, &pStrEventName, WAIT_TIMEOUT), S_OK);
  EXPECT_EQ(message, messageRecived);

  EXPECT_EQ(server.PipeMessagePut("ch2", message, pStrEventName, WAIT_TIMEOUT), S_OK);
  EXPECT_EQ(client.PipeMessageGet("ch2", &messageRecived, &pStrEventName, WAIT_TIMEOUT), S_OK);

  EXPECT_EQ(message, messageRecived);
}

TEST_P(MFPipeTest, BigData)
{
  std::srand(0);
  std::string address = std::get<0>(GetParam());
  std::string pStrEventName = "ch1";

  std::string message;
  message.resize(1024 * 5);
  // message.resize(1024 * 1024);
  for (int i = 0; i < message.size(); i++)
    message[i] = std::rand() / ((RAND_MAX + 1u) / std::numeric_limits<char>::max());

  std::string messageRecived;

  MFPipeImpl server;

  EXPECT_EQ(server.PipeCreate(address, ""), S_OK);
  MFPipeImpl client;
  EXPECT_EQ(client.PipeOpen(address, 32, ""), S_OK);

  EXPECT_EQ(server.PipeMessagePut("ch1", message, pStrEventName, WAIT_TIMEOUT), S_OK);

  EXPECT_EQ(client.PipeMessageGet("ch1", &messageRecived, &pStrEventName, WAIT_TIMEOUT * 10), S_OK);
  EXPECT_EQ(message.size(), messageRecived.size());
}

// INSTANTIATE_TEST_SUITE_P(MFPipeTest, MFPipeSimpleTest, ::testing::Combine(
//    ::testing::Values(
//        "udp://127.0.0.1:12345"
////                                                               "Udp://127.0.0.1:12345",
////                                                               "udp://127.0.0.1:git"
//    )));

INSTANTIATE_TEST_SUITE_P(CreateUDP,
                         MFPipeTest,
                         ::testing::Combine(::testing::Values("udp://127.0.0.1:12345"
                                                              //                                                    "Udp://127.0.0.1:12345",
                                                              //                                                    "udp://127.0.0.1:git"
                                                              )));

TEST_P(MFPipeTest, ReBind)
{
  std::string address = std::get<0>(GetParam());

  MFPipeImpl server1;
  EXPECT_EQ(server1.PipeCreate(address, ""), S_OK);

  MFPipeImpl server2;
  EXPECT_EQ(server2.PipeCreate(address, ""), S_FALSE);

  MFPipeImpl client1;
  EXPECT_EQ(client1.PipeOpen(address, 32, ""), S_OK);

  // TODO Fix return code
  //  MFPipeImpl client2;
  //  EXPECT_EQ(client2.PipeOpen(address, 32, ""), S_OK);

  std::string message = "rebind";

  std::string messageRecived;
  std::string pStrEventName = "ch1";
  EXPECT_EQ(server1.PipeMessagePut("ch1", message, pStrEventName, WAIT_TIMEOUT), S_OK);

  EXPECT_EQ(client1.PipeMessageGet("ch1", &messageRecived, &pStrEventName, WAIT_TIMEOUT), S_OK);
  EXPECT_EQ(message.size(), messageRecived.size());
}

template<typename TYPE>
std::vector<TYPE>
generateRandomVector()
{
  const int MAX_DATA_LENGTH = 128;
  std::vector<TYPE> data;
  data.resize(MAX_DATA_LENGTH);
  for (int i = 0; i < MAX_DATA_LENGTH; i++)
    data[i] = std::rand() / ((RAND_MAX + 1u) / std::numeric_limits<TYPE>::max());

  return data;
}
std::unique_ptr<MF_BUFFER>
GenerateMfBuffer()
{
  std::unique_ptr<MF_BUFFER> buffer = std::make_unique<MF_BUFFER>();
  buffer->data = generateRandomVector<uint8_t>();

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
  buffer->av_props.vidProps.nAspectX = std::rand() / ((RAND_MAX + 1u) / std::numeric_limits<short>::max());
  buffer->av_props.vidProps.nAspectY = std::rand() / ((RAND_MAX + 1u) / std::numeric_limits<short>::max());
  buffer->av_props.vidProps.dblRate = std::rand();

  auto vec = generateRandomVector<char>();
  buffer->str_user_props = { vec.begin(), vec.end() };
  buffer->vec_video_data = generateRandomVector<uint8_t>();
  buffer->vec_audio_data = generateRandomVector<uint8_t>();

  return buffer;
}

TEST_P(MFPipeTest, MultiEvent)
{
  srand(0);
  std::string address = std::get<0>(GetParam());
  std::string pStrEventName = "ch1";
  std::string message = "ch1-message";

  std::string messageRecived;

  MFPipeImpl server;

  EXPECT_EQ(server.PipeCreate(address, ""), S_OK);
  MFPipeImpl client;
  EXPECT_EQ(client.PipeOpen(address, 32, ""), S_OK);

  const int PACKETS_COUNT = 8;
  std::string pstrEvents[PACKETS_COUNT] = { "event1", "event2", "event3", "event4", "event5", "event6", "event7", "event8" };
  std::string pstrMessages[PACKETS_COUNT] = { "message1", "message2", "message3", "message4", "message5", "message6", "message7", "message8" };
  std::string channels[PACKETS_COUNT] = { "ch1", "ch2", "ch1", "ch4", "ch1", "ch3", "ch1", "ch1" };
  for (int i = 0; i < 10; i++) {
    int packet_pos = i % PACKETS_COUNT;
    const auto& channel = channels[packet_pos];

    server.PipeMessagePut(channel, pstrEvents[packet_pos], pstrMessages[packet_pos], WAIT_TIMEOUT);

    EXPECT_EQ(client.PipeMessageGet(channel, &pStrEventName, &messageRecived, WAIT_TIMEOUT), S_OK);

    EXPECT_EQ(pstrEvents[packet_pos], pStrEventName);
    EXPECT_EQ(pstrMessages[packet_pos], messageRecived);
  }

  std::shared_ptr<MF_BASE_TYPE> arrBuffersIn2[PACKETS_COUNT];

  arrBuffersIn2[0] = GenerateMfBuffer();
  arrBuffersIn2[1] = GenerateMfBuffer();
  arrBuffersIn2[2] = GenerateMfFrame();
  arrBuffersIn2[3] = GenerateMfBuffer();
  arrBuffersIn2[4] = GenerateMfBuffer();
  arrBuffersIn2[5] = GenerateMfFrame();
  arrBuffersIn2[6] = GenerateMfBuffer();
  arrBuffersIn2[7] = GenerateMfBuffer();

  for (int i = 0; i < 10; i++) {
    std::shared_ptr<MF_BASE_TYPE> arrBuffersOut;
    int packet_pos = i % PACKETS_COUNT;

    const auto& channel = channels[packet_pos];

    EXPECT_EQ(server.PipePut(channel, arrBuffersIn2[packet_pos], 0, ""), S_OK);

    EXPECT_EQ(client.PipeGet(channel, arrBuffersOut, WAIT_TIMEOUT, ""), S_OK);

    EXPECT_NE(arrBuffersOut.get(), nullptr);
    EXPECT_EQ(arrBuffersIn2[packet_pos]->serialize(), arrBuffersOut->serialize());
  }

  for (int i = 0; i < 10; i++) {
    std::shared_ptr<MF_BASE_TYPE> arrBuffersOut;
    int packet_pos = i % PACKETS_COUNT;

    const auto& channel = channels[packet_pos];

    EXPECT_EQ(server.PipePut(channel, arrBuffersIn2[packet_pos], 0, ""), S_OK);
  }

  for (int i = 0; i < 10; i++) {
    std::shared_ptr<MF_BASE_TYPE> arrBuffersOut;
    int packet_pos = i % PACKETS_COUNT;

    const auto& channel = channels[packet_pos];

    EXPECT_EQ(client.PipeGet(channel, arrBuffersOut, WAIT_TIMEOUT, ""), S_OK);

    EXPECT_NE(arrBuffersOut.get(), nullptr);
    EXPECT_EQ(arrBuffersIn2[packet_pos]->serialize(), arrBuffersOut->serialize());
  }
}

TEST_P(MFPipeTest, Serialization)
{
  const int PACKETS_COUNT = 4;
  std::unique_ptr<MF_FRAME> frame_in[PACKETS_COUNT];
  std::unique_ptr<MF_BUFFER> buffer_in[PACKETS_COUNT];
  for (int i = 0; i < PACKETS_COUNT; i++) {
    frame_in[i] = GenerateMfFrame();
    buffer_in[i] = GenerateMfBuffer();
  }

  for (int i = 0; i < 10; i++) {

    int packet_pos = i % PACKETS_COUNT;

    std::vector<uint8_t> data = frame_in[packet_pos]->serialize();
    std::unique_ptr<MF_FRAME> frame_out = MF_FRAME::deserialize(data.data());

    EXPECT_EQ(*(frame_in[packet_pos]), *frame_out.get());

    std::vector<uint8_t> buffer_data = buffer_in[packet_pos]->serialize();
    std::unique_ptr<MF_BUFFER> buffer_out = MF_BUFFER::deserialize(buffer_data.data());

    EXPECT_EQ(*(buffer_in[packet_pos]), *buffer_out.get());
  }
}

TEST_P(MFPipeTest, PipeClose)
{
  srand(0);
  std::string address = std::get<0>(GetParam());
  std::string pStrEventName = "ch1";
  std::string message = "ch1-message";

  std::string messageRecived;

  MFPipeImpl server;

  MFPipeImpl client;
  EXPECT_EQ(server.PipeCreate(address, ""), S_OK);

  EXPECT_EQ(client.PipeOpen(address, 32, ""), S_OK);

  std::string channel = "ch1";
  std::string strEventName = "event";

  EXPECT_EQ(client.PipeMessagePut(channel, strEventName, "", WAIT_TIMEOUT), S_OK);

  std::string strEventOut;
  std::string pStrEventParamOut;
  EXPECT_EQ(server.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, WAIT_TIMEOUT), S_OK);

  EXPECT_EQ(strEventName, strEventOut);

  EXPECT_EQ(server.PipeMessagePut(channel, strEventName, "", WAIT_TIMEOUT), S_OK);

  strEventOut.clear();
  pStrEventParamOut.clear();
  EXPECT_EQ(client.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, WAIT_TIMEOUT), S_OK);

  EXPECT_EQ(strEventName, strEventOut);

  // PipeClose client`s pipe
  EXPECT_EQ(client.PipeClose(), S_OK);

  EXPECT_EQ(server.PipeMessagePut(channel, strEventName, "", WAIT_TIMEOUT), S_FALSE);

  strEventOut.clear();
  pStrEventParamOut.clear();
  EXPECT_EQ(client.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, WAIT_TIMEOUT), S_FALSE);

  EXPECT_EQ(server.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, WAIT_TIMEOUT), S_FALSE);

  // restart pipes

  sleep(1);
  EXPECT_EQ(server.PipeCreate(address, ""), S_OK);

  EXPECT_EQ(client.PipeOpen(address, 32, ""), S_OK);

  EXPECT_EQ(server.PipeMessagePut(channel, strEventName, "", WAIT_TIMEOUT), S_OK);

  strEventOut.clear();
  pStrEventParamOut.clear();
  EXPECT_EQ(client.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, WAIT_TIMEOUT), S_OK);

  EXPECT_EQ(strEventName, strEventOut);

  // PipeClose server`s pipe
  EXPECT_EQ(server.PipeClose(), S_OK);

  EXPECT_EQ(client.PipeMessagePut(channel, strEventName, "", WAIT_TIMEOUT), S_FALSE);

  strEventOut.clear();
  pStrEventParamOut.clear();
  EXPECT_EQ(server.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, WAIT_TIMEOUT), S_FALSE);

  EXPECT_EQ(client.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, WAIT_TIMEOUT), S_FALSE);
}

TEST_P(MFPipeTest, PipeInfo)
{

  srand(0);
  std::string address = std::get<0>(GetParam());
  std::string pStrEventName = "ch1";
  std::string message = "ch1-message";

  std::string messageRecived;

  MFPipeImpl server;

  EXPECT_EQ(server.PipeCreate(address, ""), S_OK);

  MFPipeImpl client;
  EXPECT_EQ(client.PipeOpen(address, 32, ""), S_OK);

  std::string pipe_name;
  std::string strChannel;
  MFPipe::MF_PIPE_INFO info;
  std::string channel = "ch";
  std::string strEventName = "event";

  EXPECT_EQ(server.PipeMessagePut(channel, strEventName, "", WAIT_TIMEOUT), S_OK);
  sleep(1);

  EXPECT_EQ(client.PipeInfoGet(&pipe_name, channel, &info), S_OK);

  EXPECT_EQ(pipe_name, address);
  EXPECT_EQ(info.nPipeMode, MF_PIPE_MODE::PIPE_READER);
  EXPECT_EQ(info.nChannels, 1);

  //
  //  std::string strPipeName;
  //  server.PipeInfoGet(&strPipeName, "", NULL);
}

TEST_P(MFPipeTest, OverFlow)
{
  srand(0);
  std::string address = std::get<0>(GetParam());

  std::string messageRecived;

  MFPipeImpl server;

  EXPECT_EQ(server.PipeCreate(address, ""), S_OK);
  MFPipeImpl client;

  int nMaxBuffers = 4;
  EXPECT_EQ(client.PipeOpen(address, nMaxBuffers, ""), S_OK);

  const int PACKETS_COUNT = 32;
  std::shared_ptr<MF_BASE_TYPE> arrBuffersIn[PACKETS_COUNT];

  arrBuffersIn[0] = GenerateMfBuffer();
  arrBuffersIn[1] = GenerateMfBuffer();
  arrBuffersIn[2] = GenerateMfFrame();
  arrBuffersIn[3] = GenerateMfBuffer();
  arrBuffersIn[4] = GenerateMfBuffer();
  arrBuffersIn[5] = GenerateMfFrame();
  arrBuffersIn[6] = GenerateMfBuffer();
  arrBuffersIn[7] = GenerateMfBuffer();

  std::string pstrEvents[PACKETS_COUNT] = { "event1", "event2", "event3", "event4", "event5", "event6", "event7", "event8" };
  std::string pstrMessages[PACKETS_COUNT] = { "message1", "message2", "message3", "message4", "message5", "message6", "message7", "message8" };
  std::string channels[PACKETS_COUNT] = { "ch1", "ch2", "ch1", "ch4", "ch1", "ch3", "ch1", "ch1" };

  for (int j = 0; j < 4; j++) {

    for (int i = 0; i < nMaxBuffers / 2; i++) {
      int packet_pos = i % PACKETS_COUNT;
      const auto& channel = channels[packet_pos];

      EXPECT_EQ(server.PipePut(channel, arrBuffersIn[packet_pos], WAIT_TIMEOUT, ""), S_OK);

      EXPECT_EQ(server.PipeMessagePut(channel, pstrEvents[packet_pos], pstrMessages[packet_pos], WAIT_TIMEOUT), S_OK);
    }

    sleep(1);
    // send packets for dropping
    for (int i = 0; i < nMaxBuffers / 2; i++) {
      int packet_pos = i % PACKETS_COUNT;
      const auto& channel = channels[packet_pos];

      EXPECT_EQ(server.PipePut(channel, arrBuffersIn[packet_pos], WAIT_TIMEOUT, ""), S_OK);

      EXPECT_EQ(server.PipeMessagePut(channel, pstrEvents[packet_pos], pstrMessages[packet_pos], WAIT_TIMEOUT), S_OK);
    }

    // checks first
    for (int i = 0; i < nMaxBuffers / 2; i++) {

      std::shared_ptr<MF_BASE_TYPE> arrBuffersOut;
      int packet_pos = i % PACKETS_COUNT;
      const auto& channel = channels[packet_pos];

      EXPECT_EQ(client.PipeGet(channel, arrBuffersOut, WAIT_TIMEOUT, ""), S_OK);

      EXPECT_NE(arrBuffersOut.get(), nullptr);
      EXPECT_EQ(arrBuffersIn[packet_pos]->serialize(), arrBuffersOut->serialize());

      // m_message
      std::string pStrEventName;
      std::string message;
      EXPECT_EQ(client.PipeMessageGet(channel, &pStrEventName, &messageRecived, WAIT_TIMEOUT), S_OK);

      EXPECT_EQ(pstrEvents[packet_pos], pStrEventName);
      EXPECT_EQ(pstrMessages[packet_pos], messageRecived);
    }

    // wait until all packages will be received by client
    sleep(1);

    for (int i = 0; i < nMaxBuffers / 2; i++) {
      std::shared_ptr<MF_BASE_TYPE> arrBuffersOut;
      int packet_pos = i % PACKETS_COUNT;
      const auto& channel = channels[packet_pos];

      EXPECT_EQ(client.PipeGet(channel, arrBuffersOut, 100, ""), S_FALSE);

      // m_message
      std::string pStrEventName;
      std::string message;
      EXPECT_EQ(client.PipeMessageGet(channel, &pStrEventName, &messageRecived, 100), S_FALSE);
    }
  }
}
