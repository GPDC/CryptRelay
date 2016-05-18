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
	// or what? update: i don't think so, i think i remember it saying that it essential calls self-> on everything for you.

#if 0// TEMP SEND AUTO MSG
	DEPRECATED DO NOT USE;
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

	int bytes = 0;	
	while (1)
	{
		
		bytes = recv(global_socket, (char *)recv_buf, recv_buf_len, 0);
		if (bytes > 0)
		{
			// State machine that processes recv_buf and decides what to do
			// based on the information in the buffer.
			if (processRecvBuf(recv_buf, recv_buf_len, bytes) == false)
				break;
		}
		else
		{
			SockStuff.getError(bytes);
			std::cout << "recv() failed. loopedReceiveMessagesThread(). State machine.\n";
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
				std::string file_name_and_loc = split_strings[i + 1];
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
				sendFileThread(copied_file.c_str());
				//exitThread(NULL);
			}
			else // Done looking for arguments as -e was the last thing to check for
			{
				sendFileThread(string.c_str());
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
	int Connection::sendMutex(const char * sendbuf, int amount_to_send)
	{
		// Whatever thread gets here first, will lock
		// the door so that nobody else can come in.
		// That means all other threads will form a queue here,
		// and won't be able to go past this point until the
		// thread that got here first unlocks it.
		m.lock();
		total_amount_sent = amount_to_send;

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

		return total_amount_sent;// returning the amount of bytes sent.
	}

	

	// The user's input is getlined here and checked for things
	// that the user might want to do.
	void Connection::LoopedGetUserInput()
	{
		std::string user_input;

		while (1)
		{
			std::getline(std::cin, user_input);
			if (user_input == "exit()")
			{
				// Exit program.
				return;
			}
			if (doesUserWantToSendAFile(user_input) == true)
			{
				StringManip StrManip;
				std::vector <std::string> split_strings;

				// Split the string into multiple strings for every space.
				if (StrManip.split(user_input, ' ', split_strings) == false)
					std::cout << "Split failed?\n";	// Currently no return false?

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
				std::string file_encryption_option;
				// Determine if user wants to send a file or Encrypt & send a file.
				for (long long i = 0; i < split_strings_size; ++i)
				{
					if (split_strings[i] == "-f" && i < split_strings_size - 1)
					{
						file_name_and_loca = split_strings[i + 1];

						// Fix the user's input to add an escape character to every '\'
						StrManip.duplicateCharacter(file_name_and_loca, '\\');

						// Send the file
						sendFileThread(file_name_and_loca);
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

						// Send the file

						// MAKE THIS THREADED PLEASE
						if (sendFileThread(copied_file_name_and_location) == false)
							return;//exit please?
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
					return;
				}

				long long amount_to_send = CR_RESERVED_BUFFER_SPACE + user_input_length;
				const long long BUF_LEN = 4096;	// try catch see if enough memory available?
				char* buf = new char[BUF_LEN];

				if (BUF_LEN >= CR_RESERVED_BUFFER_SPACE)
				{
					// Copy the type of message flag into the buf
					buf[0] = CR_CHAT;
					// Copy the size of the message into the buf as big endian.
					long long temp1 = user_input_length >> 8;
					buf[1] = (char)temp1;
					long long temp2 = user_input_length;
					buf[2] = (char)temp2;


					// This is the same as ^, but maybe easier to understand? idk.
					//buf[0] = CR_CHAT;	// Message flag
					//// Copy the size of the message into the buf as big endian.
					//u_short msg_sz = htons((u_short)user_input_length);
					//if (BUF_LEN - 1 >= sizeof(msg_sz))
					//	memcpy(buf + 1, &msg_sz, sizeof(msg_sz));


					// Copy the user's message into the buf
					if ((BUF_LEN >= (user_input_length - CR_RESERVED_BUFFER_SPACE))
						&& user_input_length < UINT_MAX)
					{
						memcpy(buf + CR_RESERVED_BUFFER_SPACE, user_input.c_str(), (size_t)user_input_length);
					}
					else
					{
						std::cout << "Message was too big for the send buf.\n";
						return;
					}
					std::cout << "dbg OUTPUT:\n";
					for (long long z = 0; z < amount_to_send; ++z)
					{
						std::cout << z << "_" << (int)buf[z] << "\n";
					}
				}
				else
				{
					"Programmer error. BUF_LEN < 3. loopedGetUserInput()";
					return;
				}
				int b = sendMutex((char *)buf, (int)amount_to_send);
				if (b == SOCKET_ERROR)
				{
					if (global_verbose == true)
						std::cout << "Exiting loopedGetUserInput()\n";
					delete[]buf;
					return;
				}

				delete[]buf;
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

		long long bytes_read;
		long long bytes_written;
		unsigned long long total_bytes_written = 0;

		unsigned long long twenty_five_percent = FileStatBuf.st_size / 4;	// divide it by 4
		unsigned long long fifty_percent = FileStatBuf.st_size / 2;	//divide it by two
		unsigned long long seventy_five_percent = FileStatBuf.st_size - twenty_five_percent;

		bool twenty_five_already_spoke = false;
		bool fifty_already_spoke = false;
		bool seventy_five_already_spoke = false;

		// (8 * 1024) == 8,192 aka 8KB, and 8192 * 1024 == 8,388,608 aka 8MB
		const long long buffer_size = 8 * 1024 * 1024;
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

	bool Connection::sendFileThread(std::string name_and_location_of_file)
	{
		// MAKE THIS A THREADED FUNCTION


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
		ReadFile = fopen(file_name_and_loc.c_str(), "rb");
		if (ReadFile == NULL)
		{
			perror("Error opening file for reading binary");
			return false;
		}

		// Retrieve the name of the file from the string that contains
		// the path and the name of the file.
		std::string file_name = (name_and_location_of_file);
		if (file_name.empty() == true)
		{
			perror("dbg file_name.empty() return true.\n");
			return false; //exit please, file name couldn't be found.
		}
		std::cout << "dbg name of the file: " << file_name << "\n";


		// Send the file name to peer
		if (sendFileName(buf, BUF_LEN, file_name) == false)
		{
			return false; //exit please
		}

		long long size_of_file = 0;
		// Get some statistics on the file, such as size, time created, etc.
		struct stat FileStatBuf;
		int r = stat(file_name_and_loc.c_str(), &FileStatBuf);
		if (r == -1)
		{
			perror("Error getting file statistics");
		}
		else
		{
			// Output to terminal the size of the file in Bytes, KB, MB, GB.
			displayFileSize(file_name_and_loc.c_str(), &FileStatBuf);

			size_of_file = FileStatBuf.st_size;
		}

		// Send file size to peer
		if (sendFileSize(buf, BUF_LEN, size_of_file) == false)
		{
			delete[]buf;
			return false;// exit please
		}


		// Send the file to peer.
		long long bytes_read;
		long long bytes_sent;
		long long total_bytes_sent = 0;

		// We are treating bytes_read as a u_short, instead of size_t
		static_assert(BUF_LEN <= USHRT_MAX,
			"Buffer size must NOT be bigger than USHRT_MAX.");

		std::cout << "Sending file: " << file_name << "\n";
		long long temp1;
		long long temp2;
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
			bytes_read = fread(buf + CR_RESERVED_BUFFER_SPACE, 1, BUF_LEN - CR_RESERVED_BUFFER_SPACE, ReadFile);
			if (bytes_read)
			{
				// Copy the flag into the buf
				buf[0] = CR_FILE;

				// Copy the size of the message into the buf as big endian.
				temp1 = bytes_read >> 8;
				buf[1] = temp1;

				temp2 = bytes_read;
				buf[2] = temp2;

				// Send the message
				bytes_sent = sendMutex(buf, bytes_read);
				if (bytes_sent == SOCKET_ERROR)
				{
					std::cout << "sendMutex() in sendFileThread() failed. File transfer stopped.\n";
					break;
				}
			}
			else
				bytes_sent = 0;

			total_bytes_sent += bytes_sent;

			// 0 means error. If they aren't equal to eachother
			// then it didn't write as much as it read for some reason.
		} while ((bytes_read > 0) && (bytes_read == bytes_sent));


		if (total_bytes_sent == size_of_file)
			std::cout << "File transfer to peer is complete.\n";

		// Please implement sha hash checking here to make sure the file is legit.

		if (bytes_sent)
			perror("Error while transfering the file");

		if (fclose(ReadFile))
			perror("Error closing file designated for reading");

		delete[]buf;

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

	bool Connection::processRecvBuf(char * recv_buf, long long recv_buf_len, long long received_bytes)
	{
		position_in_recv_buf = CR_BEGIN;	// the current cursor position inside the buffer.

		// RecvStateMachin
		while (1)
		{
			switch (process_recv_buf_state)
			{
			case DECIDE_ACTION_BASED_ON_FLAG:
			{
				// If we haven't finished dealing with the user's message, enter here.
				if (position_in_message < message_size)
				{
					if (type_of_message_flag == CR_CHAT)
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
					}
					else if (type_of_message_flag == CR_FILE_NAME)
					{
						// SHOULD PROBABLY CLEAN THE FILE NAME OF INCORRECT SYMBOLS 
						// AND PREVENT PERIODS FROM BEING USED, ETC.

						// Set the file name variable.
						for (;
							(position_in_recv_buf < received_bytes)
							&& (position_in_message < message_size)
							&& (position_in_message < INCOMING_FILE_NAME_FROM_PEER_SIZE - RESERVED_NULL_CHAR_FOR_FILE_NAME);
							++position_in_recv_buf, ++position_in_message
							)
						{
							incoming_file_name_from_peer_cstr[position_in_message] = recv_buf[position_in_recv_buf];
						}
						// If the file name was too big, then say so, but don't error.
						if (position_in_message >= INCOMING_FILE_NAME_FROM_PEER_SIZE - RESERVED_NULL_CHAR_FOR_FILE_NAME)
						{
							std::cout << "Receive File: WARNING: Peer's file name is too long. Exceeded " << INCOMING_FILE_NAME_FROM_PEER_SIZE << " characters.\n";
							std::cout << "Receive File: File name will be incorrect on your computer.\n";
						}

						std::string incoming_file_name_from_peer(incoming_file_name_from_peer_cstr);
						std::cout << "Incoming file name: " << incoming_file_name_from_peer << "\n";

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
					else if (type_of_message_flag == CR_FILE_SIZE)
					{
						// convert the file size in the buffer from network long long to
						// host long long. It assigns the variable incoming_file_size_from_peer
						// a value.
						if (assignFileSizeFromPeer(recv_buf, recv_buf_len, received_bytes) != FINISHED)
						{
							return true;// go recv() again
						}
						else
						{
							std::cout << "Size of Peer's file: " << incoming_file_size_from_peer << "\n";
						}
					}
					else if (type_of_message_flag == CR_FILE)
					{
						// write file
						writePartOfTheFileFromPeer(recv_buf, received_bytes);
						//position_in_recv_buf = received_bytes;
					}
					else
					{
						std::cout << "Fatal Error: Unidentified message has been received.\n";
						process_recv_buf_state = ERROR_STATE;
						break;
					}

					// Time to recv() again if we have reached the end of the byte count in the buffer.
					if (position_in_recv_buf == received_bytes)
					{
						return true;
					}
					// If we have reached the end of the peer's message (not the
					// buffer, and not the amount of bytes received)
					else if (position_in_message == message_size)
					{
						//position_in_message = CR_BEGIN;
						//message_size = CR_SIZE_NOT_ASSIGNED;
						process_recv_buf_state = CHECK_FOR_FLAG;
						break;
					}
				}
				else if (position_in_recv_buf >= received_bytes)
				{
					return true;
				}
				else// must have a new message from the peer. Lets check the flag and size of the message.
				{
					//position_in_message = CR_BEGIN;
					//message_size = CR_SIZE_NOT_ASSIGNED;
					process_recv_buf_state = CHECK_FOR_FLAG;
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
					return true;//process_recv_buf_state = RECEIVE;
				}
				else
				{
					type_of_message_flag = recv_buf[position_in_recv_buf];
					++position_in_recv_buf;// always have to ++ this in order to access the next element in the array.
					process_recv_buf_state = CHECK_MESSAGE_SIZE_PART_ONE;
					break;
				}
			}
			case CHECK_MESSAGE_SIZE_PART_ONE:
			{
				// Getting half of the u_short size of the message
				if (position_in_recv_buf >= received_bytes)
				{
					return true;//process_recv_buf_state = RECEIVE;
				}
				else
				{
					message_size_part_one = recv_buf[position_in_recv_buf];
					message_size_part_one = message_size_part_one << 8;
					++position_in_recv_buf;
					process_recv_buf_state = CHECK_MESSAGE_SIZE_PART_TWO;
					break;
				}
			}
			case CHECK_MESSAGE_SIZE_PART_TWO:
			{
				// getting the second half of the u_short size of the message
				if (position_in_recv_buf >= received_bytes)
				{
					return true;//process_recv_buf_state = RECEIVE;
				}
				else
				{
					message_size_part_two = recv_buf[position_in_recv_buf];
					message_size = message_size_part_one | message_size_part_two;
					//message_size = message_size_part_one | (int8_t)recv_buf[position_in_recv_buf];
					++position_in_recv_buf;
					process_recv_buf_state = DECIDE_ACTION_BASED_ON_FLAG;
					break;

					// [1]				[2]
					// 0000'0000       1111'1110
				}
			}
			case ERROR_STATE:
			{
				std::cout << "Exiting with error, state machine for recv().\n";
				return false;
			}
			default:// currently nothing will cause this to execute.
			{
				std::cout << "State machine for recv() got to default. Exiting.\n";
				return false;
			}
			}//end switch
		}

		return true;
	}

	// This function is only for use with the RecvStateMachine
	bool Connection::writePartOfTheFileFromPeer(char * recv_buf, long long bytes_received)
	{
		// Write the file to disk.
		FILE *WriteFile;

		WriteFile = fopen(incoming_file_name_from_peer.c_str(), "wb");
		if (WriteFile == NULL)
		{
			perror("Error opening file for writing binary");
			return false;
		}

		//int did_file_stat_error = false;
		//struct stat FileStatBuf;
		//// Get some statistics on the file, such as size, time created, etc.
		//int r = stat(read_file_name_and_location, &FileStatBuf);
		//if (r == -1)
		//{
		//	perror("Error getting file statistics");
		//	did_file_stat_error = true;
		//}
		//else
		//{
		//	// Output to terminal the size of the file in Bytes, KB, MB, GB.
		//	displayFileSize(read_file_name_and_location, &FileStatBuf);
		//}



		// Please make a sha hash of the file here so it can be checked with the
		// hash of the copy later.

		long long bytes_read;
		long long bytes_written;
		long long total_bytes_written = 0;

		// put these variables in the header so it won't lose its state.
		// state machine recv()'s , checks flag, then it calls this function.
		// this function writes the recv()'d buffer into the open file.
		// ONLY open file if it hasn't been opened yet.
		// close file once it is done, delete WriteFile;

		
		// recv()
		if (bytes_received)
			bytes_written = fwrite(recv_buf, 1, bytes_received, WriteFile);
		else
			bytes_written = 0;

		total_bytes_written += bytes_written;

		if (total_bytes_written == incoming_file_size_from_peer)
			std::cout << "File copy complete.\n";

		// Please implement sha hash checking here to make sure the file is legit.

		if (bytes_written)
			perror("Error while copying the file");

		if (fclose(WriteFile))
			perror("Error closing file designated for writing");

		//delete[]buf;

		return true;
	}

	// For use with RecvBufStateMachine only.
	int Connection::assignFileSizeFromPeer(char * recv_buf, long long recv_buf_len, long long received_bytes)
	{
		while (1)
		{
			switch (state_ntohll)
			{
			case CHECK_INCOMING_FILE_SIZE_PART_ONE:
			{
				if (position_in_recv_buf >= received_bytes)
				{
					return RECV_AGAIN;//process_recv_buf_state = RECEIVE;
				}
				file_size_part_one = recv_buf[position_in_recv_buf];
				file_size_part_one <<= 56;
				++position_in_recv_buf;
				++position_in_message;
				state_ntohll = CHECK_INCOMING_FILE_SIZE_PART_TWO;
				break;
			}
			case CHECK_INCOMING_FILE_SIZE_PART_TWO:
			{
				if (position_in_recv_buf >= received_bytes)
				{
					return RECV_AGAIN;//process_recv_buf_state = RECEIVE;
				}
				file_size_part_two = recv_buf[position_in_recv_buf];
				file_size_part_two <<= 48;
				++position_in_recv_buf;
				++position_in_message;
				state_ntohll = CHECK_INCOMING_FILE_SIZE_PART_THREE;
				break;
			}
			case CHECK_INCOMING_FILE_SIZE_PART_THREE:
			{
				if (position_in_recv_buf >= received_bytes)
				{
					return RECV_AGAIN;//process_recv_buf_state = RECEIVE;
				}
				file_size_part_three = recv_buf[position_in_recv_buf];
				file_size_part_three <<= 40;
				++position_in_recv_buf;
				++position_in_message;
				state_ntohll = CHECK_INCOMING_FILE_SIZE_PART_FOUR;
				break;
			}
			case CHECK_INCOMING_FILE_SIZE_PART_FOUR:
			{
				if (position_in_recv_buf >= received_bytes)
				{
					return RECV_AGAIN;//process_recv_buf_state = RECEIVE;
				}
				file_size_part_four = recv_buf[position_in_recv_buf];
				file_size_part_four <<= 32;
				++position_in_recv_buf;
				++position_in_message;
				state_ntohll = CHECK_INCOMING_FILE_SIZE_PART_FIVE;
				break;
			}
			case CHECK_INCOMING_FILE_SIZE_PART_FIVE:
			{
				if (position_in_recv_buf >= received_bytes)
				{
					return RECV_AGAIN;//process_recv_buf_state = RECEIVE;
				}
				file_size_part_five = recv_buf[position_in_recv_buf];
				file_size_part_five <<= 24;
				++position_in_recv_buf;
				++position_in_message;
				state_ntohll = CHECK_INCOMING_FILE_SIZE_PART_SIX;
				break;
			}
			case CHECK_INCOMING_FILE_SIZE_PART_SIX:
			{
				if (position_in_recv_buf >= received_bytes)
				{
					return RECV_AGAIN;//process_recv_buf_state = RECEIVE;
				}
				file_size_part_six = recv_buf[position_in_recv_buf];
				file_size_part_six <<= 16;
				++position_in_recv_buf;
				++position_in_message;
				state_ntohll = CHECK_INCOMING_FILE_SIZE_PART_SEVEN;
				break;
			}
			case CHECK_INCOMING_FILE_SIZE_PART_SEVEN:
			{
				if (position_in_recv_buf >= received_bytes)
				{
					return RECV_AGAIN;//process_recv_buf_state = RECEIVE;
				}
				file_size_part_seven = recv_buf[position_in_recv_buf];
				file_size_part_seven <<= 8;
				++position_in_recv_buf;
				++position_in_message;
				state_ntohll = CHECK_INCOMING_FILE_SIZE_PART_EIGHT;
				break;
			}
			case CHECK_INCOMING_FILE_SIZE_PART_EIGHT:
			{
				if (position_in_recv_buf >= received_bytes)
				{
					return RECV_AGAIN;//process_recv_buf_state = RECEIVE;
				}
				file_size_part_eight = recv_buf[position_in_recv_buf];
				++position_in_recv_buf;
				++position_in_message;

				// Now combine all the parts together
				incoming_file_size_from_peer |= file_size_part_one;
				incoming_file_size_from_peer |= file_size_part_two;
				incoming_file_size_from_peer |= file_size_part_three;
				incoming_file_size_from_peer |= file_size_part_four;
				incoming_file_size_from_peer |= file_size_part_five;
				incoming_file_size_from_peer |= file_size_part_six;
				incoming_file_size_from_peer |= file_size_part_seven;
				incoming_file_size_from_peer |= file_size_part_eight;

				// Set it to the default value so when we come by here again it
				// will still work properly.
				state_ntohll = CHECK_INCOMING_FILE_SIZE_PART_ONE;
				return FINISHED;
			}
			}//end switch
		}
	}

	bool Connection::intoBufferHostToNetworkLongLong(char * buf, const long long BUF_LEN, long long variable_to_convert)
	{
		if (BUF_LEN - CR_RESERVED_BUFFER_SPACE > sizeof(variable_to_convert))
		{
			char temp1 = variable_to_convert >> 56;
			buf[3] = temp1;
			char temp2 = variable_to_convert >> 48;
			buf[4] = temp2;
			char temp3 = variable_to_convert >> 40;
			buf[5] = temp3;
			char temp4 = variable_to_convert >> 32;
			buf[6] = temp4;
			char temp5 = variable_to_convert >> 24;
			buf[7] = temp5;
			char temp6 = variable_to_convert >> 16;
			buf[8] = temp6;
			char temp7 = variable_to_convert >> 8;
			buf[9] = temp7;
			char temp8 = variable_to_convert;
			buf[10] = temp8;
			return true;
		}

		return false;
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
			char temp1 = length_of_msg >> 8;
			buf[1] = temp1;
			char temp2 = length_of_msg;
			buf[2] = temp2;
		}

		// Converting to network byte order, and copying it into
		// the buffer.
		if (intoBufferHostToNetworkLongLong(buf, BUF_LEN, size_of_file) == false)
		{
			std::cout << "Programmer error. Buffer length is too small for operation. sendFileThread(), intoBufferHostToNetworkLongLong().\n";
			return false;//exit please?
		}
		int amount_to_send = CR_RESERVED_BUFFER_SPACE + length_of_msg;
		int b = sendMutex((char *)buf, amount_to_send);
		if (b == SOCKET_ERROR)
		{
			if (global_verbose == true)
				std::cout << "Exiting sendFileThread()\n";
			return false;
		}
		return true;
	}

	bool Connection::sendFileName(char * buf, const long long BUF_LEN, std::string name_of_file)
	{
		// Make usre file name isn't too big
		int length_of_msg = name_of_file.length();
		if (length_of_msg < 0 || length_of_msg >= 255)
		{
			return false;//exit please
		}
		// Send the file size to the peer
		// Set flag and size of the message that is being sent.
		if (BUF_LEN > CR_RESERVED_BUFFER_SPACE)
		{
			// Copy the type of message flag into the buf
			buf[0] = CR_FILE_NAME;
			// Copy the size of the message into the buf as big endian.
			char temp1 = length_of_msg >> 8;
			buf[1] = temp1;
			char temp2 = length_of_msg;
			buf[2] = temp2;
		}

		// Converting to network byte order, and copying it into
		// the buffer.
		int amount_to_send = CR_RESERVED_BUFFER_SPACE + length_of_msg;
		int b = sendMutex((char *)buf, amount_to_send);
		if (b == SOCKET_ERROR)
		{
			if (global_verbose == true)
				std::cout << "Exiting sendFileThread()\n";
			return false;
		}
		return true;
	}

	std::string Connection::returnFileNameFromFileNameAndPath(std::string name_and_location_of_file)
	{
		std::string error_empty_string;
		// Remove the last \ or / in the name if there is one, so that the name
		// of the file can be separated from the path of the file.
		if (name_and_location_of_file.back() == '\\' || name_and_location_of_file.back() == '/')
		{
			perror("ERROR: Found a '\\' or a '/' at the end of the file name.\n");
			std::cout << "dbg name and location of file: " << name_and_location_of_file << "\n";
			return error_empty_string; //exit please
		}

		const int NO_SLASHES_DETECTED = -1;
		int last_seen_slash_location = NO_SLASHES_DETECTED;
		long long name_and_location_of_file_length = name_and_location_of_file.length();
		std::string file_name;

		if (name_and_location_of_file_length < INT_MAX - 1)
		{
			// iterate backwords looking for a slash
			for (int i = name_and_location_of_file_length; i > 0; --i)
			{
				if (name_and_location_of_file[i] == '\\' || name_and_location_of_file[i] == '/')
				{
					last_seen_slash_location = i;
					break;
				}
			}

			if (last_seen_slash_location == NO_SLASHES_DETECTED)
			{
				perror("File name and location is invalid. There was not a single '\\' or '/' found.\n");
				return error_empty_string;
			}
			else if (last_seen_slash_location < name_and_location_of_file_length)
			{
				// Copy the file name from name_and_location_of_file to file_name.
				memcpy(
					&file_name,
					&name_and_location_of_file + last_seen_slash_location,
					last_seen_slash_location - name_and_location_of_file_length
				);
				return file_name;
			}
			else
			{
				perror("File name is invalid. Found '/' or '\\' at the end of the file name.\n");
				return error_empty_string; // exit please
			}
		}
		else
		{
			perror("name_and_location_of_file_length is >= INT_MAX -1");
			return error_empty_string;
		}

		// Shouldn't get here, but if it does, it is an error.
		perror("Impossible location,returnFileNameFromFileNameAndPath()\n");
		return error_empty_string;
	}