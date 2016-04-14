//TCPConnection.cpp
// SCHEDULED FOR DELETION, DEPRECATED
// SCHEDULED FOR DELETION, DEPRECATED
// SCHEDULED FOR DELETION, DEPRECATED
// SCHEDULED FOR DELETION, DEPRECATED
// SCHEDULED FOR DELETION, DEPRECATED
// SCHEDULED FOR DELETION, DEPRECATED
#define _WINSOCK_DEPRECATED_NO_WARNINGS	// TEMPORARY, and this has to be at the top
#ifdef __linux__
#include <iostream>
#include <vector>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <cerrno>
#include <pthread.h>			//<process.h>
#include <iomanip>				// std::setw(2) && std::setfill('0')

#include <arpa/inet.h>
#include <signal.h>

#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "CrossPlatformSleep.h"
#include "SocketClass.h"
#endif//__linux__

#ifdef _WIN32
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <process.h>			//<pthread.h>
#include <iomanip>				// std::setw(2) && std::setfill('0')
#include <chrono>				// for microsleep		is this windows only??
#include <thread>				// for microsleep this_thread	is this windows only??

#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "CrossPlatformSleep.h"
#include "SocketClass.h"
#endif//_WIN32

//NAT WIKI: Many NAT implementations follow the port preservation design for TCP: for a given outgoing TCP communication, they use the same values as internal and external port numbers.NAT port preservation for outgoing TCP connections is crucial for TCP NAT traversal, because as TCP requires that one port can only be used for one communication at a time, programs bind distinct TCP sockets to ephemeral ports for each TCP communication, rendering NAT port prediction impossible for TCP.[2]
//NAT WIKI: On the other hand, for UDP, NATs do not need to have port preservation.Indeed, multiple UDP communications(each with a distinct endpoint) can occur on the same source port, and applications usually reuse the same UDP socket to send packets to distinct hosts.This makes port prediction straightforward, as it is the same source port for each packet.

//send the target a packet like a trace-route would?

//from reading up on NATs, it -seems- like i should be able to connect just fine... unless it is a symmetric NAT i'm dealing with.
//maybe i could send a packet to every single port on someone's computer(NAT) in order to open a connection with them for a short amount of time?
//or should i do something with UDP? ambiguous
//what about upnp/upnpc?

//NOTE/WARNING/CHANGE: printf is used here, unlike std::cout everywhere else. CHANGE this now that i'm used to using both

//note from MSDN: The Iphlpapi.h header file is required if an application is using the IP Helper APIs. When the Iphlpapi.h header file is required, the #include line for the Winsock2.h header this file should be placed before the #include line for the Iphlpapi.h header file. 
//winsock2.h automatically includes core elements from Windows.h

#pragma comment(lib, "Ws2_32.lib") //tell the linker that Ws2_32.lib file is needed.
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

//#pragma comment(lib, "Kernel32.lib")			//???? is this necessary for GetConsoleScreenBufferInfo?

#ifdef __linux__
#define INVALID_SOCKET	((SOCKET)(~0))			//~0 is kinda like -1, but for usage with unsigned ints?
#define SOCKET_ERROR	(-1)
#define SD_SEND         0x01
#endif//__linux__

// Everything below are here because they are static(available to all instances of this class). 
const int TCPConnection::NOBODY_WON = -30;
const int TCPConnection::SERVER_WON = -7;
const int TCPConnection::CLIENT_WON = -8;

const std::string TCPConnection::DEFAULT_PORT_TO_LISTEN = "7172";			//currently, set this one to have the server listen on this port
const int TCPConnection::DEFAULT_BUFLEN = 512;
const std::string TCPConnection::DEFAULT_IP_TO_LISTEN = "192.168.1.116";	//currently, set this one to have the server listen on this IP
int TCPConnection::recvbuflen = DEFAULT_BUFLEN;
char TCPConnection::recvbuf[DEFAULT_BUFLEN];

int TCPConnection::global_winner = NOBODY_WON;
SOCKET TCPConnection::globalSocket = INVALID_SOCKET;

char globalvalue = 9;


#ifdef __linux__
int TCPConnection::ret1 = 0;//server race thread
int TCPConnection::ret2 = 0;//client race thread
int TCPConnection::ret3 = 0;//send thread

pthread_t TCPConnection::thread1;//server
pthread_t TCPConnection::thread2;//client
pthread_t TCPConnection::thread3;//send
#endif//__linux__

#ifdef _WIN32
HANDLE TCPConnection::ghEvents[3]{};
#endif//_WIN32


Connection::Connection()
{

}
Connection::~Connection()
{

}


//-------------------------------------------------------------------------------
TCPConnection::TCPConnection()
{
	memset(&UDPSockaddr_in, 0, sizeof(UDPSockaddr_in));
	memset(&incomingAddr, 0, sizeof(incomingAddr));
	memset(&hints, 0, sizeof(hints));
	//ZeroMemory(&incomingAddr, sizeof(incomingAddr));		//ZeroMemory sux, not linux compatible.
	//ZeroMemory(&hints, sizeof(hints));

	//struct pointers being assign nullptr
	PclientSockaddr_in = nullptr;
	result = nullptr;
	ptr = nullptr;
	
	//SOCKET  (just a typedef. on a 32 bit windows it is an unsigned int. 64 bit windows it is an unsigned __int64 ...fun fact :D)
	ListenSocket = INVALID_SOCKET;
	ConnectSocket = INVALID_SOCKET;
	AcceptedSocket = INVALID_SOCKET;
	UDPSpamSocket = INVALID_SOCKET;

	//string
	target_local_ip = "";
	target_port = DEFAULT_PORT_TO_LISTEN;
	my_host_ip_addr = DEFAULT_IP_TO_LISTEN;
	my_host_port = DEFAULT_PORT_TO_LISTEN;

	//int
	iResult = 0;
	errchk = 0;
	iSendResult = 0;

	// bool
	is_addrinfo_freed_already = false;
}
TCPConnection::~TCPConnection()
{
	if (is_addrinfo_freed_already == false)
	{
		myWSACleanup();	// Free addrinfo and WSACleanup();
	}
}

// The main chat program function that houses all functionality for the chat program.
bool TCPConnection::startChatProgram()
{
	TCPConnection serverObj;
	TCPConnection clientObj;
	// BEGIN THREAD RACE
	createServerRaceThread(&serverObj);			//<-----------------
	createClientRaceThread(&clientObj);			//<----------------


	// Wait for 1 thread to finish
#ifdef __linux__
	pthread_join(TCPConnection::thread1, NULL);
	//pthread_join(TCPConnection::thread2, NULL);				//TEMPORARILY IGNORED, not that i want to wait for both anyways, just any 1 thread.
#endif//__linux__
#ifdef _WIN32
	WaitForMultipleObjects(
		2,			//number of objects in array
		TCPConnection::ghEvents,	//array of objects
		FALSE,		//wait for all objects if it is set to TRUE, otherwise FALSE means it waits for any one object to finish. return value indicates the finisher.
		INFINITE);	//its going to wait this long, OR until all threads are finished, in order to continue.
#endif//_WIN32

}

void TCPConnection::giveIPandPort(std::string target_extern_ip, std::string target_port_, std::string my_external_ip_address, std::string my_ip_address, std::string my_port)
{
	target_ext_ip = target_extern_ip;
	target_port = target_port_;
	my_ext_ip = my_external_ip_address;
	my_host_ip_addr = my_ip_address;
	my_host_port = my_port;
}

void TCPConnection::threadEntranceClientLoopAttack(void* instance)
{
#ifdef __linux__
	ret1 = pthread_create(&thread2, NULL, clientLoopAttackThread, instance);	//why am i going to a function that just goes to another function?
	if (ret1)
	{
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret2);
		exit(EXIT_FAILURE);
	}
#endif

#ifdef _WIN32
	ghEvents[0] = (HANDLE)_beginthread(clientLoopAttackThread, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
#endif//_WIN32
}

void TCPConnection::clientLoopAttackThread(void* instance)
{
	if (instance == NULL)
	{
		std::cout << "ERROR: Null instance. Thread Fail.\n";
		return;
	}
	TCPConnection* self = (TCPConnection*)instance;

	SOCKET s = INVALID_SOCKET;
	SOCKET connected_socket_stuff = INVALID_SOCKET;	// currently using this as wonky error checking... not good

	//CrossPlatformSleep Snooze;

	// Start Winsock
	if (self->initializeWinsock() == false)
		return;//false;

	// Set hints
	self->hints.ai_family = AF_UNSPEC;
	self->hints.ai_socktype = SOCK_STREAM;
	self->hints.ai_protocol = IPPROTO_TCP;

	// put address info into addrinfo struct
	if (self->getAddress(self->target_ext_ip, self->target_port) == false)
		return;// false;

	// Create socket
	s = self->createSocket();
	if (s == false)
		return;// false;

	// Connect to the target on socket s
	// >= 1 would mean the socket is assigned a legitimate value.
	while (connected_socket_stuff == INVALID_SOCKET)
	{
		if (global_winner == SERVER_WON)
		{
			if (global_verbose == true)
			{
				std::cout << "CTHREAD::Server won. Exiting thread.\n";
				return;// true
			}
		}
		connected_socket_stuff = self->connectToTarget(s);
		if (connected_socket_stuff == false)
			return;// false;
		else if (connected_socket_stuff == INVALID_SOCKET)	// No real errors, just can't connect yet.
			continue;
		else // must be true
		{
			global_winner = CLIENT_WON;
			return; // true
		}
		//Snooze.mySleep(1);// Might want to make several threads since it doesn't appear easy to sleep less than 1 ms?
		std::this_thread::sleep_for(std::chrono::microseconds(10000)); // oR WHAT ABOut this one?!! 1,000 microseconds in a millisecond
	}

	// Must be connected to someone if we are here
	std::cout << "not possible.\n";
	return;// true;
}

#ifdef __linux__
void* TCPConnection::PosixThreadEntranceServerCompetitionThread(void* instance)
{
	serverCompetitionThread(instance);
	pthread_exit(&ret1);
}
#endif//__linux__

void TCPConnection::serverCompetitionThread(void* instance)
{
	if (instance == NULL)
		return;

	TCPConnection *self = (TCPConnection*)instance;
	int iFeedback;

	fd_set	fdSet;
	memset(&fdSet, 0, sizeof(fdSet));

	timeval timeValue,
			*PtimeValue;
			PtimeValue = &timeValue;
	memset(&timeValue, 0, sizeof(timeValue));

	timeValue.tv_usec = 500000; // 1 million microseconds = 1 second

	if (global_verbose == true)
		std::cout << "STHREAD >> ";
	if (self->getAddress(self->my_host_ip_addr, self->my_host_port) == false) return; //1;			//puts the local port info in the addrinfo structure
	if (global_verbose == true)
		std::cout << "STHREAD >> ";
	self->ListenSocket = self->createSocket();	//creates listening socket to listen on thhe local port you gave it in getAddress()
	if (self->ListenSocket == 0)
		return; //false;				
	FD_SET(self->ListenSocket, &fdSet);		// Putting the ListenSocket into
	//fdSet.fd_count = 1;					//not for linux!
	//fdSet.fd_array[0] = self->ListenSocket;	//windows sux
	if (global_verbose == true) {
		std::cout << "STHREAD >> ";
		std::cout << "SOCKET in fd_array: " << "\n";
		std::cout << "STHREAD >> ";
		std::cout << "SOCKET ListenSocket: " << self->ListenSocket << "\n";
		std::cout << "STHREAD >> ";
	}
	if (self->bindToSocket(self->ListenSocket) == false) return; //1;		//assigns a socket to an address
	//self->putSocketInFdSet();//put socket in fd_set 
	if (global_verbose == true)
		std::cout << "STHREAD >> ";
	if (self->listenToSocket(self->ListenSocket) == false) return; //1;	//thread waits here until someone connects or it errors.

	while (1)
	{
		FD_SET(self->ListenSocket, &fdSet);
		//fdSet.fd_count = 1;
		//fdSet.fd_array[0] = self->ListenSocket;
		iFeedback = select(self->ListenSocket + 1, &fdSet, NULL, NULL, PtimeValue);//select returns the number of handles that are ready and contained in the fd_set structures.
		//std::cout << "STHREAD >> currently my value for the winner is: " << self->global_winner << "\n";
		if (iFeedback == SOCKET_ERROR)
		{
			self->getError();
			std::cout << "STHREAD >> ";
			std::cout << "ServerCompetitionThread select Error.\n";
			self->closeThisSocket(self->ListenSocket);
			std::cout << "STHREAD >> ";
			std::cout << "Closing listening socket b/c of error. Ending Server Thread.\n";
			return;
		}
		else if (self->global_winner == CLIENT_WON)
		{
			self->closeThisSocket(self->ListenSocket);
			if (global_verbose == true)
			{
				std::cout << "STHREAD >> ";
				std::cout << "Closed listening socket, because the winner is: " << self->global_winner << ". Ending Server thread.\n";
			}
			return;
		}
		else if (iFeedback > 0)
		{
			if (global_verbose == true)
			{
				std::cout << "STHREAD >> ";
				std::cout << "attempting to accept a client now that select() returned a readable socket\n";
				std::cout << "STHREAD >> ";
			}
			if ((iFeedback = self->acceptClient(self->ListenSocket)) == false) return; //1;				//creates a new socket to communicate with the person on
			//else
			if (global_verbose == true)
			{
				std::cout << "STHREAD >> ";
				std::cout << "accept must have been successful, setting globalwinner and globalsocket\n";
			}
			self->globalSocket = iFeedback;
			self->global_winner = SERVER_WON;
			self->closeThisSocket(self->ListenSocket);
			return;
		}
	}
}

#ifdef __linux__
void* TCPConnection::PosixThreadEntranceClientCompetitionThread(void* instance)
{
	clientCompetitionThread(instance);
	pthread_exit(&ret2);
}
#endif//__linux__

void TCPConnection::clientCompetitionThread(void* instance)
{
	TCPConnection *self = (TCPConnection*)instance;
	if (global_verbose == true)
		std::cout << "CTHREAD :: ";
	if (self->getAddress(self->target_local_ip, self->target_port) == false)	//puts the local port info in the addrinfo structure
		return; //1;

	CrossPlatformSleep Sleeping;
	while (global_winner != SERVER_WON && global_winner != CLIENT_WON)
	{
		int iFeedback;
		if (global_verbose == true)
			std::cout << "CTHREAD :: ";
		//create a socket then try to connect to that socket.
		if ((iFeedback = self->connectToTarget(self->ConnectSocket)) == false)	// to change timeout options check out setsockopt: http://www.freebsd.org/cgi/man.cgi?query=setsockopt&sektion=2	
			return; //0;
		else if (iFeedback == INVALID_SOCKET)	// If it is just unable to connect, but doesn't error, then continue trying to connect.
		{
			if (global_winner == SERVER_WON)
			{
				if (global_verbose == true)
				{
					std::cout << "CTHREAD :: ";
					std::cout << "Client established a connection, but the server won the race.\n";
					std::cout << "CTHREAD :: ";
					std::cout << "Closing connected socket.\n";
				}
				self->closeThisSocket(iFeedback);
				return;
			}
			Sleeping.mySleep(1000);
			continue;
		}
		else	//connection is established, the client has won the competition.
		{	
			if (global_winner != SERVER_WON)		//make sure the server hasn't already won during the time it took to attempt a TCPConnection
			{	
				std::cout << "CTHREAD :: ";
				std::cout << "setting global values.\n";
				globalSocket = iFeedback;
				global_winner = CLIENT_WON;
				if (global_verbose == true)
				{
					std::cout << "CTHREAD :: " << global_winner << " is the winner.\n";
					std::cout << "CTHREAD :: " << globalSocket << " is the socket now.\n";
				}
				return;
			}
			else	//server won the race already, close the socket that was created by connectToTarget();
			{	
				if (global_verbose == true)
					std::cout << "CTHREAD :: Client established a connection, but the server won the race. Closing connected socket.\n";
				self->closeThisSocket(iFeedback);
				return;
			}
		}
		return;
	}
	return;
}


bool TCPConnection::initializeWinsock()
{
#ifdef _WIN32
	if (global_verbose == true)
		std::cout << "Initializing Winsock... ";
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		getError();
		std::cout << "WSAStartup failed\n";
		return false;
	}
	std::cout << "Success\n";
#endif//_WIN32
	return true;
}

void TCPConnection::ServerSetHints()
{
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;	
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
}

void TCPConnection::ClientSetHints()
{
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
}

bool TCPConnection::getAddress(std::string target_ip, std::string targ_port)
{
	if (global_verbose == true)
		std::cout << "addrinfo given: IP address and port... ";
	// Resolve the LOCAL server address and port.
	errchk = getaddrinfo(target_ip.c_str(), targ_port.c_str(), &hints, &result);
	if (errchk != 0)
	{
		//std::cout << "CTHREAD :: ";
		printf("getaddrinfo failed with error: %d\n", errchk);
		//myWSACleanup();
		return false;
	}
	std::cout << "Success\n";
	return true;
	//std::cout << "IP: " << hints.ai_addr << ". Port: " << hints.sockaddr->ai_addr\n";
}

// Assumes you already put ai_family, ai_socktype, ai_protocol inside struct addrinfo *result
SOCKET TCPConnection::createSocket()
{
	SOCKET s;
	if (global_verbose == true)
		std::cout << "Creating Socket to listen on... ";
	// Create a SOCKET handle for connecting to server(no ip address here, that is with bind)
	s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (s == INVALID_SOCKET)
	{
		getError();
		std::cout << "Socket failed.\n";
		//freeaddrinfo(result);
		closeThisSocket(s);
		//myWSACleanup();
		return 0;//false
	}
	std::cout << "Success\n";
	return s;
}

bool TCPConnection::bindToSocket(SOCKET fd)//my local ip address and port
{
	if (global_verbose == true)
		std::cout << "Binding ... associating local address with the socket... ";
	// Setup the TCP listening socket (putting ip address on the allocated socket)
	errchk = bind(fd, result->ai_addr, result->ai_addrlen);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "Bind failed.\n";
		//freeaddrinfo(result);
		closeThisSocket(fd);
		//myWSACleanup();
		return false;
	}
	std::cout << "Success\n";
	freeaddrinfo(result);   //shouldn't need the info gathered by getaddrinfo now that bind has been called
	return true;
}

SOCKET TCPConnection::connectToTarget(SOCKET fd)
{
	if (global_verbose == true)
		std::cout << "Attempting to connect to someone...\n";					//test
	// Attempt to connect to the first address returned by the call to getaddrinfo
	ptr = result;

	PclientSockaddr_in = (sockaddr_in *)ptr->ai_addr;
	//void *voidAddr;
	//char ipstr[INET_ADDRSTRLEN];
	//voidAddr = &(PclientSockaddr_in->sin_addr);
	//inet_ntop(ptr->ai_family, voidAddr, ipstr, sizeof(ipstr));
	//InetNtop(ptr->ai_family, voidAddr, ipstr, sizeof(ipstr));		//windows only

	// Connect to server
	errchk = connect(fd, ptr->ai_addr, ptr->ai_addrlen);	// Returns 0 on success
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "Connect failed. Socket Error.\n";
		closeThisSocket(fd);
		return false;
	}
	//freeaddrinfo(result);

	if (fd == INVALID_SOCKET)
	{
		getError();
		std::cout << "Connect Failed. Invalid socket.\n";
		closeThisSocket(fd);
		return INVALID_SOCKET;
	}
	else
	{
		std::cout << "Connection established.\n";	
		if (global_verbose == true)
			std::cout << "Using socket ID: " << fd << "\n";
		return fd;
	}
}

bool TCPConnection::listenToSocket(SOCKET fd)
{
	std::cout << "Listening on IP: " << "IP HERE" << " PORT: " << "PORTHERE...\n";
	errchk = listen(fd, SOMAXCONN);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "listen failed.\n";
		closeThisSocket(fd);
		//myWSACleanup();
		return false;
	}
	return true;
}

int TCPConnection::acceptClient(SOCKET fd)
{

#ifdef __linux__
	socklen_t addr_size;
#endif//__linux__
#ifdef _WIN32
	int addr_size;
#endif//_WIN32
	addr_size = sizeof(incomingAddr);
	std::cout << "Waiting for someone to connect.\n";

	// Accept a client socket by listening on a socket
	AcceptedSocket = accept(fd, (sockaddr*)&incomingAddr, &addr_size);
	if (AcceptedSocket == INVALID_SOCKET)
	{
		getError();
		std::cout << "accept failed.\n";
		closeThisSocket(fd);
		//myWSACleanup();
		return false;
	}
	if (global_verbose == true)
		std::cout << "Connected to " << ":" << "ip here\n";
	if (global_verbose == true)
		std::cout << "Connected to someone on socket ID: " << AcceptedSocket << "\n";
	return AcceptedSocket;
}

void TCPConnection::closeThisSocket(SOCKET fd)
{
#ifdef __linux__
	close(fd);
#endif//__linux__

#ifdef _WIN32
	closesocket(fd);
#endif
}

//STATUS: NOT USED
bool TCPConnection::echoReceiveUntilShutdown()	//STATUS: NOT USED
{
	std::cout << "Echo receive loop started...\n";
	// Receive until the peer shuts down the connection
	do
	{
		iResult = recv(AcceptedSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			printf("Message received: ");
			for (int i = 0; i < iResult; i++)
			{
				std::cout << recvbuf[i];
				//goto a new line if at the end of the msg.
				if (i == iResult - 1)
					std::cout << "\n";
			}
			printf("Bytes received: %d\n", iResult);	//this just prints out on the server's console the amount of bytes received.

														// Echo the buffer back to the sender
			printf("Message sent: ");
			for (int i = 0; i < iResult; i++)
			{
				std::cout << recvbuf[i];
				//goto a new line if at the end of the msg.
				if (i == iResult - 1)
					std::cout << "\n";
			}
			iSendResult = send(AcceptedSocket, recvbuf, iResult, 0);	//this sends a message back to the client (b/c this is the server). in this case, it echos back the same msg it received.
			if (iSendResult == SOCKET_ERROR)
			{
				getError();
				std::cout << "send failed.\n";
				closeThisSocket(AcceptedSocket);
				//myWSACleanup();
				return false;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else
		{
			std::cout << "recv failed.\n";
			closeThisSocket(AcceptedSocket);
			//myWSACleanup();
			return false;
		}
	} while (iResult > 0);

	std::cout << "Ending receive loop...\n";
	return true;
}

bool TCPConnection::receiveUntilShutdown()
{
	const char* sendbuf = "First message sent.";
	std::string message_to_send = "Automated message sent from recv func.\n";

	//send this message once
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	iResult = send(globalSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
	if (iResult == SOCKET_ERROR)
	{
		getError();
		std::cout << "send failed.\n";
		closeThisSocket(globalSocket);
		//myWSACleanup();
		//return false;
	}
	std::cout << "Message sent: " << sendbuf << "\n";
	if (global_verbose == true)
		std::cout << "Bytes Sent: " << iResult << "\n";

	// Receive until the peer shuts down the connection
	if (global_verbose == true)
		std::cout << "Recv loop started...\n";
	do
	{
		iResult = recv(globalSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
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
			printf("Msg recvd: ");
			for (int i = 0; i < iResult; i++)
			{
				std::cout << recvbuf[i];
				//goto a new line if at the end of the msg.
				if (i == iResult - 1)
					std::cout << "\n";
			}
			if (global_verbose == true)
				printf("Bytes recvd: %d\n", iResult);						//this just prints out on the server's console the amount of bytes received.
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else
		{
			getError();
			std::cout << "recv failed.\n";
			closeThisSocket(globalSocket);
			//myWSACleanup();
			return false;
		}
	} while (iResult > 0);

	return true;
}

bool TCPConnection::shutdownConnection(SOCKET fd)
{
	std::cout << "Shutting down the connection...\n";
	// shutdown the connection since we're done
	errchk = shutdown(fd, SD_SEND);
	if (errchk == SOCKET_ERROR)
	{
		std::cout << "shutdown failed.\n";
		closeThisSocket(fd);
		std::cout << "cosing socket...\n";
		//myWSACleanup();
		return false;
	}
	return true;
}

void TCPConnection::myWSACleanup()//WHEN BIND FAILS, and it tries to call freeaddrinfo in here, unkown error occurs.
{
	try
	{
		if (global_verbose == true)
			std::cout << "Cleaning up. Freeing addrinfo...\n";
	}
	catch(std::exception excep_one)
	{
		int zeros = 1;
	}
	freeaddrinfo(result);									//frees information gathered that the getaddrinfo() dynamically allocated into addrinfo structures
	//closeThisSocket(AcceptedSocket);
#ifdef _WIN32
	WSACleanup();
#endif//_WIN32
}

bool TCPConnection::clientSetIpAndPort(std::string user_defined_ip_address, std::string user_defined_port)
{
	//HANDLE ghEvents[2];
	if (global_verbose == true)
	{
		std::cout << "DEFAULT_IP:   " << DEFAULT_IP_TO_LISTEN << "\n";
		std::cout << "DEFAULT_PORT: " << DEFAULT_PORT_TO_LISTEN << "\n";
	}
	target_local_ip = user_defined_ip_address;
	target_port = user_defined_port;
	if (global_verbose == true)
	{
		std::cout << "User defined IP to connect to:   " << target_local_ip << "\n";
		std::cout << "User defined port to connect to: " << target_port << "\n";
	}
	//std::stoi(port);//convert string to int
	return false;
}

bool TCPConnection::serverSetIpAndPort(std::string user_defined_ip_address, std::string user_defined_port)
{
	//HANDLE ghEvents[2];
	if (global_verbose == true)
	{
		std::cout << "DEFAULT_IP:   " << DEFAULT_IP_TO_LISTEN << "\n";
		std::cout << "DEFAULT_PORT: " << DEFAULT_PORT_TO_LISTEN << "\n";
	}
	my_host_ip_addr = user_defined_ip_address;
	my_host_port = user_defined_port;
	if (global_verbose == true)
	{
		std::cout << "User defined IP to listen on:   " << my_host_ip_addr << "\n";
		std::cout << "User defined port to listen on: " << my_host_port << "\n";
	}
	return false;
}

void TCPConnection::createServerRaceThread(void* instance)
{
#ifdef __linux__
	ret1 = pthread_create(&thread1, NULL, PosixThreadEntranceServerCompetitionThread, instance);
	if (ret1)
	{
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret1);
		exit(EXIT_FAILURE);
	}
#endif//__linux__


#ifdef _WIN32
	ghEvents[0] = (HANDLE)_beginthread(serverCompetitionThread, 0, instance);
#endif//__WIN32
}

void TCPConnection::createClientRaceThread(void* instance)
{
#ifdef __linux__
	ret2 = pthread_create(&thread2, NULL, PosixThreadEntranceClientCompetitionThread, instance);	//why am i going to a function that just goes to another function?
	if (ret2)
	{
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret2);
		exit(EXIT_FAILURE);
	}
#endif

#ifdef _WIN32
	ghEvents[1] = (HANDLE)_beginthread(clientCompetitionThread, 0, instance);//c style typecast    from: uintptr_t    to: HANDLE.
#endif//_WIN32
}

#ifdef __linux__
void* TCPConnection::PosixThreadSendThread(void* instance)
{
	sendThread(instance);
	pthread_exit(&ret3);
}
#endif//__linux__

void TCPConnection::serverCreateSendThread(void* instance)
{
#ifdef __linux__

	ret3 = pthread_create(&thread3, NULL, PosixThreadSendThread, instance);
	if (ret3)
	{
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret3);
		exit(EXIT_FAILURE);
	}
#endif

#ifdef _WIN32
	ghEvents[2] = (HANDLE)_beginthread(TCPConnection::sendThread, 0, instance);
#endif//_WIN32
}

void TCPConnection::clientCreateSendThread(void* instance)
{
#ifdef __linux__

	ret3 = pthread_create(&thread3, NULL, PosixThreadSendThread, instance);
	if (ret3)
	{
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret3);
		exit(EXIT_FAILURE);
	}
#endif//__linux__

#ifdef _WIN32
		ghEvents[2] = (HANDLE)_beginthread(sendThread, 0, instance);
#endif//_WIN32
}

void TCPConnection::sendThread(void* instance)
{
	if (instance == NULL)
	{
		std::cout << "Instance was NULL. sendThread.\n";
		return;
	}
	TCPConnection* self = (TCPConnection*)instance;
	int intResult;
	const char* sendbuf = "";
	std::string message_to_send = "Automated message from SendThread.";

	//send this message once
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	intResult = send(self->globalSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
	if (intResult == SOCKET_ERROR)
	{
		self->getError();
		std::cout << "send failed.\n";
		self->closeThisSocket(self->globalSocket);
		//self->myWSACleanup();
		//return false;
	}
	printf("Message sent: %s\n", sendbuf);
	if (global_verbose == true)
		printf("Bytes Sent: %ld\n", intResult);

	//ask & send the user's input the rest of the time.
	while (1)
	{
		std::getline(std::cin, message_to_send);
		sendbuf = message_to_send.c_str();	//c_str converts from string to char *
		intResult = send(self->globalSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
		if (intResult == SOCKET_ERROR)
		{
			self->getError();
			std::cout << "send failed.\n";
			self->closeThisSocket(self->globalSocket);
			//self->myWSACleanup();
			return;//return 1
		}
		printf("Message sent: %s\n", sendbuf);
		if (global_verbose == true)
			printf("Bytes Sent: %ld\n", intResult);
	}
}

// errno needs to be set to 0 before every function call that returns errno  // ERR30-C  /  SEI CERT C Coding Standard https://www.securecoding.cert.org/confluence/pages/viewpage.action?pageId=6619179
void TCPConnection::getError()
{
#ifdef __linux__
	int errsv = errno;
	std::cout << "ERROR " << errsv << ". ";
	if (errsv == 10013)
	{
		std::cout << "Permission Denied. ";
	}
	return;
#endif//__linux__

#ifdef _WIN32
	int errsv = WSAGetLastError();
	std::cout << "ERROR: " << errsv << ". ";
	if (errsv == 10013)
	{
		std::cout << "Permission Denied. ";
	}
	return;
#endif//_WIN32
}




/************************************ UDP Section **************************************/
UDPConnection::UDPConnection()
{

}
UDPConnection::~UDPConnection()
{

}








/*
void UDPConnection::spamSetHints()
{
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
}

int UDPConnection::spamCreateSocket() {
	if (global_verbose == true)
		std::cout << "SPAM :: Creating Socket to listen on...\n";
	// Create UDP socket
	UDPSpamSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (UDPSpamSocket == INVALID_SOCKET) {
		getError();
		std::cout << "SPAM :: Socket failed.\n";
		//freeaddrinfo(result);
		closeThisSocket(ListenSocket);
		//myWSACleanup();
		return false;
	}
	return true;
	
	//ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
}

bool UDPConnection::spamPortsWithSendTo()
{

	int intResult;
	const char* sendbuf = "";
	std::string message_to_send = "Autobot Hi\n";
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	size_t current_sendbuf_len = strlen(sendbuf);

	UDPSockaddr_in.sin_family = AF_INET;
	UDPSockaddr_in.sin_port = htons(5050);
	//InetPton(UDPSockaddr_in.sin_family, target_ip_addr.c_str(), &UDPSockaddr_in.sin_addr.S_un.S_addr);
	UDPSockaddr_in.sin_addr.s_addr = inet_addr( target_ext_ip.c_str() );

	iResult = sendto(UDPSpamSocket, sendbuf, current_sendbuf_len, 0, (SOCKADDR *)& UDPSockaddr_in, sizeof(UDPSockaddr_in));
	if (iResult == SOCKET_ERROR) {
		getError();
		std::cout << "sendto failed.\n";
		closeThisSocket(UDPSpamSocket);
		//myWSACleanup();
		return 0;
	}

	// Send a packet to every single one of the target's ports
	std::cout << "Spamming target's ports...\n";
	for (int i = 1; i <= 65535; i++) {
		UDPSockaddr_in.sin_port = htons(i);
		intResult = sendto(UDPSpamSocket,	sendbuf, current_sendbuf_len, 0, (SOCKADDR *)& UDPSockaddr_in, sizeof(UDPSockaddr_in));
		if (intResult == SOCKET_ERROR) {
			printf("SPAM :: sendto failed with error: %d\n", WSAGetLastError());
			closesocket(UDPSpamSocket);
			WSACleanup();
			return false;
		}
		std::cout << "a\n" << "b\n";
	}
	std::cout << "Portspam complete.\n";
	return true;
}
*/


