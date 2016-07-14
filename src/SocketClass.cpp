// SocketClass.cpp
// See SocketClass.h for more details about SocketClass.cpp

#ifdef __linux__
#include <iostream>
#include <vector>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <cerrno>
#include <arpa/inet.h>
#include <signal.h>

#include "SocketClass.h"
#include "GlobalTypeHeader.h"
#endif//__linux__

#ifdef _WIN32
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "SocketClass.h"
#include "GlobalTypeHeader.h"
#endif//_WIN32

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")		//tell the linker that Ws2_32.lib file is needed.
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#endif//_WIN32
#ifdef __linux__
#define INVALID_SOCKET	(-1)	// To indicate INVALID_SOCKET, Windows returns (~0) from socket functions, and linux returns -1.
#define SOCKET_ERROR	(-1)	// I belive this was just because linux didn't already have a SOCKET_ERROR macro.
#define SD_RECEIVE      SHUT_RD//0x00			// This is for shutdown(); SD_RECEIVE is the code to shutdown receive operations.
#define SD_SEND         SHUT_WR//0x01			// ^
#define SD_BOTH			SHUT_RDWR//0x02			// ^
#endif//__linux__

#ifdef __linux__
#define WSAETIMEDOUT 10060	// possibly temporary... myRecvFrom() isn't being used?
#endif//__linux__

SocketClass::SocketClass()
{
	// Unsure If I should have this here quite frankly, but
	// it IS necessary for 99% of things in this class.
	myWSAStartup();
}
SocketClass::~SocketClass()
{
	myWSACleanup();
}

// Necessary to do anything with sockets on Windows
bool SocketClass::myWSAStartup()
{
#ifdef _WIN32
	if (global_verbose == true)
		std::cout << "Initializing Winsock... ";

	int errchk = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (errchk != 0)
	{
		getError();
		std::cout << "WSAStartup failed\n";
		return false;
	}
	if (global_verbose == true)
		std::cout << "Success\n";
#endif//_WIN32
	return true;
}

// Use this to set socket options such as broadcast, keepalive, max msg size, etc
bool SocketClass::mySetSockOpt(SOCKET sock, int level, int option_name, const char* option_value, int option_length)
{
	if (global_verbose == true)
		std::cout << "Setting socket options... ";
	int errchk = setsockopt(sock, level, option_name, option_value, option_length);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "mySetSockOpt() failed.\n";
		// hmm should i close socket here? or leave it up to whoever called this function?
		return false;
	}
	if (global_verbose == true)
		std::cout << "Success\n";
	return true;
}

// Create a socket. Returns INVALID_SOCKET on error.
SOCKET SocketClass::mySocket(int address_family, int socket_type, int protocol)
{
	if (global_verbose == true)
		std::cout << "Creating Socket to listen on... ";

	// Create a SOCKET handle for connecting to server(no ip address here when using TCP. IP addr is assigned with bind)
	SOCKET s = socket(address_family, socket_type, protocol);
	if (s == INVALID_SOCKET)
	{
		getError();
		std::cout << "Socket failed.\n";
		myCloseSocket(s);
		return INVALID_SOCKET;//false
	}
	if (global_verbose == true)
		std::cout << "Success\n";

	return s;
}

bool SocketClass::myBind(SOCKET fd, const sockaddr *name, int name_len)
{
	if (global_verbose == true)
		std::cout << "Binding ... associating local address with the socket... ";
	// Setup the TCP listening socket (putting ip address on the allocated socket)
	int errchk = bind(fd, name, name_len);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "Bind failed.\n";
		myCloseSocket(fd);
		return false;
	}
	if (global_verbose == true)
		std::cout << "Success\n";
	//myFreeAddrInfo(result);   //shouldn't need the info gathered by getaddrinfo now that bind has been called

	return true;
}

// There might be issues with multiple threads calling send() on the same socket. Needs further inquiry.
int SocketClass::mySend(SOCKET s, const char* buffer, int buffer_length, int flags)
{
	// There might be issues with multiple threads calling send() on the same socket. Needs further inquiry.
	int errchk = send(s, buffer, buffer_length, flags);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "send failed.\n";
		return SOCKET_ERROR;
	}

	return errchk; // returns number of bytes sent
}

// returns total number of bytes sent if there is no error.
int SocketClass::mySendTo(SOCKET s, const char* buf, int buf_len, int flags, const sockaddr *target, int target_len)
{
	int errchk = sendto(s, buf, buf_len, flags, target, target_len);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "Sendto failed.\n";
		myCloseSocket(s);
		return SOCKET_ERROR;
	}
	if (global_verbose == true)
		std::cout << "Sendto sent msg: " << buf << "\n";
	return errchk;// Number of bytes sent
}

int SocketClass::myRecv(SOCKET s, char* buf, int buf_len, int flags)
{
	int errchk = recv(s, buf, buf_len, flags);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "recv failed.\n";
		return SOCKET_ERROR;
	}
	if (errchk == 0)
	{
		std::cout << "Connection gracefully closed.\n";
		return 0;
	}
	
	return errchk; // Number of bytes received
}

BYTE_SIZE SocketClass::myRecvFrom(SOCKET s, char *buf, int buf_len, int flags, sockaddr* from, socklen_t* from_len)
{
	if (global_verbose == true)
		std::cout << "Waiting to receive a msg...\n";
	BYTE_SIZE errchk = recvfrom(s, buf, buf_len, flags, from, from_len);// changed from int to ssize_t
	if (errchk == SOCKET_ERROR)
	{
		int saved_errno = getError();

#ifdef __linux__
		if (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK)
		{
			std::cout << "Finished receiving messages.\n";
			return WSAETIMEDOUT;
		}
		else
		{
			std::cout << "recvfrom failed.\n";
			myCloseSocket(s);
			return SOCKET_ERROR;
		}
#endif//__linux__
#ifdef _WIN32
		if (saved_errno == WSAETIMEDOUT)
		{
			std::cout << "Finished receiving messages.\n";
			return WSAETIMEDOUT;
		}
		else
		{
			std::cout << "recvfrom failed.\n";
			myCloseSocket(s);
			return SOCKET_ERROR;
		}
#endif//_WIN32

	}
	if (errchk == 0)	// Connection gracefully closed.
	{
		std::cout << "Connection gracefully closed.\n";
		myCloseSocket(s);
		return 0;
	}
	return errchk;
}

// For TCP use, not UDP
int SocketClass::myConnect(SOCKET fd, const sockaddr* name, int name_len)
{
	if (global_verbose == true)
		std::cout << "Attempting to connect to someone...\n";

	//ptr = result;	// can't remember why this was necessary

	//PclientSockaddr_in = (sockaddr_in *)ptr->ai_addr;
	//void *voidAddr;
	//char ipstr[INET_ADDRSTRLEN];
	//voidAddr = &(PclientSockaddr_in->sin_addr);
	//inet_ntop(ptr->ai_family, voidAddr, ipstr, sizeof(ipstr));
	//InetNtop(ptr->ai_family, voidAddr, ipstr, sizeof(ipstr));		//windows only

	// Connect to server
	int errchk = connect(fd, name, name_len);	// Returns 0 on success
	if (errchk == SOCKET_ERROR)
	{
		int r = getError();
		if (r == 10060)
			return -10060; // -10060 is a timeout error.
		std::cout << "Connect failed. Socket Error.\n";
		myCloseSocket(fd);
		return SOCKET_ERROR;
	}
	else
	{
		// Connection established
		if (global_verbose == true)
			std::cout << "Connection established using socket ID: " << fd << "\n";
		return 0;// Success
	}
	//myFreeAddrInfo(result);
}

// TCP use, not UDP
bool SocketClass::myListen(SOCKET fd)
{
	if (global_verbose == true)
		std::cout << "listen() called.\n";
	int errchk = listen(fd, SOMAXCONN);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "listen failed.\n";
		myCloseSocket(fd);
		return false;
	}
	return true;
}

// TCP use, not UDP
SOCKET SocketClass::myAccept(SOCKET fd)
{
#ifdef __linux__
	socklen_t addr_size;
#endif//__linux__
#ifdef _WIN32
	int addr_size;
#endif//_WIN32
	sockaddr_storage incomingAddr;
	memset(&incomingAddr, 0, sizeof(incomingAddr));

	addr_size = sizeof(incomingAddr);
	std::cout << "Waiting for someone to connect...\n";

	// Accept a client socket by listening on a socket
	SOCKET accepted_socket = accept(fd, (sockaddr*)&incomingAddr, &addr_size);
	if (accepted_socket == INVALID_SOCKET)
	{
		getError();
		std::cout << "accept failed.\n";
		myCloseSocket(fd);
		return INVALID_SOCKET;
	}
	if (global_verbose == true)
		std::cout << "Connected to " << ":" << "ip here, on socket ID: "<< accepted_socket << "\n";

	return accepted_socket;
}

// Using the given Hints, ip addr, and port, it then gives you a pointer
// to a linked list of on or more addrinfo structures and gives you the
// pointer to it. The pointer address is located in whatever u gave it for ppresult.
// This is not to be confused with the return value of the function.
bool SocketClass::myGetAddrInfo(std::string target_ip, std::string target_port, const addrinfo *phints, addrinfo **ppresult)
{
	if (global_verbose == true)
		std::cout << "getaddrinfo given: IP address and port... ";
	
	int errchk = getaddrinfo(target_ip.c_str(), target_port.c_str(), phints, ppresult);    //added & too ppresult on linux
	if (errchk != 0)
	{
		getError();;
		std::cout << "getaddrinfo failed.\n";
		return false;
	}
	if (global_verbose == true)
		std::cout << "Success\n";

	return true;
	//std::cout << "IP: " << Hints.ai_addr << ". Port: " << Hints.sockaddr->ai_addr\n";
}

// paddr_buf would be something like this:
// struct sockaddr_in storage;
// so it would be:  myinet_pton(AF_INET, "192.168.1.1", &storage.sin_addr);
int SocketClass::myinet_pton(int family, char* ip_addr, void* paddr_buf)
{
	int errchk = inet_pton(family, ip_addr,paddr_buf);
	if (errchk == 0)
		std::cout << "inet_pton: paddr_buf points to invalid IPV4 or IPV6 string.\n";
	else if (errchk == -1)
	{
		getError();
		std::cout << "inet_pton failed\n";
		return 0;
	}
	return 1;// Success
}

// Shuts down the current connection that is active on the given socket.
// The shutdown operation is one of three macros.
// SD_RECEIVE, SD_SEND, SD_BOTH.
bool SocketClass::myShutdown(SOCKET fd, int operation)
{
	std::cout << "Shutting down the connection... ";
	// shutdown the connection since we're done
	int errchk = shutdown(fd, operation);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "Shutdown failed.\n";
		std::cout << "Closing socket.\n";
		myCloseSocket(fd);
		return false;
	}
	std::cout << "Success\n";
	return true;
}

void SocketClass::myCloseSocket(SOCKET fd)
{
#ifdef __linux__
	close(fd);
#endif//__linux__

#ifdef _WIN32
	closesocket(fd);
#endif

	if (global_verbose == true)
		std::cout << "Closed the socket.\n";
}

void SocketClass::myWSACleanup()
{
#ifdef _WIN32
	WSACleanup();
#endif//_WIN32
}

void SocketClass::myFreeAddrInfo(addrinfo*& pAddrInfo)
{
	freeaddrinfo(pAddrInfo);

	pAddrInfo = nullptr;	// Set the structure address to nullptr
							// That will just tell us it is no longer in use
							// and it is not available to be freed (because it already has been).
	if (global_verbose == true)
		std::cout << "Freed addr info.\n";
}


// errno needs to be set to 0 before every function call that returns errno  // ERR30-C  /  SEI CERT C Coding Standard https://www.securecoding.cert.org/confluence/pages/viewpage.action?pageId=6619179
int SocketClass::getError()
{
	// If you are getting an error but WSAGetLastError() is saying there is no error, then make sure
	// that WSAStartup() has been performed. WSAGetLastError() doesn't work unless you start WSAStartup();

#ifdef __linux__
	int errsv = errno;	// Quickly saving the error incase it is quickly lost.
	if (errsv == 10060)	// This is a rcvfrom() timeout error. Not really much of an error, so don't report it as one.
		return errsv;
	std::cout << "ERRNO: " << errsv << ".\n";
	if (errsv == 10013)
	{
		std::cout << "Permission Denied.\n";
	}
	return errsv;
#endif//__linux__

#ifdef _WIN32
	int errsv = WSAGetLastError();

	// This is a rcvfrom() timeout error. Not really much of an error, so don't report it as one.
	if (errsv == 10060)	// This doesn't seem like the best way to do this...
		return errsv;

	std::cout << "WSAERROR: " << errsv << ".\n";
	if (errsv == 10013)
		std::cout << "Permission Denied.\n";

	return errsv;
#endif//_WIN32
}
