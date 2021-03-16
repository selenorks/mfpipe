#include <iostream>

#include <MFPipeImpl.cpp>

#define PACKETS_COUNT (8)

#ifndef SIZEOF_ARRAY
#define SIZEOF_ARRAY(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif // SIZEOF_ARRAY

int
TestMethod1()
{
  std::shared_ptr<MF_BUFFER> arrBuffersIn[PACKETS_COUNT];
  for (int i = 0; i < SIZEOF_ARRAY(arrBuffersIn); ++i) {
    size_t cbSize = 128 * 1024 + rand() % (256 * 1024);
    arrBuffersIn[i] = std::make_shared<MF_BUFFER>();

    arrBuffersIn[i]->flags = eMFBF_Buffer;
    // Fill buffer
    // TODO: fill by test data here
  }

  std::string pstrEvents[] = { "event1", "event2", "event3", "event4", "event5", "event6", "event7", "event8" };
  std::string pstrMessages[] = { "message1", "message2", "message3", "message4", "message5", "message6", "message7", "message8" };

  //////////////////////////////////////////////////////////////////////////
  // Pipe creation

  // Write pipe
  MFPipeImpl MFPipe_Write;
  MFPipe_Write.PipeCreate("udp://127.0.0.1:12345", "");

  // Read pipe
  MFPipeImpl MFPipe_Read;
  MFPipe_Read.PipeOpen("udp://127.0.0.1:12345", 32, "");

  //////////////////////////////////////////////////////////////////////////
  // Test code (single-threaded)
  // TODO: multi-threaded

  // Note: channels ( "ch1", "ch2" is optional)

  for (int i = 0; i < 128; ++i) {
    // Write to pipe
    MFPipe_Write.PipePut("ch1", arrBuffersIn[i % PACKETS_COUNT], 0, "");
    MFPipe_Write.PipePut("ch2", arrBuffersIn[(i + 1) % PACKETS_COUNT], 0, "");

    MFPipe_Write.PipeMessagePut("ch1", pstrEvents[i % PACKETS_COUNT], pstrMessages[i % PACKETS_COUNT], 100); ///
    MFPipe_Write.PipeMessagePut("ch2", pstrEvents[(i + 1) % PACKETS_COUNT], pstrMessages[(i + 1) % PACKETS_COUNT], 100);

    MFPipe_Write.PipePut("ch1", arrBuffersIn[i % PACKETS_COUNT], 0, "");
    MFPipe_Write.PipePut("ch2", arrBuffersIn[(i + 1) % PACKETS_COUNT], 0, "");
    //
    //		std::string strPipeName;
    //		MFPipe_Write.PipeInfoGet(&strPipeName, "", NULL);
    //
    //		MFPipe::MF_PIPE_INFO mfInfo = {};
    //		MFPipe_Write.PipeInfoGet(NULL, "ch2", &mfInfo);
    //		MFPipe_Write.PipeInfoGet(NULL, "ch1", &mfInfo);

    // Read from pipe
    std::string arrStrings[4];

    MFPipe_Read.PipeMessageGet("ch1", &arrStrings[0], &arrStrings[1], 100); //
    MFPipe_Read.PipeMessageGet("ch2", &arrStrings[2], &arrStrings[3], 100);

    if (pstrEvents[i % PACKETS_COUNT] != arrStrings[0] && pstrMessages[i % PACKETS_COUNT] == arrStrings[1]) {
      std::cerr << "pstrEvents != arrStrings, " << pstrEvents[i % PACKETS_COUNT] << " != " << arrStrings[0] << std::endl;
      return 1;
    }

    if (pstrEvents[(i + 1) % PACKETS_COUNT] != arrStrings[2] && pstrMessages[(i + 1) % PACKETS_COUNT] == arrStrings[3]) {
      std::cerr << "pstrEvents != arrStrings, " << pstrEvents[i % PACKETS_COUNT] << " != " << arrStrings[0] << std::endl;
      return 1;
    }

    //        MFPipe_Write.PipeMessageGet("ch2", NULL, &arrStrings[2], 100);
    //
    //
    //
    std::shared_ptr<MF_BASE_TYPE> arrBuffersOut[8];
    MFPipe_Read.PipeGet("ch1", arrBuffersOut[0], 100, "");
    MFPipe_Read.PipeGet("ch2", arrBuffersOut[1], 100, "");

    MFPipe_Read.PipeGet("ch1", arrBuffersOut[4], 100, "");
    MFPipe_Read.PipeGet("ch2", arrBuffersOut[5], 100, "");
    MFPipe_Read.PipeGet("ch2", arrBuffersOut[6], 100, "");

    if (!arrBuffersOut[0] || arrBuffersOut[0]->serialize() != arrBuffersIn[i % PACKETS_COUNT]->serialize()) {
      return 1;
    }

    // TODO: Your test code here
  }

  return 0;
}

int
main(void)
{
  if (TestMethod1()) {
    std::cerr << "TestMethod1: Failed" << std::endl;
    return 1;
  }

  return 0;
}