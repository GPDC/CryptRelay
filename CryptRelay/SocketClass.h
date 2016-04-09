// SocketClass.h

// Purpose:
// Purpose of this class is to place all sockets related things here, and take
// the normal socket API (winsock, linux sockets) and provide
// a slightly higher level function to replace them. The functions will include
// these new features in order to reduce clutter in the rest of the program:
// 1. Cross platform windows, linux.
// 2. Super simple error checking and output to command prompt
// 3. Close socket if error occured.
// 4. if global_verbose == true, cout extra info to command prompt

// Terminology:
// Below is terminology with simple descriptions for anyone new to socket programming:
// fd stands for File Descriptor. It is linux's version of a SOCKET.
// buf is buffer

#ifndef SocketClass_h__
#define SocketClass_h__


#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>	// <Winsock2.h>
#include <netdb.h>
#include <string>
#endif//__linux__

#ifdef _WIN32			// Linux equivalent:
#include <WinSock2.h>	// <sys/socket.h>
#include <string>
#endif//_WIN32


#ifdef __linux__
typedef u_int SOCKET;		// Linux doesn't come with SOCKET defined, unlike Windows.
#endif//__linux__

class SocketClass
{
public:
	SocketClass();
	~SocketClass();


	SOCKET mySocket(int address_family, int type, int protocol);
	SOCKET myConnect(SOCKET fd, const sockaddr* name, int name_len);
	SOCKET myAccept(SOCKET fd);

	// All the bool functions return false when there is an error. True if everything went fine.
	bool myWSAStartup();
	bool mySetSockOpt(SOCKET sock, int level, int option_name, const char* option_value, int option_length);
	bool myBind(SOCKET fd, const sockaddr *name, int name_len);
	bool myShutdown(SOCKET fd, int operation);
	bool myListen(SOCKET fd);
	bool myGetAddrInfo(std::string target_ip, std::string target_port, const ADDRINFOA *phints, PADDRINFOA *ppresult);

	int myinet_pton(int family, PCSTR ip_addr, PVOID paddr_buf);
	int mySend(SOCKET s, const char* buffer, int buffer_length, int flags);
	int mySendTo(SOCKET s, const char* buf, int len, int flags, const sockaddr *to, int to_len);
	int myRecv(SOCKET s, char* buf, int buf_len, int flags);
	int	myRecvFrom(SOCKET s, char *buf, int buf_len, int flags, sockaddr* from, int *from_len);

	void myCloseSocket(SOCKET fd);
	void myWSACleanup();
	void myFreeAddrInfo(PADDRINFOA pAddrInfo);

	sockaddr_storage incomingAddr;
	addrinfo		 hints;		// once hints is given to getaddrinfo() it will return *result
	addrinfo		 *result;	// result now contains all relevant info for ip address, family, protocol, etc
								// *result is ONLY used if there is a getaddrinfo() and result is put in the arguments.
	addrinfo		 *ptr;


protected:
private:

#ifdef _WIN32
	WSADATA wsaData;
#endif//_WIN32

	// getError() 99% of cases you won't need to do anything with the return value.
	//	the return value is just incase you want to do something specific with the
	//	WSAGetLastError(), or errno, code. Example would be to check to see if
	//	recvfrom() errored because of a timeout, not because of a real error.
	int getError(int errchk_number);	// Noteable oddity here! This shouldn't really be in the SocketClass - it just retrieves errors.

};


#endif