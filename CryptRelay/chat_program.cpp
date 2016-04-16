// chat_program.cpp
#ifdef __linux__
#include <iostream>		// cout
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
#endif // __linux__

#ifdef _WIN32
// These are needed for Windows sockets
#pragma comment(lib, "Ws2_32.lib") //tell the linker that Ws2_32.lib file is needed.
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

// HANDLE storage for threads
HANDLE ChatProgram::ghEvents[2]{};

HANDLE ChatProgram::ghEventsSend[1];
#endif//_WIN32


// this default_port being static isn't good i think
const std::string ChatProgram::default_port = "30001";
bool ChatProgram::fatal_thread_error = false;

// Variables necessary for determining who won the connection race
const int ChatProgram::SERVER_WON = -29;
const int ChatProgram::CLIENT_WON = -30;
const int ChatProgram::NOBODY_WON = -25;
int ChatProgram::global_winner = NOBODY_WON;

// Used by threads after a race winner has been established
SOCKET ChatProgram::global_socket;



ChatProgram::ChatProgram()
{

}
ChatProgram::~ChatProgram()
{
	// ****IMPORTANT****
	// All addrinfo structures must be freed once they are done being used.
	// Making sure we never freeaddrinfo twice. Ugly bugs other wise.
	// Check comments in the myFreeAddrInfo() to see how its done.
	//if (result != nullptr)
	//	SockStuff.myFreeAddrInfo(result);
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
	if (empty(target_extrnl_ip_address) == false)
		target_external_ip = target_extrnl_ip_address;
	if (empty(target_port) == false)
		target_external_port = target_port;
	if (empty(my_ext_ip) == false)
		my_external_ip = my_ext_ip;
	if (empty(my_internal_ip) == false)
		my_local_ip = my_internal_ip;
	if (empty(my_internal_port) == false)
		my_local_port = my_internal_port;
}

// Thread entrance for startServerThread();
void ChatProgram::createStartServerThread(void * instance)
{
	if (global_verbose == true)
		std::cout << "Starting server thread.\n";
#ifdef __linux__
	ret0 = pthread_create(&thread0, NULL, startServerThread, instance);
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
		fatal_thread_error = true;	// what should i do with this hmm
		return;
	}

	// previous way of doing it:
	/*
	ghEvents[0] = (HANDLE)_beginthread(startServerThread, 0, instance);		//c style typecast    from: uintptr_t    to: HANDLE.
	if (ghEvents[0] == (HANDLE)(-1L) )
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		fatal_thread_error = true;
		return;
	} 
	*/

#endif//_WIN32
}

void ChatProgram::startServerThread(void * instance)
{
	if (instance == NULL)
	{
		std::cout << "startServerThread() thread instance NULL\n";
		return;
	}
	ChatProgram * self = (ChatProgram*)instance;

	memset(&self->hints, 0, sizeof(self->hints));
	// These are the settings for the connection
	self->hints.ai_family = AF_INET;			//ipv4
	self->hints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	self->hints.ai_flags = AI_PASSIVE;		// Let anyone connect, not just a specific IP address
	self->hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	// Place target ip and port, and hints about the connection type into a linked list named addrinfo *result
	// Now we use result instead of hints.
	// Remember we are only listening as the server, so put in local IP:port
	if (self->SockStuff.myGetAddrInfo(self->my_local_ip, self->my_local_port, &self->hints, &self->result) == false)
		_endthread();

	// Create socket
	SOCKET listen_socket = self->SockStuff.mySocket(self->result->ai_family, self->result->ai_socktype, self->result->ai_protocol);
	if (listen_socket == INVALID_SOCKET)
		_endthread();
	else if (listen_socket == SOCKET_ERROR)
		_endthread();

	// Assign the socket to an address:port

	// Binding the socket to the user's local address
	if (self->SockStuff.myBind(listen_socket, self->result->ai_addr, self->result->ai_addrlen) == false)
		_endthread();

	// Set the socket to listen for incoming connections
	if (self->SockStuff.myListen(listen_socket) == false)
		_endthread();


	// Needed to provide a time to select()
	timeval TimeValue;
	memset(&TimeValue, 0, sizeof(TimeValue));

	// Used by select(). It makes it wait for x amount of time to wait for a readable socket to appear.
	TimeValue.tv_usec = 500'000; // 1 million microseconds = 1 second

	// This struct contains the array of sockets that select() will check for readability
	fd_set FdSet;
	memset(&FdSet, 0, sizeof(FdSet));

	// loop check for incoming connections
	int errchk;
	while (1)
	{
		std::cout << "dbg Listen thread active...\n";
		// Putting the socket into the array so select() can check it for readability
		// Format is:  FD_SET(int fd, fd_set *FdSet);
		// Please use the macros FD_SET, FD_CHECK, FD_CLEAR, when dealing with struct fd_set
		FD_SET(listen_socket, &FdSet);

		// select() returns the number of handles that are ready and contained in the fd_set structure
		errchk = select(listen_socket + 1, &FdSet, NULL, NULL, &TimeValue);

		if (errchk == SOCKET_ERROR)
		{
			self->SockStuff.getError(errchk);
			std::cout << "startServerThread() select Error.\n";
			self->SockStuff.myCloseSocket(listen_socket);
			std::cout << "Closing listening socket b/c of error. Ending Server Thread.\n";
			_endthread();
		}
		else if (global_winner == CLIENT_WON)
		{
			self->SockStuff.myCloseSocket(listen_socket);
			if (global_verbose == true)
			{
				std::cout << "Closed listening socket, because the winner is: " << global_winner << ". Ending Server thread.\n";
			}
			_endthread();
		}
		else if (errchk > 0)	// select() told us that atleast 1 readable socket has appeared!
		{
			if (global_verbose == true)
				std::cout << "attempting to accept a client now that select() returned a readable socket\n";

			SOCKET accepted_socket;
			// Accept the connection and create a new socket to communicate on.
			accepted_socket = self->SockStuff.myAccept(listen_socket);
			if (accepted_socket == INVALID_SOCKET)
				_endthread();
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

	self->coutPeerIPAndPort(global_socket);

	// Looped checking for user input and sending it.
	createThreadedLoopedSendMessages(instance);

	// Receive incoming messages (not threaded)
	self->loopedReceiveMessages();


	// WAIT HERE FOR LOOPED SENDMESSAGES THREAD
	// this is so we can exit smoothly

	_endthread();
}

void ChatProgram::createStartClientThread(void * instance)
{
	if (global_verbose == true)
		std::cout << "Starting client thread.\n";
#ifdef __linux__
	ret1 = pthread_create(&thread1, NULL, startClientThread, instance);
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
		fatal_thread_error = true;	// what should i do with this hmm
		return;
	}

	// previous way of doing it:
	/*
	ghEvents[1] = (HANDLE)_beginthread(startClientThread, 0, instance);		//c style typecast    from: uintptr_t    to: HANDLE.
	if (ghEvents[1] == (HANDLE)(-1L) )
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		fatal_thread_error = true;
		return;
	} 
	*/

#endif//_WIN32
}

void ChatProgram::startClientThread(void * instance)
{
	if (instance == NULL)
	{
		std::cout << "startClientThread() thread instance NULL\n";
		_endthread();
	}
	ChatProgram* self = (ChatProgram*)instance;

	// These are the settings for the connection
	memset(&self->hints, 0, sizeof(self->hints));
	self->hints.ai_family = AF_INET;		//ipv4
	self->hints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	self->hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	// Place target ip and port, and hints about the connection type into a linked list named addrinfo *result
	// Now we use result instead of hints.
	if (self->SockStuff.myGetAddrInfo(self->target_external_ip, self->target_external_port, &self->hints, &self->result) == false)
		_endthread();
	
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
	int r = INVALID_SOCKET;
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
			_endthread();
		}

		// Create socket
		SOCKET s = self->SockStuff.mySocket(self->result->ai_family, self->result->ai_socktype, self->result->ai_protocol);
		if (s == INVALID_SOCKET)
		{
			std::cout << "Closing client thread due to INVALID_SOCKET.\n";
			_endthread();
		}

		// Attempt to connect to target
		r = self->SockStuff.myConnect(s, self->result->ai_addr, self->result->ai_addrlen);
		if (r == SOCKET_ERROR)
		{
			std::cout << "Closing client thread due to error.\n";
			_endthread();
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
			_endthread();
		}
	}

	// Display who the user is connected with.
	self->coutPeerIPAndPort(global_socket);


#if 0 // easy uncomment block
	// Find out who you are connected to.
	sockaddr PeerIPAndPortStorage;
	int peer_ip_and_port_storage_len = sizeof(sockaddr);
	memset(&PeerIPAndPortStorage, 0, peer_ip_and_port_storage_len);

	// getting the peer's ip and port info and placing it into the PeerIPAndPortStorage sockaddr structure
	int errchk = getpeername(global_socket, &PeerIPAndPortStorage, &peer_ip_and_port_storage_len);
	if (errchk == -1)
	{
		self->SockStuff.getError(errchk);
		std::cout << "getpeername() failed.\n";
		// continuing, b/c this isn't a big problem.
	}



	// If we are here, we must be connected to someone.
	char remote_host[NI_MAXHOST];
	char remote_hosts_port[NI_MAXSERV];

	// Let us see the IP:Port we are connecting to. the flag NI_NUMERICSERV
	// will make it return the port instead of the service name.
	errchk = getnameinfo(&PeerIPAndPortStorage, sizeof(sockaddr), remote_host, NI_MAXHOST, remote_hosts_port, NI_MAXSERV, NI_NUMERICSERV);
	if (errchk != 0)
	{
		self->SockStuff.getError(errchk);
		std::cout << "getnameinfo() failed.\n";
		// still going to continue the program, this isn't a big deal
	}
	else
		std::cout << "Connection established with: " << remote_host << ":" << remote_hosts_port << "\n";

#endif //0 or 1
	



	// Send messages inputted by user until there is an error or connection is closed.
	createThreadedLoopedSendMessages(instance);

	// Receive messages as until there is an error or connection is closed.
	self->loopedReceiveMessages(/*remote_host*/);


	// WAIT HERE FOR LOOPED SENDMESSAGES THREAD
	// this is so we can exit smoothly


	// Exiting chat program
	_endthread();
}

// remote_host is /* optional */    default == "Peer".
// remote_host is the IP that we are connected to.
// To find out who we were connected to, use getnameinfo()
int ChatProgram::loopedReceiveMessages(const char* remote_host)
{
	/* TEMP SEND AUTO MSG **********************/

	const char* sendbuf = "First message sent.";
	std::string message_to_send = "Automated message sent from recv func.\n";

	//send this message once
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	int wombocombo = send(global_socket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
	if (wombocombo == SOCKET_ERROR)
	{
		SockStuff.getError(wombocombo);
		std::cout << "send failed.\n";
		SockStuff.myCloseSocket(global_socket);
		//myWSACleanup();
		//return false;
	}
	else
	{
		std::cout << "Message sent: " << sendbuf << "\n";
		std::cout << "Bytes Sent: " << wombocombo << "\n";
	}




	/* TEMP SEND AUTO MSG *********************/




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
				printf("Bytes recvd: %d\n", bytes);
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
	std::cout << "receiveMessage() impossible area.\n";	// Incase I messed something up, i'll know!
	return EXIT_SUCCESS;
}


void ChatProgram::createThreadedLoopedSendMessages(void * instance)
{
	if (global_verbose == true)
		std::cout << "Starting client thread.\n";
#ifdef __linux__
	ret2 = pthread_create(&thread2, NULL, threadedLoopedSendMessages, instance);
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
	uintptr_t thread_handle = _beginthread(threadedLoopedSendMessages, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
	ghEventsSend[0] = (HANDLE)thread_handle;	// i should be using vector of ghEvents instead
	if (thread_handle == -1L)
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		fatal_thread_error = true;	// what should i do with this hmm
		return;
	}

	// previous way of doing it:
	/*
	ghEvents[2] = (HANDLE)_beginthread(startClientThread, 0, instance);		//c style typecast    from: uintptr_t    to: HANDLE.
	if (ghEvents[2] == (HANDLE)(-1L) )
	{
	int errsv = errno;
	std::cout << "_beginthread() error: " << errsv << "\n";
	fatal_thread_error = true;
	return;
	}
	*/

#endif//_WIN32
}

void ChatProgram::threadedLoopedSendMessages(void * instance)
{
	if (instance == NULL)
	{
		std::cout << "Instance was NULL.\n";
		return;
	}
	ChatProgram* self = (ChatProgram*)instance;
	int bytes;
	std::string message_to_send = "first sendmsg thread\n";	// User's input is put in here
	const char* send_buf = "";



	/* TEMP SEND AUTO MSG **********************/


	//send this message once
	send_buf = message_to_send.c_str();	//c_str converts from string to char *
	int wombocombo = send(global_socket, send_buf, (int)strlen(send_buf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
	if (wombocombo == SOCKET_ERROR)
	{
		self->SockStuff.getError(wombocombo);
		std::cout << "send failed.\n";
		self->SockStuff.myCloseSocket(global_socket);
		//myWSACleanup();
		//return false;
	}
	std::cout << "Message sent: " << send_buf << "\n";
	if (global_verbose == true)
		std::cout << "Bytes Sent: " << wombocombo << "\n";



	/* TEMP SEND AUTO MSG *********************/




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
			std::cout << "Bytes Sent: " << bytes << "\n";
	}

	_endthread();
}

// Output to console the the peer's IP and port that you have connected to the peer with.
void ChatProgram::coutPeerIPAndPort(SOCKET s)
{
	sockaddr PeerIPAndPortStorage;
	int peer_ip_and_port_storage_len = sizeof(sockaddr);
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
		std::cout << "Connection established with: " << remote_host << ":" << remote_hosts_port << "\n";
}