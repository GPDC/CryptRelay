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
#include "SocketClass.h"
#include "GlobalTypeHeader.h"
#endif//__linux__

#ifdef _WIN32
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "PortKnock.h"
#include "SocketClass.h"
#include "GlobalTypeHeader.h"
#endif//_WIN32


PortKnock::PortKnock(bool turn_verbose_output_on)
{
	if (turn_verbose_output_on == true)
		verbose_output = true;
}
PortKnock::~PortKnock()
{

}

// returns < 0 on error
// returns 0 if port is available
// returns 1 if port is in use
int32_t PortKnock::isLocalPortInUse(std::string my_local_port, std::string my_local_ip)
{
	SocketClass Socket;
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
		Socket.getError();
		std::cout << "getaddrinfo() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (ServerConnectionInfo != nullptr)
			Socket.freeaddrinfo(&ServerConnectionInfo);
		return -1;
	}
		

	// Create socket
	Socket.fd_socket = socket(ServerConnectionInfo->ai_family, ServerConnectionInfo->ai_socktype, ServerConnectionInfo->ai_protocol);
	if (Socket.fd_socket == INVALID_SOCKET || Socket.fd_socket == SOCKET_ERROR)
	{
		Socket.getError();
		std::cout << "socket() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (ServerConnectionInfo != nullptr)
			Socket.freeaddrinfo(&ServerConnectionInfo);
		return -1;
	}


	// Assign the socket to an address:port

	// Binding the socket to the user's local address
	int32_t errchk = bind(Socket.fd_socket, ServerConnectionInfo->ai_addr, ServerConnectionInfo->ai_addrlen);
	if (errchk == SOCKET_ERROR)
	{

#ifdef __linux__
		int32_t errsv = errno;		//saving the error so it isn't lost
		if (errsv == EADDRINUSE)
		{
			Socket.closesocket(Socket.fd_socket);
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
			Socket.closesocket(Socket.fd_socket);
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
	Socket.closesocket(Socket.fd_socket);
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
	SocketClass Socket;
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
		Socket.getError();
		std::cout << "getaddrinfo() failed. isPortOpen() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (ClientConnectionInfo != nullptr)
			Socket.freeaddrinfo(&ClientConnectionInfo);
		return -1;
	}

	// Create socket
	Socket.fd_socket = socket(ClientHints.ai_family, ClientHints.ai_socktype, ClientHints.ai_protocol);
	if (Socket.fd_socket == INVALID_SOCKET || Socket.fd_socket == SOCKET_ERROR)
	{
		Socket.getError();
		std::cout << "socket() failed. isPortOpen() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (ClientConnectionInfo != nullptr)
			Socket.freeaddrinfo(&ClientConnectionInfo);
		return -1;
	}

	// If connection is successful, it must be an open port
	int32_t errchk = connect(Socket.fd_socket, ClientConnectionInfo->ai_addr, ClientConnectionInfo->ai_addrlen);
	if (errchk == SOCKET_ERROR)
	{
		Socket.closesocket(Socket.fd_socket);
		if (ClientConnectionInfo != nullptr)
			Socket.freeaddrinfo(&ClientConnectionInfo);
		return 0;
	}
	else // connected
	{
		Socket.shutdown(Socket.fd_socket, SD_BOTH);
		Socket.closesocket(Socket.fd_socket);
		if (ClientConnectionInfo != nullptr)
			Socket.freeaddrinfo(&ClientConnectionInfo);
		return 1;
	}
}
