#include <gtest/gtest.h>

#include "common_pipe_test.h"
#include <MFPipeImpl.h>

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
  ASSERT_EQ(MFPipe_Write.PipeCreate(url, ""), E_INVALIDARG);
}

INSTANTIATE_TEST_SUITE_P(CreateInvalid, MFPipeInvalidTest, ::testing::Combine(::testing::Values("", "", "tcp://127.0.0.1:12345")));

class MFPipeTest : public testing::TestWithParam<::std::tuple<std::string>>
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

constexpr static int PIPE_WAIT_TIMEOUT = 500;

INSTANTIATE_TEST_SUITE_P(CreateUDP,
                         MFPipeTest,
                         ::testing::Combine(::testing::Values("udp://127.0.0.1:12345",
                                                              "Udp://127.0.0.1:12345")));

TEST_P(MFPipeTest, SingleChannel)
{
  std::string address = std::get<0>(GetParam());
  std::string pStrEventName = "ch1";
  std::string message = "test";

  std::string messageRecived;

  MFPipeImpl MFPipe_Write;

  ASSERT_EQ(MFPipe_Write.PipeCreate(address, ""), S_OK);
  MFPipeImpl MFPipe_Read;
  ASSERT_EQ(MFPipe_Read.PipeOpen(address, 32, ""), S_OK);

  ASSERT_EQ(MFPipe_Write.PipeMessagePut("ch1", message, pStrEventName, PIPE_WAIT_TIMEOUT), S_OK);
  //  sleep(10);

  ASSERT_EQ(MFPipe_Read.PipeMessageGet("ch2", &messageRecived, &pStrEventName, PIPE_WAIT_TIMEOUT), S_FALSE);
  ASSERT_EQ(MFPipe_Read.PipeMessageGet("ch1", &messageRecived, &pStrEventName, PIPE_WAIT_TIMEOUT), S_OK);

  ASSERT_EQ(message, messageRecived);
  std::cout << "finished" << std::endl;
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

  std::string messageRecieved;

  MFPipeImpl server;

  ASSERT_EQ(server.PipeCreate(address, ""), S_OK);
  MFPipeImpl client;
  ASSERT_EQ(client.PipeOpen(address, 32, ""), S_OK);

  ASSERT_EQ(server.PipeMessagePut("ch1", message, pStrEventName, PIPE_WAIT_TIMEOUT), S_OK);

  ASSERT_EQ(client.PipeMessageGet("ch2", &messageRecieved, &pStrEventName, PIPE_WAIT_TIMEOUT), S_FALSE);
  ASSERT_EQ(client.PipeMessageGet("ch1", &messageRecieved, &pStrEventName, PIPE_WAIT_TIMEOUT), S_OK);
  ASSERT_EQ(message, messageRecieved);

  ASSERT_EQ(server.PipeMessagePut("ch2", message, pStrEventName, PIPE_WAIT_TIMEOUT), S_OK);
  ASSERT_EQ(client.PipeMessageGet("ch2", &messageRecieved, &pStrEventName, PIPE_WAIT_TIMEOUT), S_OK);

  ASSERT_EQ(message, messageRecieved);
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
    message[i] = std::rand() / ((RAND_MAX + 1u) / (std::numeric_limits<char>::max)());

  std::string messageRecieved;

  MFPipeImpl server;

  ASSERT_EQ(server.PipeCreate(address, ""), S_OK);
  MFPipeImpl client;
  ASSERT_EQ(client.PipeOpen(address, 32, ""), S_OK);

  ASSERT_EQ(server.PipeMessagePut("ch1", message, pStrEventName, PIPE_WAIT_TIMEOUT), S_OK);

  ASSERT_EQ(client.PipeMessageGet("ch1", &messageRecieved, &pStrEventName, PIPE_WAIT_TIMEOUT * 10), S_OK);
  ASSERT_EQ(message.size(), messageRecieved.size());
}

TEST_P(MFPipeTest, ReBind)
{
  std::string address = std::get<0>(GetParam());

  MFPipeImpl server1;
  ASSERT_EQ(server1.PipeCreate(address, ""), S_OK);

  MFPipeImpl server2;
  ASSERT_EQ(server2.PipeCreate(address, ""), S_FALSE);

  MFPipeImpl client1;
  ASSERT_EQ(client1.PipeOpen(address, 32, ""), S_OK);

  std::string message = "rebind";

  std::string messageRecived;
  std::string pStrEventName = "ch1";
  ASSERT_EQ(server1.PipeMessagePut("ch1", message, pStrEventName, PIPE_WAIT_TIMEOUT), S_OK);

  ASSERT_EQ(client1.PipeMessageGet("ch1", &messageRecived, &pStrEventName, PIPE_WAIT_TIMEOUT), S_OK);
  ASSERT_EQ(message.size(), messageRecived.size());
}

TEST_P(MFPipeTest, MultiEvent)
{
  srand(0);
  std::string address = std::get<0>(GetParam());
  std::string pStrEventName = "ch1";
  std::string message;

  std::string messageReceived;

  MFPipeImpl server;

  ASSERT_EQ(server.PipeCreate(address, ""), S_OK);
  MFPipeImpl client;
  ASSERT_EQ(client.PipeOpen(address, 32, ""), S_OK);

  const int PACKETS_COUNT = 8;
  std::string pstrEvents[PACKETS_COUNT] = { "event1", "event2", "event3", "event4", "event5", "event6", "event7", "event8" };
  std::string pstrMessages[PACKETS_COUNT] = { "message1", "message2", "message3", "message4", "message5", "message6", "message7", "message8" };
  std::string channels[PACKETS_COUNT] = { "ch1", "ch2", "ch1", "ch4", "ch1", "ch3", "ch1", "ch1" };
  for (int i = 0; i < 10; i++) {
    int packet_pos = i % PACKETS_COUNT;
    const auto& channel = channels[packet_pos];

    server.PipeMessagePut(channel, pstrEvents[packet_pos], pstrMessages[packet_pos], PIPE_WAIT_TIMEOUT);

    ASSERT_EQ(client.PipeMessageGet(channel, &pStrEventName, &messageReceived, PIPE_WAIT_TIMEOUT), S_OK);

    ASSERT_EQ(pstrEvents[packet_pos], pStrEventName);
    ASSERT_EQ(pstrMessages[packet_pos], messageReceived);
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

    ASSERT_EQ(server.PipePut(channel, arrBuffersIn2[packet_pos], 0, ""), S_OK);

    ASSERT_EQ(client.PipeGet(channel, arrBuffersOut, PIPE_WAIT_TIMEOUT, ""), S_OK);

    EXPECT_NE(arrBuffersOut.get(), nullptr);
    ASSERT_EQ(arrBuffersIn2[packet_pos]->serialize(), arrBuffersOut->serialize());
  }

  for (int i = 0; i < 10; i++) {
    std::shared_ptr<MF_BASE_TYPE> arrBuffersOut;
    int packet_pos = i % PACKETS_COUNT;

    const auto& channel = channels[packet_pos];

    ASSERT_EQ(server.PipePut(channel, arrBuffersIn2[packet_pos], 0, ""), S_OK);
  }

  for (int i = 0; i < 10; i++) {
    std::shared_ptr<MF_BASE_TYPE> arrBuffersOut;
    int packet_pos = i % PACKETS_COUNT;

    const auto& channel = channels[packet_pos];

    ASSERT_EQ(client.PipeGet(channel, arrBuffersOut, PIPE_WAIT_TIMEOUT, ""), S_OK);

    EXPECT_NE(arrBuffersOut.get(), nullptr);
    ASSERT_EQ(arrBuffersIn2[packet_pos]->serialize(), arrBuffersOut->serialize());
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
    std::string_view s(reinterpret_cast<const char*>(data.data()), data.size());
    std::unique_ptr<MF_FRAME> frame_out = MF_FRAME::deserialize(s);

    ASSERT_EQ(*(frame_in[packet_pos]), *frame_out.get());

    std::vector<uint8_t> buffer_data = buffer_in[packet_pos]->serialize();

    s = { reinterpret_cast<const char*>(buffer_data.data()), buffer_data.size() };
    std::unique_ptr<MF_BUFFER> buffer_out = MF_BUFFER::deserialize(s);

    ASSERT_EQ(*(buffer_in[packet_pos]), *buffer_out.get());
  }
}

TEST_P(MFPipeTest, PipeClose)
{
  srand(0);
  std::string address = std::get<0>(GetParam());
  std::string pStrEventName;
  std::string message;

  std::string messageReceived;

  MFPipeImpl MFPipe_Write;

  MFPipeImpl MFPipe_Reader;
  ASSERT_EQ(MFPipe_Write.PipeCreate(address, ""), S_OK);

  ASSERT_EQ(MFPipe_Reader.PipeOpen(address, 32, ""), S_OK);

  std::string channel = "ch1";
  std::string strEventName = "event";

  std::string strEventOut;
  std::string pStrEventParamOut;

  ASSERT_EQ(MFPipe_Write.PipeMessagePut(channel, strEventName, "", PIPE_WAIT_TIMEOUT), S_OK);

  strEventOut.clear();
  pStrEventParamOut.clear();
  ASSERT_EQ(MFPipe_Reader.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, PIPE_WAIT_TIMEOUT), S_OK);

  ASSERT_EQ(strEventName, strEventOut);

  // PipeClose MFPipe_Reader`s pipe
  ASSERT_EQ(MFPipe_Reader.PipeClose(), S_OK);
  std::this_thread::sleep_for(std::chrono::milliseconds(PIPE_WAIT_TIMEOUT));

  ASSERT_EQ(MFPipe_Write.PipeMessagePut(channel, strEventName, "", PIPE_WAIT_TIMEOUT), S_FALSE);

  strEventOut.clear();
  pStrEventParamOut.clear();
  ASSERT_EQ(MFPipe_Reader.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, PIPE_WAIT_TIMEOUT), S_FALSE);

  ASSERT_EQ(MFPipe_Write.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, PIPE_WAIT_TIMEOUT), S_FALSE);

  // restart pipes

  std::this_thread::sleep_for(std::chrono::milliseconds(PIPE_WAIT_TIMEOUT));
  ASSERT_EQ(MFPipe_Write.PipeCreate(address, ""), S_OK);

  ASSERT_EQ(MFPipe_Reader.PipeOpen(address, 32, ""), S_OK);

  ASSERT_EQ(MFPipe_Write.PipeMessagePut(channel, strEventName, "", PIPE_WAIT_TIMEOUT), S_OK);

  strEventOut.clear();
  pStrEventParamOut.clear();
  ASSERT_EQ(MFPipe_Reader.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, PIPE_WAIT_TIMEOUT), S_OK);

  ASSERT_EQ(strEventName, strEventOut);

  // PipeClose MFPipe_Write`s pipe
  ASSERT_EQ(MFPipe_Write.PipeClose(), S_OK);
  std::this_thread::sleep_for(std::chrono::milliseconds(PIPE_WAIT_TIMEOUT));

  ASSERT_EQ(MFPipe_Reader.PipeMessagePut(channel, strEventName, "", PIPE_WAIT_TIMEOUT), S_FALSE);

  strEventOut.clear();
  pStrEventParamOut.clear();
  ASSERT_EQ(MFPipe_Write.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, PIPE_WAIT_TIMEOUT), S_FALSE);

  ASSERT_EQ(MFPipe_Reader.PipeMessageGet(channel, &strEventOut, &pStrEventParamOut, PIPE_WAIT_TIMEOUT), S_FALSE);
}

TEST_P(MFPipeTest, PipeInfo)
{

  srand(0);
  std::string address = std::get<0>(GetParam());
  std::string pStrEventName;
  std::string message;

  std::string messageReceived;

  MFPipeImpl MFPipe_Write;

  ASSERT_EQ(MFPipe_Write.PipeCreate(address, ""), S_OK);

  MFPipeImpl client;
  ASSERT_EQ(client.PipeOpen(address, 32, ""), S_OK);

  std::string pipe_name;
  std::string strChannel;
  MFPipe::MF_PIPE_INFO info;
  std::string channel = "ch";
  std::string strEventName = "event";

  ASSERT_EQ(MFPipe_Write.PipeMessagePut(channel, strEventName, "", PIPE_WAIT_TIMEOUT), S_OK);
  std::this_thread::sleep_for(std::chrono::seconds(1));

  ASSERT_EQ(client.PipeInfoGet(&pipe_name, channel, &info), S_OK);

  ASSERT_EQ(pipe_name, address);
  ASSERT_EQ(info.nPipeMode, MF_PIPE_MODE::PIPE_READER);
  ASSERT_EQ(info.nChannels, 1);

  ASSERT_EQ(MFPipe_Write.PipeMessagePut(channel, strEventName, "", PIPE_WAIT_TIMEOUT), S_OK);
  std::this_thread::sleep_for(std::chrono::seconds(1));

  //  info={};
  //  ASSERT_EQ(client.PipeInfoGet(nullptr, channel, &info), S_OK);
  //  ASSERT_EQ(info.nPipeMode, MF_PIPE_MODE::PIPE_READER);
  //  ASSERT_EQ(info.nChannels, 1);
  //  ASSERT_EQ(info.nObjectsHave, 1);
  //  ASSERT_EQ(info.nObjectsMax, 1);

  //
  //  std::string strPipeName;
  //  MFPipe_Write.PipeInfoGet(&strPipeName, "", NULL);
}

TEST_P(MFPipeTest, OverFlow)
{
  srand(0);
  std::string address = std::get<0>(GetParam());

  std::string messageRecieved;

  MFPipeImpl server;

  ASSERT_EQ(server.PipeCreate(address, ""), S_OK);
  MFPipeImpl client;

  int nMaxBuffers = 4;
  ASSERT_EQ(client.PipeOpen(address, nMaxBuffers, ""), S_OK);

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

      ASSERT_EQ(server.PipePut(channel, arrBuffersIn[packet_pos], PIPE_WAIT_TIMEOUT, ""), S_OK);

      ASSERT_EQ(server.PipeMessagePut(channel, pstrEvents[packet_pos], pstrMessages[packet_pos], PIPE_WAIT_TIMEOUT), S_OK);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    // send packets for dropping
    for (int i = 0; i < nMaxBuffers / 2; i++) {
      int packet_pos = i % PACKETS_COUNT;
      const auto& channel = channels[packet_pos];

      ASSERT_EQ(server.PipePut(channel, arrBuffersIn[packet_pos], PIPE_WAIT_TIMEOUT, ""), S_OK);

      ASSERT_EQ(server.PipeMessagePut(channel, pstrEvents[packet_pos], pstrMessages[packet_pos], PIPE_WAIT_TIMEOUT), S_OK);
    }
    // wait until all packets are dropped
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // checks first
    for (int i = 0; i < nMaxBuffers / 2; i++) {

      std::shared_ptr<MF_BASE_TYPE> arrBuffersOut;
      int packet_pos = i % PACKETS_COUNT;
      const auto& channel = channels[packet_pos];

      ASSERT_EQ(client.PipeGet(channel, arrBuffersOut, PIPE_WAIT_TIMEOUT, ""), S_OK);

      EXPECT_NE(arrBuffersOut.get(), nullptr);
      ASSERT_EQ(arrBuffersIn[packet_pos]->serialize(), arrBuffersOut->serialize());

      // m_message
      std::string pStrEventName;
      std::string message;
      ASSERT_EQ(client.PipeMessageGet(channel, &pStrEventName, &messageRecieved, PIPE_WAIT_TIMEOUT), S_OK);

      ASSERT_EQ(pstrEvents[packet_pos], pStrEventName);
      ASSERT_EQ(pstrMessages[packet_pos], messageRecieved);
    }

    // wait until all packages will be received by client
    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int i = 0; i < nMaxBuffers / 2; i++) {
      std::shared_ptr<MF_BASE_TYPE> arrBuffersOut;
      int packet_pos = i % PACKETS_COUNT;
      const auto& channel = channels[packet_pos];

      ASSERT_EQ(client.PipeGet(channel, arrBuffersOut, 100, ""), S_FALSE);

      // m_message
      std::string pStrEventName;
      std::string message;
      ASSERT_EQ(client.PipeMessageGet(channel, &pStrEventName, &messageRecieved, 100), S_FALSE);
    }
  }
}
struct TestMessage
{
  std::string channel;
  std::string strEventName;
  std::string strEventParam;
  bool operator==(const TestMessage& other) const
  {
    return channel == other.channel && strEventName == other.strEventName && strEventParam == other.strEventParam;
  }
};
