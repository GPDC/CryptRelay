//connection.cpp
//could i use vectors instead of char arrays for messages?
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#undef UNICODE
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
#include <pthread.h>	//<process.h>
#include <iomanip>	// std::setw(2) && std::setfill('0')

#include <arpa/inet.h>
#include <signal.h>

#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "CrossPlatformSleep.h"
#endif//__linux__

#ifdef _WIN32
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <process.h>	//<pthread.h>
#include <iomanip>	// std::setw(2) && std::setfill('0')

#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "CrossPlatformSleep.h"
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
const int connection::NOBODY_WON = -30;
const int connection::SERVER_WON = -7;
const int connection::CLIENT_WON = -8;

const std::string connection::DEFAULT_PORT_TO_LISTEN = "7172";			//currently, set this one to have the server listen on this port
const int connection::DEFAULT_BUFLEN = 512;
const std::string connection::DEFAULT_IP_TO_LISTEN = "192.168.1.116";	//currently, set this one to have the server listen on this IP
int connection::recvbuflen = DEFAULT_BUFLEN;
char connection::recvbuf[DEFAULT_BUFLEN];

int connection::globalWinner = NOBODY_WON;
SOCKET connection::globalSocket = INVALID_SOCKET;

char globalvalue = 9;

#ifdef __linux__
int connection::ret1 = 0;//server race thread
int connection::ret2 = 0;//client race thread
int connection::ret3 = 0;//send thread

pthread_t connection::thread1;//server
pthread_t connection::thread2;//client
pthread_t connection::thread3;//send
#endif//__linux__

#ifdef _WIN32
HANDLE connection::ghEvents[3]{};
#endif//_WIN32



connection::connection()
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
	target_ip_addr = "";
	target_port = DEFAULT_PORT_TO_LISTEN;
	my_host_ip_addr = DEFAULT_IP_TO_LISTEN;
	my_host_port = DEFAULT_PORT_TO_LISTEN;

	//int
	iResult = 0;
	errchk = 0;
	iSendResult = 0;
}
connection::~connection()
{

}


#ifdef __linux__
void* connection::PosixThreadEntranceServerCompetitionThread(void* instance)
{
	serverCompetitionThread(instance);
	pthread_exit(&ret1);
}
#endif//__linux__

void connection::serverCompetitionThread(void* instance)
{
	if (instance == NULL)
		return;

	connection *self = (connection*)instance;
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
	if (self->serverGetAddress() == false) return; //1;			//puts the local port info in the addrinfo structure
	if (global_verbose == true)
		std::cout << "STHREAD >> ";
	if (self->createSocket() == false) return; //1;				//creates listening socket to listen on that local port
	FD_SET(self->ListenSocket, &fdSet);
	//fdSet.fd_count = 1;					//not for linux!
	//fdSet.fd_array[0] = self->ListenSocket;	//windows sux
	if (global_verbose == true) {
		std::cout << "STHREAD >> ";
		std::cout << "SOCKET in fd_array: " << "\n";
		std::cout << "STHREAD >> ";
		std::cout << "SOCKET ListenSocket: " << self->ListenSocket << "\n";
		std::cout << "STHREAD >> ";
	}
	if (self->bindToListeningSocket() == false) return; //1;		//assigns a socket to an address
	self->putSocketInFdSet();//put socket in fd_set 
	if (global_verbose == true)
		std::cout << "STHREAD >> ";
	if (self->listenToListeningSocket() == false) return; //1;	//thread waits here until someone connects or it errors.

	while (1)
	{
		FD_SET(self->ListenSocket, &fdSet);
		//fdSet.fd_count = 1;
		//fdSet.fd_array[0] = self->ListenSocket;
		iFeedback = select(self->ListenSocket + 1, &fdSet, NULL, NULL, PtimeValue);//select returns the number of handles that are ready and contained in the fd_set structures.
		//std::cout << "STHREAD >> currently my value for the winner is: " << self->globalWinner << "\n";
		if (iFeedback == SOCKET_ERROR){
			self->getError();
			std::cout << "STHREAD >> ";
			std::cout << "ServerCompetitionThread select Error.\n";
			self->closeThisSocket(self->ListenSocket);
			std::cout << "STHREAD >> ";
			std::cout << "Closing listening socket b/c of error. Ending Server Thread.\n";
			return;
		}
		else if (self->globalWinner == CLIENT_WON){
			self->closeThisSocket(self->ListenSocket);
			if (global_verbose == true) {
				std::cout << "STHREAD >> ";
				std::cout << "Closed listening socket, because the winner is: " << self->globalWinner << ". Ending Server thread.\n";
			}
			return;
		}
		else if (iFeedback > 0){
			if (global_verbose == true) {
				std::cout << "STHREAD >> ";
				std::cout << "attempting to accept a client now that select() returned a readable socket\n";
				std::cout << "STHREAD >> ";
			}
			if ((iFeedback = self->acceptClient()) == false) return; //1;				//creates a new socket to communicate with the person on
			//else
			if (global_verbose == true) {
				std::cout << "STHREAD >> ";
				std::cout << "accept must have been successful, setting globalwinner and globalsocket\n";
			}
			self->globalSocket = iFeedback;
			self->globalWinner = SERVER_WON;
			self->closeThisSocket(self->ListenSocket);
			return;
		}
	}
}

#ifdef __linux__
void* connection::PosixThreadEntranceClientCompetitionThread(void* instance)
{
	clientCompetitionThread(instance);
	pthread_exit(&ret2);
}
#endif//__linux__

void connection::clientCompetitionThread(void* instance)
{
	connection *self = (connection*)instance;
	if (global_verbose == true)
		std::cout << "CTHREAD :: ";
	if (self->clientGetAddress() == false) return; //1;		//puts the local port info in the addrinfo structure

	CrossPlatformSleep Sleeping;
	while (globalWinner != SERVER_WON && globalWinner != CLIENT_WON) 
	{
		int iFeedback;
		if (global_verbose == true)
			std::cout << "CTHREAD :: ";
		//create a socket then try to connect to that socket.
		if ((iFeedback = self->connectToTarget()) == 0) return; //1;to change timeout options check out setsockopt: http://www.freebsd.org/cgi/man.cgi?query=setsockopt&sektion=2								
		else if (iFeedback == 1) {//if it is just unable to connect, but doesn't error, then continue trying to connect.
			if (globalWinner == SERVER_WON){
				if (global_verbose == true) {
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
		else{	//connection is established, the client has won the competition.
			if (globalWinner != SERVER_WON){	//make sure the server hasn't already won during the time it took to attempt a connection
				std::cout << "CTHREAD :: ";
				std::cout << "setting global values.\n";
				globalSocket = iFeedback;
				globalWinner = CLIENT_WON;
				if (global_verbose == true) {
					std::cout << "CTHREAD :: " << globalWinner << " is the winner.\n";
					std::cout << "CTHREAD :: " << globalSocket << " is the socket now.\n";
				}
				return;
			}
			else{	//server won the race already, close the socket that was created by connectToTarget();
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


bool connection::initializeWinsock()
{
#ifdef _WIN32
	if (global_verbose == true)
		std::cout << "Initializing Winsock...\n";
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return false;
	}
#endif//_WIN32
	return true;
}

void connection::ServerSetHints()
{
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;	
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
}

void connection::ClientSetHints()
{
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
}

bool connection::serverGetAddress()
{
	if (global_verbose == true)
		std::cout << "Retreiving info: IP address and port...\n";
	// Resolve the LOCAL server address and port.
	errchk = getaddrinfo(my_host_ip_addr.c_str(), my_host_port.c_str(), &hints, &result);
	if (errchk != 0){
		std::cout << "STHREAD >> ";
		printf("getaddrinfo failed with error: %d\n", errchk);
		myWSACleanup();
		return false;
	}
	return true;
}

bool connection::clientGetAddress()
{
	if (global_verbose == true)
		std::cout << "Retreiving info: IP address and port...\n";
	// Resolve the LOCAL server address and port.
	errchk = getaddrinfo(target_ip_addr.c_str(), target_port.c_str(), &hints, &result);
	if (errchk != 0){
		std::cout << "CTHREAD :: ";
		printf("getaddrinfo failed with error: %d\n", errchk);
		myWSACleanup();
		return false;
	}
	return true;
	//std::cout << "IP: " << hints.ai_addr << ". Port: " << hints.sockaddr->ai_addr\n";
}

bool connection::createSocket()
{
	if (global_verbose == true)
		std::cout << "Creating Socket to listen on...\n";
	// Create a SOCKET handle for connecting to server(no ip address here, that is with bind)
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		getError();
		std::cout << "Socket failed.\n";
		//freeaddrinfo(result);
		closeThisSocket(ListenSocket);
		myWSACleanup();
		return false;
	}
	return true;
}

bool connection::bindToListeningSocket()//my local ip address and port
{
	if (global_verbose == true)
		std::cout << "Binding ... associating local address with the listen socket...\n";
	// Setup the TCP listening socket (putting ip address on the allocated socket)
	errchk = bind(ListenSocket, result->ai_addr, result->ai_addrlen);
	if (errchk == SOCKET_ERROR) {
		getError();
		std::cout << "bind failed.\n";
		//freeaddrinfo(result);
		closeThisSocket(ListenSocket);
		myWSACleanup();
		return false;
	}
	freeaddrinfo(result);   //shouldn't need the info gathered by getaddrinfo now that bind has been called
	return true;
}

//STATUS: NOT USED, what was this for?
void connection::putSocketInFdSet()//STATUS: NOT USED, what was this for?
{
	return;
}

int connection::connectToTarget()
{
	if (global_verbose == true)
		std::cout << "Attempting to connect to someone...\n";					//test
	//attempt to connect to the first address returned by the call to getaddrinfo
	ptr = result;

	//create a SOCKET for connecting to server.
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	
	//catch any errors that may have occured while creating the socket / check for errors to make sure the socket is valid
	if (ConnectSocket == INVALID_SOCKET){
		getError();
		std::cout << "Error at socket().\n";
		//freeaddrinfo(result);
		closeThisSocket(ConnectSocket);
		myWSACleanup();
		return false;
	}

	PclientSockaddr_in = (sockaddr_in *)ptr->ai_addr;
	void *voidAddr;
	char ipstr[INET_ADDRSTRLEN];
	voidAddr = &(PclientSockaddr_in->sin_addr);
	//inet_ntop(ptr->ai_family, voidAddr, ipstr, sizeof(ipstr));
	InetNtop(ptr->ai_family, voidAddr, ipstr, sizeof(ipstr));		//windows only
	if (global_verbose == true)
		std::cout << "CTHREAD :: Trying to connect to: " << ipstr << "\n";
	//connect to server
	errchk = connect(ConnectSocket, ptr->ai_addr, ptr->ai_addrlen);
	if (errchk == SOCKET_ERROR){
		//closeThisSocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}
	//std::cout << "Sucessfully connected to: " << (sockaddr)serverStoreAddr.sin_addr << ":";	

	//freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("CTHREAD :: Unable to connect to server!\n");
		closeThisSocket(ConnectSocket);
		return true;
	}
	else{
		std::cout << "Connection established.\n";	
		if (global_verbose == true)
			std::cout << "Using socket ID: " << ConnectSocket << "\n";
		return ConnectSocket;
	}
}

bool connection::listenToListeningSocket()
{
	std::cout << "Listening on IP: " << "IP HERE" << " PORT: " << "PORTHERE...\n";
	errchk = listen(ListenSocket, SOMAXCONN);
	if (errchk == SOCKET_ERROR) {
		getError();
		std::cout << "listen failed.\n";
		closeThisSocket(ListenSocket);
		myWSACleanup();
		return false;
	}
	return true;
}

int connection::acceptClient()
{

#ifdef __linux__
	socklen_t addr_size;
#endif//__linux__
#ifdef _WIN32
	int addr_size;
#endif//_WIN32
	addr_size = sizeof(incomingAddr);
	std::cout << "Waiting for someone to connect.\n";

	// Accept a client socket by listening on: ListenSocket.
	AcceptedSocket = accept(ListenSocket, (sockaddr*)&incomingAddr, &addr_size);
	if (AcceptedSocket == INVALID_SOCKET) {
		getError();
		std::cout << "accept failed.\n";
		closeThisSocket(ListenSocket);
		myWSACleanup();
		return false;
	}
	if (global_verbose == true)
		std::cout << "Connected to " << ":" << "ip here\n";
	if (global_verbose == true)
		std::cout << "Connected to someone on socket ID: " << AcceptedSocket << "\n";
	return AcceptedSocket;
}

void connection::closeTheListeningSocket()//get rid of this?
{
	if (global_verbose == true)
		std::cout << "Closing the listening socket...\n";
	closeThisSocket(ListenSocket);
}

void connection::closeThisSocket(SOCKET fd)
{
#ifdef __linux__
	close(fd);
#endif//__linux__

#ifdef _WIN32
	closesocket(fd);
#endif
}


//STATUS: NOT USED
bool connection::echoReceiveUntilShutdown()	//STATUS: NOT USED
{
	std::cout << "Echo receive loop started...\n";
	// Receive until the peer shuts down the connection
	do {
		iResult = recv(AcceptedSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {

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
			if (iSendResult == SOCKET_ERROR) {
				getError();
				std::cout << "send failed.\n";
				closeThisSocket(AcceptedSocket);
				myWSACleanup();
				return false;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			std::cout << "recv failed.\n";
			closeThisSocket(AcceptedSocket);
			myWSACleanup();
			return false;
		}
	} while (iResult > 0);

	std::cout << "Ending receive loop...\n";
	return true;
}

bool connection::receiveUntilShutdown()
{
	const char* sendbuf = "First message sent.";
	std::string message_to_send = "Automated message sent from recv func.\n";

	//send this message once
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	iResult = send(globalSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
	if (iResult == SOCKET_ERROR){
		getError();
		std::cout << "send failed.\n";
		closeThisSocket(globalSocket);
		myWSACleanup();
		//return false;
	}
	printf("Message sent: %s\n", sendbuf);
	if (global_verbose == true)
		printf("Bytes Sent: %ld\n", iResult);
	if (global_verbose == true)
		std::cout << "Recv loop started...\n";

	// Receive until the peer shuts down the connection
	do {
		iResult = recv(globalSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {

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
			for (int i = 0; i < iResult; i++){
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
		else {
			getError();
			std::cout << "recv failed.\n";
			closeThisSocket(globalSocket);
			myWSACleanup();
			return false;
		}
	} while (iResult > 0);
	return true;
}

bool connection::shutdownConnection()
{
	std::cout << "Shutting down the connection...\n";
	// shutdown the connection since we're done
	errchk = shutdown(globalSocket, SD_SEND);
	if (errchk == SOCKET_ERROR) {
		std::cout << "shutdown failed.\n";
		closeThisSocket(globalSocket);
		myWSACleanup();
		return false;
	}
	return true;
}

// myWSACleanup
void connection::myWSACleanup()//WHEN BIND FAILS, and it tries to call freeaddrinfo in here, unkown error occurs.
{
	if (global_verbose == true)
		std::cout << "Cleaning up. Freeing addrinfo...\n";
	freeaddrinfo(result);									//frees information gathered that the getaddrinfo() dynamically allocated into addrinfo structures
	//closeThisSocket(AcceptedSocket);
#ifdef _WIN32
	WSACleanup();
#endif//_WIN32
}

bool connection::clientSetIpAndPort(std::string user_defined_ip_address, std::string user_defined_port)
{
	//HANDLE ghEvents[2];
	if (global_verbose == true) {
		std::cout << "DEFAULT_IP:   " << DEFAULT_IP_TO_LISTEN << "\n";
		std::cout << "DEFAULT_PORT: " << DEFAULT_PORT_TO_LISTEN << "\n";
	}
	target_ip_addr = user_defined_ip_address;
	target_port = user_defined_port;
	if (global_verbose == true) {
		std::cout << "User defined IP to connect to:   " << target_ip_addr << "\n";
		std::cout << "User defined port to connect to: " << target_port << "\n";
	}
	//std::stoi(port);//convert string to int
	return false;
}

bool connection::serverSetIpAndPort(std::string user_defined_ip_address, std::string user_defined_port)
{
	//HANDLE ghEvents[2];
	if (global_verbose == true) {
		std::cout << "DEFAULT_IP:   " << DEFAULT_IP_TO_LISTEN << "\n";
		std::cout << "DEFAULT_PORT: " << DEFAULT_PORT_TO_LISTEN << "\n";
	}
	my_host_ip_addr = user_defined_ip_address;
	my_host_port = user_defined_port;
	if (global_verbose == true) {
		std::cout << "User defined IP to listen on:   " << my_host_ip_addr << "\n";
		std::cout << "User defined port to listen on: " << my_host_port << "\n";
	}
	return false;
}

void connection::createServerRaceThread(void* instance)
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

void connection::createClientRaceThread(void* instance)
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
void* connection::PosixThreadSendThread(void* instance)
{
	sendThread(instance);
	pthread_exit(&ret3);
}
#endif//__linux__

void connection::serverCreateSendThread(void* instance)
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
	ghEvents[2] = (HANDLE)_beginthread(connection::sendThread, 0, instance);
#endif//_WIN32
}

void connection::clientCreateSendThread(void* instance)
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

void connection::sendThread(void* instance)
{
	connection* self = (connection*)instance;
	int intResult;
	const char* sendbuf = "";
	std::string message_to_send = "Automated message from SendThread.";

	//send this message once
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	intResult = send(self->globalSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
	if (intResult == SOCKET_ERROR){
		self->getError();
		std::cout << "send failed.\n";
		self->closeThisSocket(self->globalSocket);
		self->myWSACleanup();
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
		if (intResult == SOCKET_ERROR){
			self->getError();
			std::cout << "send failed.\n";
			self->closeThisSocket(self->globalSocket);
			self->myWSACleanup();
			return;//return 1
		}
		printf("Message sent: %s\n", sendbuf);
		if (global_verbose == true)
			printf("Bytes Sent: %ld\n", intResult);
	}
}

void connection::getError()
{
#ifdef __linux__
	int errsv = errno;
	std::cout << "ERROR " << errsv << " :" << strerror(errsv);
	return;
#endif
#ifdef _WIN32
	int errsv = WSAGetLastError();
	std::cout << "ERROR: " << errsv << " ";
	return;
	//returns int
#endif
}




/************************************ UDP Section **************************************/
/* BEGIN UDP COMMENT OUT

void connection::UDPSpamSetHints()
{
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
}

int connection::UDPSpamCreateSocket() {
	if (global_verbose == true)
		std::cout << "SPAM :: Creating Socket to listen on...\n";
	// Create UDP socket
	UDPSpamSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (UDPSpamSocket == INVALID_SOCKET) {
		getError();
		std::cout << "SPAM :: Socket failed.\n";
		//freeaddrinfo(result);
		closeThisSocket(ListenSocket);
		myWSACleanup();
		return false;
	}
	return true;
	
	//ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
}

bool connection::UDPSpamPortsWithSendTo()
{

	int intResult;
	const char* sendbuf = "";
	std::string message_to_send = "Autobot Hi\n";
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	size_t current_sendbuf_len = strlen(sendbuf);

	UDPSockaddr_in.sin_family = AF_INET;
	UDPSockaddr_in.sin_port = htons(5050);
	//InetPton(UDPSockaddr_in.sin_family, target_ip_addr.c_str(), &UDPSockaddr_in.sin_addr.S_un.S_addr);
	UDPSockaddr_in.sin_addr.s_addr = inet_addr( target_ip_addr.c_str() );

//	iResult = sendto(UDPSpamSocket, sendbuf, current_sendbuf_len, 0, (SOCKADDR *)& UDPSockaddr_in, sizeof(UDPSockaddr_in));
//	if (iResult == SOCKET_ERROR) {
//		getError();
//		std::cout << "sendto failed.\n";
//		closeThisSocket(UDPSpamSocket);
//		myWSACleanup();
//		return 0;
//	}

	// Send a packet to every single one of the target's ports
	std::cout << "Spamming target's ports...\n";
	for (int i = 1; i <= 65535; i++) {
		UDPSockaddr_in.sin_port = htons(i);
		iResult = sendto(UDPSpamSocket,	sendbuf, current_sendbuf_len, 0, (SOCKADDR *)& UDPSockaddr_in, sizeof(UDPSockaddr_in));
		if (iResult == SOCKET_ERROR) {
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

END UDP COMMENT OUT *********/



/******************** Raw class ************************/
// Reminder: Carefully read every. single. word. inside documentation for berkley sockets (this applies to everything).
//		Otherwise you might skip the 2 most important words in the gigantic document.
bool Raw::stop_echo_request_loop = false;
HANDLE Raw::ghEvents2[1];
Raw::Raw()
{
	// SOCKET
	created_socket = INVALID_SOCKET;

	// int
	iResult = 0;
	errchk = 0;

	// size_t
	current_sendbuf_len = 0;

	// Gotta make those structs don't have random values in them!
	//memset(&Hints, 0, sizeof(Hints));					// addrinfo
	//memset(&StorageHints, 0, sizeof(StorageHints));	// SOCKADDR_STORAGE
	memset(&IPV4Header, 0, sizeof(IPV4Header));			// iphdr, and the 2 structs inside iphd
	memset(&ICMPHeader, 0, sizeof(ICMPHeader));			// icmpheader_echorequest;
	memset(&TargetSockAddrIn, 0, sizeof(TargetSockAddrIn));					// SOCKADDR_IN
}
Raw::~Raw()
{
}

// Decided not to use this. In reality, not necessary for this program. Could use boost library, or look up every compiler's info myself.
bool Raw::isLittleEndian()	// Figuring out where to store this since i'm not using it, but its cool.
{
	// A union is only big enough to hold the largest data member that it contains.
	// Don't read from something unless it was just written; it probably won't give back the value you are thinking it will.
	union
	{
		std::uint32_t i;
		std::uint8_t b[4];	// Please treat as a 8 bit int. (if you wish to cout this number, cast it to (int) )
	}iEndians;
	iEndians.i = 0x01020304;// If the machine is little endian, then iEndians.i will show up as:
							//		0x//  01 02 03 04
							//		dec// 16,909,060
							//		0b//  0000 0001 0000 0010 0000 0011 0000 0100
							// So what we're checking for is, as a decimal number, b[0] == 4. If true, then it's == little endian.
							
							// If the machine is big endian, then iEndians.i will show up as:
							//		0x//  04 03 02 01
							//		dec// 1,086,341,248
							//		0b//  0010 0000 1100 0000 0100 0000 1000 0000.
							// So what we're checking for is, as a decimal number, b[0] == 1. If true, then it's == big endian

	if (iEndians.b[0] == 4)			// Little endian
	{
		if (global_verbose == true)
		{
			std::cout << "Machine is detected as little endian. " << "b[0] == " << (int)iEndians.b[0] << "\n";
		}
		return true;
	}
	else if (iEndians.b[0] == 1)	// Must be big endian, and b[0] == 1
	{
		if (global_verbose == true)
		{
			std::cout << "Machine is detected as big endian. " << "b[0] == " << (int)iEndians.b[0] << "\n";
		}
		return false;	
	}
	else							// Shouldn't ever happen.
	{
		std::cout << "Major catastrophical problem determining byte order. Exiting.\n";
		exit(0);	
	}


}

// ihl min is 5, max is 15;
//std::uint8_t Raw::setIHLAndVer(u_int ihl, u_int ver)
//{
//	return (ver << 4) | ihl;
//}		
//std::uint8_t Raw::setDSCPAndECN(u_int dscp, u_int ecn)
//{
//	return (dscp << 2) | ecn;	//dscp takes up 6 bits, ecn 2.
//
//
//	// For example if dscp = 4 and ecn = 1
//	//00010000	dscp |		//after having been shifted already
//	//00000001 ecn  =
//	//---------
//	//00010001
//}

// Unsure if this is working correctly or if wireshark is displaying it incorrectly?
std::uint16_t Raw::setFlagsAndFragOffset(uint16_t flags, uint16_t frag_offset)
{
	return  (flags << 5) | htons(frag_offset);

	// This means max frag_offset is 65,528, and theoretical flags max is 7 (but not really, see next line below)
	// bit 0 is reserved. bit 1 == don't fragment. bit 2 == more fragments. This concludes the flags available to be set.
	
	//573 = 0000'0010   0011'1101
	//1   = 0000'0000   0010'0000


//htons573 = 0011'1101   0000'0010
	 //1   = 0000'0000   0010'0000			//has already been shifted
//			----------------------
//			 0011'1101   0010'0010
//htons	   = 0010'0010   0011'1101



		  // 0000'1111   1110'0001


	//		 0000'1111   0011'1100   1111'0000
	//htons  1111'0000   0011'1100   0000'1111


	//bitmask
	// htons(frag) & 0b10000000'00000000
	//.....................................................................


	// Explanation:
	// Let the compiler figure out if the machine we are in is big endian or little endian by using bitwise shift operators.
	// << shifts in the direction of the most significant bit, and >> shifts in the dir. of the least significant bit.
	// This is great because the compiler knows the direction of the most significant bit. It could be big endian or
	//		little endian. We don't know which it is, but the compiler does, therefore we let the compiler handle the direction to shift.	

	// This is what it looks like:	
	// ver == 0b0000'0100  |
	// ihl == 0b1111'0000  =
	// ------------------
	// spb == 0b1111'0100

	// I COULD instead do either:
	// A. Look through all the compiler specific stuff and things for all the compilers ever... OR
	// B. Create 2 structures. One is for little endian. The other is for big endian.
	//		Create function for determining endianness.
}

uint16_t Raw::roL(uint16_t unsigned_number_to_shift, uint16_t unsigned_shift_count)
{
	
	uint16_t most_significant_bitmask = 1 << ( (sizeof(unsigned_number_to_shift) * 8 - 1) );		// multiply the size of x by 8.. because sizeof returns the num of bytes.
																					// multiply by 8 to get the bit count. Then -1 so we don't shift it off into space.
	uint16_t least_significant_bitmask = 1;	// the least_significant_bitmask			

	for ( ; unsigned_shift_count > 0; unsigned_shift_count--)
	{														
		if (unsigned_number_to_shift & most_significant_bitmask)
		{																	// Checking to see if the most significant bit is set ... bool checks for nonzero. if it is nonzero, true, else if it is 0, it is false.		
			unsigned_number_to_shift <<= 1;									// If it is set, then shift left 1, and set the least significant bit
			unsigned_number_to_shift |= least_significant_bitmask;			// to 1 (b/c they come in as a 0) by bitwise ORing the number to
		}
		else
		{
			unsigned_number_to_shift <<= 1;
		}
	}
	return unsigned_number_to_shift;


	//in regards to rotating right on signed int's:
	// it can retain its signededness  after shifting to the right (aka most significant bit is still set to 1)
	// this could be unexpected since a 0 should be there. so this needs to be accounted for.
}


//uint16_t Raw::rolVersionTwo(uint16_t number, uint16_t shift_count)
//{
//	if ((shift_count &= 31) == 0)
//		return number;
//	return (number << shift_count) | (number >> (32 - shift_count));
//	// This is Bertrand Marron's code from stack overflow. It is much more concise and maybe even faster?
//}


bool Raw::initializeWinsock()
{
#ifdef _WIN32
	if (global_verbose == true)
	{
		std::cout << "Initializing Winsock...\n";
	}
	errchk = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (errchk != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return false;
	}
#endif//_WIN32
	return true;
}


void Raw::setAddress(std::string target_ip, std::string target_port, std::string my_ip, std::string my_host_port)
{
	if (global_verbose == true)
	{
		std::cout << "Setting info: IP address and port...\n";
	}

	// Retrieving all the info that was gathered by CommandLineInput and checked by FormatCheck.cpp
	TargetSockAddrIn.sin_family = AF_INET;
	TargetSockAddrIn.sin_addr.s_addr = inet_addr(target_ip.c_str());
	TargetSockAddrIn.sin_port = htons(atoi( target_port.c_str() ));
	my_host_ip_addr = my_ip;
	my_host_port = my_host_port;

	return;
}

SOCKET Raw::createSocket(int family, int socktype, int protocol)	// Purely optional return value.
{
	if (global_verbose == true)
	{
		std::cout << "Creating RAW Socket...\n";
	}
	// Create a SOCKET handle for connecting to server(no ip address here, that is with bind)
	created_socket = socket(family, socktype, protocol);	// Must be admin / root / SUID 0  in order to open a RAW socket
	if (created_socket == INVALID_SOCKET)
	{
		getError();
		std::cout << "Socket failed.\n";
		closeThisSocket(created_socket);
		myWSACleanup();
		return false;
	}
	return created_socket;
}



// This echo request is going to a dead end IP address at 3.3.3.3
bool Raw::craftFixedICMPEchoRequestPacket()
{
	const char on = 1;
	int on_len = sizeof(int);

	// Tell kernel that we are doing our own IP structures
	errchk = setsockopt(created_socket, IPPROTO_IP, IP_HDRINCL, (char*)&on, on_len);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "setsockopt failed\n";
		closeThisSocket(created_socket);
		myWSACleanup();
		return false;
	}

	size_t ipv4header_size = sizeof(IPV4Header);
	size_t icmpheader_size = sizeof(ICMPHeader);


	// ipv4 Header
	std::string dead_end_ip_addr = "3.3.3.3";

	IPV4Header.ihl = sizeof(IPV4Header) >> 2;							// ihl min value == 5, max == 15; sizeof(IPV4Header) >> 2 is just dividing it by 4 (shifting to the right 2x);
	IPV4Header.ver = 4;													// 4 == ipv4
	IPV4Header.dscp = 0;												// https://en.wikipedia.org/wiki/Differentiated_Services_Code_Point
	IPV4Header.ecn = 0;													// https://en.wikipedia.org/wiki/Explicit_Congestion_Notification
	IPV4Header.total_len = ipv4header_size + icmpheader_size + payload_max_length;
	IPV4Header.id = htons(55155);										//?
	//IPV4Header.flags_and_frag_offset = setFlagsAndFragOffset((uint16_t)0, (uint16_t)0);	// POSSIBLY BROKEN. leave it set to 0.
	IPV4Header.flags = 0;
	IPV4Header.frag_offset = htons(0);
	IPV4Header.ttl = 64;
	IPV4Header.protocol = 1;											// 1 == ICMP
	IPV4Header.chksum = /*datchecksum*/ 0;								// need a checksum
	IPV4Header.src_ip.s_addr = inet_addr( my_host_ip_addr.c_str() );	// needs conversion to big endian
	IPV4Header.dst_ip.s_addr = inet_addr( dead_end_ip_addr.c_str() );	// do not convert to big endian if assigning it something located inside a sockaddr_in structure
																		// TargetSockAddrIn.sin_addr.s_addr;	// this is one that is set to the target instead of a dead end


	// ICMP header														//https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol
	ICMPHeader.type = ICMP_ECHO;
	ICMPHeader.code = ICMP_ECHO_CODE_ZERO;
	ICMPHeader.checksum = htons(~(ICMP_ECHO << 8));						// This trick only works with the most basic icmpheader; no payload. do not use.
	//ICMPHeader.id = htons(12345);
	//ICMPHeader.seq = 1;


	std::string package_for_payload = "OneTwoSkyTree";
	size_t package_for_payload_size = strlen(package_for_payload.c_str());

	// Putting stuff in the payload
	if ((package_for_payload_size <= payload_max_length) && package_for_payload_size > 0)
	{
		memcpy(payload, package_for_payload.c_str(), package_for_payload_size);
	}
	// Copying IPV4Header into the sendbuffer starting at the address of sendbuf (that is sendbuf[0])
	memcpy_s(
		sendbuf, sendbuf_max_length,
		&IPV4Header,
		ipv4header_size
		);
	current_sendbuf_len = ipv4header_size;

	// Copying the ICMPHeader into the sendbuffer starting at sendbuf[current_sendbuf_len]
	memcpy_s(
		sendbuf + current_sendbuf_len,
		sendbuf_max_length - current_sendbuf_len,
		&ICMPHeader,
		icmpheader_size
		);
	current_sendbuf_len += icmpheader_size;

	// Copying payload into the sendbuffer
	memcpy_s(
		sendbuf + current_sendbuf_len,
		sendbuf_max_length - current_sendbuf_len,
		payload,
		package_for_payload_size
		);
	current_sendbuf_len += package_for_payload_size;


	// Output buffer to screen in hex
	if (global_verbose == true)
	{
		std::cout << "Buffer contents:\n";
		for (int i = 0; i < sendbuf_max_length; i++)
		{
			std::cout 
				<< " 0x" 
				<< std::setfill('0') 
				<< std::setw(2) 
				<< std::hex 
				<< (int)(u_char)sendbuf[i];
		}
		std::cout << std::dec;					// Gotta set the stream back to decimal or else it will forever output in hex
	}

	if (global_verbose == true)
	{
		std::cout << "Sizeof iphdr: " << sizeof(IPV4Header) << " \n";
		std::cout << "sizeof icmphdr: " << sizeof(ICMPHeader) << " \n";
		std::cout << "current_sendbuf_len: " << current_sendbuf_len << "\n";
	}
	return true;
}

void Raw::createThreadLoopEchoRequestToDeadEnd(void *instance)
{
#ifdef __linux__

#endif __linux__
#ifdef _WIN32
	if (instance == NULL)
	{
		return;
	}
	Raw* self = (Raw*)instance;
	self->ghEvents2[0] = (HANDLE)_beginthread(loopEchoRequestToDeadEnd, 0, instance);
	return;
#endif//_WIN32
}

// Send EchoRequest to ip: 3.3.3.3 every 3 seconds
void Raw::loopEchoRequestToDeadEnd(void* instance)
{
	if (instance == NULL)
	{
		return;
	}
	Raw* self = (Raw*)instance;		// gotta make sure we are using this thread on the correct object instance!

	CrossPlatformSleep Sleeping;
	while (self->stop_echo_request_loop == false)
	{
		if (self->stop_echo_request_loop == true)
		{
			return;
		}
		self->errchk = self->sendTheThing();
		if (self->errchk == false)
		{
			exit(0);		// hmmmmm not so sure about using exit() yet...
		}
		Sleeping.mySleep(3000);			// milliseconds
	}
	return;
}

bool Raw::sendTheThing()
{
	errchk = sendto(
		created_socket,
		sendbuf,
		current_sendbuf_len,
		0,
		(sockaddr*)&TargetSockAddrIn,
		sizeof(TargetSockAddrIn)
	);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "Sendto failed.\n";
		closeThisSocket(created_socket);
		myWSACleanup();
		return false;
	}
	return true;
}

void Raw::closeThisSocket(SOCKET fd)
{
#ifdef __linux__
	close(fd);
#endif//__linux__

#ifdef _WIN32
	closesocket(fd);
#endif
}

bool Raw::shutdownConnection(SOCKET socket)
{
	std::cout << "Shutting down the connection...\n";
	// shutdown the connection since we're done
	errchk = shutdown(socket, SD_SEND);
	if (errchk == SOCKET_ERROR)
	{
		std::cout << "shutdown failed.\n";
		closeThisSocket(socket);
		myWSACleanup();
		return false;
	}
	return true;
}

void Raw::myWSACleanup()
{
#ifdef _WIN32
	if (global_verbose == true)
	{
		std::cout << "Performing WSACleanup...\n";
	}
	WSACleanup();
#endif//_WIN32
}

// errno needs to be set to 0 before every function call that returns errno  // ERR30-C  /  SEI CERT C Coding Standard https://www.securecoding.cert.org/confluence/pages/viewpage.action?pageId=6619179
void Raw::getError()
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