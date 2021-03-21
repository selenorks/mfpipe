#ifndef UUID_72F0CFE5_5C09_46D4_9765_BF71494CD1D3
#define UUID_72F0CFE5_5C09_46D4_9765_BF71494CD1D3
#include <algorithm>
#include <cstring>
#include <iostream>
#ifdef WIN32
#include "winsock2.h"
#else
#include <netdb.h>
#endif
#include <sys/types.h>

#include "internal/protocols/ipipe.h"

class UDPPipeClient : public IPipe
{
public:
  UDPPipeClient(const std::string& address, const std::string& port);
  ~UDPPipeClient() override;

  bool create() override;
  size_t read(void* buffer, uint32_t buffer_len) override;
  size_t write(const void* data, uint32_t len) override;

protected:
  bool init();

  int m_socket = -1;
  sockaddr_in m_src_address{};
  sockaddr_in m_dst_address{};
  std::string m_address;
  std::string m_port;
};

class UDPPipeServer : public UDPPipeClient
{
public:
  UDPPipeServer(const std::string& address, const std::string& port);

  bool create() override;
  size_t read(void* buffer, uint32_t buffer_len) override;
  size_t write(const void* data, uint32_t len) override;

protected:
  bool m_is_client_connected = false;
};



#endif