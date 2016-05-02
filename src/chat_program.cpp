// chat_program.cpp
#ifdef __linux__
#include <iostream>		// cout
#include <string>
#include <string.h> //memset
#include <pthread.h>	// <process.h>  multithreading
#include <vector>
#include <mutex>

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
#include <mutex>		// btw, need to use std::lock_guard if you want to be able to use exceptions and avoid having it never reach the unlock.

#include "chat_program.h"
#include "GlobalTypeHeader.h"
#include "string_manipulation.h"
#include "UPnP.h"
#endif //_WIN32

std::mutex m;

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

// for the loopedSendChatMessagesThread()
HANDLE Connection::ghEventsSend[1];// [0] == send()
#endif//_WIN32

// If something errors in the server thread, sometimes we
// might want to do something with that information.
// 0 == no error, 0 == no function was given.
int server_thread_error_code = 0;
int function_that_errored = 0;

// this default_port being static might not be a good idea.
// This is the default port for the Connection as long as
// Connection isn't given any port from the UPnP class,
// which has its own default port, or given any port from the
// command line.
const std::string Connection::default_port = "30248";

// Variables necessary for determining who won the connection race
const int Connection::SERVER_WON = -29;
const int Connection::CLIENT_WON = -30;
const int Connection::NOBODY_WON = -25;
int Connection::global_winner = NOBODY_WON;

// Used by threads after a race winner has been established
SOCKET Connection::global_socket;


// Flags for sendMutex() that indicated what the message is being used for.
const int8_t Connection::CR_NO_FLAG = 0;
const int8_t Connection::CR_BEGIN = 0;
const int8_t Connection::CR_SIZE_NOT_ASSIGNED = 0;
const int8_t Connection::CR_CHAT = 1;
const int8_t Connection::CR_ENCRYPTED_CHAT_MESSAGE = 2;
const int8_t Connection::CR_FILE = 30;
const int8_t Connection::CR_ENCRYPTED_FILE = 31;



// How many characters at the beginning of the buffer that should be
// reserved for usage of a flag, and size of the "packet"
const int8_t Connection::CR_RESERVED_BUFFER_SPACE = 3;


Connection::Connection()
{
	ConnectionInfo = nullptr;
}
Connection::~Connection()
{
	// Giving this a try, shutdown the connection even if ctrl-c is hit?
	SockStuff.myShutdown(global_socket, SD_BOTH);



	// ****IMPORTANT****
	// All addrinfo structures must be freed once they are done being used.
	// Making sure we never freeaddrinfo twice. Ugly bugs otherwise.
	// Check comments in the myFreeAddrInfo() to see how its done.
	if (ConnectionInfo != nullptr)
		SockStuff.myFreeAddrInfo(ConnectionInfo);
}


// This is where the Connection class receives information about IPs and ports.
// /*optional*/ target_port         default value will be assumed
// /*optional*/ my_internal_port    default value will be assumed
void Connection::giveIPandPort(std::string target_extrnl_ip_address, std::string my_ext_ip, std::string my_internal_ip, std::string target_port, std::string my_internal_port)
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
	uintptr_t thread_handle = _beginthread(serverThread, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
	ghEvents[0] = (HANDLE)thread_handle;	// i should be using vector of ghEvents instead
	if (thread_handle == -1L)
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
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
	Connection * self = (Connection*)instance;

	if (instance == nullptr)
	{
		std::cout << "startServerThread() thread instance NULL\n";
		self->exitThread(nullptr);
	}

	memset(&self->Hints, 0, sizeof(self->Hints));
	// These are the settings for the connection
	self->Hints.ai_family = AF_INET;		//ipv4
	self->Hints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	//self->Hints.ai_flags = AI_PASSIVE;
	self->Hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	// Place target ip and port, and Hints about the connection type into a linked list named addrinfo *ConnectionInfo
	// Now we use ConnectionInfo instead of Hints.
	// Remember we are only listening as the server, so put in local IP:port
	if (self->SockStuff.myGetAddrInfo(self->my_local_ip, self->my_local_port, &self->Hints, &self->ConnectionInfo) == false)
		self->exitThread(nullptr);

	// Create socket
	SOCKET listen_socket = self->SockStuff.mySocket(self->ConnectionInfo->ai_family, self->ConnectionInfo->ai_socktype, self->ConnectionInfo->ai_protocol);
	if (listen_socket == INVALID_SOCKET)
		self->exitThread(nullptr);
	else if (listen_socket == SOCKET_ERROR)
		self->exitThread(nullptr);

	// Assign the socket to an address:port
	// Binding the socket to the user's local address
	if (self->SockStuff.myBind(listen_socket, self->ConnectionInfo->ai_addr, self->ConnectionInfo->ai_addrlen) == false)
		self->exitThread(nullptr);

	// Set the socket to listen for incoming connections
	if (self->SockStuff.myListen(listen_socket) == false)
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
		std::cout << "dbg Listen thread active...\n";
		// Putting the socket into the array so select() can check it for readability
		// Format is:  FD_SET(int fd, fd_set *ReadSet);
		// Please use the macros FD_SET, FD_CHECK, FD_CLEAR, when dealing with struct fd_set
		FD_ZERO(&ReadSet);
		FD_SET(listen_socket, &ReadSet);
		TimeValue.tv_sec = 2;	// This has to be in this loop! Linux resets this value every time select() times out.
		// select() returns the number of handles that are ready and contained in the fd_set structure
		errchk = select(listen_socket + 1, &ReadSet, NULL, NULL, &TimeValue);

		if (errchk == SOCKET_ERROR)
		{
			self->SockStuff.getError(errchk);
			std::cout << "startServerThread() select Error.\n";
			self->SockStuff.myCloseSocket(listen_socket);
			std::cout << "Closing listening socket b/c of error. Ending Server Thread.\n";
			self->exitThread(nullptr);
		}
		else if (global_winner == CLIENT_WON)
		{
			self->SockStuff.myCloseSocket(listen_socket);
			if (global_verbose == true)
			{
				std::cout << "Closed listening socket, because the winner is: " << global_winner << ". Ending Server thread.\n";
			}
			self->exitThread(nullptr);
		}
		else if (errchk > 0)	// select() told us that atleast 1 readable socket has appeared!
		{
			if (global_verbose == true)
				std::cout << "attempting to accept a client now that select() returned a readable socket\n";

			SOCKET accepted_socket;
			// Accept the connection and create a new socket to communicate on.
			accepted_socket = self->SockStuff.myAccept(listen_socket);
			if (accepted_socket == INVALID_SOCKET)
				self->exitThread(nullptr);
			if (global_verbose == true)
				std::cout << "accept() succeeded. Setting global_winner and global_socket\n";

			// Assigning global values to let the client thread know it should stop trying.
			if (self->setWinnerMutex(SERVER_WON) != SERVER_WON)
			{
				if (global_verbose == true)
				{
					std::cout << "Server: Extremely rare race condition was almost reached.";
					std::cout << "It was prevented using a mutex. The client is the real winner.\n";
				}
				self->SockStuff.myCloseSocket(listen_socket);
				self->SockStuff.myCloseSocket(accepted_socket);
				self->exitThread(nullptr);
			}
			else
				global_socket = accepted_socket;


			if (global_verbose == true)
				std::cout << "dbg closing socket after retrieving new one from accept()\n";
			// Not using this socket anymore since we created a new socket after accept() ing the connection.
			self->SockStuff.myCloseSocket(listen_socket);

			break;
		}
	}

	// Display who the user has connected to.
	self->coutPeerIPAndPort(global_socket);

	// Receive messages until there is an error or connection is closed.
	// Pattern is:  rcv_thread(function, point to class that function is in (aka, this), function argument)
	std::thread rcv_thread(&Connection::loopedReceiveMessagesThread, self, instance);

	// Get the user's input from the terminal, and check
	// to see if the user wants to do something special,
	// else just send the message that the user typed out.
	self->LoopedGetUserInput();

	// wait here until x thread finishes.
	if (rcv_thread.joinable())
		rcv_thread.join();

	// Done communicating with peer. Proceeding to exit.
	self->SockStuff.myShutdown(global_socket, SD_BOTH);	// SD_BOTH == shutdown both send and receive on the socket.
	self->SockStuff.myCloseSocket(global_socket);

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
	ghEvents[1] = (HANDLE)thread_handle;	// i should be using vector of ghEvents instead
	if (thread_handle == -1L)
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
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
    	Connection* self = (Connection*)instance;
	if (instance == nullptr)
	{
		std::cout << "startClientThread() thread instance NULL\n";
		self->exitThread(nullptr);
	}


	// These are the settings for the connection
	memset(&self->Hints, 0, sizeof(self->Hints));
	self->Hints.ai_family = AF_INET;		//ipv4
	self->Hints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	self->Hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	// Place target ip and port, and Hints about the connection type into a linked list named addrinfo *ConnectionInfo
	// Now we use ConnectionInfo instead of Hints.
	if (self->SockStuff.myGetAddrInfo(self->target_external_ip, self->target_external_port, &self->Hints, &self->ConnectionInfo) == false)
		self->exitThread(nullptr);
	
	std::cout << "Attempting to connect...\n";

	// Connect to the target on the given socket
	// while checking to see if the server hasn't already connected to someone.
	// if connected_socket == INVALID_SOCKET   that just means a connection couldn't
	// be established yet.
	//
	// A more normal method would be: try each address until we successfully bind(2).
    // If socket(2) (or bind(2)) fails, we close the socket and try
	// the next address in the list.
	int TIMEOUT_ERROR = -10060;
	int r;
	while (1)
	{
		std::cout << "dbg client thread active...\n";
		// Check to see if server has connected to someone
		if (global_winner == SERVER_WON)
		{
			if (global_verbose == true)
			{
				std::cout << "Server won. Exiting thread.\n";
			}
			self->exitThread(nullptr);
		}

		// Create socket
		SOCKET s = self->SockStuff.mySocket(self->ConnectionInfo->ai_family, self->ConnectionInfo->ai_socktype, self->ConnectionInfo->ai_protocol);
		if (s == INVALID_SOCKET)
		{
			std::cout << "Closing client thread due to INVALID_SOCKET.\n";
			self->exitThread(nullptr);
		}

		// Attempt to connect to target
		r = self->SockStuff.myConnect(s, self->ConnectionInfo->ai_addr, self->ConnectionInfo->ai_addrlen);
		if (r == SOCKET_ERROR)
		{
			std::cout << "Closing client thread due to error.\n";
			self->exitThread(nullptr);
		}
		else if (r == TIMEOUT_ERROR)	// No real errors, just can't connect yet
		{
			std::cout << "dbg not real error, timeout client connect\n";
			self->SockStuff.myCloseSocket(s);
			continue;
		}
		else if (r == 0)				// Must have succeeded in connecting
		{
			// Assigning global values to let the server thread know it should stop trying.
			if (self->setWinnerMutex(CLIENT_WON) != CLIENT_WON)
			{
				if (global_verbose == true)
				{
					std::cout << "Client: Extremely rare race condition was almost reached.";
					std::cout << "It was prevented using a mutex. The server is the real winner.\n";
				}
				self->SockStuff.myCloseSocket(s);
				self->exitThread(nullptr);
			}
			else
				global_socket = s;

			break;
		}
		else
		{
			std::cout << "Unkown ERROR. connect()\n";
			self->exitThread(nullptr);
		}
	}

	// Display who the user is connected with.
	self->coutPeerIPAndPort(global_socket);




	// Receive messages until there is an error or connection is closed.
	// Pattern is:  rcv_thread(function, point to class that function is in (aka, this), function argument)
	std::thread rcv_thread(&Connection::loopedReceiveMessagesThread, self, instance);

	// Get the user's input from the terminal, and check
	// to see if the user wants to do something special,
	// else just send the message that the user typed out.
	self->LoopedGetUserInput();

	// wait here until x thread finishes.
	if (rcv_thread.joinable())
		rcv_thread.join();

	// Done communicating with peer. Proceeding to exit.
	self->SockStuff.myShutdown(global_socket, SD_BOTH);	// SD_BOTH == shutdown both send and receive on the socket.
	self->SockStuff.myCloseSocket(global_socket);

	// Exiting chat program
	self->exitThread(nullptr);
}

// remote_host is /* optional */    default == "Peer".
// remote_host is the IP that we are connected to.
// To find out who we were connected to, use getnameinfo()
void Connection::loopedReceiveMessagesThread(void * instance)
{
	// so is this needed with std::thread ???
	if (instance == nullptr)
	{
		std::cout << "Instance was null. loopedReceiveMessagesThread()\n";
		return;
	}

	Connection* self = (Connection*)instance;
	// or what?

#if 0// TEMP SEND AUTO MSG
	const char* sendbuf = nullptr;
	std::string message_to_send = "This is an automated message from my receive loop.\n";

	//send this message once
	sendbuf = message_to_send.c_str();
	int b = send(global_socket, sendbuf, (int)strlen(sendbuf), 0);
	if (b == SOCKET_ERROR)
	{
		self->SockStuff.getError(b);
		std::cout << "send failed.\n";
		self->SockStuff.myCloseSocket(global_socket);
		//return;
	}
	//else
	//{
	//	std::cout << "dbg Message sent: " << sendbuf << "\n";
	//	std::cout << "dbg Bytes Sent: " << wombocombo << "\n";
	//}

#endif//1 TEMP SEND AUTO MSG
	




	// Receive until the peer shuts down the connection
	if (global_verbose == true)
		std::cout << "Recv loop started...\n";

	// Buffer for receiving messages
	static const size_t RECV_BUF_LEN = 512;
	char recv_buf[RECV_BUF_LEN];

	const int DONE_READING_BYTES_FROM_LAST_RECV = 0;

	int bytes = 0;
	size_t position_in_message = 0;

	size_t message_size = CR_SIZE_NOT_ASSIGNED;		// peer told us this size
	size_t message_size_part_one = CR_SIZE_NOT_ASSIGNED;

	size_t position_in_recv_buf = RECV_BUF_LEN;	// the current cursor position inside the buffer.
	uint8_t type_of_message_flag = CR_NO_FLAG;
	while (1)
	{
		if (position_in_recv_buf >= bytes)
		{
			bytes = recv(global_socket, recv_buf, RECV_BUF_LEN, 0);
			position_in_recv_buf = CR_BEGIN;
		}
		// If successfully received something
		if (bytes > 0)
		{
			// Set the cursor in the buffer to the start position.
			//position_in_recv_buf = CR_BEGIN;

			// If the position_in_message >= message_size, then either we have finished
			// displaying or writing the peer's message, or we don't know the message_size yet.
			// If we don't know the message_size yet that would be like trying to compare: something < 0
			// which means if we don't know message_size yet, lets find out (go to else)
			if (position_in_message < message_size)
			{
				// if true, print out the message to terminal
				if (type_of_message_flag == CR_CHAT)
				{
					// Print out the message to terminal
					std::cout << "\n";
					std::cout << "Peer: ";
					for (; (position_in_recv_buf < (u_int)bytes) && (position_in_message < message_size); ++position_in_recv_buf, ++position_in_message)
					{
						std::cout << recv_buf[position_in_recv_buf];
						//++position_in_recv_buf; // plus one the recv_buf cursor
						//++position_in_message;  // position_in_message out of message_size messages have been printed out.
					}
					std::cout << "\n";
				}
				else if (type_of_message_flag == CR_FILE)
				{
					// Write the file to disk.

				}
				else
				{
					std::cout << "Fatal Error: Unidentified message has been received.\n";
					return;
				}

				
				// If we have reached the end of the peer's message (not the
				// buffer, and not the amount of bytes received)
				if (position_in_message == message_size)
				{
					// Resetting all the variables that are relevant to the peer's message
					// Notably leaving variables for recv_buf alone.
					message_size = CR_SIZE_NOT_ASSIGNED;// reset the incoming size because there is no current size.
					position_in_message = CR_BEGIN;
					type_of_message_flag = CR_NO_FLAG;
					message_size_part_one = CR_SIZE_NOT_ASSIGNED;
					//bytes = DONE_READING_BYTES_FROM_LAST_RECV;// THIS IS WRONG
					// read the first 3 bytes again to determine flags

					if (position_in_recv_buf >= bytes)
						continue; //recv() again
					else if (type_of_message_flag == CR_NO_FLAG) // safe to access first element of recv_buf
					{
						type_of_message_flag = (int8_t)recv_buf[position_in_recv_buf];
						//++position_in_message; // because this is still considered part of the peer's new message
						++position_in_recv_buf;// always have to ++ this in order to access the next element in the array.
					}

					// Getting half of the u_short size of the message
					if (position_in_recv_buf >= bytes)
						continue;
					else if (message_size_part_one == CR_SIZE_NOT_ASSIGNED)
					{
						message_size_part_one = (int8_t)recv_buf[position_in_recv_buf];
						message_size_part_one = message_size_part_one << 8;
						//++position_in_message;
						++position_in_recv_buf;
					}

					// getting the second half of the u_short size of the message
					if (position_in_recv_buf >= bytes)
						continue;
					else if (message_size == CR_SIZE_NOT_ASSIGNED)
					{
						message_size = message_size_part_one | (int8_t)recv_buf[2];
						//++position_in_message;
						++position_in_recv_buf;
					}
				}

				// continue the while loop, recv() again.
			}

			/**************** Get flags and message size ******************/
			// messag_b_count must be the same size as message_size. Therefore we
			// have completed printing out the peer's message, and now need to look
			// for the flag and size of the peer's next incoming message.
			else
			{
				if (position_in_recv_buf >= RECV_BUF_LEN)
					continue; //recv() again
				else if (type_of_message_flag == CR_NO_FLAG) // safe to access first element of recv_buf
				{
					type_of_message_flag = (int8_t)recv_buf[position_in_recv_buf];
					//++position_in_message; // because this is still considered part of the peer's new message
					++position_in_recv_buf;// always have to ++ this in order to access the next element in the array.
				}

				// Getting half of the u_short size of the message
				if (position_in_recv_buf >= RECV_BUF_LEN)
					continue;
				else if (message_size_part_one == CR_SIZE_NOT_ASSIGNED)
				{
					message_size_part_one = (int8_t)recv_buf[position_in_recv_buf];
					message_size_part_one = message_size_part_one << 8;
					//++position_in_message;
					++position_in_recv_buf;
				}

				// getting the second half of the u_short size of the message
				if (position_in_recv_buf >= RECV_BUF_LEN)
					continue;
				else if (message_size == CR_SIZE_NOT_ASSIGNED)
				{
					message_size = message_size_part_one | (int8_t)recv_buf[2];
					//++position_in_message;
					++position_in_recv_buf;
				}
			}
		}
		else
		{
			std::cout << "Exiting.\n";
			break;
		}
	}

	return;

	/*
	~~~~~~~~~~~~~~~~This is a possible idea for fixing only having 1 mitten~~~~~~~~~~~~~~~~
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;	//console screen buffer info
	COORD CursorCoordinatesStruct;
	ZeroMemory(&CursorCoordinatesStruct, sizeof(CursorCoordinatesStruct));

	if (GetConsoleScreenBufferInfo(hStdout, &csbiInfo) == 0)	//0,0 is top left of console
	{
	getError();
	std::cout << "GetConsoleScreenBufferInfo failed.\n";
	}

	CursorCoordinatesStruct.X = csbiInfo.dwCursorPosition.X;
	CursorCoordinatesStruct.Y = csbiInfo.dwCursorPosition.Y;

	//only 299 or 300 lines in the Y position?
	SetConsoleCursorPosition(hStdout, CursorCoordinatesStruct);
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	*/
}

// DEPRECATED
void Connection::createLoopedSendChatMessagesThread(void * instance)
{
	if (global_verbose == true)
		std::cout << "Starting client thread.\n";
#ifdef __linux__
	ret2 = pthread_create(&thread2, NULL, posixLoopedSendMessagesThread, instance);
	if (ret2)
	{
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret2);
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
	uintptr_t thread_handle = _beginthread(loopedSendChatMessagesThread, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
	ghEventsSend[0] = (HANDLE)thread_handle;	// i should be using vector of ghEvents instead
	if (thread_handle == -1L)
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		return;
	}

	// previous way of doing it:
	/*
	ghEvents[2] = (HANDLE)_beginthread(startClientThread, 0, instance);		//c style typecast    from: uintptr_t    to: HANDLE.
	if (ghEvents[2] == (HANDLE)(-1L) )
	{
	int errsv = errno;
	std::cout << "_beginthread() error: " << errsv << "\n";
	return;
	}
	*/

#endif//_WIN32
}

// DEPRECATED
// This function exists because threads on linux have to return a void*.
// Conversely on windows it doesn't return anything because threads return void.
void* Connection::posixLoopedSendChatMessagesThread(void * instance)
{
	if (instance != nullptr)
		loopedSendChatMessagesThread(instance);
	return nullptr;
}

// DEPRECATED
void Connection::loopedSendChatMessagesThread(void * instance)
{
	Connection* self = (Connection*)instance;

	if (instance == nullptr)
	{
		std::cout << "Instance was NULL.\n";
		self->exitThread(nullptr);
	}

	int bytes;
	std::string message_to_send = "Nothing.\n";

#if 1	/* TEMP SEND AUTO MSG **********************/
	message_to_send = "This is an automated message from my send loop.\n";
	const char* send_buf = "";

	//send this message once
	send_buf = message_to_send.c_str();
	int wombocombo = send(global_socket, send_buf, (int)strlen(send_buf), 0);
	if (wombocombo == SOCKET_ERROR)
	{
		self->SockStuff.getError(wombocombo);
		std::cout << "send failed.\n";
		self->SockStuff.myCloseSocket(global_socket);
		//return;
	}
	/*std::cout << "dbg Message sent: " << send_buf << "\n";
	if (global_verbose == true)
		std::cout << "dbg Bytes Sent: " << wombocombo << "\n";*/

#endif	/* TEMP SEND AUTO MSG *********************/




	//ask & send the user's input the rest of the time.
	while (1)
	{
		// Get user's input
		std::getline(std::cin, message_to_send);

		// Do some checks to see if the user wants to xfer a file or exit
		if (self->doesUserWantToSendAFile(message_to_send) == true)
		{
			// start send file process.
		}

		// Put user's input into the send buffer
		send_buf = message_to_send.c_str();
		// Send it
		bytes = send(global_socket, send_buf, (int)strlen(send_buf), 0);
		if (bytes == SOCKET_ERROR)
		{
			self->SockStuff.getError(bytes);
			std::cout << "send failed.\n";
			self->SockStuff.myCloseSocket(global_socket);
			break;
		}
		std::cout << "Me: "<< send_buf << "\n";
		if (global_verbose == true)
			std::cout << "dbg Bytes Sent: " << bytes << "\n";
	}

	self->exitThread(nullptr);
}

// Output to console the the peer's IP and port that you have connected to the peer with.
void Connection::coutPeerIPAndPort(SOCKET s)
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
	int errchk = getpeername(s, &PeerIPAndPortStorage, &peer_ip_and_port_storage_len);
	if (errchk == -1)
	{
		SockStuff.getError(errchk);
		std::cout << "getpeername() failed.\n";
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
		SockStuff.getError(errchk);
		std::cout << "getnameinfo() failed.\n";
		// still going to continue the program, this isn't a big deal
	}
	else
		std::cout << "\n\n\n\n\n\nConnection established with: " << remote_host << ":" << remote_hosts_port << "\n\n\n";
}

// Cross platform windows and linux thread exiting
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

	// DEPRECATED?
	void Connection::readAndSendTheFileThread(std::string string)
	{
		// Please make a sha hash of the file here so it can be checked with the
		// hash of the copy later.

		//StringManip StrManip;



		/*
		std::vector <std::string> split_strings;

		// Split the string into multiple strings for every space.
		if (StrManip.split(string, ' ', split_strings) == false)
			std::cout << "Split failed?\n";

		// get the size of the array
		size_t split_strings_size = split_strings.size();

		// There should be two strings in the vector.
		// [0] should be -f
		// [1] should be the file name
		if (split_strings_size < 2)
		{
			std::cout << "Error, too few arguments supplied to -f.\n";
			return;
		}

		std::cout << "NOTICE: You will not be able to send chat messages while the file is being transfered.\n";

		



		// Modifying the the user's input here.
		// string is now the file name that will be transfered.
		string = StrManip.duplicateCharacter(split_strings[1], '\\');

		// 0 means no flag set.
		std::string encryption_flag = 0;
		for (u_int i = 0; i < split_strings_size; ++i)
		{
			if (split_strings[i] == "-f" && i < split_strings_size - 1)
			{
				std::string file_name = split_strings[i + 1];
				++i;
			}

			// Does the user want to encrypt it?
			else if (split_strings[i] == "-e")
			{
				// making a guess to see if the user wants to supply an
				// argument for -e or not.
				// Obviously not if the next element is a '-'.
				if (i < split_strings_size - 1)
				{
					if (split_strings[i + 1][0] != '-')
					{
						encryption_flag = split_strings[i + 1];
						++i;
					}
				}

				// Copy the file so we can encrypt the copied version.
				// We don't want to encrypt the user's only copy of the file.
				// Adding .enc as a file extension. This should be changed to
				// a more appropriate one at some point.
				std::string copied_file = string + ".enc";
				if (copyFile(string.c_str(), copied_file.c_str()) == false)
				{
					//handle error
					std::cout << "Failed to make a copy of the file before encrypting it.\n";
					//exitThread(NULL);
				}

				// encryptFile(copied_file.c_str(), asynchronous?, RSA?)
				sendFile(copied_file.c_str());
				//exitThread(NULL);
			}
			else // Done looking for arguments as -e was the last thing to check for
			{
				sendFile(string.c_str());
				//exitThread(NULL);
			}
		}
		*/


	}
	


	// NEW SECTION***********************

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
	int Connection::sendMutex(const char * sendbuf, size_t amount_to_send, int flag)
	{
		// Whatever thread gets here first, will lock
		// the door so that nobody else can come in.
		// That means all other threads will form a queue here,
		// and won't be able to go past this point until the
		// thread that got here first unlocks it.
		m.lock();

		do
		{
			bytes_sent = send(global_socket, sendbuf, amount_to_send, NULL);
			if (bytes_sent == SOCKET_ERROR)
			{
				SockStuff.getError(bytes_sent);
				std::cout << "ERROR: send() failed.\n";
				SockStuff.myCloseSocket(global_socket);
				m.unlock();
				return SOCKET_ERROR;
			}

			amount_to_send -= bytes_sent;

		} while (amount_to_send > 0);

		// Unlock the door now that the thread is done with this function.
		m.unlock();

		return bytes_sent;
	}

	

	// The user's input is getlined here and checked for things
	// that the user might want to do.
	void Connection::LoopedGetUserInput()
	{
		std::string user_input;

		while (user_input != "exit()")
		{
			std::getline(std::cin, user_input);
			if (doesUserWantToSendAFile(user_input) == true)
			{
				StringManip StrManip;
				std::vector <std::string> split_strings;

				// Split the string into multiple strings for every space.
				if (StrManip.split(user_input, ' ', split_strings) == false)
					std::cout << "Split failed?\n";	// Currently no return false?

				// get the size of the array
				size_t split_strings_size = split_strings.size();

				// There should be two strings in the vector.
				// [0] should be -f
				// [1] should be the file name
				if (split_strings_size < 2)
				{
					std::cout << "Error, not enough arguments given.\n";
				}

				std::string file_name_and_loca;
				std::string file_encryption_option;
				// Determine if user wants to send a file or Encrypt & send a file.
				for (size_t i = 0; i < split_strings_size; ++i)
				{
					if (split_strings[i] == "-f" && i < split_strings_size - 1)
					{
						file_name_and_loca = split_strings[i + 1];

						// Fix the user's input to add an escape character to every '\'
						StrManip.duplicateCharacter(file_name_and_loca, '\\');

						break;
					}
					else if (split_strings[i] == "-fE" && i < split_strings_size -2)
					{
						file_name_and_loca = split_strings[i + 1];
						file_encryption_option = split_strings[i + 2];

						// Fix the user's input to add an escape character to every '\'
						std::string copied_file_name_and_location = StrManip.duplicateCharacter(file_name_and_loca, '\\');
						copied_file_name_and_location += ".enc";

						// Copy the file before encrypting it.
						copyFile(file_name_and_loca.c_str(), copied_file_name_and_location.c_str());

						// Encrypt the copied file.
						// beep boop encryption(file_encryption_option);

						break;
					}
				}
			
				// Send the file
				sendFile(file_name_and_loca.c_str());
			}


			else // Continue doing normal chat operation.
			{
				// User input can't exceed USHRT_MAX b/c that is the maximum size
				// that is able to be sent to the peer. (artifically limited).
				size_t user_input_length = user_input.length();
				if (user_input_length > USHRT_MAX)
				{
					std::cout << "User input exceeded " << USHRT_MAX << ". Exiting\n";
					return;
				}
				const size_t buf_len = CR_RESERVED_BUFFER_SPACE + user_input_length ;
				char* buf = new char[buf_len];// this is quite intensive, maybe have an already created one?

				if (buf_len >= 3)
				{
					//// Copy the size of the message into the buf as big endian.
					//buf[0] = CR_CHAT;								// Message flag
					//buf[1] = (char)(user_input_length >> 8);		// size of the message we are sending
					//buf[2] = (char)(user_input_length << 8) & 0xff;	// size of the message we are sending

					// This is the same as ^, but maybe easier to understand? idk.

					buf[0] = CR_CHAT;								// Message flag
					// Copy the size of the message into the buf as big endian.
					u_short msg_sz = (u_short)htons(user_input_length);
					size_t msg_sz_size = sizeof(msg_sz);
					memcpy(buf + 1, &msg_sz, msg_sz_size);

					

					if (buf_len >= (user_input_length - CR_RESERVED_BUFFER_SPACE))
						memcpy(buf + CR_RESERVED_BUFFER_SPACE, user_input.c_str(), user_input_length);
					std::cout << "DEBUG OUTPUT:\n";
					for (u_int z = 0; z < buf_len; ++z)
					{
						std::cout << z << "_" << +buf[z] << "\n";
					}
				}
				else
				{
					"Programmer error. buf_len < 3. loopedGetUserInput()";
					return;
				}
				int b = sendMutex(buf, buf_len, CR_CHAT);
				if (b == SOCKET_ERROR)
				{
					if (global_verbose == true)
						std::cout << "Exiting loopedGetUserInput()\n";
					delete buf;
					return;
				}

				delete buf;
			}
		}

		return;
	}



	// have 1 thread to send stuff over the network //this encompasses chat and file messages
	// have 1 thread to getline() user input and send that to the network thread
	// have 1 thread for sending file data to the network thread
	// there will be a vector in the networkThread that will store all messages.
	// if you want to send a message or file, put that message or file into vector.pushback();
	// networkThread will send(vector[i]) and increment after sending each message... but
	// problem with packet size. and gotta delete the message you just sent, from the vector.
	// lockless queue with thread safety

	// getline() thread```````````
	// while(1)
	// {
	// getline(cin, getline_msg);
	// if (doesUserWantToSendFile() == true)
	//	    createthread(sendfilethread);
	// else
	// {
	//		networkThread_msg_to_send = getline_msg;
	//		++networkThread_message_counter;
	// }


	// networkThread```````````````````
	// bool should_i_send_the_message;
	// std::string message_to_send;
	// char * sendbuf;
	// while(1)
	// {
	//		if(should_i_send_the_message == true)
	//		{
	//			sendbuf = message_to_send.c_str();
	//			send(sendbuf);
	//			should_i_send_the_message = false;
	//		}	



	// Requires an a stat structure that was already filled out by stat();
	// might need to include these on linux?:
	//<sys/types.h>
	//<sys/stat.h>
	//<unistd.h>

	bool Connection::displayFileSize(const char* file_name_and_location, struct stat * FileStatBuf)
	{
		if (FileStatBuf == NULL)
		{
			std::cout << "displayFileSize() failed. NULL pointer.\n";
			return false;
		}
		// Shifting the bits over by 10. This divides it by 2^10 aka 1024
		off_t KB = FileStatBuf->st_size >> 10;
		off_t MB = KB >> 10;
		off_t GB = MB >> 10;
		std::cout << "Displaying file size as Bytes: " << FileStatBuf->st_size << "\n";
		std::cout << "As KB: " << KB << "\n";
		std::cout << "As MB: " << MB << "\n";
		std::cout << "As GB: " << GB << "\n";

		return true;
	}

	bool Connection::copyFile(const char * read_file_name_and_location, const char * write_file_name_and_location)
	{

		FILE *ReadFile;
		FILE *WriteFile;

		ReadFile = fopen(read_file_name_and_location, "rb");
		if (ReadFile == NULL)
		{
			perror("Error opening file for reading binary");
			return false;
		}
		WriteFile = fopen(write_file_name_and_location, "wb");
		if (WriteFile == NULL)
		{
			perror("Error opening file for writing binary");
			return false;
		}

		int did_file_stat_error = false;
		struct stat FileStatBuf;

		// Get some statistics on the file, such as size, time created, etc.
		int r = stat(read_file_name_and_location, &FileStatBuf);
		if (r == -1)
		{
			perror("Error getting file statistics");
			did_file_stat_error = true;
		}
		else
		{
			// Output to terminal the size of the file in Bytes, KB, MB, GB.
			displayFileSize(read_file_name_and_location, &FileStatBuf);
		}



		// Please make a sha hash of the file here so it can be checked with the
		// hash of the copy later.

		size_t bytes_read;
		size_t bytes_written;
		unsigned long long total_bytes_written = 0;

		unsigned long long twenty_five_percent = FileStatBuf.st_size / 4;	// divide it by 4
		unsigned long long fifty_percent = FileStatBuf.st_size / 2;	//divide it by two
		unsigned long long seventy_five_percent = FileStatBuf.st_size - twenty_five_percent;

		bool twenty_five_already_spoke = false;
		bool fifty_already_spoke = false;
		bool seventy_five_already_spoke = false;

		// (8 * 1024) == 8,192 aka 8KB, and 8192 * 1024 == 8,388,608 aka 8MB
		const size_t buffer_size = 8 * 1024 * 1024;
		unsigned char* buffer = new unsigned char[buffer_size];

		std::cout << "Copying file...\n";
		do
		{
			bytes_read = fread(buffer, 1, buffer_size, ReadFile);
			if (bytes_read)
				bytes_written = fwrite(buffer, 1, bytes_read, WriteFile);
			else
				bytes_written = 0;

			total_bytes_written += bytes_written;

			// If we have some statistics to work with, then display
			// ghetto progress indicators.
			if (did_file_stat_error == false)
			{
				if (total_bytes_written > twenty_five_percent && total_bytes_written < fifty_percent && twenty_five_already_spoke == false)
				{
					std::cout << "File copy 25% complete.\n";
					twenty_five_already_spoke = true;
				}
				else if (total_bytes_written > fifty_percent && total_bytes_written < seventy_five_percent && fifty_already_spoke == false)
				{
					std::cout << "File copy 50% complete.\n";
					fifty_already_spoke = true;
				}
				else if (total_bytes_written > seventy_five_percent && seventy_five_already_spoke == false)
				{
					std::cout << "File copy 75% complete.\n";
					seventy_five_already_spoke = true;
				}
			}


			// 0 means error. If they aren't equal to eachother
			// then it didn't write as much as it read for some reason.
		} while ((bytes_read > 0) && (bytes_read == bytes_written));

		if (total_bytes_written == FileStatBuf.st_size)
			std::cout << "File copy complete.\n";

		// Please implement sha hash checking here to make sure the file is legit.

		if (bytes_written)
			perror("Error while copying the file");



		if (fclose(WriteFile))
			perror("Error closing file designated for writing");
		if (fclose(ReadFile))
			perror("Error closing file designated for reading");

		delete[]buffer;

		return true;

		//// Give a compiler error if streamsize > sizeof(long long)
		//// This is so we can safely do this: (long long) streamsize
		//static_assert(sizeof(std::streamsize) <= sizeof(long long), "myERROR: streamsize > sizeof(long long)");
	}

	bool Connection::sendFile(const char * file_name)
	{

		// sha checking?


		// Open the file
		// split_strings[0] should be "-f"
		// split_strings[1] should be the file name, and location.
		FILE *ReadFile;
		ReadFile = fopen(file_name, "rb");
		if (ReadFile == NULL)
		{
			perror("Error opening file for reading binary");
			return false;
		}


		//int did_file_stat_error = false;
		struct stat FileStatBuf;

		// Get some statistics on the file, such as size, time created, etc.
		int r = stat(file_name, &FileStatBuf);
		if (r == -1)
		{
			perror("Error getting file statistics");
			//did_file_stat_error = true;
		}
		else
		{
			// Output to terminal the size of the file in Bytes, KB, MB, GB.
			displayFileSize(file_name, &FileStatBuf);
		}




		size_t bytes_read;
		size_t bytes_sent;
		unsigned long long total_bytes_sent = 0;


		// (8 * 1024) == 8,192 aka 8KB, and 8192 * 1024 == 8,388,608 aka 8MB
		// Buffer size must NOT be bigger than 65,536 bytes (u_short).
		const size_t buffer_size = USHRT_MAX;
		char* buffer = new char[buffer_size];

		// We are treating bytes_read as a u_short, instead of size_t
		static_assert(buffer_size <= USHRT_MAX,
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
			// So you sendMutex() 10,000 bytes at a time. The u_short [1] and [2]
			// will tell the peer to treat the next 10,000 bytes as whatever the flag is set to.
			// In this case the flag would be set to 'file'.
			bytes_read = fread(buffer + CR_RESERVED_BUFFER_SPACE, 1, buffer_size - CR_RESERVED_BUFFER_SPACE, ReadFile);
			if (bytes_read)
			{
				buffer[0] = CR_FILE;
				// Converting bytes_read to Big endian and placing it in [1] and [2]
				// so that the person we are sending to will know the size of our "packet"
				// so to speak.
				buffer[1] = (char)(bytes_read >> 8);
				buffer[2] = (bytes_read << 8) & 0xff;

				bytes_sent = sendMutex(buffer, bytes_read, CR_FILE);
				if (bytes_sent == SOCKET_ERROR)
				{
					std::cout << "sendMutex() in sendFile() failed. File transfer stopped.\n";
					break;
				}
			}
			else
				bytes_sent = 0;

			total_bytes_sent += bytes_sent;

			// 0 means error. If they aren't equal to eachother
			// then it didn't write as much as it read for some reason.
		} while ((bytes_read > 0) && (bytes_read == bytes_sent));


		if (total_bytes_sent == FileStatBuf.st_size)
			std::cout << "File transfer to peer is complete.\n";

		// Please implement sha hash checking here to make sure the file is legit.

		if (bytes_sent)
			perror("Error while transfering the file");

		if (fclose(ReadFile))
			perror("Error closing file designated for reading");

		delete[]buffer;

		return true;
	}

	// Server and Client thread must use this function to prevent
	// a race condition.
	int Connection::setWinnerMutex(int the_winner)
	{
		m.lock();

		// If nobody has won the race yet, then set the winner.
		if (global_winner == NOBODY_WON)
		{
			global_winner = the_winner;
		}

		m.unlock();
		return global_winner;
	}