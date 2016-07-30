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
#define WSAETIMEDOUT 10060	// possibly temporary... recvfrom() isn't being used?
#endif//__linux__

// Used by threads after a race winner has been established
SOCKET SocketClass::global_socket;


SocketClass::SocketClass()
{
	// Unsure If I should have this here quite frankly, but
	// it IS necessary for 99% of things in this class.
	WSAStartup();
}
SocketClass::~SocketClass()
{
	WSACleanup();
}

// Necessary to do anything with sockets on Windows
bool SocketClass::WSAStartup()
{
#ifdef _WIN32
	if (global_verbose == true)
		std::cout << "Initializing Winsock... ";

	int errchk = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
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
// If this function returns false, it is up to you to close the socket if desired.
bool SocketClass::setsockopt(int level, int option_name, const char* option_value, int option_length)
{
	if (global_verbose == true)
		std::cout << "Setting socket options... ";
	int errchk = ::setsockopt(fd_socket, level, option_name, option_value, option_length);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "setsockopt() failed.\n";
		return false;
	}
	if (global_verbose == true)
		std::cout << "Success\n";
	return true;
}

// Create a socket. Returns INVALID_SOCKET on error.
SOCKET SocketClass::socket(int address_family, int socket_type, int protocol)
{
	if (global_verbose == true)
		std::cout << "Creating Socket to listen on... ";

	// Create a SOCKET handle for connecting to server(no ip address here when using TCP. IP addr is assigned with bind)
	fd_socket = ::socket(address_family, socket_type, protocol);
	if (fd_socket == INVALID_SOCKET)
	{
		getError();
		std::cout << "Socket failed.\n";
		closesocket(fd_socket);
		return INVALID_SOCKET;//false
	}
	if (global_verbose == true)
		std::cout << "Success\n";

	return fd_socket;
}

bool SocketClass::bind(const sockaddr *name, int name_len)
{
	if (global_verbose == true)
		std::cout << "Binding ... associating local address with the socket... ";
	// Setup the TCP listening socket (putting ip address on the allocated socket)
	int errchk = ::bind(fd_socket, name, name_len);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "Bind failed.\n";
		closesocket(fd_socket);
		return false;
	}
	if (global_verbose == true)
		std::cout << "Success\n";
	//freeaddrinfo(result);   //shouldn't need the info gathered by getaddrinfo now that bind has been called

	return true;
}

// There might be issues with multiple threads calling send() on the same socket. Needs further inquiry.
int SocketClass::send(const char* buffer, int buffer_length, int flags)
{
	// There might be issues with multiple threads calling send() on the same socket. Needs further inquiry.
	int errchk = ::send(fd_socket, buffer, buffer_length, flags);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "send failed.\n";
		return SOCKET_ERROR;
	}

	return errchk; // returns number of bytes sent
}

// returns total number of bytes sent if there is no error.
int SocketClass::sendto(const char* buf, int buf_len, int flags, const sockaddr *target, int target_len)
{
	int errchk = ::sendto(fd_socket, buf, buf_len, flags, target, target_len);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "Sendto failed.\n";
		closesocket(fd_socket);
		return SOCKET_ERROR;
	}
	if (global_verbose == true)
		std::cout << "Sendto sent msg: " << buf << "\n";
	return errchk;// Number of bytes sent
}

int SocketClass::recv(char* buf, int buf_len, int flags)
{
	int errchk = ::recv(fd_socket, buf, buf_len, flags);
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

BYTE_SIZE SocketClass::recvfrom(char *buf, int buf_len, int flags, sockaddr* from, socklen_t* from_len)
{
	if (global_verbose == true)
		std::cout << "Waiting to receive a msg...\n";
	BYTE_SIZE errchk = ::recvfrom(fd_socket, buf, buf_len, flags, from, from_len);// changed from int to ssize_t
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
			closesocket(fd_socket);
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
			closesocket(fd_socket);
			return SOCKET_ERROR;
		}
#endif//_WIN32

	}
	if (errchk == 0)	// Connection gracefully closed.
	{
		std::cout << "Connection gracefully closed.\n";
		closesocket(fd_socket);
		return 0;
	}
	return errchk;
}

// For TCP use, not UDP
int SocketClass::connect(const sockaddr* name, int name_len)
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
	int errchk = ::connect(fd_socket, name, name_len);	// Returns 0 on success
	if (errchk == SOCKET_ERROR)
	{
		/*
		int r = getError();
#ifdef _WIN32
		if (r == 10060)
			return TIMEOUT_ERROR; // -10060 is a timeout error.
		if (r == 10061)
			return CONNECTION_REFUSED;
#endif//_WIN32
#ifdef __linux__
		if (r == ETIMEDOUT)
			return TIMEOUT_ERROR;
		if (r == ECONNREFUSED)
			return CONNECTION_REFUSED;
#endif//__linux__
		std::cout << "Connect failed. Socket Error.\n";
		closesocket(fd_socket);
		return SOCKET_ERROR;
		*/
		return SOCKET_ERROR;
	}
	else
	{
		// Connection established
		if (global_verbose == true)
			std::cout << "Connection established using socket ID: " << fd_socket << "\n";
		return 0;// Success
	}
}

// TCP use, not UDP
bool SocketClass::listen()
{
	if (global_verbose == true)
		std::cout << "listen() called.\n";
	int errchk = ::listen(fd_socket, SOMAXCONN);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "listen failed.\n";
		closesocket(fd_socket);
		return false;
	}
	return true;
}

// TCP use, not UDP
SOCKET SocketClass::accept()
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
	SOCKET accepted_socket = ::accept(fd_socket, (sockaddr*)&incomingAddr, &addr_size);
	if (accepted_socket == INVALID_SOCKET)
	{
		getError();
		std::cout << "accept failed.\n";
		closesocket(fd_socket);
		return INVALID_SOCKET;
	}

	// Now close the old listening SOCKET
	closesocket(fd_socket);
	// Assign fd_socket the newly accepted SOCKET
	fd_socket = accepted_socket;

	return fd_socket;
}

// Using the given Hints, ip addr, and port, it then gives you a pointer
// to a linked list of on or more addrinfo structures and gives you the
// pointer to it. The pointer address is located in whatever u gave it for ppresult.
// This is not to be confused with the return value of the function.
bool SocketClass::getaddrinfo(std::string target_ip, std::string target_port, const addrinfo *phints, addrinfo **ppresult)
{
	if (global_verbose == true)
		std::cout << "getaddrinfo given: IP address and port... ";
	
	int errchk = ::getaddrinfo(target_ip.c_str(), target_port.c_str(), phints, ppresult);    //added & too ppresult on linux
	if (errchk != 0)
	{
		getError();;
		std::cout << "getaddrinfo failed.\n";
		return false;
	}
	if (global_verbose == true)
		std::cout << "Success\n";

	return true;
}

// paddr_buf would be something like this:
// struct sockaddr_in storage;
// so it would be:  inet_pton(AF_INET, "192.168.1.1", &storage.sin_addr);
int SocketClass::inet_pton(int family, char* ip_addr, void* paddr_buf)
{
	int errchk = ::inet_pton(family, ip_addr,paddr_buf);
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
bool SocketClass::shutdown(SOCKET socket, int operation)
{
	std::cout << "Shutting down the connection... ";
	// shutdown the connection since we're done
	int errchk = ::shutdown(socket, operation);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "Shutdown failed.\n";
		std::cout << "Closing socket.\n";
		closesocket(socket);
		return false;
	}
	std::cout << "Success\n";
	return true;
}

void SocketClass::closesocket(SOCKET socket)
{
#ifdef __linux__
	::close(socket);
#endif//__linux__

#ifdef _WIN32
	::closesocket(socket);
#endif

	if (global_verbose == true)
		std::cout << "Closed the socket.\n";
}

void SocketClass::WSACleanup()
{
#ifdef _WIN32
	::WSACleanup();
#endif//_WIN32
}

void SocketClass::freeaddrinfo(addrinfo** ppAddrInfo)
{
	::freeaddrinfo(*ppAddrInfo);

	*ppAddrInfo = nullptr;	// Set the structure address to nullptr
							// That will just tell us it is no longer in use
							// and it is not available to be freed (because it already has been).
	if (global_verbose == true)
		std::cout << "Freed addr info.\n";
}


// For most consistent results, errno needs to be set to 0 before
// every function call that can return an errno.
// This method is only intended for use with things that deal with sockets.
// On windows, this function returns WSAERROR codes.
// On linux, this function returns errno codes.
// getError() will output the error code + description unless
// output_to_console arg is given false.
int SocketClass::getError(bool output_to_console)
{
	// If you are getting an error but WSAGetLastError() is saying there is no error, then make sure
	// that WSAStartup() has been performed. WSAGetLastError() doesn't work unless you start WSAStartup();

	// Buffer for strerror_r() and strerror_s() to put text into.
	const int STR_BUF_SIZE = 100;
	char str_buf[STR_BUF_SIZE] = { 0 };

#ifdef __linux__
	int errsv = errno;	// Quickly saving the error incase it is quickly lost.

	if (output_to_console == true)
	{
		outputSocketErrorToConsole(errsv);
	}

	return errsv;
#endif//__linux__

#ifdef _WIN32
	int errsv = ::WSAGetLastError();

	if (output_to_console == true)
	{
		outputSocketErrorToConsole(errsv);
	}
	
	return errsv;
#endif//_WIN32
}

// On windows it expects a WSAERROR code.
// On linux it expects an errno code.
void SocketClass::outputSocketErrorToConsole(int error_code)
{
	// Buffer for strerror_r() and strerror_s() to put text into.
	const int STR_BUF_SIZE = 100;
	char str_buf[STR_BUF_SIZE] = { 0 };
#ifdef __linux__
	// If strerror_s didn't error, print it out
	if (strerror_r(error_code, str_buf, STR_BUF_SIZE) == 0)
	{
		std::cout << "Errno: " << error_code << ", " << str_buf << "\n";
	}
	else  // Just print the error code
	{
		std::cout << "Errno: " << error_code << ".\n";
	}
#endif//__linux__
#ifdef _WIN32
	int tchar_count = 0;

	// FORMAT_MESSAGE_ALLOCATE_BUFFER will allocate a buffer on local heap for error text, therefore
	// it needs to be freed with LocalFree();
	// FORMAT_MESSAGE_FROM_SYSTEM tells it to search the system message-table resources for the requested error txt
	// FORMAT_MESSAGE_IGNORE_INSERTS is 100% necessary. It makes it so insert sequences in the message definition
	// are ignored and passed through to the output buffer unchanged.
	// returns 0 on fail.
	// returns the number of TCHARs stored in the output buffer, excluding the terminating null char.
	LPTSTR message_for_console = NULL;
	tchar_count = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,   // Location of message definitions. NULL b/c we told it to FORMAT_MESSAGE_FROM_SYSTEM
		(DWORD)error_code, // Source to look for the error code.
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Use the default language for the output message.
		(LPTSTR)&message_for_console,   // Places the message here so you can cout it.
		0, // number of TCHARs to allocate for an output buffer (b/c we set FORMAT_MESSAGE_ALLOCATE_BUFFER)
		NULL // optional args
	);
	if (tchar_count != 0) // success
	{
		std::cout << "WSAError: " << error_code << ". ";
		fprintf(stderr, "%S\n", message_for_console);
		LocalFree(message_for_console);
		message_for_console = NULL;
	}
	else
	{
		// Still print out the error code even though there is no description of it.
		std::cout << "WSAError: " << error_code << "\n";

		// Now display error information for FormatMessage() failing.
		perror("FormatMessage() failed");
		DBG_DISPLAY_ERROR_LOCATION();
	}
#endif//_WIN32

	return;
}

// Output to console the the peer's IP and port that you have connected to the peer with.
void SocketClass::coutPeerIPAndPort()
{
	sockaddr PeerIPAndPortStorage;
#ifdef _WIN32
	int peer_ip_and_port_storage_len = sizeof(sockaddr);
#endif//_WIN32
#ifdef __linux__
	socklen_t peer_ip_and_port_storage_len = sizeof(sockaddr);
#endif//__linux__
	memset(&PeerIPAndPortStorage, 0, peer_ip_and_port_storage_len);

	// getting the peer's ip and port info and placing it into the PeerIPAndPortStorage sockaddr structure
	int errchk = getpeername(fd_socket, &PeerIPAndPortStorage, &peer_ip_and_port_storage_len);
	if (errchk == -1)
	{
		getError();
		std::cout << "getpeername() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		// continuing, b/c this isn't a big problem.
	}



	// If we are here, we must be connected to someone.
	char remote_host[NI_MAXHOST];
	char remote_hosts_port[NI_MAXSERV];

	// Let us see the IP:Port we are connecting to. the flag NI_NUMERICSERV
	// will make it return the port instead of the service name.
	errchk = getnameinfo(&PeerIPAndPortStorage, sizeof(sockaddr), remote_host, NI_MAXHOST, remote_hosts_port, NI_MAXSERV, NI_NUMERICHOST);
	if (errchk != 0)
	{
		getError();
		std::cout << "getnameinfo() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		// still going to continue the program, this isn't a big deal
	}
	else
		std::cout << "\n\n\n\n\n\nConnection established with: " << remote_host << ":" << remote_hosts_port << "\n\n\n";
}

// With this method you can enable or disable blocking for a given socket.
// DISABLE_BLOCKING == 1;
// ENABLE_BLOCKING == 0;
bool SocketClass::setBlockingSocketOpt(SOCKET socket, const u_long* option)
{
	u_long mode = *option;
#ifdef _WIN32
	int errchk = ioctlsocket(socket, FIONBIO, &mode);
	if (errchk == NO_ERROR)
	{
		return false;
	}
	else
	{
		std::cout << "ioctlsocket() failed.\n";
		getError();
		DBG_DISPLAY_ERROR_LOCATION();
		return true;
	}
#endif//_WIN32
#ifdef __linux__
	int errchk = ioctl();
	if (errchk != zxjkxzjoij)
	{
		std::cout << "ioctl() failed.\n";
		getError();
		DBG_DISPLAY_ERROR_LOCATION();
	}
#endif//__linux__
}