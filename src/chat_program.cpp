// chat_program.cpp
#ifdef __linux__
#include <stdio.h>
#include <stdlib.h>
#include <iostream>		// cout
#include <string>
#include <string.h> //memset
#include <pthread.h>	// <process.h>  multithreading
#include <thread>
#include <vector>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "chat_program.h"
#include "GlobalTypeHeader.h"
#include "string_manipulation.h"
#include "UPnP.h"
#endif //__linux__

#ifdef _WIN32
#include <iostream>		// cout
#include <process.h>	// <pthread.h> multithreading
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <vector>
#include <climits>

#include "chat_program.h"
#include "GlobalTypeHeader.h"
#include "string_manipulation.h"
#include "UPnP.h"
#endif //_WIN32


#ifdef __linux__
#define INVALID_SOCKET	((SOCKET)(~0))	// To indicate INVALID_SOCKET, Linux returns (~0) from socket functions, and windows returns -1.
#define SOCKET_ERROR	(-1)			// I belive this was just because linux didn't already have a SOCKET_ERROR macro.
#define THREAD_RETURN_VALUE return NULL
#endif // __linux__

#ifdef __linux__
	pthread_t Connection::thread0 = 0;	// Server
	pthread_t Connection::thread1 = 0;	// Client
	pthread_t Connection::thread2 = 0;	// Send()
	int Connection::ret0 = 0;	// Server
	int Connection::ret1 = 0;	// Client
    int Connection::ret2 = 0;	// Send()

    // for the shutdown() function
    const int SD_BOTH = 2;
#endif //__linux__

#ifdef _WIN32
// These are needed for Windows sockets
#pragma comment(lib, "Ws2_32.lib") //tell the linker that Ws2_32.lib file is needed.
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#pragma warning(disable:4996)		// disable deprecated warning for fopen()

// HANDLE storage for threads
HANDLE Connection::ghEvents[2]{};	// i should be using vector of ghevents[] instead...
									// [0] == server
									// [1] == client
#endif//_WIN32

									//mutex for use in this class' send()
std::mutex Connection::SendMutex;
// mutex for use with server and client threads to prevent a race condition.
std::mutex Connection::RaceMutex;


// this default_port being static might not be a good idea.
// This is the default port for the Connection as long as
// Connection isn't given any port from the UPnP class,
// which has its own default port, or given any port from the
// command line.
const std::string Connection::DEFAULT_PORT = "30248";

// Variables necessary for determining who won the connection race
const int Connection::SERVER_WON = -29;
const int Connection::CLIENT_WON = -30;
const int Connection::NOBODY_WON = -25;
int Connection::global_winner = NOBODY_WON;


// Flags for send() that indicated what the message is being used for.
// CR == CryptRelay
const int8_t Connection::CR_NO_FLAG = 0;
const int8_t Connection::CR_BEGIN = 0;
const int8_t Connection::CR_SIZE_NOT_ASSIGNED = 0;
const int8_t Connection::CR_CHAT = 1;
const int8_t Connection::CR_ENCRYPTED_CHAT_MESSAGE = 2;
const int8_t Connection::CR_FILE_NAME = 30;
const int8_t Connection::CR_FILE_SIZE = 31;
const int8_t Connection::CR_FILE = 32;
const int8_t Connection::CR_ENCRYPTED_FILE = 33;

// How many characters at the beginning of the buffer that should be
// reserved for usage of a flag, (1) and size of the message (2). (1 + 2 == 3)
const int8_t Connection::CR_RESERVED_BUFFER_SPACE = 3;


Connection::Connection()
{
	ConnectionInfo = nullptr;
	memset(incoming_file_name_from_peer_cstr, 0, INCOMING_FILE_NAME_FROM_PEER_SIZE);
}
Connection::~Connection()
{
	// ****IMPORTANT****
	// All addrinfo structures must be freed once they are done being used.
	// Making sure we never freeaddrinfo twice. Ugly bugs otherwise.
	// Check comments in the freeaddrinfo() to see how its done.
	if (ConnectionInfo != nullptr)
		ClientServerSocketClass->freeaddrinfo(&ConnectionInfo);
	if (ConnectionInfo != nullptr)
		ClientSocketClass.freeaddrinfo(&ConnectionInfo);
	if (ConnectionInfo != nullptr)
		ServerSocketClass.freeaddrinfo(&ConnectionInfo);
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

// Thread entrance for startServerThread();
void Connection::createStartServerThread(void * instance)
{
	if (global_verbose == true)
		std::cout << "Starting server thread.\n";
#ifdef __linux__
	ret0 = pthread_create(&thread0, NULL, posixStartServerThread, instance);
	if (ret0)
	{
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret0);
		DBG_DISPLAY_ERROR_LOCATION();
		exit(EXIT_FAILURE);
	}
#endif

#ifdef _WIN32
	// If a handle is desired, do something like this:
	// ghEvents[0] = (HANDLE) _beginthread(startServerThread, 0, instance);
	// or like this because it seems a little better way to deal with error checking
	// because a negative number isn't being casted to a void *   :
	// uintptr_t thread_handle = _beginthread(startServerThread, 0, instance);
	// ghEvents[0] = (HANDLE)thread_handle;
	//   if (thread_handle == -1L)
	//		error stuff here;
	errno = 0;
	uintptr_t thread_handle = _beginthread(serverThread, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
	ghEvents[0] = (HANDLE)thread_handle;	// i should be using vector of ghEvents instead
	if (thread_handle == -1L)
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return;
	}

	// previous way of doing it:
	/*
	ghEvents[0] = (HANDLE)_beginthread(startServerThread, 0, instance);		//c style typecast    from: uintptr_t    to: HANDLE.
	if (ghEvents[0] == (HANDLE)(-1L) )
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		return;
	} 
	*/

#endif//_WIN32
}

// This function exists because threads on linux have to return a void*.
// Conversely on windows it doesn't return anything because threads return void.
void* Connection::posixStartServerThread(void * instance)
{
	if(instance != nullptr)
		serverThread(instance);
	return nullptr;
}

void Connection::serverThread(void * instance)
{
	Connection * self = static_cast <Connection*> (instance);

	if (instance == nullptr)
	{
		std::cout << "startServerThread() thread instance NULL\n";
		DBG_DISPLAY_ERROR_LOCATION();
		self->exitThread(nullptr);
	}

	memset(&self->Hints, 0, sizeof(self->Hints));
	// These are the settings for the connection
	self->Hints.ai_family = AF_INET;		//ipv4
	self->Hints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	//self->Hints.ai_flags = AI_PASSIVE;	// Use local-host address
	self->Hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	// Place target ip and port, and Hints about the connection type into a linked list named addrinfo *ConnectionInfo
	// Now we use ConnectionInfo instead of Hints.
	// Remember we are only listening as the server, so put in local IP:port
	if (self->ServerSocketClass.getaddrinfo(self->my_local_ip, self->my_local_port, &self->Hints, &self->ConnectionInfo) == true)
		self->exitThread(nullptr);

	// Create socket
	SOCKET errchk_socket = self->ServerSocketClass.socket(self->ConnectionInfo->ai_family, self->ConnectionInfo->ai_socktype, self->ConnectionInfo->ai_protocol);
	if (errchk_socket == INVALID_SOCKET)
		self->exitThread(nullptr);
	else if (errchk_socket == SOCKET_ERROR)
		self->exitThread(nullptr);

	// Assign the socket to an address:port
	// Binding the socket to the user's local address
	if (self->ServerSocketClass.bind(self->ConnectionInfo->ai_addr, self->ConnectionInfo->ai_addrlen) == true)
		self->exitThread(nullptr);

	// Set the socket to listen for incoming connections
	if (self->ServerSocketClass.listen() == true)
		self->exitThread(nullptr);


	// Needed to provide a time to select()
	timeval TimeValue;
	memset(&TimeValue, 0, sizeof(TimeValue));

	// Used by select(). It makes it wait for x amount of time to wait for a readable socket to appear.
	TimeValue.tv_sec = 5;
	//TimeValue.tv_usec = 500'000; // 1 million microseconds = 1 second

	// This struct contains the array of sockets that select() will check for readability
	fd_set ReadSet;
	memset(&ReadSet, 0, sizeof(ReadSet));

	// loop check for incoming connections
	int errchk;
	while (1)
	{
		DBG_TXT("Server thread active...");

		// Please use the macros FD_SET, FD_CHECK, FD_CLEAR, when dealing with struct fd_set
		// set the socket count in the ReadSet struct to 0.
		FD_ZERO(&ReadSet);
		// Put specified socket into the socket array located in ReadSet
		// Format is:  FD_SET(int fd, fd_set *ReadSet);
		FD_SET(self->ServerSocketClass.fd_socket, &ReadSet);

		// This has to be in this loop! Linux resets this value every time select() times out.
		TimeValue.tv_sec = 2;

		// select() returns the number of handles that are ready and contained in the fd_set structure
		errno = 0;
		errchk = select(self->ServerSocketClass.fd_socket + 1, &ReadSet, NULL, NULL, &TimeValue);
		if (errchk == SOCKET_ERROR)
		{
			self->ServerSocketClass.getError();
			std::cout << "serverThread() select Error.\n";
			DBG_DISPLAY_ERROR_LOCATION();
			self->ServerSocketClass.closesocket(self->ServerSocketClass.fd_socket);
			std::cout << "Closing listening socket b/c of the error. Ending Server Thread.\n";
			self->exitThread(nullptr);
		}
		else if (global_winner == CLIENT_WON)
		{
			// Close the socket because the client thread won the race.
			self->ServerSocketClass.closesocket(self->ServerSocketClass.fd_socket);
			if (global_verbose == true)
			{
				std::cout << "Closed listening socket, because the winner is: " << global_winner << ". Ending Server thread.\n";
			}
			self->exitThread(nullptr);
		}
		else if (errchk > 0)	// select() told us that atleast 1 readable socket has appeared!
		{
			if (global_verbose == true)
				std::cout << "Attempting to accept a client now that select() returned a readable socket\n";

			SOCKET errchk_socket;
			// Accept the connection and create a new socket to communicate on.
			errchk_socket = self->ServerSocketClass.accept();
			if (errchk_socket == INVALID_SOCKET)
			{
				self->ServerSocketClass.getError();
				DBG_DISPLAY_ERROR_LOCATION();
				self->exitThread(nullptr);
			}

			if (global_verbose == true)
				std::cout << "accept() succeeded. Setting global_winner and global_socket\n";

			// Assigning global values to let the client thread know it should stop trying.
			if (self->setWinnerMutex(SERVER_WON) != SERVER_WON)
			{
				self->ServerSocketClass.closesocket(self->ServerSocketClass.fd_socket);
				self->exitThread(nullptr);
			}
			else
			{
				SocketClass::global_socket = self->ServerSocketClass.fd_socket;
			}

			break;
		}
	}

	// Display who the user has connected to.
	self->ServerSocketClass.coutPeerIPAndPort();
	
	// Assigning the ClientServerSocketClass to the winner of the race.
	// This is because functions like loopedGetUserInputThread() need a way
	// to reference the winning instance.
	self->ClientServerSocketClass = &self->ServerSocketClass;

	// Receive messages until there is an error or connection is closed.
	// Pattern is:  rcv_thread(function, point to class that function is in (aka, this), function argument)
	std::thread rcv_thread(&Connection::loopedReceiveMessagesThread, self);

	// Get the user's input from the terminal, and check
	// to see if the user wants to do something special,
	// else just send the message that the user typed out.
	std::thread get_user_input_thread(&Connection::loopedGetUserInputThread, self);

	// Wait here until get_user_input_thread finishes
	if (get_user_input_thread.joinable() == true)
		get_user_input_thread.join();

	// Wait here until recv_thread finishes.
	if (rcv_thread.joinable())
		rcv_thread.join();

	// Exiting chat program
	self->exitThread(nullptr);
}

void Connection::createStartClientThread(void * instance)
{
	if (global_verbose == true)
		std::cout << "Starting client thread.\n";
#ifdef __linux__
	ret1 = pthread_create(&thread1, NULL, posixStartClientThread, instance);
	if (ret1)
	{
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret1);
		DBG_DISPLAY_ERROR_LOCATION();
		exit(EXIT_FAILURE);
	}
#endif

#ifdef _WIN32
	// If a handle is desired, do something like this:
	// ghEvents[0] = (HANDLE) _beginthread(startServerThread, 0, instance);
	// or like this because it seems a little better way to deal with error checking
	// because a negative number isn't being casted to a void *   :
	// uintptr_t thread_handle = _beginthread(startServerThread, 0, instance);
	// ghEvents[0] = (HANDLE)thread_handle;
	//   if (thread_handle == -1L)
	//		error stuff here;
	uintptr_t thread_handle = _beginthread(clientThread, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
	ghEvents[1] = (HANDLE)thread_handle;
	if (thread_handle == -1L)
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return;
	}

	// previous way of doing it:
	/*
	ghEvents[1] = (HANDLE)_beginthread(startClientThread, 0, instance);		//c style typecast    from: uintptr_t    to: HANDLE.
	if (ghEvents[1] == (HANDLE)(-1L) )
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		return;
	} 
	*/

#endif//_WIN32
}

// This function exists because threads on linux have to return a void*.
// Conversely on windows it doesn't return anything because threads return void.
void* Connection::posixStartClientThread(void * instance)
{
	if (instance != nullptr)
		clientThread(instance);
	return nullptr;
}






void Connection::clientThread(void * instance)
{
	Connection* self = static_cast <Connection*> (instance);
	if (instance == nullptr)
	{
		std::cout << "clientThread() thread instance NULL\n";
		DBG_DISPLAY_ERROR_LOCATION();
		self->exitThread(nullptr);
	}

	int errchk;
	addrinfo ClientHints;
	addrinfo* ClientConnectionInfo;
	

	memset(&ClientHints, 0, sizeof(ClientHints));
	// These are the settings for the connection
	ClientHints.ai_family = AF_INET;		// ipv4
	ClientHints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	//ClientHints.ai_flags = AI_PASSIVE;    // Use local-host address
	ClientHints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	// Place target ip and port, and ClientHints about the connection type into a linked list named addrinfo *ConnectionInfo
	// Now we use ClientConnectionInfo instead of ClientHints in order to access the information.
	if (self->ClientSocketClass.getaddrinfo(self->target_external_ip, self->target_external_port, &ClientHints, &ClientConnectionInfo) == true)
		self->exitThread(nullptr);

	// Create socket
	SOCKET errchk_socket = self->ClientSocketClass.socket(ClientConnectionInfo->ai_family, ClientConnectionInfo->ai_socktype, ClientConnectionInfo->ai_protocol);
	if (errchk_socket == INVALID_SOCKET)
		self->exitThread(nullptr);
	else if (errchk_socket == SOCKET_ERROR)
		self->exitThread(nullptr);

	// Disable blocking on the socket
	if (self->ClientSocketClass.setBlockingSocketOpt(self->ClientSocketClass.fd_socket, &self->ClientSocketClass.DISABLE_BLOCKING) == true)
	{
		self->exitThread(nullptr);
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
		const int WOULD_BLOCK = WSAEWOULDBLOCK;
		const int OPERATION_ALREADY_IN_PROGRESS = WSAEALREADY;
		const int OPERATION_NOW_IN_PROGRESS = WSAEINPROGRESS;
#endif//_WIN32
#ifdef __linux__
		const int WOULD_BLOCK = EWOULDBLOCK;
		const int OPERATION_ALREADY_IN_PROGRESS = EALREADY;
		const int OPERATION_NOW_IN_PROGRESS = EINPROGRESS;
#endif//__linux__


		// Please use the macros FD_SET, FD_CHECK, FD_CLEAR, FD_ZERO, FD_ISSET when dealing with struct fd_set.
		// Set the socket count in the WriteSet struct to 0.
		FD_ZERO(&WriteSet);

		// Attempt to connect to target
		errno = 0;
		int conn_return_val = self->ClientSocketClass.connect(ClientConnectionInfo->ai_addr, ClientConnectionInfo->ai_addrlen);
		if (self->exit_now == true)
		{
			self->ClientSocketClass.closesocket(self->ClientSocketClass.fd_socket);
			self->exitThread(nullptr);
		}
		if (conn_return_val == 0)
		{
			// Connected immediately
			break;
		}
		if (conn_return_val != 0)
		{
			int err_chk = self->ClientSocketClass.getError(self->ClientSocketClass.DISABLE_CONSOLE_OUTPUT);
			if (err_chk == OPERATION_ALREADY_IN_PROGRESS || err_chk == OPERATION_NOW_IN_PROGRESS || err_chk == WOULD_BLOCK)
			{
				// Let us move on to select() to see if we have completed the connection.

				// If fd_socket isn't set in the WriteSet struct, set it.
				if (FD_ISSET(self->ClientSocketClass.fd_socket, &WriteSet) == false)
				{
					// Put specified socket into the socket array located in WriteSet
					// Format is:  FD_SET(int fd, fd_set *WriteSet);
					FD_SET(self->ClientSocketClass.fd_socket, &WriteSet);
				}

				// Used by select(). It makes it wait for x amount of time to wait for a readable socket to appear.
				// Linux select() modifies these values to reflect the amount of time not slept, therefore
				// they need to be assigned every time.
				TimeValue.tv_sec = 2;
				TimeValue.tv_usec = 0;

				// select() returns the number of socket handles that are ready and contained in the fd_set structure
				// returns 0 if the time limit has expired and it still hasn't seen any ready sockets handles.
				errno = 0;
				errchk = select(self->ClientSocketClass.fd_socket + 1, NULL, &WriteSet, NULL, &TimeValue);
				if (self->exit_now == true)
				{
					self->ClientSocketClass.closesocket(self->ClientSocketClass.fd_socket);
					self->exitThread(nullptr);
				}
				if (errchk == SOCKET_ERROR)
				{
					self->ClientSocketClass.getError();
					std::cout << "ClientThread() select Error.\n";
					DBG_DISPLAY_ERROR_LOCATION();

					// Get error information from the socket, not just from select()
					// The effectiveness of this is untested so far as I haven't seen select error yet.
					int errorz = self->ClientSocketClass.getSockOptError(self->ClientSocketClass.fd_socket);
					if (errorz == SOCKET_ERROR)
					{
						std::cout << "getsockopt() failed.\n";
						self->ClientSocketClass.getError();
						DBG_DISPLAY_ERROR_LOCATION();
					}
					else
					{
						std::cout << "getsockopt() returned: " << errorz << "\n";
					}

					self->ClientSocketClass.closesocket(self->ClientSocketClass.fd_socket);
					std::cout << "Closing connect socket b/c of the error. Ending client thread.\n";
					self->exitThread(nullptr);
				}
				else if (global_winner == SERVER_WON)
				{
					// Close the socket because the server thread won the race.
					self->ClientSocketClass.closesocket(self->ClientSocketClass.fd_socket);
					if (global_verbose == true)
					{
						std::cout << "Closed connect socket, because the winner is: " << global_winner << ". Ending client thread.\n";
					}
					self->exitThread(nullptr);
				}
				else if (errchk > 0)	// select() told us that at least 1 readable socket has appeared
				{
					// Try to tell everyone that the client thread is the winner
					if (setWinnerMutex(CLIENT_WON) != CLIENT_WON)
					{
						// Server thread must have won the race
						self->ClientSocketClass.closesocket(self->ClientSocketClass.fd_socket);
						if (global_verbose == true)
							std::cout << "Exiting client thread because the server thread won the race.\n";
						self->exitThread(nullptr);
					}
					else// Client is confirmed as the winner of the race.
					{
						SocketClass::global_socket = self->ClientSocketClass.fd_socket;
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
				self->ClientSocketClass.outputSocketErrorToConsole(errchk);
				DBG_DISPLAY_ERROR_LOCATION();
				std::cout << "Exiting client thread.\n";
				self->exitThread(nullptr);
			}
		}
	}

	// Display who the user is connected with.
	self->ClientSocketClass.coutPeerIPAndPort();

	// Assigning the ClientServerSocketClass to the winner of the race.
	// This is because functions like loopedGetUserInputThread() need a way
	// to reference the winning instance.
	self->ClientServerSocketClass = &self->ClientSocketClass;

	// Re-enable blocking for the socket.
	if (self->ClientServerSocketClass->setBlockingSocketOpt(self->ClientServerSocketClass->fd_socket, &self->ClientServerSocketClass->ENABLE_BLOCKING) == true)
	{
		self->exitThread(nullptr);
	}

	// Receive messages until there is an error or connection is closed.
	// Pattern is:  rcv_thread(function, point to class that function is in (aka, this), function argument)
	std::thread rcv_thread(&Connection::loopedReceiveMessagesThread, self);

	// Get the user's input from the terminal, and check
	// to see if the user wants to do something special,
	// else just send the message that the user typed out.
	std::thread get_user_input_thread(&Connection::loopedGetUserInputThread, self);

	// Wait here until get_user_input_thread finishes
	if (get_user_input_thread.joinable() == true)
		get_user_input_thread.join();

	// Wait here until recv_thread finishes.
	if (rcv_thread.joinable())
		rcv_thread.join();

	// Exiting chat program
	self->exitThread(nullptr);
}


// remote_host is /* optional */    default == "Peer".
// remote_host is the IP that we are connected to.
// To find out who we were connected to, use getnameinfo()
void Connection::loopedReceiveMessagesThread()
{
	// Helpful Information:
	// Any time a 'message' is mentioned, it is referring to the idea of
	// a whole message. That message may be broken up into packets and sent over the
	// network, and when it gets to this recv() loop, it will read in as much as it can
	// at a time. The entirety of a message is not always contained in the recv_buf,
	// because the message could be so large that you will have to recv() multiple times
	// in order to get the whole message. 
	// References to 'message' have nothing to do with the recv_buf.
	// the size and type of a message is determined by the peer, and the peer tells us
	// the type and size of the message in the first CR_RESERVED_BUFFER_SPACE characters
	// of his message that he sent us.

	// Receive until the peer shuts down the connection
	if (global_verbose == true)
		std::cout << "Recv loop started...\n";


	position_in_message = CR_BEGIN;	// current cursor position inside the imaginary message sent by the peer.

	type_of_message_flag = CR_NO_FLAG;
	message_size_part_one = CR_SIZE_NOT_ASSIGNED;
	message_size = CR_SIZE_NOT_ASSIGNED;		// peer told us this size

	process_recv_buf_state = CHECK_FOR_FLAG;

	// Buffer for receiving messages
	static const long long recv_buf_len = 512;
	char recv_buf[recv_buf_len];

	const int CONNECTION_GRACEFULLY_CLOSED = 0; // when recv() returns 0, it means gracefully closed.
	int bytes = 0;	
	while (1)
	{
		bytes = ::recv(SocketClass::global_socket, (char *)recv_buf, recv_buf_len, 0);
		if (bytes > 0)
		{
			// State machine that processes recv_buf and decides what to do
			// based on the information in the buffer.
			if (processRecvBuf(recv_buf, recv_buf_len, bytes) == true)
				break;
		}
		else if (bytes == SOCKET_ERROR)
		{
            #ifdef __linux__
			const int CONNECTION_RESET = ECONNRESET;
			const int BLOCKING_OPERATION_CANCELED = EINTR;
            #endif// __linux__
			#ifdef _WIN32
			const int CONNECTION_RESET = WSAECONNRESET;
			const int BLOCKING_OPERATION_CANCELED = WSAEINTR;
			#endif// _WIN32

			int errchk = ClientServerSocketClass->getError(ClientServerSocketClass->DISABLE_CONSOLE_OUTPUT);

			// If errchk == BLOCKING_OPERATION_CANCELED, don't report the error.
			// else, report whatever error happened.
			if (errchk != BLOCKING_OPERATION_CANCELED)
			{
				std::cout << "recv() failed.\n";
				ClientServerSocketClass->outputSocketErrorToConsole(errchk);
				DBG_DISPLAY_ERROR_LOCATION();
				if (errchk == CONNECTION_RESET)
				{
					std::cout << "Peer might have hit ctrl-c.\n";
				}
				if (is_file_done_being_written == false)
				{
					// Close the file that was being written inside the processRecvBuf() state machine.
					if (fclose(WriteFile) != 0)
					{
						perror("Error closing file for writing in binary mode.\n");
					}
					std::cout << "File transfer was interrupted. File name: " << incoming_file_name_from_peer << "\n";
				}
			}

			break;
		}
		else if (bytes == CONNECTION_GRACEFULLY_CLOSED)
		{
			std::cout << "Connection with peer has been gracefully closed.\n";
			break;
		}
	}
	exit_now = true;
	std::cout << "\n";
	std::cout << "# Press 'Enter' to exit CryptRelay.\n";

	return;
}

// Cross platform windows and linux thread exiting. Not for use with std::thread
void Connection::exitThread(void* ptr)
{
#ifdef _WIN32
    _endthread();
#endif//_WIN32
#ifdef __linux
    pthread_exit(ptr); // Not wanting to return anything, just exit
#endif//__linux__
}

// Check the user's message that he put into the terminal to see if he
// used the flag -f to indicate he wants to send a file.
bool Connection::doesUserWantToSendAFile(std::string& user_msg_from_terminal)
{
	size_t user_msg_from_terminal_len = user_msg_from_terminal.length();
	// pls split into multiple strings based on spaces
	// make a function that takes a string as input, and then return a vector filled with the strings that it split up.

	if (user_msg_from_terminal_len >= 2)
	{
		if (user_msg_from_terminal[0] == '-' && user_msg_from_terminal[1] == 'f')
			return true;
	}
		
	return false;
}

// WARNING:
// This function expects the caller to protect against accessing
// invalid memory.
// All information that gets sent over the network MUST go through
// this function. This is to avoid abnormal behavior / problems
// with multiple threads trying to send() using the same socket.
// The flag argument is to tell the receiver of these messages
// how to interpret the incoming message. For example as a file,
// or as a chat message.
// To see a list of flags, look in the header file.
int Connection::send(const char * sendbuf, int amount_to_send)
{
	// Whatever thread gets here first, will lock
	// the door so that nobody else can come in.
	// That means all other threads will form a queue here,
	// and won't be able to go past this point until the
	// thread that got here first unlocks it.
	SendMutex.lock();
	total_amount_sent = amount_to_send;

	do
	{
		bytes_sent = ::send(SocketClass::global_socket, sendbuf, amount_to_send, 0);
		if (bytes_sent == SOCKET_ERROR)
		{
			ClientServerSocketClass->getError();
			perror("ERROR: send() failed.");
			DBG_DISPLAY_ERROR_LOCATION();
			ClientServerSocketClass->closesocket(SocketClass::global_socket);
			SendMutex.unlock();
			return SOCKET_ERROR;
		}

		amount_to_send -= bytes_sent;

	} while (amount_to_send > 0);

	// Unlock the door now that the thread is done with this function.
	SendMutex.unlock();

	return total_amount_sent;// returning the amount of bytes sent.
}

	

// The user's input is getlined here and checked for things
// that the user might want to do.
void Connection::loopedGetUserInputThread()
{
	std::thread file_transfer;
	std::string user_input;
		
	const long long CHAT_BUF_LEN = 4096;
	char* chat_buf = new char[CHAT_BUF_LEN];

	while (1)
	{
		std::getline(std::cin, user_input);

		// If some other thread has errored, or has already closed the connection,
		// then it will set exit_now = true;
		if (exit_now == true)
		{
			//Exit program
			break;
		}
		if (user_input == "exit()")
		{
			// Exit program.
			exit_now = true;
			break;
		}
		if (doesUserWantToSendAFile(user_input) == true)
		{
			// Join the thread if the thread has finished execution.
			if (is_send_file_thread_in_use == false)
			{
				if (file_transfer.joinable() == true)
				{
					file_transfer.join();
				}
			}
			else // The thread is still active, don't go trying to do anything.
			{
				std::cout << "Please wait for the file(s) to finish sending before sending another.\n";
				continue; // start at the beginning of the while loop.
			}

			StringManip StrManip;
			std::vector <std::string> split_strings;

			// Split the string into multiple strings for every space.
			if (StrManip.split(user_input, ' ', split_strings) == true)
			{
				std::cout << "Unexpected error: ";
				DBG_DISPLAY_ERROR_LOCATION();
			}


			// get the size of the array
			long long split_strings_size = split_strings.size();

			// There should be two strings in the vector.
			// [0] should be -f
			// [1] should be the file name
			if (split_strings_size < 2)
			{
				std::cout << "Error, not enough arguments given.\n";
			}

			std::string file_name_and_loca;
			std::string file_encryption_opt;
			// Determine if user wants to send a file or Encrypt & send a file.
			for (long long i = 0; i < split_strings_size; ++i)
			{
				if (split_strings[(u_int)i] == "-f" && i < split_strings_size - 1)
				{
					// The first string will always be -f. All strings after that
					// will be concatenated, and then have the spaces re-added after
					// to prevent issues with spaces in file names and paths.
					for (int b = 2; b < split_strings_size; ++b)
					{
						split_strings[1] += ' ' + split_strings[b];
					}
					if (split_strings_size >= 2)
					{
						file_name_and_loca = split_strings[1];

						// Fix the user's input to add an escape character to every '\'
						StrManip.duplicateCharacter(file_name_and_loca, '\\');
					}

					// Send the file
					// Making sure the thread doesn't already exist before creating it.
					if (file_transfer.joinable() == false)
					{
						is_send_file_thread_in_use = true;
						file_transfer = std::thread(&Connection::sendFileThread, this, file_name_and_loca);
					}

					break;
				}
				else if (split_strings[(u_int)i] == "-fE" && i < split_strings_size -2)
				{
					std::cout << "-fE hasn't been implemented yet.\n";
					break;

					// THIS SCOPE IS INCOMPLETE, NON-FUNCTIONAL, AND COMPLETELY WRONG IN SOME AREAS

					file_name_and_loca = split_strings[(u_int)i + 1];
					file_encryption_opt = split_strings[(u_int)i + 2];
					std::string copied_file_name_and_location;

					// The first string will always be -f. All strings after that
					// will be concatenated, and then have the spaces re-added after
					// to prevent issues with spaces in file names and paths.
					for (int b = 2; b < split_strings_size; ++b)
					{
						split_strings[1] += ' ' + split_strings[b];
					}
					if (split_strings_size >= 2)
					{
						file_name_and_loca = split_strings[1];

						// Fix the user's input to add an escape character to every '\'
						copied_file_name_and_location = StrManip.duplicateCharacter(file_name_and_loca, '\\');
						copied_file_name_and_location += ".enc";
					}

					// Copy the file before encrypting it.
					if (copyFile(file_name_and_loca.c_str(), copied_file_name_and_location.c_str()) == true)
					{
						std::cout << "Error: couldn't copy file for encryption. Abandoning attempt to send the file.\n";
					}

					// Encrypt the copied file.
					// beep boop encryption(file_encryption_option);

					// Send the file
					if (file_transfer.joinable() == false)
					{
						is_send_file_thread_in_use = true;
						file_transfer = std::thread(&Connection::sendFileThread, this, copied_file_name_and_location);
					}
					//if (sendFileThread(copied_file_name_and_location) == true)
						//return;//exit please?
					break;
				}
			}
		}
		else // Continue doing normal chat operation.
		{
			// User input can't exceed USHRT_MAX b/c that is the maximum size
			// that is able to be sent to the peer. (artifically limited).
			long long user_input_length = user_input.length();
			if (user_input_length > USHRT_MAX)
			{
				std::cout << "User input exceeded " << USHRT_MAX << ". Exiting\n";
				DBG_DISPLAY_ERROR_LOCATION();
				break;
			}

			long long amount_to_send = CR_RESERVED_BUFFER_SPACE + user_input_length;

			if (CHAT_BUF_LEN >= CR_RESERVED_BUFFER_SPACE)
			{
				// Copy the type of message flag into the buf
				chat_buf[0] = CR_CHAT;
				// Copy the size of the message into the buf as big endian.
				chat_buf[1] = (char)(user_input_length >> 8);
				chat_buf[2] = (char)user_input_length;

				// Copy the user's message into the buf
				if ((CHAT_BUF_LEN >= (user_input_length - CR_RESERVED_BUFFER_SPACE))
					&& user_input_length < UINT_MAX)
				{
					memcpy(chat_buf + CR_RESERVED_BUFFER_SPACE, user_input.c_str(), (size_t)user_input_length);
				}
				else
				{
					std::cout << "Message was too big for the send buf.\n";
					DBG_DISPLAY_ERROR_LOCATION();
					break;
				}
			}
			else
			{
				std::cout << "Programmer error. BUF_LEN < 3.\n";
				DBG_DISPLAY_ERROR_LOCATION();
				break;
			}
			int b = send((char *)chat_buf, (int)amount_to_send);
			if (b == SOCKET_ERROR)
			{
				if (global_verbose == true)
					std::cout << "Exiting loopedGetUserInputThread()\n";
				break;
			}

			// Reset user_input.
			user_input = "";
		}
	}

	delete[]chat_buf;

	// This will shutdown the connection on the socket.
	// closesocket() will interrupt recv() if it is currently blocking.
	// Done communicating with peer. Proceeding to exit.
	ClientServerSocketClass->shutdown(SocketClass::global_socket, SD_BOTH);	// SD_BOTH == shutdown both send and receive on the socket.
	ClientServerSocketClass->closesocket(SocketClass::global_socket);

	if (file_transfer.joinable() == true)
		file_transfer.join();

	return;
}

bool Connection::displayFileSize(const char* file_name_and_location, myStat * FileStatBuf)
{
	if (FileStatBuf == NULL)
	{
		std::cout << "displayFileSize() failed. NULL pointer.\n";
		return true;
	}
	// Shifting the bits over by 10. This divides it by 2^10 aka 1024
	long long KB = FileStatBuf->st_size >> 10;
	long long MB = KB >> 10;
	long long GB = MB >> 10;
	std::cout << "Displaying file size as Bytes: " << FileStatBuf->st_size << "\n";
	std::cout << "As KB: " << KB << "\n";
	std::cout << "As MB: " << MB << "\n";
	std::cout << "As GB: " << GB << "\n";

	return false;
}

// This function expects the file name and location of the file
// to be properly formatted already. That means escape characters
// must be used to make a path valid '\\'.
bool Connection::copyFile(const char * read_file_name_and_location, const char * write_file_name_and_location)
{

	FILE *ReadFile;
	FILE *WriteF;

	ReadFile = fopen(read_file_name_and_location, "rb");
	if (ReadFile == NULL)
	{
		perror("Error opening file for reading binary");
		return true;
	}
	WriteF = fopen(write_file_name_and_location, "wb");
	if (WriteF == NULL)
	{
		if (fclose(ReadFile))
			perror("Error closing file designated for writing");

		perror("Error opening file for writing binary");
		return true;
	}

	// Get file stastics and cout the size of the file
	long long size_of_file_to_be_copied = getFileStatsAndDisplaySize(read_file_name_and_location);

	// Please make a sha hash of the file here so it can be checked with the
	// hash of the copy later.

	long long bytes_read;
	long long bytes_written;
	long long total_bytes_written_to_file = 0;

	long long twenty_five_percent = size_of_file_to_be_copied / 4;	// divide it by 4
	long long fifty_percent = size_of_file_to_be_copied / 2;	//divide it by two
	long long seventy_five_percent = size_of_file_to_be_copied - twenty_five_percent;

	bool twenty_five_already_spoke = false;
	bool fifty_already_spoke = false;
	bool seventy_five_already_spoke = false;

	// (8 * 1024) == 8,192 aka 8KB, and 8192 * 1024 == 8,388,608 aka 8MB
	const long long buffer_size = 8 * 1024 * 1024;
	char* buffer = new char[buffer_size];

	std::cout << "Starting Copy file...\n";
	do
	{
		bytes_read = fread(buffer, 1, buffer_size, ReadFile);
		if (bytes_read)
			bytes_written = fwrite(buffer, 1, (size_t)bytes_read, WriteF);
		else
			bytes_written = 0;

		total_bytes_written_to_file += bytes_written;

		// If we have some statistics to work with, then display
		// ghetto progress indicators.
		if (size_of_file_to_be_copied >= 0)
		{
			if (total_bytes_written_to_file > twenty_five_percent && total_bytes_written_to_file < fifty_percent && twenty_five_already_spoke == false)
			{
				std::cout << "File copy 25% complete.\n";
				twenty_five_already_spoke = true;
			}
			else if (total_bytes_written_to_file > fifty_percent && total_bytes_written_to_file < seventy_five_percent && fifty_already_spoke == false)
			{
				std::cout << "File copy 50% complete.\n";
				fifty_already_spoke = true;
			}
			else if (total_bytes_written_to_file > seventy_five_percent && seventy_five_already_spoke == false)
			{
				std::cout << "File copy 75% complete.\n";
				seventy_five_already_spoke = true;
			}
		}


		// 0 means error. If they aren't equal to eachother
		// then it didn't write as much as it read for some reason.
	} while ((bytes_read > 0) && (bytes_read == bytes_written));

	if (total_bytes_written_to_file == size_of_file_to_be_copied)
		std::cout << "File copy complete.\n";

	// Please implement sha hash checking here to make sure the file is legit.

	if (bytes_written)
		perror("Error while copying the file");


	if (fclose(WriteF))
		perror("Error closing file designated for writing");
	if (fclose(ReadFile))
		perror("Error closing file designated for reading");

	delete[]buffer;
	return false;
}

bool Connection::sendFileThread(std::string name_and_location_of_file)
{
	// sha checking?


	// Maybe could make another flag message and send a flag saying
	// that CR_END_OF_FILE_REACHED to signal the file is done being sent?
	// just incase this filestat errors?


	// (8 * 1024) == 8,192 aka 8KB, and 8192 * 1024 == 8,388,608 aka 8MB
	// Buffer size must NOT be bigger than 65,536 bytes (u_short).
	const long long BUF_LEN = USHRT_MAX;
	char * buf = new char[BUF_LEN];

	// Open the file
	FILE *ReadFile;
	ReadFile = fopen(name_and_location_of_file.c_str(), "rb");
	if (ReadFile == NULL)
	{
		perror("Error opening file for reading binary sendFileThread");
		delete[]buf;
		is_send_file_thread_in_use = false;
		return true;
	}

	// Retrieve the name of the file from the string that contains
	// the path and the name of the file.
	std::string file_name = retrieveFileNameFromPath(name_and_location_of_file);
	if (file_name.empty() == true)
	{
		delete[]buf;
		if (fclose(ReadFile))
			perror("Error closing file designated for writing");
		is_send_file_thread_in_use = false;
		return false; //exit please, file name couldn't be found.
	}

	// *** The order in which file name and file size is sent DOES matter! ***
	// Send the file name to peer
	if (sendFileName(buf, BUF_LEN, file_name) == false)
	{
		delete[]buf;
		if (fclose(ReadFile))
			perror("Error closing file designated for writing");
		is_send_file_thread_in_use = false;
		return true; //exit please
	}

	// Get file statistics and display the size of the file
	long long size_of_file = 0;
	size_of_file = getFileStatsAndDisplaySize(name_and_location_of_file.c_str());
	if (size_of_file == -1)
	{
		delete[]buf;
		if (fclose(ReadFile))
			perror("Error closing file designated for writing");
		is_send_file_thread_in_use = false;
		return true; //exit please
	}


	// Send file size to peer
	if (sendFileSize(buf, BUF_LEN, size_of_file) == false)
	{
		delete[]buf;
		if (fclose(ReadFile))
			perror("Error closing file designated for writing");
		is_send_file_thread_in_use = false;
		return true;// exit please
	}


	// Send the file to peer.
	long long bytes_read = 0;
	long long bytes_sent = 0;
	long long total_bytes_sent = 0;

	// We are treating bytes_read as a u_short, instead of size_t
	// because we don't want to clog the send() with only file
	// packets. We want the user to still be able to send a message
	// within a reasonable amount of time while transfering a file.
	// File transfer jumps into mutex send() queue.
	// File transfer begins sending.
	// Chat message jumps into mutex send() queue. It is waiting
	//    for file transfer to leave the mutex first.
	// File transfer finishes sending USHRT_MAX bytes, and exits mutex.
	// Chat message now begins sending.
	//
	// This is just to illustrate that if we try sending too much into
	// the mutex send() queue at a time, then a chat message will take
	// forever to be sent out to the peer, causing an uncomfortable delay.
	static_assert(BUF_LEN <= USHRT_MAX,
		"Buffer size must NOT be bigger than USHRT_MAX.");

	std::cout << "Sending file: " << file_name << "\n";
	do
	{
		// The first 3 chars of the buffer are reserved for:
		// [0] Flag to tell the peer what kind of packet this is (file, chat)
		// [1] both [1] and [2] combined are considered a u_short.
		// [2] the u_short is to tell the peer what the size of the buffer is.
		//
		// Example: you want to send your peer a 50,000 byte file.
		// So you send() 10,000 bytes at a time. The u_short [1] and [2]
		// will tell the peer to treat the next 10,000 bytes as whatever the flag is set to.
		// In this case the flag would be set to 'file'.
		bytes_read = fread(buf + CR_RESERVED_BUFFER_SPACE, 1, BUF_LEN - CR_RESERVED_BUFFER_SPACE, ReadFile);
		if (bytes_read)
		{
			// Copy the flag into the buf
			buf[0] = CR_FILE;

			// Copy the size of the message into the buf as big endian.
			buf[1] = (char)(bytes_read >> 8);
			buf[2] = (char)(bytes_read);

			// Send the message
			bytes_sent = send(buf, (int)bytes_read + CR_RESERVED_BUFFER_SPACE);
			if (bytes_sent == SOCKET_ERROR)
			{
				std::cout << "send() in sendFileThread() failed. File transfer of: " << file_name << " stopped.\n";
				break;
			}
		}
		else
			bytes_sent = 0;

		total_bytes_sent += bytes_sent + CR_RESERVED_BUFFER_SPACE;

		// 0 means error. If they aren't equal to eachother
		// then it didn't write as much as it read for some reason.
	} while (bytes_read > 0);

	// Please implement sha hash checking here to make sure the file is legit.

	std::cout << "File transfer complete\n";
		
	if (fclose(ReadFile))
		perror("Error closing file designated for reading");

	delete[]buf;
	is_send_file_thread_in_use = false;
	return false;
}

// Server and Client thread must use this function to prevent
// a race condition.
int Connection::setWinnerMutex(int the_winner)
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

// Returns false when everything is fine, and wants to be given more to process.
bool Connection::processRecvBuf(char * recv_buf, long long recv_buf_len, long long received_bytes)
{
	position_in_recv_buf = CR_BEGIN;	// the current cursor position inside the buffer.

	// RecvStateMachine
	while (1)
	{
		switch (process_recv_buf_state)
		{
		case WRITE_FILE_FROM_PEER:
		{
			if (position_in_message < message_size)
			{
				long long amount_to_write = message_size - position_in_message;
				if (amount_to_write > received_bytes - position_in_recv_buf)
					amount_to_write = received_bytes - position_in_recv_buf;

				bytes_written = fwrite(recv_buf + position_in_recv_buf, 1, (size_t)(amount_to_write), WriteFile);
				if (!bytes_written)// uhh this might be a negative number sometimes which makes !bytes_written not work?
				{
					perror("Error while writing the file from peer");

					// close file
					if (fclose(WriteFile))
					{
						perror("Error closing file for writing");
					}

					// delete file here? This could be dangerous, and should
					// require user confirmation. Or check to make sure
					// file didn't exist before trying to open it, then
					// it would be safe to delete.

					process_recv_buf_state = ERROR_STATE;
					break;
				}
				else
				{
					total_bytes_written_to_file += bytes_written;
					position_in_message += bytes_written;
					position_in_recv_buf += bytes_written;

					if (total_bytes_written_to_file >= incoming_file_size_from_peer)
					{
						std::cout << "Finished receiving file from peer.\n";
						std::cout << "Expected " << incoming_file_size_from_peer << ".\n";
						std::cout << "Wrote " << total_bytes_written_to_file << "\n";
						std::cout << "Difference: " << incoming_file_size_from_peer - total_bytes_written_to_file << "\n";
						process_recv_buf_state = CLOSE_FILE_FOR_WRITE;
						is_file_done_being_written = true;
						break;
					}
				}
			}

			if (position_in_recv_buf >= received_bytes)
			{
				return false; // go recv() again to get more bytes
			}
			else if (position_in_message == message_size)// must have a new message from the peer.
			{
				process_recv_buf_state = CHECK_FOR_FLAG;
				break;
			}

			// this shouldn't be reached
			std::cout << "Unrecognized message?\n";
			std::cout << "Unreachable area, switchcase DECIDE_ACTION, recv() loop\n";
			std::cout << "Catastrophic failure.\n";
			process_recv_buf_state = ERROR_STATE;

			break;
		}
		case OUTPUT_CHAT_FROM_PEER:
		{
			// Print out the message to terminal
			std::cout << "\n";
			std::cout << "Peer: ";
			for (; (position_in_recv_buf < received_bytes) && (position_in_message < message_size);
				++position_in_recv_buf, ++position_in_message)
			{
				std::cout << recv_buf[position_in_recv_buf];
			}
			std::cout << "\n";

			if (position_in_recv_buf >= received_bytes)
			{
				if (position_in_message == message_size)// must have a new message from the peer.
					process_recv_buf_state = CHECK_FOR_FLAG;

				return false; // go recv() again to get more bytes
			}
			else if (position_in_message == message_size)// must have a new message from the peer.
			{
				process_recv_buf_state = CHECK_FOR_FLAG;
				break;
			}
			// this shouldn't be reached
			std::cout << "Unreachable area, switchcase DECIDE_ACTION, recv() loop\n";
			std::cout << "Catastrophic failure.\n";
			process_recv_buf_state = ERROR_STATE;
			break;
		}
		case TAKE_FILE_NAME_FROM_PEER:
		{
			// Set the file name variable.
			for (;
				(position_in_recv_buf < received_bytes)
				&& (position_in_message < message_size)
				&& (position_in_message < INCOMING_FILE_NAME_FROM_PEER_SIZE - RESERVED_NULL_CHAR_FOR_FILE_NAME);
				++position_in_recv_buf, ++position_in_message)
			{
				incoming_file_name_from_peer_cstr[position_in_message] = recv_buf[position_in_recv_buf];
			}
			// If the file name was too big, then say so, but don't error.
			if (position_in_message >= INCOMING_FILE_NAME_FROM_PEER_SIZE - RESERVED_NULL_CHAR_FOR_FILE_NAME)
			{
				std::cout << "Receive File: WARNING: Peer's file name is too long. Exceeded " << INCOMING_FILE_NAME_FROM_PEER_SIZE << " characters.\n";
				std::cout << "Receive File: File name will be incorrect on your computer.\n";
			}

			// Null terminate it.
			if (position_in_message <= INCOMING_FILE_NAME_FROM_PEER_SIZE)
			{
				incoming_file_name_from_peer_cstr[position_in_message] = '\0';
			}

			std::string temporary_incoming_file_name_from_peer(incoming_file_name_from_peer_cstr);
			incoming_file_name_from_peer = temporary_incoming_file_name_from_peer;
			std::cout << "Incoming file name: " << temporary_incoming_file_name_from_peer << "\n";


			if (position_in_message == message_size)// must have a new message from the peer.
			{
				process_recv_buf_state = CHECK_FOR_FLAG;
				break;
			}
			// this shouldn't be reached?
			std::cout << "Unreachable area:";
			DBG_DISPLAY_ERROR_LOCATION();
			std::cout << "Catastrophic failure.\n";
			process_recv_buf_state = ERROR_STATE;
			break;
			// Linux:
			// Max file name length is 255 chars on most filesystems, and max path 4096 chars.
			//
			// Windows:
			// Max file name length + subdirectory path is 255 chars. or MAX_PATH .. 260 chars?
			// 1+2+256+1 or [drive][:][path][null] = 260
			// This is not strictly true as the NTFS filesystem supports paths up to 32k characters.
			// You can use the win32 api and "\\?\" prefix the path to use greater than 260 characters. 
			// However using the long path "\\?\" is not a very good idea.
		}
		case TAKE_FILE_SIZE_FROM_PEER:
		{
			// convert the file size in the buffer from network long long to
			// host long long. It assigns the variable incoming_file_size_from_peer
			// a value.
			if (assignFileSizeFromPeer(recv_buf, recv_buf_len, received_bytes) != FINISHED)
			{
				return false;// go recv() again
			}
			else
			{
				std::cout << "Size of Peer's file: " << incoming_file_size_from_peer << "\n";
			}
				
			if (position_in_message == message_size)// must have a new message from the peer.
			{
				process_recv_buf_state = OPEN_FILE_FOR_WRITE;
				break;
			}
			// this shouldn't be reached?
			std::cout << "Unreachable area, switchcase DECIDE_ACTION, recv() loop\n";
			std::cout << "Catastrophic failure.\n";
			process_recv_buf_state = ERROR_STATE;
			break;
		}
		case CHECK_FOR_FLAG:
		{
			// Because if we are here, that means we currently
			// aren't in a message at the moment.
			position_in_message = CR_BEGIN;

			if (position_in_recv_buf >= received_bytes)
			{
				return false;
			}
			else
			{
				type_of_message_flag = (u_char)recv_buf[position_in_recv_buf];
				++position_in_recv_buf;// always have to ++ this in order to access the next element in the array.
				process_recv_buf_state = CHECK_MESSAGE_SIZE_PART_ONE;
				break;
			}
			break;
		}
		case CHECK_MESSAGE_SIZE_PART_ONE:
		{
			// Getting half of the u_short size of the message
			if (position_in_recv_buf >= received_bytes)
			{
				return false;
			}
			else
			{
				message_size_part_one = (u_char)recv_buf[position_in_recv_buf];
				message_size_part_one = message_size_part_one << 8;
				++position_in_recv_buf;
				process_recv_buf_state = CHECK_MESSAGE_SIZE_PART_TWO;
				break;
			}
			break;
		}
		case CHECK_MESSAGE_SIZE_PART_TWO:
		{
			// getting the second half of the u_short size of the message
			if (position_in_recv_buf >= received_bytes)
			{
				return false;
			}
			else
			{
				message_size_part_two = (u_char)recv_buf[position_in_recv_buf];
				message_size = message_size_part_one | message_size_part_two;
				++position_in_recv_buf;

				if (type_of_message_flag == CR_FILE)
					process_recv_buf_state = WRITE_FILE_FROM_PEER;
				else if (type_of_message_flag == CR_FILE_NAME)
					process_recv_buf_state = TAKE_FILE_NAME_FROM_PEER;
				else if (type_of_message_flag == CR_FILE_SIZE)
					process_recv_buf_state = TAKE_FILE_SIZE_FROM_PEER;
				else if (type_of_message_flag == CR_CHAT)
					process_recv_buf_state = OUTPUT_CHAT_FROM_PEER;
				else
				{
					std::cout << "Fatal Error: Unidentified message has been received.\n";
					process_recv_buf_state = ERROR_STATE;
					break;
				}

				if (position_in_recv_buf >= received_bytes)
				{
					return false;
				}
				break;
			}

			break;
		}
		case OPEN_FILE_FOR_WRITE:
		{
			WriteFile = new FILE;
			WriteFile = fopen(incoming_file_name_from_peer.c_str(), "wb");
			if (WriteFile == nullptr)
			{
				perror("Error opening file for writing in binary mode.\n");
			}
				
			is_file_done_being_written = false;
			process_recv_buf_state = CHECK_FOR_FLAG;
				
			break;
		}
		case CLOSE_FILE_FOR_WRITE:
		{
			if (fclose(WriteFile) != 0)		// 0 == successful close
			{
				perror("Error closing file for writing in binary mode.\n");
			}

			// Set it back to default
			total_bytes_written_to_file = 0;

			//is this necessary or does fclose do this?
			WriteFile = nullptr;

			if (position_in_message < message_size)
			{
				DBG_TXT("position_in_message < message_size, closefileforwrite");
			}
			if (position_in_message == message_size)
			{
				DBG_TXT("position_in_message == message_size, everything A OK closefileforwrite");
				std::cout << "File transfer from peer is complete.\n";
			}
			if (position_in_message > message_size)
			{
				std::cout << "ERROR: position_in_message > message_size , closefileforwrite\n";
			}
			process_recv_buf_state = CHECK_FOR_FLAG;
			break;
		}
		case ERROR_STATE:
		{
			std::cout << "Exiting with error, state machine for recv().\n";
			if (fclose(WriteFile) != 0)		// 0 == successful close
			{
				perror("Error closing file for writing in binary mode.\n");
				std::cout << "Error occured in RecvStateMachine, case ERROR_STATE:\n";
			}

			return true;
		}
		default:// currently nothing should cause this to execute.
		{
			std::cout << "State machine for recv() got to default. Exiting.\n";
			return true;
		}
		}//end switch
	}

	return false;
}

// For use with RecvBufStateMachine only.
int Connection::assignFileSizeFromPeer(char * recv_buf, long long recv_buf_len, long long received_bytes)
{
	// The peer is sending the size of his file as a long long.
	// This means we will receive 8 file size fragments.
	if (file_size_fragment == 0)
	{
		incoming_file_size_from_peer = 0;
	}

	while (file_size_fragment < sizeof(long long))
	{
		if (position_in_recv_buf >= received_bytes)
		{
			return RECV_AGAIN;
		}
		incoming_file_size_from_peer |= ((long long)(u_char)recv_buf[position_in_recv_buf]) << (64 - 8 * (file_size_fragment + 1));
		++position_in_recv_buf;
		++position_in_message;
		++file_size_fragment;
	}

	file_size_fragment = 0;

	return FINISHED;
}

bool Connection::intoBufferHostToNetworkLongLong(char * buf, const long long BUF_LEN, long long variable_to_convert)
{
	if (BUF_LEN > (long long)sizeof(variable_to_convert))
	{
		buf[3] = (char)(variable_to_convert >> 56);
		buf[4] = (char)(variable_to_convert >> 48);
		buf[5] = (char)(variable_to_convert >> 40);
		buf[6] = (char)(variable_to_convert >> 32);
		buf[7] = (char)(variable_to_convert >> 24);
		buf[8] = (char)(variable_to_convert >> 16);
		buf[9] = (char)(variable_to_convert >> 8);
		buf[10] = (char)variable_to_convert;
		return false;
	}

	return true;
}

bool Connection::sendFileSize(char * buf, const long long BUF_LEN, long long size_of_file)
{
	char length_of_msg = sizeof(long long);
	// Send the file size to the peer
	// Set flag and size of the message that is being sent.
	if (BUF_LEN > CR_RESERVED_BUFFER_SPACE)
	{
		// Copy the type of message flag into the buf
		buf[0] = CR_FILE_SIZE;
		// Copy the size of the message into the buf as big endian.
		buf[1] = (char)(length_of_msg >> 8);
		buf[2] = (char)length_of_msg;
	}

	// Converting to network byte order, and copying it into
	// the buffer.
	if (intoBufferHostToNetworkLongLong(buf, BUF_LEN - CR_RESERVED_BUFFER_SPACE, size_of_file) == true)
	{
		std::cout << "Programmer error. Buffer length is too small for operation. sendFileThread(), intoBufferHostToNetworkLongLong().\n";
		return true;//exit please?
	}
	int amount_to_send = CR_RESERVED_BUFFER_SPACE + length_of_msg;

	int b = send((char *)buf, amount_to_send);
	if (b == SOCKET_ERROR)
	{
		if (global_verbose == true)
			std::cout << "Exiting sendFileThread()\n";
		return true;
	}
	return false;
}

bool Connection::sendFileName(char * buf, const long long BUF_LEN, const std::string& name_of_file)
{
	// Make sure file name isn't too big
	long long length_of_msg = name_of_file.length();
	if (length_of_msg < 0 || length_of_msg >= 255)
	{
		return true;//exit please
	}
	// Send the file size to the peer
	// Set flag and size of the message that is being sent.
	if (BUF_LEN > CR_RESERVED_BUFFER_SPACE)
	{
		// Copy the type of message flag into the buf
		buf[0] = CR_FILE_NAME;
		// Copy the size of the message into the buf as big endian.
		buf[1] = (char)(length_of_msg >> 8);
		buf[2] = (char)length_of_msg;
	}

	if (BUF_LEN - CR_RESERVED_BUFFER_SPACE > length_of_msg)
	{
		memcpy(buf + CR_RESERVED_BUFFER_SPACE, name_of_file.c_str(), (size_t)length_of_msg);
	}
	else
	{
		std::cout << "Error: BUF_LEN too small for file_name. sendFileThread\n";
		return true;//exit please
	}

	// Converting to network byte order, and copying it into
	// the buffer.
	int amount_to_send = CR_RESERVED_BUFFER_SPACE + (int)length_of_msg;

	int b = send((char *)buf, amount_to_send);
	if (b == SOCKET_ERROR)
	{
		if (global_verbose == true)
			std::cout << "Exiting sendFileThread()\n";
		return true;
	}
	return false;
}

std::string Connection::retrieveFileNameFromPath(std::string name_and_location_of_file)
{
	std::string error_empty_string;
	// Remove the last \ or / in the name if there is one, so that the name
	// of the file can be separated from the path of the file.
	if (name_and_location_of_file.back() == '\\' || name_and_location_of_file.back() == '/')
	{
		perror("ERROR: Found a '\\' or a '/' at the end of the file name.\n");
		std::cout <<"Name and location of file that caused the error: " << name_and_location_of_file << "\n";
		return error_empty_string; //exit please
	}

	const int NO_SLASHES_DETECTED = -1;
	long long last_seen_slash_location = NO_SLASHES_DETECTED;
	long long name_and_location_of_file_length = name_and_location_of_file.length();

	if (name_and_location_of_file_length < INT_MAX - 1)
	{
		// iterate backwords looking for a slash
		for (long long i = name_and_location_of_file_length; i > 0; --i)
		{
			if (name_and_location_of_file[(u_int)i] == '\\' || name_and_location_of_file[(u_int)i] == '/')
			{
				last_seen_slash_location = i;
				break;
			}
		}

		if (last_seen_slash_location == NO_SLASHES_DETECTED)
		{
			perror("File name and location is invalid. There was not a single \'\\\' or \'/\' found.\n");
			return error_empty_string;
		}
		else if (last_seen_slash_location < name_and_location_of_file_length)
		{
			// Copy the file name from name_and_location_of_file to file_name.
			std::string file_name(
				name_and_location_of_file,
				(size_t)last_seen_slash_location + 1,
				(size_t)((last_seen_slash_location + 1) - name_and_location_of_file_length)
			);
			return file_name;
		}
		else
		{
			perror("File name is invalid. Found \'/\' or \'\\\' at the end of the file name.\n");
			return error_empty_string; // exit please
		}
	}
	else
	{
		perror("name_and_location_of_file_length is >= INT_MAX -1");
		return error_empty_string;
	}

	// Shouldn't get here, but if it does, it is an error.
	std::cout << "Impossible location, retrieveFileNameFromPath()\n";
	return error_empty_string;
}

// Returns size of the file.
long long Connection::getFileStatsAndDisplaySize(const char * file_name_and_location)
{
	// Get some statistics on the file, such as size, time created, etc.
#ifdef _WIN32
	struct __stat64 FileStatBuf;
	int err_chk = _stat64(file_name_and_location, &FileStatBuf);
#endif//_WIN32
#ifdef __linux__
	struct stat FileStatBuf;
	int err_chk = stat(file_name_and_location, &FileStatBuf);
#endif//__linux__

	if (err_chk == -1)
	{
		perror("Error getting file statistics");
		return -1;
	}
	else
	{
		// Output to terminal the size of the file in Bytes, KB, MB, GB.
		displayFileSize(file_name_and_location, &FileStatBuf);

		return FileStatBuf.st_size;
	}
}
