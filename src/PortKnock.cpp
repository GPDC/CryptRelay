// PortKnock.cpp

#ifdef __linux__
#include <iostream>
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

#include "PortKnock.h"
#include "IXBerkeleySockets.h"
#include "GlobalTypeHeader.h"
#endif//__linux__

#ifdef _WIN32
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "PortKnock.h"
#include "IXBerkeleySockets.h"
#include "GlobalTypeHeader.h"
#endif//_WIN32

#ifdef __linux__
#define INVALID_SOCKET	(-1)	// To indicate INVALID_SOCKET, Windows returns (~0) from socket functions, and linux returns -1.
#define SOCKET_ERROR	(-1)	// Linux doesn't have a SOCKET_ERROR macro.
#define SD_RECEIVE      SHUT_RD//0x00			// This is for shutdown(); SD_RECEIVE is the code to shutdown receive operations.
#define SD_SEND         SHUT_WR//0x01			// ^
#define SD_BOTH         SHUT_RDWR//0x02			// ^
#endif//__linux__

PortKnock::PortKnock(IXBerkeleySockets * IXBerkeleySocketsInstance, bool turn_verbose_output_on)
{
	if (turn_verbose_output_on == true)
		verbose_output = true;
	IBerkeleySockets = IXBerkeleySocketsInstance;
}
PortKnock::~PortKnock()
{

}

// returns < 0 on error
// returns 0 if port is available
// returns 1 if port is in use
int32_t PortKnock::isLocalPortInUse(std::string my_local_port, std::string my_local_ip)
{
	const int32_t IN_USE = 1;
	const int32_t AVAILABLE = 0;
	addrinfo ServerHints;
	addrinfo* ServerConnectionInfo;
	memset(&ServerHints, 0, sizeof(ServerHints));

	// These are the settings for the connection
	ServerHints.ai_family = AF_INET;		//ipv4
	ServerHints.ai_socktype = SOCK_STREAM;	// Peer will see incoming data as a stream of data, not as packets.
	ServerHints.ai_protocol = IPPROTO_TCP;	// Connect using TCP, reliable connection

	// Place target ip and port, and ServerHints about the connection type into a linked list named addrinfo *ServerConnectionInfo
	// Now we use ServerConnectionInfo instead of ServerHints to access the information.
	// We are only listening as the server, so put in local IP:port
	if (getaddrinfo(my_local_ip.c_str(), my_local_port.c_str(), &ServerHints, &ServerConnectionInfo) != 0)
	{	
		IBerkeleySockets->getError();
		std::cout << "getaddrinfo() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (ServerConnectionInfo != nullptr)
			IBerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
		return -1;
	}
		

	// Create socket
	SOCKET fd_socket = socket(ServerConnectionInfo->ai_family, ServerConnectionInfo->ai_socktype, ServerConnectionInfo->ai_protocol);
	if (fd_socket == INVALID_SOCKET || fd_socket == SOCKET_ERROR)
	{
		IBerkeleySockets->getError();
		std::cout << "socket() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (ServerConnectionInfo != nullptr)
			IBerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
		return -1;
	}


	// Assign the socket to an address:port

	// Binding the socket to the user's local address
	int32_t errchk = bind(fd_socket, ServerConnectionInfo->ai_addr, (int)ServerConnectionInfo->ai_addrlen);
	if (errchk == SOCKET_ERROR)
	{

#ifdef __linux__
		int32_t errsv = errno;		//saving the error so it isn't lost
		if (errsv == EADDRINUSE)
		{
			IBerkeleySockets->closesocket(fd_socket);
			return IN_USE;
		}
		else     // Must have been a different error
		{
			std::cout << "bind() failed.\n";
			DBG_DISPLAY_ERROR_LOCATION();
			return -1;
		}
#endif//__linux__
#ifdef _WIN32
		int32_t errsv = WSAGetLastError();	//saving the error so it isn't lost
		if (errsv == WSAEADDRINUSE)
		{
			IBerkeleySockets->closesocket(fd_socket);
			return IN_USE;
		}
		else     // Must have been a different error
		{
			std::cout << "bind() failed.\n";
			DBG_DISPLAY_ERROR_LOCATION();
			return -1;
		}
#endif//_WIN32

	}

	// No errors, must be available
	IBerkeleySockets->closesocket(fd_socket);
	return AVAILABLE;
}

// Very simple checking of 1 port. Not for checking many ports quickly.
// return 1, port is open
// return 0, port is closed
// return -1, error
int32_t PortKnock::isPortOpen(std::string ip, std::string port)
{
	if (verbose_output == true)
		std::cout << "Checking to see if port is open...\n";

	const int32_t IN_USE = 1;
	const int32_t AVAILABLE = 0;
	addrinfo ClientHints;
	addrinfo* ClientConnectionInfo = nullptr;
	memset(&ClientHints, 0, sizeof(ClientHints));

	ClientHints.ai_family = AF_INET;
	ClientHints.ai_socktype = SOCK_STREAM;
	ClientHints.ai_protocol = IPPROTO_TCP;

	// Place target ip and port, and ClientHints about the connection type into a linked list named addrinfo *ClientConnectionInfo
	// Now we use ClientConnectionInfo instead of ClientHints to access the information.
	if (getaddrinfo(ip.c_str(), port.c_str(), &ClientHints, &ClientConnectionInfo) != 0)
	{
		IBerkeleySockets->getError();
		std::cout << "getaddrinfo() failed. isPortOpen() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (ClientConnectionInfo != nullptr)
			IBerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
		return -1;
	}

	// Create socket
	SOCKET fd_socket = socket(ClientHints.ai_family, ClientHints.ai_socktype, ClientHints.ai_protocol);
	if (fd_socket == INVALID_SOCKET || fd_socket == SOCKET_ERROR)
	{
		IBerkeleySockets->getError();
		std::cout << "socket() failed. isPortOpen() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (ClientConnectionInfo != nullptr)
			IBerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
		return -1;
	}

	// If connection is successful, it must be an open port
	int32_t errchk = connect(fd_socket, ClientConnectionInfo->ai_addr, (int)ClientConnectionInfo->ai_addrlen);
	if (errchk == SOCKET_ERROR)
	{
		IBerkeleySockets->closesocket(fd_socket);
		if (ClientConnectionInfo != nullptr)
			IBerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
		return 0;
	}
	else // connected
	{
		IBerkeleySockets->shutdown(fd_socket, SD_BOTH);
		IBerkeleySockets->closesocket(fd_socket);
		if (ClientConnectionInfo != nullptr)
			IBerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
		return 1;
	}
}
