// Connection.cpp
#ifdef __linux__
#include <stdio.h>
#include <stdlib.h>
#include <iostream>		// cout
#include <string>
#include <string.h>		//memset
#include <pthread.h>	// <process.h>  multithreading
#include <thread>
#include <vector>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Connection.h"
#include "GlobalTypeHeader.h"
#include "StringManip.h"
#include "UPnP.h"
#endif //__linux__

#ifdef _WIN32
#include <iostream>		// cout
#include <process.h>	// <pthread.h> multithreading
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <vector>
#include <climits>

#include "Connection.h"
#include "GlobalTypeHeader.h"
#include "StringManip.h"
#include "UPnP.h"
#endif //_WIN32


#ifdef __linux__
    // for the shutdown() function
    const int32_t SD_BOTH = 2;
#endif //__linux__

#ifdef _WIN32
// These are needed for Windows sockets
#pragma comment(lib, "Ws2_32.lib") //tell the linker that Ws2_32.lib file is needed.
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#pragma warning(disable:4996)		// disable deprecated warning for fopen()
#endif//_WIN32

//mutex for use in this class' send()
std::mutex Connection::SendMutex;
// mutex for use with server and client threads to prevent a race condition.
std::mutex Connection::RaceMutex;


// This is the default port for the Connection as long as
// Connection isn't given any port from the UPnP class,
// which has its own default port, or given any port from the
// command line.
const std::string Connection::DEFAULT_PORT = "30248";

// Variables necessary for determining who won the connection race
const int32_t Connection::SERVER_WON = -29;
const int32_t Connection::CLIENT_WON = -30;
const int32_t Connection::NOBODY_WON = -25;
int32_t Connection::global_winner = NOBODY_WON;


Connection::Connection(SocketClass* SocketClassInstance)
{
	Socket = SocketClassInstance;
}
Connection::~Connection()
{

}


// This is where the Connection class receives information about IPs and ports.
// /*optional*/ target_port         default value will be assumed
// /*optional*/ my_internal_port    default value will be assumed
void Connection::setIPandPort(std::string target_extrnl_ip_address, std::string my_ext_ip, std::string my_internal_ip, std::string target_port, std::string my_internal_port)
{
	if (global_verbose == true)
		std::cout << "Giving IP and Port information to the chat program.\n";

	// If empty == false
	//		user must have desired their own custom ip and port. Let's take the
	//		information and store it in the corresponding variables.
	// If empty == true
	//		user must not have desired their own custom
	//		ports / IPs. Therefore, we leave it as is a.k.a. default value.
	if (target_extrnl_ip_address.empty() == false)
		target_external_ip = target_extrnl_ip_address;
	if (target_port.empty() == false)
		target_external_port = target_port;
	if (my_ext_ip.empty() == false)
		my_external_ip = my_ext_ip;
	if (my_internal_ip.empty() == false)
		my_local_ip = my_internal_ip;
	if (my_internal_port.empty() == false)
		my_local_port = my_internal_port;
}

void Connection::serverThread()
{	
	// ServerHints is used by getaddrinfo()
	// once ServerHints is given to getaddrinfo() it will fill out *ServerConnectionInfo
	addrinfo ServerHints;

	// After being given to getaddrinfo(), it will fill out ServerConnectionInfo.
	// It now contains all relevant info for ip address, family, protocol, etc.
	// *ServerConnectionInfo is ONLY used if there is a getaddrinfo().
	addrinfo * ServerConnectionInfo = nullptr;

	memset(&ServerHints, 0, sizeof(ServerHints));

	// These are the settings for the connection
	ServerHints.ai_family = AF_INET;		//ipv4
	ServerHints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	//ServerHints.ai_flags = AI_PASSIVE;	// Use local-host address
	ServerHints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	// Place target ip and port, and ServerHints about the connection type into a linked list named addrinfo *ServerConnectionInfo
	// Now we use ServerConnectionInfo instead of ServerHints.
	// Remember we are only listening as the server, so put in local IP:port
	if (Socket->getaddrinfo(my_local_ip, my_local_port, &ServerHints, &ServerConnectionInfo) == true)
	{
		if (ServerConnectionInfo != nullptr)
			Socket->freeaddrinfo(&ServerConnectionInfo);
		return;
	}


	// Create socket
	SOCKET errchk_socket = Socket->socket(ServerConnectionInfo->ai_family, ServerConnectionInfo->ai_socktype, ServerConnectionInfo->ai_protocol);
	if (errchk_socket == INVALID_SOCKET || errchk_socket == SOCKET_ERROR)
	{
		if (ServerConnectionInfo != nullptr)
			Socket->freeaddrinfo(&ServerConnectionInfo);
		return;
	}

	// Assign the socket to an address:port
	// Binding the socket to the user's local address
	if (Socket->bind(ServerConnectionInfo->ai_addr, ServerConnectionInfo->ai_addrlen) == true)
	{
		if (ServerConnectionInfo != nullptr)
			Socket->freeaddrinfo(&ServerConnectionInfo);
		return;
	}

	// Set the socket to listen for incoming connections
	if (Socket->listen() == true)
	{
		if (ServerConnectionInfo != nullptr)
			Socket->freeaddrinfo(&ServerConnectionInfo);
		return;
	}


	// Needed to provide a time to select()
	timeval TimeValue;
	memset(&TimeValue, 0, sizeof(TimeValue));

	// This struct contains the array of sockets that select() will check for readability
	fd_set ReadSet;
	memset(&ReadSet, 0, sizeof(ReadSet));

	// loop check for incoming connections
	int32_t errchk;
	while (1)
	{
		DBG_TXT("Server thread active...");

		// Please use the macros FD_SET, FD_CHECK, FD_CLEAR, when dealing with struct fd_set
		// set the socket count in the ReadSet struct to 0.
		FD_ZERO(&ReadSet);
		// Put specified socket into the socket array located in ReadSet
		// Format is:  FD_SET(int32_t fd, fd_set *ReadSet);
		FD_SET(Socket->fd_socket, &ReadSet);

		// Used by select(). It makes it wait for x amount of time to wait for a readable socket to appear.
		// This has to be in this loop! Linux resets this value every time select() times out.
		TimeValue.tv_sec = 1;
		TimeValue.tv_usec = 0; // 1 million microseconds = 1 second

		// select() returns the number of handles that are ready and contained in the fd_set structure
		errno = 0;
		errchk = select(Socket->fd_socket + 1, &ReadSet, NULL, NULL, &TimeValue);
		if (errchk == SOCKET_ERROR)
		{
			Socket->getError();
			std::cout << "serverThread() select Error.\n";
			DBG_DISPLAY_ERROR_LOCATION();
			Socket->closesocket(Socket->fd_socket);
			std::cout << "Closing listening socket b/c of the error. Ending Server Thread.\n";
			if (ServerConnectionInfo != nullptr)
				Socket->freeaddrinfo(&ServerConnectionInfo);
			return;
		}
		else if (global_winner == CLIENT_WON)
		{
			// Close the socket because the client thread won the race.
			Socket->closesocket(Socket->fd_socket);
			if (global_verbose == true)
				std::cout << "Closed listening socket, because the winner is: " << global_winner << ". Ending Server thread.\n";
			if (ServerConnectionInfo != nullptr)
				Socket->freeaddrinfo(&ServerConnectionInfo);
			return;
		}
		else if (errchk > 0)	// select() told us that atleast 1 readable socket has appeared!
		{
			if (global_verbose == true)
				std::cout << "Attempting to accept a client now that select() returned a readable socket\n";

			SOCKET errchk_socket;
			// Accept the connection and create a new socket to communicate on.
			errchk_socket = Socket->accept();
			if (errchk_socket == INVALID_SOCKET)
			{
				Socket->getError();
				DBG_DISPLAY_ERROR_LOCATION();
				if (ServerConnectionInfo != nullptr)
					Socket->freeaddrinfo(&ServerConnectionInfo);
				return;
			}

			if (global_verbose == true)
				std::cout << "accept() succeeded. Setting global_winner and global_socket\n";

			// Assigning global values to let the client thread know it should stop trying.
			if (setWinnerMutex(SERVER_WON) != SERVER_WON)
			{
				Socket->closesocket(Socket->fd_socket);
				if (ServerConnectionInfo != nullptr)
					Socket->freeaddrinfo(&ServerConnectionInfo);
				return;
			}

			break;
		}
	}

	// Display who the user has connected to.
	Socket->coutPeerIPAndPort();
	std::cout << "Acting as the server.\n\n\n\n\n\n\n\n\n\n";

	if (ServerConnectionInfo != nullptr)
		Socket->freeaddrinfo(&ServerConnectionInfo);
	return;
}


void Connection::clientThread()
{
	int32_t errchk;

	// ClientHints is used by getaddrinfo()
	// once ClientHints is given to getaddrinfo() it will fill out *ServerConnectionInfo
	addrinfo ClientHints;

	// After being given to getaddrinfo(), it will fill out ServerConnectionInfo.
	// It now contains all relevant info for ip address, family, protocol, etc.
	// *ClientConnectionInfo is ONLY used if there is a getaddrinfo().
	addrinfo * ClientConnectionInfo = nullptr;

	memset(&ClientHints, 0, sizeof(ClientHints));
	// These are the settings for the connection
	ClientHints.ai_family = AF_INET;		// ipv4
	ClientHints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	//ClientHints.ai_flags = AI_PASSIVE;    // Use local-host address
	ClientHints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	// Place target ip and port, and ClientHints about the connection type into a linked list named addrinfo* ClientConnectionInfo
	// Now we use ClientConnectionInfo instead of ClientHints in order to access the information.
	if (Socket->getaddrinfo(target_external_ip, target_external_port, &ClientHints, &ClientConnectionInfo) == true)
	{
		if (ClientConnectionInfo != nullptr)
			Socket->freeaddrinfo(&ClientConnectionInfo);
		return;
	}

	// Create socket
	SOCKET errchk_socket = Socket->socket(ClientConnectionInfo->ai_family, ClientConnectionInfo->ai_socktype, ClientConnectionInfo->ai_protocol);
	if (errchk_socket == INVALID_SOCKET || errchk_socket == SOCKET_ERROR)
	{
		if (ClientConnectionInfo != nullptr)
			Socket->freeaddrinfo(&ClientConnectionInfo);
		return;
	}

	// Disable blocking on the socket
	if (Socket->setBlockingSocketOpt(Socket->fd_socket, &Socket->DISABLE_BLOCKING) == true)
	{
		std::cout << "Exiting client thread due to error.\n";
		if (ClientConnectionInfo != nullptr)
			Socket->freeaddrinfo(&ClientConnectionInfo);
		return;
	}

	// Needed to provide a time to select()
	timeval TimeValue;
	memset(&TimeValue, 0, sizeof(TimeValue));

	// This struct contains the array of sockets that select() will check for readability
	fd_set WriteSet;
	memset(&WriteSet, 0, sizeof(WriteSet));

	// loop check for incoming connections
	while (1)
	{
		DBG_TXT("Client thread active...");


		// OPERATION_ALREADY_IN_PROGRESS == connect() is still trying to connect to the peer.
		// OPERATION_NOW_IN_PROGRESS == connect() is in the middle of negotiating the connection to the peer.
		// WOULD_BLOCK == connect() is in the process of trying to connect to the peer. The peer might not even exist.
#ifdef _WIN32
		const int32_t WOULD_BLOCK = WSAEWOULDBLOCK;
		const int32_t OPERATION_ALREADY_IN_PROGRESS = WSAEALREADY;
		const int32_t OPERATION_NOW_IN_PROGRESS = WSAEINPROGRESS;
#endif//_WIN32
#ifdef __linux__
		const int32_t WOULD_BLOCK = EWOULDBLOCK;
		const int32_t OPERATION_ALREADY_IN_PROGRESS = EALREADY;
		const int32_t OPERATION_NOW_IN_PROGRESS = EINPROGRESS;
#endif//__linux__


		// Please use the macros FD_SET, FD_CHECK, FD_CLEAR, FD_ZERO, FD_ISSET when dealing with struct fd_set.
		// Set the socket count in the WriteSet struct to 0.
		FD_ZERO(&WriteSet);

		// Attempt to connect to target
		errno = 0;
		int32_t conn_return_val = Socket->connect(ClientConnectionInfo->ai_addr, ClientConnectionInfo->ai_addrlen);
		if (exit_now == true)
		{
			Socket->closesocket(Socket->fd_socket);
			if (ClientConnectionInfo != nullptr)
				Socket->freeaddrinfo(&ClientConnectionInfo);
			return;
		}
		if (conn_return_val == 0)
		{
			// Connected immediately
			break;
		}
		if (conn_return_val != 0)
		{
			const int32_t NO_SOCKET_ERROR = 0;
			int32_t err_chk = Socket->getError(Socket->DISABLE_CONSOLE_OUTPUT);
			if (err_chk == OPERATION_ALREADY_IN_PROGRESS || err_chk == OPERATION_NOW_IN_PROGRESS || err_chk == WOULD_BLOCK || err_chk == NO_SOCKET_ERROR)
			{
				// Let us move on to select() to see if we have completed the connection.

				// If fd_socket isn't set in the WriteSet struct, set it.
				if (FD_ISSET(Socket->fd_socket, &WriteSet) == false)
				{
					// Put specified socket into the socket array located in WriteSet
					// Format is:  FD_SET(int32_t fd, fd_set *WriteSet);
					FD_SET(Socket->fd_socket, &WriteSet);
				}

				// Used by select(). It makes it wait for x amount of time to wait for a readable socket to appear.
				// Linux select() modifies these values to reflect the amount of time not slept, therefore
				// they need to be assigned every time.
				TimeValue.tv_sec = 1;
				TimeValue.tv_usec = 0;

				// select() returns the number of socket handles that are ready and contained in the fd_set structure
				// returns 0 if the time limit has expired and it still hasn't seen any ready sockets handles.
				errno = 0;
				errchk = select(Socket->fd_socket + 1, NULL, &WriteSet, NULL, &TimeValue);
				if (exit_now == true)
				{
					Socket->closesocket(Socket->fd_socket);
					if (ClientConnectionInfo != nullptr)
						Socket->freeaddrinfo(&ClientConnectionInfo);
					return;
				}
				if (errchk == SOCKET_ERROR)
				{
					Socket->getError();
					std::cout << "ClientThread() select Error.\n";
					DBG_DISPLAY_ERROR_LOCATION();

					// Get error information from the socket, not just from select()
					// The effectiveness of this is untested so far as I haven't seen select error yet.
					int32_t errorz = Socket->getSockOptError(Socket->fd_socket);
					if (errorz == SOCKET_ERROR)
					{
						std::cout << "getsockopt() failed.\n";
						Socket->getError();
						DBG_DISPLAY_ERROR_LOCATION();
					}
					else
					{
						std::cout << "getsockopt() returned: " << errorz << "\n";
					}

					Socket->closesocket(Socket->fd_socket);
					std::cout << "Closing connect socket b/c of the error. Ending client thread.\n";
					if (ClientConnectionInfo != nullptr)
						Socket->freeaddrinfo(&ClientConnectionInfo);
					return;
				}
				else if (global_winner == SERVER_WON)
				{
					// Close the socket because the server thread won the race.
					Socket->closesocket(Socket->fd_socket);
					if (global_verbose == true)
						std::cout << "Closed connect socket, because the winner is: " << global_winner << ". Ending client thread.\n";
					if (ClientConnectionInfo != nullptr)
						Socket->freeaddrinfo(&ClientConnectionInfo);
					return;
				}
				else if (errchk > 0)	// select() told us that at least 1 readable socket has appeared
				{
					// Try to tell everyone that the client thread is the winner
					if (setWinnerMutex(CLIENT_WON) != CLIENT_WON)
					{
						// Server thread must have won the race
						Socket->closesocket(Socket->fd_socket);
						if (global_verbose == true)
							std::cout << "Exiting client thread because the server thread won the race.\n";
						if (ClientConnectionInfo != nullptr)
							Socket->freeaddrinfo(&ClientConnectionInfo);
						return;
					}
					else// Client is confirmed as the winner of the race.
					{
						// connect() one more time to fully connect to the available socket.
						// connect() should return 0 (success) at this point.

						// Linux needs to connect() one more time to finalize the connection.
						// Windows is already connected, so if it tries to connect() again it errors.
						#ifdef __linux__
						continue;
						#endif//__linux
						#ifdef _WIN32
						break;
						#endif//_WIN32
					}
				}
			}
			else
			{
				Socket->outputSocketErrorToConsole(errchk);
				Socket->closesocket(Socket->fd_socket);
				DBG_DISPLAY_ERROR_LOCATION();
				std::cout << "Exiting client thread.\n";
				if (ClientConnectionInfo != nullptr)
					Socket->freeaddrinfo(&ClientConnectionInfo);
				return;
			}
		}
	}

	// Display who the user is connected with.
	Socket->coutPeerIPAndPort();
	std::cout << "Acting as the client.\n\n\n\n\n\n\n\n\n\n";

	// Re-enable blocking for the socket.
	if (Socket->setBlockingSocketOpt(Socket->fd_socket, &Socket->ENABLE_BLOCKING) == true)
	{
		Socket->getError();
		DBG_DISPLAY_ERROR_LOCATION();
		if (ClientConnectionInfo != nullptr)
			Socket->freeaddrinfo(&ClientConnectionInfo);
		return;
	}

	if (ClientConnectionInfo != nullptr)
		Socket->freeaddrinfo(&ClientConnectionInfo);
	return;
}


// Server and Client thread must use this method, if they are ever
// running at the same time, to prevent a race condition.
int32_t Connection::setWinnerMutex(int32_t the_winner)
{
	RaceMutex.lock();

	// If nobody has won the race yet, then set the winner.
	if (global_winner == NOBODY_WON)
	{
		global_winner = the_winner;
	}

	RaceMutex.unlock();
	return global_winner;
}