#ifndef C17EFEAC_4B56_4D93_9F7C_51FF941032FC
#define C17EFEAC_4B56_4D93_9F7C_51FF941032FC
#include <algorithm>
#include <iostream>
#include <map>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

constexpr static int MAX_BUFFER_SIZE = 512;
void
AddressInfoDeleter(struct addrinfo* info)
{
  freeaddrinfo(info);
}

void
printErrorMessage(const char* message)
{
  std::cerr << message << ", error code: " << errno << ", error message: " << strerror(errno) << std::endl;
}

struct IPipe
{
  virtual bool create() = 0;
  virtual size_t read(void* buffer, uint16_t buffer_len) = 0;
  virtual size_t write(const void* data, uint16_t len) = 0;
  virtual ~IPipe() {}
};

class UDPPipe : public IPipe
{
public:
  typedef std::unique_ptr<addrinfo, decltype(&AddressInfoDeleter)> AddressInfoPtr;
  AddressInfoPtr getAddressInfo()
  {
    struct addrinfo hints
    {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo* addressInfoPtr;

    int ret_code = getaddrinfo(address.c_str(), port.c_str(), &hints, &addressInfoPtr);
    std::unique_ptr<addrinfo, decltype(&AddressInfoDeleter)> addressInfo(addressInfoPtr, &AddressInfoDeleter);

    if (ret_code < 0) {
      std::cout << "Failed getaddrinfo" << std::endl;
    }
    return addressInfo;
  }

  UDPPipe(std::string address, std::string port)
  {
    memset(&m_client_address, 0, sizeof(m_client_address));
    this->address = address;
    this->port = port;

    const auto& addressInfo = getAddressInfo();
    mempcpy(&m_server_address, addressInfo->ai_addr, addressInfo->ai_addrlen);

    m_socket = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
    if (m_socket < 0) {
      printErrorMessage("Failed to create socket");
      throw std::bad_alloc();
    }

    if (setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, &MAX_BUFFER_SIZE, sizeof(MAX_BUFFER_SIZE)) < 0) {
      // deal with failure, or ignore if you can live with the default size
    }

    if (setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &MAX_BUFFER_SIZE, sizeof(MAX_BUFFER_SIZE)) < 0) {
      std::cout << "error" << std::endl;
      // deal with failure, or ignore if you can live with the default size
    }

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
  }

  virtual ~UDPPipe() { close(m_socket); }

  virtual bool create() = 0;

  size_t read(void* buffer, uint16_t buffer_len) override
  {
    socklen_t len = sizeof(m_client_address);

    int n = recvfrom(m_socket, buffer, buffer_len, MSG_DONTWAIT, (struct sockaddr*)&m_client_address, &len);
    if (n < 0) {
      //      printErrorMessage("Failed to read data");
      return 0;
    }

    return n;
  }

  virtual size_t write(const void* data, uint16_t len) override
  {
    int MAX_PACKAGE_SIZE = 20;
    int pos = 0;
    while (pos < len) {
      int block_size = std::min<int>(MAX_PACKAGE_SIZE, len - pos);
      ssize_t n = sendto(
          m_socket, reinterpret_cast<const char*>(data) + pos, block_size, MSG_CONFIRM, (const struct sockaddr*)&m_client_address, sizeof(m_client_address));
      if (n < 0) {
        printErrorMessage("Failed to send data");
        return -1;
      }
      pos += n;
      // TODO add protocol over UDP for guaranty work
      std::this_thread::yield();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return pos;
  }

  int m_socket = -1;
  sockaddr_in m_client_address, m_server_address{};
  std::string address;
  std::string port;
};

class UDPPipeServer : public UDPPipe
{
public:
  UDPPipeServer(std::string address, std::string port)
      : UDPPipe(address, port)
  {}

  bool create() override
  {
    auto r = bind(m_socket, (struct sockaddr*)&m_server_address, sizeof(m_server_address));
    if (r < 0) {
      printErrorMessage("Failed to bind");
      return false;
    }
    return true;
  }
};

class UDPPipeClient : public UDPPipe
{
public:
  UDPPipeClient(std::string address, std::string port)
      : UDPPipe(address, port)
  {}
  bool create() override
  {
    auto r = connect(m_socket, (const struct sockaddr*)&m_server_address, sizeof(m_server_address));
    if (r < 0) {
      printErrorMessage("Failed to connect to server");
      return false;
    }
    m_client_address = m_server_address;
    return true;
  }
};

#endif