//connection.cpp
//could i use vectors instead of char arrays for messages?
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
#include <pthread.h>

#include <arpa/inet.h>
#include <signal.h>

#include "connection.h"
#include "GlobalTypeHeader.h"
#endif//__linux__

#ifdef _WIN32
#include "connection.h"
#include "GlobalTypeHeader.h"
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <process.h>
#endif//_WIN32



//NOTE/WARNING/CHANGE: printf is used here, unlike std::cout everywhere else. CHANGE this now that i'm used to using both

//note from MSDN: The Iphlpapi.h header file is required if an application is using the IP Helper APIs. When the Iphlpapi.h header file is required, the #include line for the Winsock2.h header this file should be placed before the #include line for the Iphlpapi.h header file. 
//winsock2.h automatically includes core elements from Windows.h

#pragma comment(lib, "Ws2_32.lib") //tell the linker that Ws2_32.lib file is needed.
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

//#pragma comment(lib, "Kernel32.lib")			//???? is this necessary for GetConsoleScreenBufferInfo?

#ifdef __linux__
#define INVALID_SOCKET	((SOCKET)(~0))
#define SOCKET_ERROR	(-1)
#define SD_SEND         0x01
#endif//__linux__



#ifdef _WIN32
HANDLE connection:: ghEvents[3]{};
#endif//_WIN32

#ifdef __linux__
int connection::ret1 = 0;//server race thread
int connection::ret2 = 0;//client race thread
int connection::ret3 = 0;//send thread
#endif//__linux__



const int connection::NOBODY_WON = -30;
const int connection::SERVER_WON = -7;
const int connection::CLIENT_WON = -8;

//these are out here because they are static(available to all instances of this class). 
const std::string connection::DEFAULT_PORT_TO_LISTEN = "7172";			//currently, set this one to have the server listen on this port
const int connection::DEFAULT_BUFLEN = 512;
const std::string connection::DEFAULT_IP_TO_LISTEN = "192.168.1.116";	//currently, set this one to have the server listen on this IP
int connection::recvbuflen = DEFAULT_BUFLEN;
char connection::recvbuf[DEFAULT_BUFLEN];

int connection::globalWinner = NOBODY_WON;
SOCKET connection::globalSocket = INVALID_SOCKET;

char globalvalue = 9;



connection::connection()
{
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
	inUseSocket = INVALID_SOCKET;

	//string
	target_ip_addr = "";
	target_port = DEFAULT_PORT_TO_LISTEN;
	my_ip_addr = DEFAULT_IP_TO_LISTEN;
	my_port = DEFAULT_PORT_TO_LISTEN;

	//int
	iResult = 0;
	errchk = 0;
	iSendResult = 0;
#ifdef __linux__
	pthread_t thread1, thread2, thread3;
#endif//__linux__
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
	fd_set fdSet;
		//*PfdSet = &fdSet;			    //UNUSED
	memset(&fdSet, 0, sizeof(fdSet));
	timeval timeValue,
			*PtimeValue;
	PtimeValue = &timeValue;
	memset(&timeValue, 0, sizeof(timeValue));
	//ZeroMemory(&timeValue, sizeof(timeValue));
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
				//or should i exit this thread, then accept the connection in the main thread?
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

	while (self->globalWinner != SERVER_WON && self->globalWinner != CLIENT_WON) 
	{
		int iFeedback;
		if (global_verbose == true)
			std::cout << "CTHREAD :: ";
		if ((iFeedback = self->connectToTarget()) == 0) return; //1;	//creates a socket then tries to connect to that socket. this func returns the socket id as an int									
		else if (iFeedback == 1) {//if it is just unable to connect, but doesn't error, then continue trying to connect.
			if (self->globalWinner == SERVER_WON){
				if (global_verbose == true) {
					std::cout << "CTHREAD :: ";
					std::cout << "Client established a connection, but the server won the race.\n";
					std::cout << "CTHREAD :: ";
					std::cout << "Closing connected socket.\n";
				}
				self->closeThisSocket(iFeedback);
				return;
			}
			self->mySleep(1000);
			continue;
		}
		else{	//connection is established, the client has won the competition.
			if (self->globalWinner != SERVER_WON){	//make sure the server hasn't already won during the time it took to attempt a connection
				std::cout << "CTHREAD :: ";
				std::cout << "setting global values.\n";
				self->globalSocket = iFeedback;
				self->globalWinner = CLIENT_WON;
				if (global_verbose == true) {
					std::cout << "CTHREAD :: " << self->globalWinner << " is the winner.\n";
					std::cout << "CTHREAD :: " << self->globalSocket << " is the socket now.\n";
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
	errchk = getaddrinfo(my_ip_addr.c_str(), my_port.c_str(), &hints, &result);
	if (errchk != 0){
		std::cout << "STHREAD >> ";
		printf("getaddrinfo failed with error: %d\n", errchk);
		cleanup();
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
		cleanup();
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
		freeaddrinfo(result);
		closeThisSocket(ListenSocket);
		cleanup();
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
		freeaddrinfo(result);
		closeThisSocket(ListenSocket);
		cleanup();
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
		freeaddrinfo(result);
		closeThisSocket(ConnectSocket);
		cleanup();
		return false;
	}
	PclientSockaddr_in = (sockaddr_in *)ptr->ai_addr;
	void *voidAddr;
	char ipstr[INET_ADDRSTRLEN];
	voidAddr = &(PclientSockaddr_in->sin_addr);
	inet_ntop(ptr->ai_family, voidAddr, ipstr, sizeof(ipstr));
	//InetNtop(ptr->ai_family, voidAddr, ipstr, sizeof(ipstr));		//windows only
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
		cleanup();
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
		cleanup();
		return false;
	}
	if (global_verbose == true)
		std::cout << "Connected to " << ":" << "\n";
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
				cleanup();
				return false;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			std::cout << "recv failed.\n";
			closeThisSocket(AcceptedSocket);
			cleanup();
			return false;
		}
	} while (iResult > 0);

	std::cout << "Ending receive loop...\n";
	return true;
}

bool connection::receiveUntilShutdown()
{
	if (AcceptedSocket != INVALID_SOCKET)
		inUseSocket = AcceptedSocket;
	else
		inUseSocket = ConnectSocket;
	const char* sendbuf = "First message sent.";
	std::string message_to_send = "Automated message sent from recv func.\n";

	//send this message once
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	iResult = send(inUseSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
	if (iResult == SOCKET_ERROR){
		getError();
		std::cout << "send failed.\n";
		closeThisSocket(inUseSocket);
		cleanup();
		//return false;
	}
	printf("Message sent: %s\n", sendbuf);
	if (global_verbose == true)
		printf("Bytes Sent: %ld\n", iResult);
	if (global_verbose == true)
		std::cout << "Recv loop started...\n";

	// Receive until the peer shuts down the connection
	do {
		iResult = recv(inUseSocket, recvbuf, recvbuflen, 0);
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
			closeThisSocket(inUseSocket);
			cleanup();
			return false;
		}
	} while (iResult > 0);
	return true;
}

bool connection::shutdownConnection()
{
	std::cout << "Shutting down the connection...\n";
	// shutdown the connection since we're done
	errchk = shutdown(AcceptedSocket, SD_SEND);
	if (errchk == SOCKET_ERROR) {
		std::cout << "shutdown failed.\n";
		closeThisSocket(AcceptedSocket);
		cleanup();
		return false;
	}
	return true;
}

// cleanup
void connection::cleanup()//WHEN BIND FAILS, and it tries to call freeaddrinfo in here, unkown error occurs.
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
	my_ip_addr = user_defined_ip_address;
	my_port = user_defined_port;
	if (global_verbose == true) {
		std::cout << "User defined IP to connect to:   " << target_ip_addr << "\n";
		std::cout << "User defined port to connect to: " << target_port << "\n";
	}
	//std::stoi(port);//convert string to int
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
	ghEvents[0] = (HANDLE)_beginthread(serverCompetitionThread, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
#endif//__WIN32
}

void connection::createClientRaceThread(void* instance)
{
#ifdef __linux__
	ret2 = pthread_create(&thread2, NULL, PosixThreadEntranceClientCompetitionThread, instance);
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
		sendThread(instance);
		pthread_exit(&ret3);
#endif//__linux__

#ifdef _WIN32
		ghEvents[2] = (HANDLE)_beginthread(sendThread, 0, instance);
#endif//_WIN32
}

void connection::sendThread(void* instance)
{
	connection* self = (connection*)instance;
	int intResult;
	const char* sendbuf = "First message sent.";
	std::string message_to_send = "Automated message from SendThread.";

	if (self->AcceptedSocket != INVALID_SOCKET)
		self->inUseSocket = self->AcceptedSocket;
	else
		self->inUseSocket = self->ConnectSocket;

	//send this message once
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	intResult = send(self->inUseSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
	if (intResult == SOCKET_ERROR){
		self->getError();
		std::cout << "send failed.\n";
		self->closeThisSocket(self->inUseSocket);
		self->cleanup();
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
		intResult = send(self->inUseSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
		if (intResult == SOCKET_ERROR){
			self->getError();
			std::cout << "send failed.\n";
			self->closeThisSocket(self->inUseSocket);
			self->cleanup();
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

//milliseconds only. xplatform windows & linux
void connection::mySleep(int number_in_ms)
{
#ifdef __linux___
	usleep(number_in_ms * 1000);		// normally 1 millionth of a sec, times it by 1000 to make it ms
#endif//__linux__
#ifdef _WIN32
	Sleep(number_in_ms);				//in milliseconds
#endif//_WIN32
}

	//have the buffer[0] be a flag, the rest a message.
	//while loop
	//checkfor msg
	//send msg if one has been put into the buffer
	//how do i continue a while loop for receiving information when the client is waiting for user input (effectively stopping the while loop b/c of getline)?
	//multithreading or message loop. could message loop lock up for too long in a particular spot so to speak if given a big task on part of the loop?
	//or what about, check for a carriage return or a enter key button press in the windows buffer or handler or whatever it is?, and when the windows handle receives a carriage return it sends the whole line before the carriage return into a getline(vectorhere)?
	//is there need to sync threads?