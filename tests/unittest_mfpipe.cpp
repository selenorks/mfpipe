#include <iostream>

#include "common_pipe_test.h"
#include <MFPipeImpl.h>

#include <gtest/gtest.h>

#define PACKETS_COUNT (8)

#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif // SIZEOF_ARRAY

TEST(Main, TestMethod1)
{
  srand(0);
  std::shared_ptr<MF_BUFFER> arrBuffersIn[PACKETS_COUNT];
  for (int i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i) {
    size_t cbSize = 128 * 1024 + rand() % (256 * 1024);
    cbSize = 128 * 1024 + rand() % (256 * 1024);
    arrBuffersIn[i] = GenerateMfBuffer(cbSize);
  }

  std::string pstrEvents[] = { "event1", "event2", "event3", "event4", "event5", "event6", "event7", "event8" };
  std::string pstrMessages[] = { "message1", "message2", "message3", "message4", "message5", "message6", "message7", "message8" };

  //////////////////////////////////////////////////////////////////////////
  // Pipe creation

  std::string pipeName = "udp://127.0.0.1:12345";
  // Write pipe
  MFPipeImpl MFPipe_Write;
  MFPipe_Write.PipeCreate(pipeName, "");

  // Read pipe
  MFPipeImpl MFPipe_Read;
  MFPipe_Read.PipeOpen(pipeName, 32, "");

  //////////////////////////////////////////////////////////////////////////
  // Test code (single-threaded)
  // TODO: multi-threaded

  // Note: channels ( "ch1", "ch2" is optional)

  struct ChannelTestStat
  {
    int in = 0;
    int out = 0;
  };
  const int channel_count = 2;
  const std::string test_channels[channel_count] = { "ch1", "ch2" };

  ChannelTestStat ch_object[channel_count];
  ChannelTestStat ch_message[channel_count];

  for (int i = 0; i < 128; ++i) {
    for (int j = 0; j < channel_count; j++)
      ASSERT_EQ(MFPipe_Write.PipePut(test_channels[j], arrBuffersIn[ch_object[j].in++ % PACKETS_COUNT], 0, ""), S_OK);

    for (int j = 0; j < channel_count; j++) {
      ASSERT_EQ(MFPipe_Write.PipeMessagePut(test_channels[j], pstrEvents[ch_message[j].in % PACKETS_COUNT], pstrMessages[ch_message[j].in % PACKETS_COUNT], 100), S_OK);
      ch_message[j].in++;
    }

    for (int j = 0; j < channel_count; j++)
      ASSERT_EQ(MFPipe_Write.PipePut(test_channels[j], arrBuffersIn[ch_object[j].in++ % PACKETS_COUNT], 0, ""), S_OK);


    // Check info
    std::string strPipeName;
    ASSERT_EQ(MFPipe_Write.PipeInfoGet(&strPipeName, "", NULL), S_OK);
    ASSERT_EQ(pipeName, strPipeName);

//    MFPipe::MF_PIPE_INFO mfInfo = {};
//    MFPipe_Write.PipeInfoGet(NULL, "ch2", &mfInfo);
//    MFPipe_Write.PipeInfoGet(NULL, "ch1", &mfInfo);

    // Read from pipe
    std::string arrStrings[2];

    // check messages
    for (int j = 0; j < channel_count; j++) {
      while (S_OK == MFPipe_Read.PipeMessageGet(test_channels[j], &arrStrings[0], &arrStrings[1], 100)) {
        int pos = ch_message[j].out % PACKETS_COUNT;

        ASSERT_EQ(arrStrings[0], pstrEvents[pos]);
        ASSERT_EQ(arrStrings[1], pstrMessages[pos]);

        arrStrings[0].clear();
        arrStrings[1].clear();

        ch_message[j].out++;
        ASSERT_GE(ch_message[j].in, ch_message[j].out);
      }
    }

    // check objects
    std::shared_ptr<MF_BASE_TYPE> arrBufferOut;

    for (int j = 0; j < channel_count; j++) {
      while (S_OK == MFPipe_Read.PipeGet(test_channels[j], arrBufferOut, 100, "")) {
        int pos = ch_object[j].out % PACKETS_COUNT;
        ASSERT_EQ(arrBufferOut, arrBuffersIn[pos]);
        arrBufferOut.reset();
        ch_object[j].out++;
        ASSERT_GE(ch_object[j].in, ch_object[j].out);
      }
    }

    ASSERT_EQ(MFPipe_Read.PipeGet("ch2", arrBufferOut, 100, ""), S_FALSE);
  }

  for (int j = 0; j < channel_count; j++) {
    ASSERT_EQ(ch_object[j].in, ch_object[j].out);
    ASSERT_EQ(ch_message[j].in, ch_message[j].out);
  }
}
