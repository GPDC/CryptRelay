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
//  defined as an int on linux, and valid SOCKETs will only be positive.

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
typedef int SOCKET;	// Linux doesn't come with SOCKET defined, unlike Windows.
#define BYTE_SIZE ssize_t// because recvfrom needs to return ssize_t on linux, and int on win
#endif//__linux__

#ifdef _WIN32
typedef int BYTE_SIZE;	// because recvfrom needs to return ssize_t on linux, and int on win
#endif//_WIN32

class SocketClass
{
public:
	SocketClass();
	virtual ~SocketClass();

	// Currently, the only time something from outside this class will use fd_socket will be to closesocket()
	// during specific situations, and if additional information is needed about the current socket it can be accessed.
	SOCKET fd_socket;

	// A global socket used as a way to communicate between threads.
	static SOCKET global_socket;

	// in accept(), make it return a whole SocketClass, not a socket.
	// 1. ::accept() the connection. it is now stored on a temporary socket
	//    inside the accept() function.
	// 2. Close the previous (non temporary) socket.
	// 3. return a new SocketClass with the socket set.


	SOCKET socket(int address_family, int type, int protocol);
	SOCKET accept();

	// All the bool functions return false when there is an error. True if everything went fine.
	bool WSAStartup();
	bool setsockopt(int level, int option_name, const char* option_value, int option_length);
	bool bind(const sockaddr *name, int name_len);
	bool shutdown(SOCKET socket, int operation);
	bool listen();
	bool getaddrinfo(std::string target_ip, std::string target_port, const addrinfo *phints, addrinfo **ppresult);

	int inet_pton(int family, char * ip_addr, void * paddr_buf);
	int connect(const sockaddr* name, int name_len);
	int send(const char* buffer, int buffer_length, int flags);
	int sendto(const char* buf, int len, int flags, const sockaddr *to, int to_len);
	int recv(char* buf, int buf_len, int flags);
	BYTE_SIZE recvfrom(char *buf, int buf_len, int flags, sockaddr* from, socklen_t* from_len);

	void closesocket(SOCKET socket);
	void WSACleanup();
	void freeaddrinfo(addrinfo** ppAddrInfo);
	void coutPeerIPAndPort();


	const int CONNECTION_REFUSED = -10061;
	const int TIMEOUT_ERROR = -10060;

	// getError() 99% of cases you won't need to do anything with the return value.
	//	the return value is just incase you want to do something specific with the
	//	WSAGetLastError(), or errno, code. Example would be to check to see if
	//	recvfrom() errored because of a timeout, not because of a real error.
	int getError(bool output_to_console = true);	// This is unlike everything else here - it just retrieves and prints errors.
	const bool DISABLE_CONSOLE_OUTPUT = false;

	// Only intended for use with Socket errors.
	// Windows expects a WSAERROR code, linux expects errno code.
	void outputSocketErrorToConsole(int error_code);

	// Enable or disable the blocking socket option.
	// By default, blocking is enabled.
	bool setBlockingSocketOpt(SOCKET socket, const u_long* option);
	const u_long DISABLE_BLOCKING = 1;
	const u_long ENABLE_BLOCKING = 0;
	

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