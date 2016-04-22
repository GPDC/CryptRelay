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

// Very simple checking of 1 port. Not for checking many ports quickly.
bool PortKnock::isPortOpen(std::string ip, std::string port)
{
	SocketClass SocketStuff;
	addrinfo hints;
	addrinfo* result = nullptr;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if ((SocketStuff.myGetAddrInfo(ip.c_str(), port.c_str(), &hints, &result)) == false)
	{
		std::cout << "isPortOpen() failed b/c of getaddrinfo().\n";
		return false;
	}

	SOCKET s = SocketStuff.mySocket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
	if (s == INVALID_SOCKET)
	{
		SocketStuff.getError(s);
		std::cout << "isPortOpen()'s sock() failed.\n";
		SocketStuff.myCloseSocket(s);
		return false;
	}

	// If connection is successful, it must be an open port
	int errchk = SocketStuff.myConnect(s, result->ai_addr, result->ai_addrlen);
	if (errchk == SOCKET_ERROR)
	{
		SocketStuff.myCloseSocket(s);
		return false;
	}
	else
	{
		SocketStuff.myShutdown(s, SD_BOTH);	// Shutting it down b/c we want it to be available to use.
		SocketStuff.myCloseSocket(s);
		return true;
	}
}