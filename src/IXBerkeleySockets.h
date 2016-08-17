// IXBerkeleySockets.h
#ifndef IXBerkeleySockets_h__
#define IXBerkeleySockets_h__

#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>	// <Winsock2.h>
#include <netdb.h>
#include <string>
#endif//__linux__
#ifdef _WIN32
#include <WS2tcpip.h>
#include <string>
#endif//_WIN32

// Interface class for XBerkeleySockets
class IXBerkeleySockets
{
	// Typedefs
public:
#ifdef __linux__
	typedef int32_t SOCKET;
#endif//__linux__

public:
	virtual ~IXBerkeleySockets() {};

	// Outputs to console that the connection is being shutdown
	// in addition to the normal shutdown() behavior.
	virtual int32_t shutdown(SOCKET socket, int32_t operation) = 0;

	// Cross-platform closing of a socket / fd.
	virtual void closesocket(SOCKET socket) = 0;

	// All addrinfo structures that have been allocated by the getaddrinfo()
	// function must be freed once they are done being used. Since getaddrinfo()
	// gives you a pointer to the allocated addrinfo structure, you should
	// do something like this example:
	// addrinfo * ServerConnectionInfo = nullptr;
	// getaddrinfo(my_local_ip, my_local_port, &ServerHints, &ServerConnectionInfo)
	// 	if (ServerConnectionInfo != nullptr)
	//		BerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
	//
	// Never freeaddrinfo() on something that has already been freed.
	// In order to avoid doing that, check for a nullptr first.
	// If nullptr, it has already been freed.
	// Example:
	// if (&ClientConnectionInfo != nullptr)
	// {
	//      this->freeaddrinfo(&ClientConnectionInfo);
	// }
	virtual void freeaddrinfo(addrinfo** ppAddrInfo) = 0;

	// Output to console the ip address and port of the peer that you are connect with.
	virtual void coutPeerIPAndPort(SOCKET connection_with_peer) = 0;

	// getError() 99% of cases you won't need to do anything with the return value.
	//	the return value is just incase you want to do something specific with the
	//	WSAGetLastError() (windows), or errno (linux), code. Example would be to check to see if
	//	recvfrom() errored because of a timeout, not because of a real fatal error.
	virtual int32_t getError(bool output_to_console = true) = 0;

	// Accessor for the const DISABLE_CONSOLE_OUTPUT.
	// For use as an arg for getError().
	virtual bool getDisableConsoleOutput() = 0;


	// Only intended for use with Socket errors.
	// Windows outputs a WSAERROR code, linux outputs errno code.
	virtual void outputSocketErrorToConsole(int32_t error_code) = 0;

	// Enable or disable the blocking socket option.
	// By default, blocking is enabled.
	virtual int32_t setBlockingSocketOpt(SOCKET fd_socket, const u_long* option) = 0;

	// Accessors for use an arg for setBlockingSocketOpt()
	virtual const unsigned long& getDisableBlocking() = 0;
	virtual const unsigned long& getEnableBlocking() = 0;

	// Get error information from the socket.
	virtual int32_t getSockOptError(SOCKET fd_socket) = 0;
};

#endif//IXBerkeleySockets_h__