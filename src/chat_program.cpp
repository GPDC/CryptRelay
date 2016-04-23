// chat_program.cpp
#ifdef __linux__
#include <iostream>		// cout
#include <string>
#include <string.h> //memset
#include <pthread.h>	// <process.h>  multithreading

#include "chat_program.h"
#include "GlobalTypeHeader.h"
#endif //__linux__

#ifdef _WIN32
#include <iostream>		// cout
#include <process.h>	// <pthread.h> multithreading
#include <Winsock2.h>
#include <WS2tcpip.h>

#include "chat_program.h"
#include "GlobalTypeHeader.h"
#endif //_WIN32


#ifdef __linux__
#define INVALID_SOCKET	((SOCKET)(~0))	// To indicate INVALID_SOCKET, Linux returns (~0) from socket functions, and windows returns -1.
#define SOCKET_ERROR	(-1)			// I belive this was just because linux didn't already have a SOCKET_ERROR macro.
#define THREAD_RETURN_VALUE return NULL
#endif // __linux__

#ifdef __linux__
	pthread_t ChatProgram::thread0 = 0;	// Server
	pthread_t ChatProgram::thread1 = 0;	// Client
	pthread_t ChatProgram::thread2 = 0;	// Send()
	int ChatProgram::ret0 = 0;	// Server
	int ChatProgram::ret1 = 0;	// Client
        int ChatProgram::ret2 = 0;	// Send()
#endif //__linux__

#ifdef _WIN32
// These are needed for Windows sockets
#pragma comment(lib, "Ws2_32.lib") //tell the linker that Ws2_32.lib file is needed.
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

// HANDLE storage for threads
HANDLE ChatProgram::ghEvents[2]{};	// i should be using vector of ghevents[] instead...
									// [0] == server
									// [1] == client

// for the loopedSendMessagesThread()
HANDLE ChatProgram::ghEventsSend[1];// [0] == send()
#endif//_WIN32

// If something errors in the server thread, sometimes we
// might want to do something with that information.
// 0 == no error, 0 == no function was given.
int server_thread_error_code = 0;
int function_that_errored = 0;

// this default_port being static might not be a good idea.
// This is the default port for the ChatProgram as long as
// ChatProgram isn't given any port from the UPnP class,
// which has its own default port, or given any port from the
// command line.
const std::string ChatProgram::default_port = "30248";

// Variables necessary for determining who won the connection race
const int ChatProgram::SERVER_WON = -29;
const int ChatProgram::CLIENT_WON = -30;
const int ChatProgram::NOBODY_WON = -25;
int ChatProgram::global_winner = NOBODY_WON;

// Used by threads after a race winner has been established
SOCKET ChatProgram::global_socket;



ChatProgram::ChatProgram()
{
	result = nullptr;
}
ChatProgram::~ChatProgram()
{
	// Giving this a try, shutdown the connection even if ctrl-c is hit?
	SockStuff.myShutdown(global_socket, SD_BOTH);

	// ****IMPORTANT****
	// All addrinfo structures must be freed once they are done being used.
	// Making sure we never freeaddrinfo twice. Ugly bugs otherwise.
	// Check comments in the myFreeAddrInfo() to see how its done.
	if (result != nullptr)
		SockStuff.myFreeAddrInfo(result);
}


// This is where the ChatProgram class receives information about IPs and ports.
// /*optional*/ target_port         default value will be assumed
// /*optional*/ my_internal_port    default value will be assumed
void ChatProgram::giveIPandPort(std::string target_extrnl_ip_address, std::string my_ext_ip, std::string my_internal_ip, std::string target_port, std::string my_internal_port)
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
void ChatProgram::createStartServerThread(void * instance)
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
	uintptr_t thread_handle = _beginthread(startServerThread, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
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
void* ChatProgram::posixStartServerThread(void * instance)
{
	if(instance != nullptr)
		startServerThread(instance);
	return nullptr;
}

void ChatProgram::startServerThread(void * instance)
{
	ChatProgram * self = (ChatProgram*)instance;

	if (instance == nullptr)
	{
		std::cout << "startServerThread() thread instance NULL\n";
		self->exitThread(nullptr);
	}

	memset(&self->hints, 0, sizeof(self->hints));
	// These are the settings for the connection
	self->hints.ai_family = AF_INET;		//ipv4
	self->hints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	self->hints.ai_flags = AI_PASSIVE;		// Let anyone connect, not just a specific IP address
	self->hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	// Place target ip and port, and hints about the connection type into a linked list named addrinfo *result
	// Now we use result instead of hints.
	// Remember we are only listening as the server, so put in local IP:port
	if (self->SockStuff.myGetAddrInfo(self->my_local_ip, self->my_local_port, &self->hints, &self->result) == false)
		self->exitThread(nullptr);

	// Create socket
	SOCKET listen_socket = self->SockStuff.mySocket(self->result->ai_family, self->result->ai_socktype, self->result->ai_protocol);
	if (listen_socket == INVALID_SOCKET)
		self->exitThread(nullptr);
	else if (listen_socket == SOCKET_ERROR)
		self->exitThread(nullptr);

	// Assign the socket to an address:port
	// Binding the socket to the user's local address
	if (self->SockStuff.myBind(listen_socket, self->result->ai_addr, self->result->ai_addrlen) == false)
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
			global_socket = accepted_socket;
			global_winner = SERVER_WON;

			if (global_verbose == true)
				std::cout << "dbg closing socket after retrieving new one from accept()\n";
			// Not using this socket anymore since we created a new socket after accept() ing the connection.
			self->SockStuff.myCloseSocket(listen_socket);

			break;
		}
	}

	// Display who the user has connected to.
	self->coutPeerIPAndPort(global_socket);

	// Looped checking for user input and sending it.
	createLoopedSendMessagesThread(instance);

	// Receive incoming messages (not threaded)
	self->loopedReceiveMessages();


	// WAIT HERE FOR LOOPED SENDMESSAGES THREAD
	// this is so we can exit smoothly

	self->exitThread(nullptr);
}

void ChatProgram::createStartClientThread(void * instance)
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
	uintptr_t thread_handle = _beginthread(startClientThread, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
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
void* ChatProgram::posixStartClientThread(void * instance)
{
	if (instance != nullptr)
		startClientThread(instance);
	return nullptr;
}

void ChatProgram::startClientThread(void * instance)
{
    	ChatProgram* self = (ChatProgram*)instance;
	if (instance == nullptr)
	{
		std::cout << "startClientThread() thread instance NULL\n";
		self->exitThread(nullptr);
	}


	// These are the settings for the connection
	memset(&self->hints, 0, sizeof(self->hints));
	self->hints.ai_family = AF_INET;		//ipv4
	self->hints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	self->hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	// Place target ip and port, and hints about the connection type into a linked list named addrinfo *result
	// Now we use result instead of hints.
	if (self->SockStuff.myGetAddrInfo(self->target_external_ip, self->target_external_port, &self->hints, &self->result) == false)
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
		SOCKET s = self->SockStuff.mySocket(self->result->ai_family, self->result->ai_socktype, self->result->ai_protocol);
		if (s == INVALID_SOCKET)
		{
			std::cout << "Closing client thread due to INVALID_SOCKET.\n";
			self->exitThread(nullptr);
		}

		// Attempt to connect to target
		r = self->SockStuff.myConnect(s, self->result->ai_addr, self->result->ai_addrlen);
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
			global_socket = s;
			global_winner = CLIENT_WON;
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

	// Send messages inputted by user until there is an error or connection is closed.
	createLoopedSendMessagesThread(instance);

	// Receive messages as until there is an error or connection is closed.
	self->loopedReceiveMessages(/*remote_host*/);


	// WAIT HERE FOR LOOPED SENDMESSAGES THREAD
	// this is so we can exit smoothly


	// Exiting chat program
	self->exitThread(nullptr);
}

// remote_host is /* optional */    default == "Peer".
// remote_host is the IP that we are connected to.
// To find out who we were connected to, use getnameinfo()
int ChatProgram::loopedReceiveMessages(const char* remote_host)
{
	
#if 1// TEMP SEND AUTO MSG
	const char* sendbuf = nullptr;
	std::string message_to_send = "This is an automated message from my receive loop.\n";

	//send this message once
	sendbuf = message_to_send.c_str();
	int wombocombo = send(global_socket, sendbuf, (int)strlen(sendbuf), 0);
	if (wombocombo == SOCKET_ERROR)
	{
		SockStuff.getError(wombocombo);
		std::cout << "send failed.\n";
		SockStuff.myCloseSocket(global_socket);
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
	static const int recv_buf_len = 512;
	char recv_buf[recv_buf_len];

	int bytes;
	do
	{
		bytes = recv(global_socket, recv_buf, recv_buf_len, 0);
		if (bytes > 0)
		{
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
			std::cout << "\n";
			std::cout << remote_host << ": ";
			for (int i = 0; i < bytes; ++i)
			{
				std::cout << recv_buf[i];
			}
			std::cout << "\n";

			if (global_verbose == true)
				std::cout << "dbg Bytes recvd: " << bytes << "\n";
		}
		else if (bytes == 0)
		{
			printf("Connection closing...\n");
			return EXIT_SUCCESS;
		}
		else
		{
			SockStuff.getError(bytes);
			std::cout << "recv failed.\n";
			SockStuff.myCloseSocket(global_socket);
			return EXIT_FAILURE;
		}
	} while (bytes > 0);

	// Should ever get here, but if it did, then the connection was closed normally.
	std::cout << "receiveMessage() impossible area.\n";	// In case I messed something up, i'll know!
	return EXIT_SUCCESS;
}


void ChatProgram::createLoopedSendMessagesThread(void * instance)
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
	uintptr_t thread_handle = _beginthread(loopedSendMessagesThread, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
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

// This function exists because threads on linux have to return a void*.
// Conversely on windows it doesn't return anything because threads return void.
void* ChatProgram::posixLoopedSendMessagesThread(void * instance)
{
	if (instance != nullptr)
		loopedSendMessagesThread(instance);
	return nullptr;
}

void ChatProgram::loopedSendMessagesThread(void * instance)
{
	ChatProgram* self = (ChatProgram*)instance;

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
		std::cout << "You: "<< send_buf << "\n";
		if (global_verbose == true)
			std::cout << "dbg Bytes Sent: " << bytes << "\n";
	}

	self->exitThread(nullptr);
}

// Output to console the the peer's IP and port that you have connected to the peer with.
void ChatProgram::coutPeerIPAndPort(SOCKET s)
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
    void ChatProgram::exitThread(void* ptr)
    {
#ifdef _WIN32
        _endthread();
#endif//_WIN32
#ifdef __linux
       pthread_exit(ptr); // Not wanting to return anything, just exit
#endif//__linux__
    }
