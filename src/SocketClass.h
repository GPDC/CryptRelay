// SocketClass.h

// I'm really doubting the usefuless of a class like this.
// I mean it does make the code smaller in chat_program
// and therefore easier to skim over and see what is going on.

// Overview:
// Purpose of this class is to place all sockets related things here, and take
// the normal socket API (winsock, linux sockets) and provide
// a slightly higher level function to replace them. The functions will include
// these new features in order to reduce clutter in the rest of the program:
// 1. Cross platform windows, linux.
// 2. 
// 3. if global_verbose == true, cout extra info to command prompt
// 4. WSAStartup() is called in the constructor
// 5. WSACleanup() is called in the deconstructor
// Please note that means freeaddrinfo() is not being called for you.
// Whoever is using this class will need to
// call freeaddrinfo(addrinfo* ) when they need to.

// Terminology:
// Below is terminology with simple descriptions for anyone new to socket programming:
// fd stands for File Descriptor. It is linux's version of a SOCKET.


#ifndef SocketClass_h__
#define SocketClass_h__


#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>	// <Winsock2.h>
#include <netdb.h>
#include <string>
#endif//__linux__

#ifdef _WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>	// <sys/socket.h>
#include <string>
#endif//_WIN32


#ifdef __linux__
typedef int32_t SOCKET;	// Linux doesn't come with SOCKET defined, unlike Windows.

#define INVALID_SOCKET	(-1)	// To indicate INVALID_SOCKET, Windows returns (~0) from socket functions, and linux returns -1.
#define SOCKET_ERROR	(-1)	// I belive this was just because linux didn't already have a SOCKET_ERROR macro.
#define SD_RECEIVE      SHUT_RD//0x00			// This is for shutdown(); SD_RECEIVE is the code to shutdown receive operations.
#define SD_SEND         SHUT_WR//0x01			// ^
#define SD_BOTH         SHUT_RDWR//0x02			// ^
#endif//__linux__


class SocketClass
{
public:
	SocketClass();
	virtual ~SocketClass();

	// Currently, the only time something from outside this class will use fd_socket will be to closesocket()
	// during specific situations, and if additional information is needed about the current socket it can be accessed.
	SOCKET fd_socket = INVALID_SOCKET;


	// Cross-platform accept(), and assigns fd_socket to the newly accept()ed socket.
	SOCKET accept();

	// Cross-platform WSAStartup();
	// For every WSAStartup() that is called, a WSACleanup() must be called.
	bool WSAStartup();

	// Outputs to console that the connection is being shutdown
	// in addition to the normal shutdown() behavior.
	bool shutdown(SOCKET socket, int32_t operation);
	
	// Cross-platform closing of a socket / fd.
	void closesocket(SOCKET socket);

	// Crossplatform WSACleanup();
	// For every WSAStartup() that is called, a WSACleanup() must be called.
	void WSACleanup();

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
	void freeaddrinfo(addrinfo** ppAddrInfo);
	void coutPeerIPAndPort();


	// getError() 99% of cases you won't need to do anything with the return value.
	//	the return value is just incase you want to do something specific with the
	//	WSAGetLastError() (windows), or errno (linux), code. Example would be to check to see if
	//	recvfrom() errored because of a timeout, not because of a real fatal error.
	int32_t getError(bool output_to_console = true);
	const bool DISABLE_CONSOLE_OUTPUT = false;

	// Only intended for use with Socket errors.
	// Windows outputs a WSAERROR code, linux outputs errno code.
	void outputSocketErrorToConsole(int32_t error_code);

	// Enable or disable the blocking socket option.
	// By default, blocking is enabled.
	bool setBlockingSocketOpt(SOCKET socket, const u_long* option);
	const u_long DISABLE_BLOCKING = 1;
	const u_long ENABLE_BLOCKING = 0;

	// Get error information from the socket.
	int32_t getSockOptError(SOCKET fd_socket);
	

protected:
private:

	// Prevent anyone from copying this class.
	SocketClass(SocketClass& SocketClassInstance) = delete;			   // disable copy operator
	SocketClass& operator=(SocketClass& SocketClassInstance) = delete; // disable assignment operator

#ifdef _WIN32
	WSADATA wsaData;	// for WSAStartup();
#endif//_WIN32
};


#endif//SocketClass_h__
