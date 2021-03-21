#include "udp.h"

#include <thread>
#ifdef WIN32
#include <Ws2tcpip.h>
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif

constexpr static int MAX_WAIT_READ_TIMEOUT_MS = 10;

static void
AddressInfoDeleter(struct addrinfo* info)
{
  freeaddrinfo(info);
}

constexpr static int MAX_BUFFER_SIZE = 1024 * 1024;
typedef std::unique_ptr<addrinfo, decltype(&AddressInfoDeleter)> AddressInfoPtr;

AddressInfoPtr
getAddressInfo(const std::string& address, const std::string& port)
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
    addressInfo.reset();
  }

  return addressInfo;
}

static void
printErrorMessage(const char* message)
{
#ifdef WIN32
  std::cerr << message << ", error code: " << GetLastError() << std::endl;
#else
  std::cerr << message << ", error code: " << errno << ", error message: " << strerror(errno) << std::endl;
#endif
}

bool
UDPPipeClient::create()
{
  if (!init())
    return false;
  auto r = connect(m_socket, (const struct sockaddr*)&m_src_address, sizeof(m_src_address));
  if (r < 0) {
    printErrorMessage("Failed to connect to server");
    return false;
  }
  return true;
}
UDPPipeClient::UDPPipeClient(const std::string& address, const std::string& port)
    : m_address(address)
    , m_port(port)
{}

bool
UDPPipeClient::init()
{
#ifdef WIN32
  WSADATA wsaData;
  int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (res != NO_ERROR) {
    std::cerr << "WSAStartup failed with error: " << res << std::endl;
    return false;
  }
#endif

  const auto& addressInfo = getAddressInfo(m_address, m_port);
  memcpy(&m_src_address, addressInfo->ai_addr, addressInfo->ai_addrlen);
  memcpy(&m_dst_address, addressInfo->ai_addr, addressInfo->ai_addrlen);

  m_socket = socket(addressInfo->ai_family, addressInfo->ai_socktype, addressInfo->ai_protocol);
  if (m_socket < 0) {
    printErrorMessage("Failed to create socket");
    return false;
  }

  if (setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&MAX_BUFFER_SIZE), sizeof(MAX_BUFFER_SIZE)) < 0) {
    std::cout << "error" << std::endl;
    printErrorMessage("Failed setsockopt");
  }

  if (setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&MAX_BUFFER_SIZE), sizeof(MAX_BUFFER_SIZE)) < 0) {
    printErrorMessage("Failed setsockopt");
  }

  struct timeval timeout
  {};
  timeout.tv_usec = MAX_WAIT_READ_TIMEOUT_MS;

  if (setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
    printErrorMessage("Failed setsockopt");
  }

  if (setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
    printErrorMessage("Failed setsockopt");
  }

  return true;
}

UDPPipeClient::~UDPPipeClient()
{
#ifdef WIN32
  closesocket(m_socket);
  WSACleanup();
#else
  close(m_socket);
#endif
}
size_t
UDPPipeClient::write(const void* data, uint32_t len)
{
#ifdef WIN32
  constexpr static int MAX_PACKAGE_SIZE = 534;
#else
  constexpr static int MAX_PACKAGE_SIZE = 65535 - 28;
#endif
  int pos = 0;
  while (pos < len) {
    int block_size = std::min<int>(MAX_PACKAGE_SIZE, len - pos);
    auto n = sendto(m_socket, reinterpret_cast<const char*>(data) + pos, block_size, 0, (const struct sockaddr*)&m_dst_address, sizeof(m_dst_address));
    if (n < 0) {
      printErrorMessage("Failed to send data");
      return -1;
    }
    pos += n;
    // TODO add protocol over UDP for guaranty work
    std::this_thread::yield();
  }
  return pos;
}

size_t
UDPPipeClient::read(void* buffer, uint32_t buffer_len)
{
#ifdef WIN32
  fd_set fds{};
  struct timeval tv{};
  FD_SET(m_socket, &fds);
  // Set up the struct timeval for the timeout.
  tv.tv_usec = MAX_WAIT_READ_TIMEOUT_MS ;
  auto r = select ( m_socket, &fds, NULL, NULL, &tv );
  if(r==0)
    return 0;

  socklen_t len = sizeof(m_dst_address);

  auto n = recvfrom(m_socket, (char*)buffer, buffer_len, 0, (struct sockaddr*)&m_dst_address, &len);
#else
  auto n = recvfrom(m_socket, buffer, buffer_len, MSG_DONTWAIT, (struct sockaddr*)&m_dst_address, &len);
#endif
  if (n < 0) {
    return 0;
  }

  return n;
}

UDPPipeServer::UDPPipeServer(const std::string& address, const std::string& port)
    : UDPPipeClient(address, port)
{}

bool
UDPPipeServer::create()
{
  if (!init())
    return false;

  auto r = bind(m_socket, (struct sockaddr*)&m_src_address, sizeof(m_src_address));
  if (r < 0) {
    printErrorMessage("Failed to bind");
    return false;
  }
  return true;
}

size_t
UDPPipeServer::write(const void* data, uint32_t len)
{
  if (!m_is_client_connected) {
    std::this_thread::yield();
    return 0;
  }
  return UDPPipeClient::write(data, len);
}

size_t
UDPPipeServer::read(void* buffer, uint32_t buffer_len)
{
  auto n = UDPPipeClient::read(buffer, buffer_len);
  if (n)
    m_is_client_connected = true;
  return n;
}
