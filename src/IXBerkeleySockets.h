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

	// Shuts down the current connection that is active on the given socket.
	// The shutdown operation is one of three macros.
	// SD_RECEIVE, SD_SEND, SD_BOTH.
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
	virtual void freeaddrinfo(addrinfo** ppAddrInfo) = 0;

	// Display the IP and Port of the peer you have a connection with on the SOCKET.
	virtual void coutPeerIPAndPort(SOCKET connection_with_peer) = 0;

	// For most consistent results, errno needs to be set to 0 before
	// every function call that can return an errno.
	// This method is only intended for use with things that deal with sockets.
	// getError() will output the error code + description unless
	// output_to_console arg is given false.
	// The return value is just incase you want to do something specific with the error code.
	// On windows, this function returns WSAERROR codes.
	// On linux, this function returns errno codes.
	virtual int32_t getError(bool output_to_console = true) = 0;

	// Accessor for the const DISABLE_CONSOLE_OUTPUT.
	// For use as an arg for getError().
	virtual bool getDisableConsoleOutput() = 0;


	// Only intended for use with Socket errors.
	// Attempts to output a description for the error code.
	// On windows it expects a WSAERROR code.
	// On linux it expects an errno code.
	virtual void outputSocketErrorToConsole(int32_t error_code) = 0;

	// With this method you can enable or disable blocking for a given socket.
	// FYI: SOCKETs are blocking by default
	// DISABLE_BLOCKING == 1;
	// ENABLE_BLOCKING == 0;
	// returns 0, success
	// returns -1, error
	virtual int32_t setBlockingSocketOpt(SOCKET fd_socket, const u_long* option) = 0;

	// Accessors for use an arg for setBlockingSocketOpt()
	virtual const unsigned long& getDisableBlocking() = 0;
	virtual const unsigned long& getEnableBlocking() = 0;

	// Get error information from the socket.
	// Returns SOCKET_ERROR, error
	// Returns the socket option error code, success
	virtual int32_t getSockOptError(SOCKET fd_socket) = 0;
};

#endif//IXBerkeleySockets_h__