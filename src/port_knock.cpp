//port_knock.cpp

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

#include "port_knock.h"
#include "SocketClass.h"
#include "GlobalTypeHeader.h"
#endif//__linux__

#ifdef _WIN32
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "port_knock.h"
#include "SocketClass.h"
#include "GlobalTypeHeader.h"
#endif//_WIN32


#ifdef __linux__
#define INVALID_SOCKET	(-1)	// To indicate INVALID_SOCKET, Windows returns (~0) from socket functions, and linux returns -1.
#define SOCKET_ERROR	(-1)	// I belive this was just because linux didn't already have a SOCKET_ERROR macro.
#define SD_RECEIVE      SHUT_RD//0x00			// This is for shutdown(); SD_RECEIVE is the code to shutdown receive operations.
#define SD_SEND         SHUT_WR//0x01			// ^
#define SD_BOTH			SHUT_RDWR//0x02			// ^
#endif//__linux__


PortKnock::PortKnock()
{

}
PortKnock::~PortKnock()
{

}

// returns < 0 on error
// returns 0 if port is available
// returns 1 if port is in use
int32_t PortKnock::isLocalPortInUse(std::string my_local_port, std::string my_local_ip)
{
	SocketClass PKSocketClass;
	const int32_t IN_USE = 1;
	const int32_t AVAILABLE = 0;
	addrinfo Hints;
	addrinfo* ServerInfo;
	memset(&Hints, 0, sizeof(Hints));

	// These are the settings for the connection
	Hints.ai_family = AF_INET;		//ipv4
	Hints.ai_socktype = SOCK_STREAM;	// Peer will see incoming data as a stream of data, not as packets.
	Hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP, reliable connection

	// Place target ip and port, and Hints about the connection type into a linked list named addrinfo *ServerInfo
	// Now we use ServerInfo instead of Hints.
	// Remember we are only listening as the server, so put in local IP:port
	if (PKSocketClass.getaddrinfo(my_local_ip, my_local_port, &Hints, &ServerInfo) == false)
		return false;

	// Create socket
	SOCKET errchk_socket = PKSocketClass.socket(ServerInfo->ai_family, ServerInfo->ai_socktype, ServerInfo->ai_protocol);
	if (errchk_socket == INVALID_SOCKET)
		return -1;
	else if (errchk_socket == SOCKET_ERROR)
		return -1;

	// Assign the socket to an address:port

	// Binding the socket to the user's local address
	int32_t errchk = ::bind(PKSocketClass.fd_socket, ServerInfo->ai_addr, ServerInfo->ai_addrlen);
	if (errchk == SOCKET_ERROR)
	{

#ifdef __linux__
		int32_t errsv = errno;			//saving the error so it isn't lost
		if (errsv == EADDRINUSE)	//needs checking on linux to make sure this is the correct macro
		{
			PKSocketClass.closesocket(PKSocketClass.fd_socket);
			return IN_USE;
		}
		else     // Must have been a different error
			return -1;
#endif//__linux__
#ifdef _WIN32
		int32_t errsv = WSAGetLastError();	//saving the error so it isn't lost
		if (errsv == WSAEADDRINUSE)
		{
			PKSocketClass.closesocket(PKSocketClass.fd_socket);
			return IN_USE;
		}
		else     // Must have been a different error
			return -1;
#endif//_WIN32

	}

	// No errors, must be available
	PKSocketClass.closesocket(PKSocketClass.fd_socket);
	return AVAILABLE;
}

// Very simple checking of 1 port. Not for checking many ports quickly.
bool PortKnock::isPortOpen(std::string ip, std::string port)
{
	if (global_verbose == true)
		std::cout << "Checking to see if port is open...\n";

	SocketClass PKSocketClass;
	addrinfo hints;
	addrinfo* result = nullptr;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if ((PKSocketClass.getaddrinfo(ip.c_str(), port.c_str(), &hints, &result)) == true)
	{
		std::cout << "isPortOpen() failed b/c of getaddrinfo().\n";
		return true;
	}

	SOCKET errchk_socket = PKSocketClass.socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (errchk_socket == INVALID_SOCKET)
	{
		PKSocketClass.getError();
		std::cout << "isPortOpen()'s sock() failed.\n";
		PKSocketClass.closesocket(PKSocketClass.fd_socket);
		return true;
	}

	// If connection is successful, it must be an open port
	int32_t errchk = PKSocketClass.connect(result->ai_addr, result->ai_addrlen);
	if (errchk == SOCKET_ERROR)
	{
		PKSocketClass.closesocket(PKSocketClass.fd_socket);
		return true;
	}
	else
	{
		PKSocketClass.shutdown(PKSocketClass.fd_socket, SD_BOTH);
		PKSocketClass.closesocket(PKSocketClass.fd_socket);
		return false;
	}
}
