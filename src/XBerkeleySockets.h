// XBerkeleySockets.h

// Overview:
// Purpose of this class is to make cross platform versions of
// the Berkeley Sockets API. The 'X' in the class name stands
// for X-platform, or cross-platform.

// Terminology:
// Below is terminology with simple descriptions for anyone new to socket programming:
// fd stands for File Descriptor. It is linux's version of a SOCKET.


#ifndef XBerkeleySockets_h__
#define XBerkeleySockets_h__


#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>	// <Winsock2.h>
#include <netdb.h>
#include <string>

#include "IXBerkeleySockets.h"
#endif//__linux__

#ifdef _WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>	// <sys/socket.h>
#include <string>

#include "IXBerkeleySockets.h"
#endif//_WIN32


class XBerkeleySockets : public IXBerkeleySockets
{
	// Typedefs
public:
#ifdef __linux__
	typedef int32_t SOCKET;
#endif//__linux__

public:
	XBerkeleySockets();
	virtual ~XBerkeleySockets();

	// Outputs to console that the connection is being shutdown
	// in addition to the normal shutdown() behavior.
	int32_t shutdown(SOCKET socket, int32_t operation);
	
	// Cross-platform closing of a socket / fd.
	void closesocket(SOCKET socket);

	// All addrinfo structures that have been allocated by the getaddrinfo()
	// function must be freed once they are done being used. Since getaddrinfo()
	// gives you a pointer to the allocated addrinfo structure, you should
	// do something like this example:
	// addrinfo * ServerConnectionInfo = nullptr;
	// getaddrinfo(my_local_ip, my_local_port, &ServerHints, &ServerConnectionInfo)
	// 	if (ServerConnectionInfo != nullptr)
	//		Socket->freeaddrinfo(&ServerConnectionInfo);
	//
	// Never freeaddrinfo() on something that has already been freed.
	// In order to avoid doing that, check for a nullptr first.
	// If nullptr, it has already been freed.
	// Example:
	// if (&ClientConnectionInfo != nullptr)
	// {
	//      this->freeaddrinfo(&ClientConnectionInfo);
	// }
	void freeaddrinfo(addrinfo** ppAddrInfo);
	void coutPeerIPAndPort(SOCKET connection_with_peer);


	// getError() 99% of cases you won't need to do anything with the return value.
	//	the return value is just incase you want to do something specific with the
	//	WSAGetLastError() (windows), or errno (linux), code. Example would be to check to see if
	//	recvfrom() errored because of a timeout, not because of a real fatal error.
	int32_t getError(bool output_to_console = true);
	static const bool DISABLE_CONSOLE_OUTPUT;


	// Only intended for use with Socket errors.
	// Windows outputs a WSAERROR code, linux outputs errno code.
	void outputSocketErrorToConsole(int32_t error_code);

	// Enable or disable the blocking socket option.
	// By default, blocking is enabled.
	int32_t setBlockingSocketOpt(SOCKET fd_socket, const u_long* option);
	static const unsigned long DISABLE_BLOCKING;
	static const unsigned long ENABLE_BLOCKING;

	// Get error information from the socket.
	int32_t getSockOptError(SOCKET fd_socket);
	

protected:
private:

	// Prevent anyone from copying this class.
	XBerkeleySockets(XBerkeleySockets& SocketClassInstance) = delete;			 // disable copy operator
	XBerkeleySockets& operator=(XBerkeleySockets& SocketClassInstance) = delete; // disable assignment operator

public:
	// Accessors
	bool getDisableConsoleOutput() { return DISABLE_CONSOLE_OUTPUT; }

	// Even though these constants are public, accessors were made so
	// that ppl can access them through the interface.
	const unsigned long& getDisableBlocking() { return DISABLE_BLOCKING; }
	const unsigned long& getEnableBlocking() { return ENABLE_BLOCKING; }
};

#endif//XBerkeleySockets_h__
