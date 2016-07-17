// SocketClass.h

// I'm really doubting the usefuless of a class like this.
// I mean it does make the code smaller in chat_program
// and therefore easier to skim over and see what is going on,
// but it kinda seems silly. I don't know. I would like some
// input from people on this.

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
// This source file expects any input that is given to it has already been checked
//  for safety and validity. For example if a user supplies a port, then
//  this source file will expect the port will be >= 0, and <= 65535.
//  As with an IP address it will expect it to be valid input, however it doesn't
//  expect you to have checked to see if there is a host at that IP address
//  before giving it to this source file.

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
// buf is buffer. It is a place where information is stored for a time.

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
#define BYTE_SIZE int	// because recvfrom needs to return ssize_t on linux, and int on win
#endif//_WIN32

class SocketClass
{
public:
	SocketClass();
	~SocketClass();


	SOCKET socket(int address_family, int type, int protocol);
	SOCKET accept(SOCKET fd);

	// All the bool functions return false when there is an error. True if everything went fine.
	bool WSAStartup();
	bool setsockopt(SOCKET sock, int level, int option_name, const char* option_value, int option_length);
	bool bind(SOCKET fd, const sockaddr *name, int name_len);
	bool shutdown(SOCKET fd, int operation);
	bool listen(SOCKET fd);
	bool getaddrinfo(std::string target_ip, std::string target_port, const addrinfo *phints, addrinfo **ppresult);

	int inet_pton(int family, char * ip_addr, void * paddr_buf);
	int connect(SOCKET fd, const sockaddr* name, int name_len);
	int send(SOCKET s, const char* buffer, int buffer_length, int flags);
	int sendto(SOCKET s, const char* buf, int len, int flags, const sockaddr *to, int to_len);
	int recv(SOCKET s, char* buf, int buf_len, int flags);
	BYTE_SIZE recvfrom(SOCKET s, char *buf, int buf_len, int flags, sockaddr* from, socklen_t* from_len);

	void closesocket(SOCKET fd);
	void WSACleanup();
	void freeaddrinfo(addrinfo*& pAddrInfo);
	static void coutPeerIPAndPort(SOCKET s);

	const int TIMEOUT_ERROR = -10060;

	// getError() 99% of cases you won't need to do anything with the return value.
	//	the return value is just incase you want to do something specific with the
	//	WSAGetLastError(), or errno, code. Example would be to check to see if
	//	recvfrom() errored because of a timeout, not because of a real error.
	static int getError();	// Noteable oddity here! This shouldn't really be in the SocketClass - it just retrieves errors.

protected:
private:

#ifdef _WIN32
	WSADATA wsaData;			// for WSAStartup();
#endif//_WIN32



};


#endif
