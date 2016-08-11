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
// 2. Super simple error checking and output to command prompt on error
// 3. Close socket if error occured.
// 4. if global_verbose == true, cout extra info to command prompt
// 5. WSAStartup() is called in the constructor
// 6. WSACleanup() is called in the deconstructor
// Please note that means whoever is using this class will need to
// call freeaddrinfo(struct addrinfo) when they need to.

// Warnings:
// This source file does not do any input validation.

// Terminology:
// Below is terminology with simple descriptions for anyone new to socket programming:
// fd stands for File Descriptor. It is linux's version of a SOCKET.
// On windows, a SOCKET handle may have any value between 0 to the maximum size of
//  and unsigned integer -1. In other words: 0 to INVALID_SOCKET. This means
//  (~0) is not a value that will ever be assigned as a valid socket. That is why
//  INVALID_SOCKET is defined as (~0) and is returned from various function calls
//  to tell the programmer that something went wrong.
//  On linux it returns -1 for an invalid socket instead of (~0) because SOCKET is
//  defined as an int32_t on linux, and valid SOCKETs will only be positive.

#ifndef SocketClass_h__
#define SocketClass_h__


#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>	// <Winsock2.h>
#include <netdb.h>
#include <string>
#endif//__linux__

#ifdef _WIN32			// Linux equivalent:
#include <WS2tcpip.h>	// socklen_t
#include <WinSock2.h>	// <sys/socket.h>
#include <string>
#endif//_WIN32


#ifdef __linux__
typedef int32_t SOCKET;	// Linux doesn't come with SOCKET defined, unlike Windows.

#define INVALID_SOCKET	(-1)	// To indicate INVALID_SOCKET, Windows returns (~0) from socket functions, and linux returns -1.
#define SOCKET_ERROR	(-1)	// I belive this was just because linux didn't already have a SOCKET_ERROR macro.
#define SD_RECEIVE      SHUT_RD//0x00			// This is for shutdown(); SD_RECEIVE is the code to shutdown receive operations.
#define SD_SEND         SHUT_WR//0x01			// ^
#define SD_BOTH			SHUT_RDWR//0x02			// ^
#endif//__linux__

#ifdef _WIN32
typedef int32_t BYTE_SIZE;	// because recvfrom needs to return ssize_t on linux, and int32_t on win
#endif//_WIN32

class SocketClass
{
public:
	SocketClass();
	virtual ~SocketClass();

	// Currently, the only time something from outside this class will use fd_socket will be to closesocket()
	// during specific situations, and if additional information is needed about the current socket it can be accessed.
	SOCKET fd_socket = INVALID_SOCKET;

	// A global socket used as a way to communicate between threads.
	//static SOCKET global_socket;

	// in accept(), make it return a whole SocketClass, not a socket.
	// 1. ::accept() the connection. it is now stored on a temporary socket
	//    inside the accept() function.
	// 2. Close the previous (non temporary) socket.
	// 3. return a new SocketClass with the socket set.


	SOCKET socket(int32_t address_family, int32_t type, int32_t protocol);
	SOCKET accept();

	// Cross-platform WSAStartup();
	// For every WSAStartup() that is called, a WSACleanup() must be called.
	bool WSAStartup();

	bool setsockopt(int32_t level, int32_t option_name, const char* option_value, int32_t option_length);
	bool bind(const sockaddr *name, int32_t name_len);
	bool shutdown(SOCKET socket, int32_t operation);
	bool listen();
	bool getaddrinfo(std::string target_ip, std::string target_port, const addrinfo *phints, addrinfo **ppresult);

	int32_t inet_pton(int32_t family, char * ip_addr, void * paddr_buf);
	int32_t connect(const sockaddr* name, int32_t name_len);

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
	WSADATA wsaData;			// for WSAStartup();
#endif//_WIN32
};


#endif//SocketClass_h__