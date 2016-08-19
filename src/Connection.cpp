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
#include "IXBerkeleySockets.h"
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
#include "IXBerkeleySockets.h"
#endif //_WIN32


#ifdef _WIN32
// These are needed for Windows sockets
#pragma comment(lib, "Ws2_32.lib") //tell the linker that Ws2_32.lib file is needed.
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#pragma warning(disable:4996)		// disable deprecated warning for fopen()
#endif//_WIN32


#ifdef __linux__
#define INVALID_SOCKET	(-1)	// To indicate INVALID_SOCKET, Windows returns (~0) from socket functions, and linux returns -1.
#define SOCKET_ERROR	(-1)	// Linux doesn't have a SOCKET_ERROR macro.
#define SD_RECEIVE      SHUT_RD//0x00			// This is for shutdown(); SD_RECEIVE is the code to shutdown receive operations.
#define SD_SEND         SHUT_WR//0x01			// ^
#define SD_BOTH         SHUT_RDWR//0x02			// ^
#endif//__linux__


std::mutex Connection::RaceMutex;

const std::string Connection::DEFAULT_PORT = "30248";

const int32_t Connection::SERVER_WON = -29;
const int32_t Connection::CLIENT_WON = -30;
const int32_t Connection::NOBODY_WON = -25;
int32_t Connection::connection_race_winner = NOBODY_WON;


Connection::Connection(IXBerkeleySockets* IXBerkeleySocketsInstance,
						callback_fn_exit_program * exit_program_ptr,
						bool turn_verbose_output_on)
{
	// Checking all callbacks for nullptr
	if (IXBerkeleySocketsInstance == nullptr
		|| exit_program_ptr == nullptr)
	{
		throw "Exception thrown: nullptr in Connection constructor";
	}

	// Enable socket use on windows.
	WSAStartup();

	// Setting the callbacks
	callbackExitProgram = exit_program_ptr;

	BerkeleySockets = IXBerkeleySocketsInstance;

	if (turn_verbose_output_on == true)
		verbose_output = true;

	fd_socket = INVALID_SOCKET;
}
Connection::~Connection()
{
	// Done with sockets.
#ifdef _WIN32
	WSACleanup();
#endif//_WIN32
}


void Connection::server()
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
	if (getaddrinfo(my_local_ip.c_str(), my_local_port.c_str(), &ServerHints, &ServerConnectionInfo) != 0)
	{
		BerkeleySockets->getError();
		std::cout << "getaddrinfo() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (ServerConnectionInfo != nullptr)
			BerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
		return;
	}


	// Create a socket
	SOCKET listening_socket = socket(ServerConnectionInfo->ai_family, ServerConnectionInfo->ai_socktype, ServerConnectionInfo->ai_protocol);
	if (listening_socket == INVALID_SOCKET || listening_socket == SOCKET_ERROR)
	{
		BerkeleySockets->getError();
		std::cout << "socket() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		BerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
		return;
	}


	// Assign the socket to an address:port
	// Since it is a socket that we are going to be listening for
	// a connection on, we shall bind the socket to the user's local address
	if (bind(listening_socket, ServerConnectionInfo->ai_addr, (int)ServerConnectionInfo->ai_addrlen) == SOCKET_ERROR)
	{
		BerkeleySockets->getError();
		std::cout << "bind() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		BerkeleySockets->closesocket(listening_socket);
		BerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
		std::cout << "Exiting server thread.\n";
		return;
	}

	// Set the socket to listen for incoming connections
	// SOMAXCONN == max length of queue of pending connections.
	// It means that the underlying service provider responsible for the socket
	// will set it to a reasonable value.
	if (listen(listening_socket, SOMAXCONN) == SOCKET_ERROR)
	{
		BerkeleySockets->getError();
		std::cout << "listen() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		BerkeleySockets->closesocket(listening_socket);
		if (ServerConnectionInfo != nullptr)
			BerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
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
		FD_SET(listening_socket, &ReadSet);

		// Used by select(). It makes it wait for x amount of time to wait for a readable socket to appear.
		// This has to be in this loop! Linux resets this value every time select() times out.
		TimeValue.tv_sec = 1;
		TimeValue.tv_usec = 0; // 1 million microseconds = 1 second

		// select() returns the number of handles that are ready and contained in the fd_set structure
		errno = 0;
		errchk = select((int)listening_socket + 1, &ReadSet, NULL, NULL, &TimeValue);
		if (errchk == SOCKET_ERROR)
		{
			BerkeleySockets->getError();
			std::cout << "server() select Error.\n";
			DBG_DISPLAY_ERROR_LOCATION();
			BerkeleySockets->closesocket(listening_socket);
			std::cout << "Closing listening socket b/c of the error. Ending Server Thread.\n";
			if (ServerConnectionInfo != nullptr)
				BerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
			return;
		}
		else if (connection_race_winner == CLIENT_WON)
		{
			// Close the socket because the client thread won the race.
			BerkeleySockets->closesocket(listening_socket);
			if (verbose_output == true)
				std::cout << "Closed listening socket, because the winner is: " << connection_race_winner << ". Ending Server thread.\n";
			if (ServerConnectionInfo != nullptr)
				BerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
			return;
		}
		else if (errchk > 0)	// select() told us that atleast 1 readable socket has appeared!
		{
			if (verbose_output == true)
				std::cout << "Attempting to accept a client now that select() returned a readable socket\n";

			// Accept the connection and create a new socket to communicate on.
			fd_socket = accept(listening_socket, nullptr, nullptr);
			if (fd_socket == INVALID_SOCKET)
			{
				BerkeleySockets->getError();
				std::cout << "accept() failed.\n";
				BerkeleySockets->closesocket(fd_socket);
				DBG_DISPLAY_ERROR_LOCATION();
				if (ServerConnectionInfo != nullptr)
					BerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
				return;
			}

			if (verbose_output == true)
				std::cout << "accept() succeeded. Setting connection_race_winner and global_socket\n";

			// Assigning global values to let the client thread know it should stop trying.
			if (setWinnerMutex(SERVER_WON) != SERVER_WON)
			{
				BerkeleySockets->closesocket(listening_socket);
				BerkeleySockets->closesocket(fd_socket);
				if (ServerConnectionInfo != nullptr)
					BerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
				return;
			}

			break;
		}
	}

	// Display who the user has connected to.
	BerkeleySockets->coutPeerIPAndPort(fd_socket);
	std::cout << "Acting as the server.\n\n\n\n\n\n\n\n\n\n";

	if (ServerConnectionInfo != nullptr)
		BerkeleySockets->freeaddrinfo(&ServerConnectionInfo);
	return;
}


void Connection::client()
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
	if (getaddrinfo(target_external_ip.c_str(), target_external_port.c_str(), &ClientHints, &ClientConnectionInfo) != 0)
	{
		BerkeleySockets->getError();
		std::cout << "getaddrinfo() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		BerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
		return;
	}

	// Create socket
	SOCKET client_socket = socket(ClientConnectionInfo->ai_family, ClientConnectionInfo->ai_socktype, ClientConnectionInfo->ai_protocol);
	if (client_socket == INVALID_SOCKET || client_socket == SOCKET_ERROR)
	{
		BerkeleySockets->getError();
		std::cout << "socket() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		BerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
		return;
	}

	// Disable blocking on the socket
	if (BerkeleySockets->setBlockingSocketOpt(client_socket, &BerkeleySockets->getDisableBlocking()) == -1)
	{
		std::cout << "Exiting client thread due to error.\n";
		if (ClientConnectionInfo != nullptr)
			BerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
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
		int32_t conn_return_val = connect(client_socket, ClientConnectionInfo->ai_addr, (int)ClientConnectionInfo->ai_addrlen);
		// Checking if the user or the program wants to exit.
		if (exit_now == true)
		{
			BerkeleySockets->closesocket(client_socket);
			if (ClientConnectionInfo != nullptr)
				BerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
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
			int32_t err_chk = BerkeleySockets->getError(BerkeleySockets->getDisableConsoleOutput());
			if (err_chk == OPERATION_ALREADY_IN_PROGRESS || err_chk == OPERATION_NOW_IN_PROGRESS || err_chk == WOULD_BLOCK || err_chk == NO_SOCKET_ERROR)
			{
				// Let us move on to select() to see if we have completed the connection.

				// If fd_socket isn't set in the WriteSet struct, set it.
				if (FD_ISSET(client_socket, &WriteSet) == false)
				{
					// Put specified socket into the socket array located in WriteSet
					// Format is:  FD_SET(int32_t fd, fd_set *WriteSet);
					FD_SET(client_socket, &WriteSet);
				}

				// Used by select(). It makes it wait for x amount of time to wait for a readable socket to appear.
				// Linux select() modifies these values to reflect the amount of time not slept, therefore
				// they need to be assigned every time.
				TimeValue.tv_sec = 1;
				TimeValue.tv_usec = 0;

				// select() returns the number of socket handles that are ready and contained in the fd_set structure
				// returns 0 if the time limit has expired and it still hasn't seen any ready sockets handles.
				errno = 0;
				errchk = select((int)client_socket + 1, NULL, &WriteSet, NULL, &TimeValue);
				if (errchk == SOCKET_ERROR)
				{
					BerkeleySockets->getError();
					std::cout << "ClientThread() select Error.\n";
					DBG_DISPLAY_ERROR_LOCATION();

					// Get error information from the socket, not just from select()
					// The effectiveness of this is untested so far as I haven't seen select error yet.
					int32_t errorz = BerkeleySockets->getSockOptError(client_socket);
					if (errorz == SOCKET_ERROR)
					{
						std::cout << "getsockopt() failed.\n";
						BerkeleySockets->getError();
						DBG_DISPLAY_ERROR_LOCATION();
					}
					else
					{
						std::cout << "getsockopt() returned: " << errorz << "\n";
					}

					BerkeleySockets->closesocket(client_socket);
					std::cout << "Closing connect socket b/c of the error. Ending client thread.\n";
					if (ClientConnectionInfo != nullptr)
						BerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
					return;
				}
				else if (connection_race_winner == SERVER_WON)
				{
					// Close the socket because the server thread won the race.
					BerkeleySockets->closesocket(client_socket);
					if (verbose_output == true)
						std::cout << "Closed connect socket, because the winner is: " << connection_race_winner << ". Ending client thread.\n";
					if (ClientConnectionInfo != nullptr)
						BerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
					return;
				}
				else if (errchk > 0)	// select() told us that at least 1 readable socket has appeared
				{
					// Try to tell everyone that the client thread is the winner
					if (setWinnerMutex(CLIENT_WON) != CLIENT_WON)
					{
						// Server thread must have won the race
						BerkeleySockets->closesocket(client_socket);
						if (verbose_output == true)
							std::cout << "Exiting client thread because the server thread won the race.\n";
						if (ClientConnectionInfo != nullptr)
							BerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
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
				BerkeleySockets->outputSocketErrorToConsole(errchk);
				BerkeleySockets->closesocket(client_socket);
				DBG_DISPLAY_ERROR_LOCATION();
				std::cout << "Exiting client thread.\n";
				if (ClientConnectionInfo != nullptr)
					BerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
				return;
			}
		}
	}
	
	// Since fd_socket is the one that will be checked / used by things outside
	// this class, assign fd_socket the connected socket.
	fd_socket = client_socket;
	// Re-enable blocking for the socket.
	if (BerkeleySockets->setBlockingSocketOpt(fd_socket, &BerkeleySockets->getEnableBlocking()) == -1)
	{
		BerkeleySockets->getError();
		std::cout << "Fatal error: failed to change the socket blocking option.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		BerkeleySockets->closesocket(fd_socket);
		if (ClientConnectionInfo != nullptr)
			BerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
		return;
	}

	// Display who the user is connected with.
	BerkeleySockets->coutPeerIPAndPort(fd_socket);
	std::cout << "Acting as the client.\n\n\n\n\n\n\n\n\n\n";
	
	if (ClientConnectionInfo != nullptr)
		BerkeleySockets->freeaddrinfo(&ClientConnectionInfo);
	return;
}


int32_t Connection::setWinnerMutex(int32_t the_winner)
{
	RaceMutex.lock();

	// If nobody has won the race yet, then set the winner.
	if (connection_race_winner == NOBODY_WON)
	{
		connection_race_winner = the_winner;
	}

	RaceMutex.unlock();
	return connection_race_winner;
}

// Necessary to do anything with sockets on Windows
// Returns 0, success.
// Returns a WSAERROR code if failed.
int32_t Connection::WSAStartup()
{
#ifdef _WIN32
	int32_t errchk = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (errchk != 0)
	{
		std::cout << "WSAStartup failed, WSAERROR: " << errchk << "\n";
		return errchk;
	}
#endif//_WIN32
	return 0;
}
