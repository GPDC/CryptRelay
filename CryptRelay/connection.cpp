//could i use vectors instead of char arrays for messages?
//need to listen over 1 tcp connection NOT 2.
#undef UNICODE
#ifdef __linux__
#include <unistd.h>
#endif

#ifdef _WIN32
#include "connection.h"
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <process.h>		//for multithreading _beginthread(,,,);, and _endthread();
#endif



//NOTE/WARNING: printf is used here, unlike std::cout everywhere else.

//note from MSDN: The Iphlpapi.h header file is required if an application is using the IP Helper APIs. When the Iphlpapi.h header file is required, the #include line for the Winsock2.h header this file should be placed before the #include line for the Iphlpapi.h header file. 
//winsock2.h automatically includes core elements from Windows.h

#pragma comment(lib, "Ws2_32.lib") //tell the linker that Ws2_32.lib file is needed.
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

//???? is this necessary for GetConsoleScreenBufferInfo?
//#pragma comment(lib, "Kernel32.lib")

//these are out here because they are static(available to all instances of this class). 
const std::string connection::DEFAULT_PORT = "7172"; //the c-style way is: #define DEFAULT_PORT "4242"
const int connection::DEFAULT_BUFLEN = 512;
const std::string connection::DEFAULT_IP = "192.168.1.116";	//currently, set this one to have the server listen on this IP
int connection::recvbuflen = DEFAULT_BUFLEN;
char connection::recvbuf[DEFAULT_BUFLEN];

int connection::globalWinner = -30;
SOCKET connection::globalSocket = INVALID_SOCKET;

char globalvalue = 9;
#define SERVER_WON -7
#define CLIENT_WON -8

connection::connection()
{
	ZeroMemory(&hints, sizeof(hints));//fill the addrinfo instance called 'hints' with 0's!
	ZeroMemory(&clientSockaddr_in, sizeof(clientSockaddr_in));
	ZeroMemory(&clientSockaddr, sizeof(clientSockaddr));
	ZeroMemory(&serverAddrRetreive, sizeof(serverAddrRetreive));
	ZeroMemory(&serverAcceptedAddr, sizeof(serverAcceptedAddr));
	//SOCKET  (just a typedef. on a 32 bit windows it is an unsigned int. 64 bit windows it is an unsigned __int64 ...fun fact :D)
	ListenSocket = INVALID_SOCKET;
	ConnectSocket = INVALID_SOCKET;
	AcceptedSocket = INVALID_SOCKET;
	inUseSocket = INVALID_SOCKET;

	//struct pointers for addrinfo
	PserverAddrRetreive = &serverAddrRetreive;
	PserverAcceptedAddr = &serverAcceptedAddr;
	PclientSockaddr = nullptr;
	PclientSockaddr_in = nullptr;

	result = nullptr;
	ptr = nullptr;

	//string
	targetIPaddr = "";
	userPort = DEFAULT_PORT;
	//strIpPtr = &targetIPaddr;
	//strPortPtr = &userPort;

	//int
	iResult = 0;
	errchk = 0;
	iSendResult = 0;
}
connection::~connection()
{

}

void connection::serverCompetitionThread(void* instance)
{
	if (instance == NULL)
		return;

	connection *self = (connection*)instance;

	fd_set fdSet,
		*PfdSet = &fdSet;

		  

	timeval timeValue,
			*PtimeValue;
	PtimeValue = &timeValue;
	ZeroMemory(&timeValue, sizeof(timeValue));
	timeValue.tv_usec = 500000; // 1 million microseconds = 1 second
	int iFeedback;
	std::cout << "STHREAD >> ";
	if (self->serverGetAddress() == false) return; //1;			//puts the local port info in the addrinfo structure
	std::cout << "STHREAD >> ";
	if (self->createSocket() == false) return; //1;				//creates listening socket to listen on that local port

	fdSet.fd_count = 1;
	fdSet.fd_array[0] = self->ListenSocket;
	std::cout << "STHREAD >> ";
	std::cout << "SOCKET in fd_array: " << fdSet.fd_array[0] << "\n";
	std::cout << "STHREAD >> ";
	std::cout << "SOCKET ListenSocket: " << self->ListenSocket << "\n";

	std::cout << "STHREAD >> ";
	if (self->bindToListeningSocket() == false) return; //1;		//assigns a socket to an address
	self->putSocketInFdSet();//put socket in fd_set 
	std::cout << "STHREAD >> ";
	if (self->listenToListeningSocket() == false) return; //1;	//thread waits here until someone connects or it errors.
	while (1)
	{
		fdSet.fd_count = 1;
		fdSet.fd_array[0] = self->ListenSocket;
		iFeedback = select(NULL, PfdSet, NULL, NULL, PtimeValue);//select returns the number of handles that are ready and contained in the fd_set structures.
		//std::cout << "STHREAD >> currently my value for the winner is: " << self->globalWinner << "\n";
		if (iFeedback == SOCKET_ERROR)
		{
			std::cout << "STHREAD >> ";
			std::cout << "ServerCompetitionThread select Error: " << WSAGetLastError() << "\n";
			closesocket(self->ListenSocket);
			std::cout << "STHREAD >> ";
			std::cout << "Closing listening socket b/c of error. Ending Server Thread.\n";
			_endthread();
		}
		else if (self->globalWinner == CLIENT_WON)
		{
			closesocket(self->ListenSocket);
			std::cout << "STHREAD >> ";
			std::cout << "Closed listening socket, because the winner is: " << self->globalWinner << ". Ending Server thread.\n";
			_endthread();
		}
		else if (iFeedback > 0)
		{
			std::cout << "STHREAD >> ";
			std::cout << "attempting to accept a client now that select() returned a readable socket\n";
			//or should i exit this thread, then accept the connection in the main thread?
			std::cout << "STHREAD >> ";
			if ((iFeedback = self->acceptClient()) == false) return; //1;				//creates a new socket to communicate with the person on
			//else
			std::cout << "STHREAD >> ";
			std::cout << "accept must have been successful, setting globalwinner and globalsocket\n";
				self->globalSocket = iFeedback;
				self->globalWinner = SERVER_WON;
				closesocket(self->ListenSocket);
				_endthread();
				return;
		}
	}
	//if ((iFeedback = self->acceptClient()) == false) return; //1;				//creates a new socket to communicate with the person on
	//thesocket = iFeedback;
	//globalvalue = SERVER_WON;
	//return;
}

void connection::clientCompetitionThread(void* instance)
{
	connection *self = (connection*)instance;
	std::cout << "CTHREAD :: ";
	if (self->clientGetAddress() == false) return; //1;		//puts the local port info in the addrinfo structure

	while (self->globalWinner != SERVER_WON && self->globalWinner != CLIENT_WON) 
	{
		int iFeedback;
		
		std::cout << "CTHREAD :: ";
		if ((iFeedback = self->connectToTarget()) == false) return; //1;	//creates a socket then tries to connect to that socket. this func returns the socket id as an int									
		else if (iFeedback == true) {//if it is just unable to connect, but doesn't error, then continue trying to connect.
			self->mySleep(1000);
			continue;
		}
		else	//connection is established, the client has won the competition.
			std::cout << "CTHREAD :: ";
		std::cout << "setting global values.\n";
			self->globalSocket = iFeedback;
			self->globalWinner = CLIENT_WON;
			std::cout << "CTHREAD :: " << self->globalWinner << " is the winner.\n";
			std::cout << "CTHREAD :: " << self->globalSocket << " is the socket now.\n";
			_endthread();
			return;
		return;
	}
	_endthread();
	return;
}

bool connection::initializeWinsock()
{
	std::cout << "Initializing Winsock...\n";
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return false;
	}
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
	std::cout << "Retreiving info: IP address and port...\n";
	// Resolve the LOCAL server address and port.
	errchk = getaddrinfo(DEFAULT_IP.c_str(), DEFAULT_PORT.c_str(), &hints, &result);
	if (errchk != 0)
	{
		printf("getaddrinfo failed with error: %d\n", errchk);
		WSACleanup();	//terminate the use of WS2_32 DLL
		return false;
	}
	return true;
}

bool connection::clientGetAddress()
{
	std::cout << "Retreiving info: IP address and port...\n";
	// Resolve the LOCAL server address and port.
	errchk = getaddrinfo(targetIPaddr.c_str(), userPort.c_str(), &hints, &result);
	if (errchk != 0)
	{
		printf("getaddrinfo failed with error: %d\n", errchk);
		WSACleanup();	//terminate the use of WS2_32 DLL
		return false;
	}
	return true;
	//std::cout << "IP: " << hints.ai_addr << ". Port: " << hints.sockaddr->ai_addr\n";
}

bool connection::createSocket()
{
	std::cout << "Creating Socket to listen on...\n";
	// Create a SOCKET handle for connecting to server(no ip address here, that is with bind)
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}
	return true;
}

bool connection::bindToListeningSocket()//my local ip address and port
{
	std::cout << "Binding ... associating local address with the listen socket...\n";
	// Setup the TCP listening socket (putting ip address on the allocated socket)
	errchk = bind(ListenSocket, result->ai_addr, result->ai_addrlen);
	if (errchk == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}
	freeaddrinfo(result);   //shouldn't need the info gathered by getaddrinfo now that bind has been called
	return true;
}

void connection::putSocketInFdSet()
{
	return;
}

int connection::connectToTarget()
{
																	//test
	std::cout << "Attempting to connect to someone...\n";					//test
	//attempt to connect to the first address returned by the call to getaddrinfo
	ptr = result;

	//create a SOCKET for connecting to server.
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	
	//catch any errors that may have occured while creating the socket / check for errors to make sure the socket is valid
	if (ConnectSocket == INVALID_SOCKET)
	{
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}
	PclientSockaddr_in = (sockaddr_in *)ptr->ai_addr;
	void *voidAddr;
	char ipstr[INET_ADDRSTRLEN];
	voidAddr = &(PclientSockaddr_in->sin_addr);
	InetNtop(ptr->ai_family, voidAddr, ipstr, sizeof(ipstr));
	std::cout << "CTHREAD :: Trying to connect to: " << ipstr << "\n";
	//connect to server
	errchk = connect(ConnectSocket, ptr->ai_addr, ptr->ai_addrlen);
	if (errchk == SOCKET_ERROR)
	{
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}
	//else
		//std::cout << "Sucessfully connected to: " << (sockaddr)serverStoreAddr.sin_addr << ":";	

	//freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("CTHREAD :: Unable to connect to server!\n");
		closesocket(ConnectSocket);
		return true;
	}
	else
	{
		std::cout << "Connection established.\n";
		std::cout << "Using socket ID: " << ConnectSocket << "\n";
		return ConnectSocket;
	}
}

bool connection::listenToListeningSocket()
{
	std::cout << "Listening...\n";
	errchk = listen(ListenSocket, SOMAXCONN);
	if (errchk == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}
	return true;
}

//always listening... need to make it connect sometimes too? try connect first, if it can't conncet, just listen?
int connection::acceptClient()
{
	char incoming_accepted_ipaddress[50];
	addr_size = sizeof(incomingAddr);
	std::cout << "Waiting for someone to connect.\n";
	// Accept a client socket by listening on: ListenSocket. Storing accepted addr at: serverAcceptedAddr.
	AcceptedSocket = accept(ListenSocket, (sockaddr*)&incomingAddr, &addr_size);
	if (AcceptedSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}
	std::cout << "Connected to " << ":" << "\n";
	std::cout << "Connected to someone on socket ID: " << AcceptedSocket << "\n";
	return AcceptedSocket;
}

void connection::closeTheListeningSocket()
{
	std::cout << "Closing the listening socket...\n";
	closesocket(ListenSocket);
}

bool connection::echoReceiveUntilShutdown()
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
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(AcceptedSocket);
				WSACleanup();
				return false;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(AcceptedSocket);
			WSACleanup();
			return false;
		}
	} while (iResult > 0);

	std::cout << "Ending receive loop...\n";
	return true;
}
bool connection::clientReceiveUntilShutdown()
{
	if (AcceptedSocket != INVALID_SOCKET)
	{
		inUseSocket = AcceptedSocket;
	}
	else
	{
		inUseSocket = ConnectSocket;
	}
	const char* sendbuf = "First message sent.";
	std::string message_to_send = "First message sent from recv.\n";

	//send this message once
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	iResult = send(inUseSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(inUseSocket);
		WSACleanup();
		//return false;
	}
	printf("Message sent: %s\n", sendbuf);
	printf("Bytes Sent: %ld\n", iResult);

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
				std::cout << "GetConsoleScreenBufferInfo failed: " << WSAGetLastError() << "\n";
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
			printf("Bytes recvd: %d\n", iResult);						//this just prints out on the server's console the amount of bytes received.
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(inUseSocket);
			WSACleanup();
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
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(AcceptedSocket);
		WSACleanup();
		return false;
	}
	return true;
}

// cleanup
void connection::cleanup()
{
	std::cout << "Cleaning up. Freeing addrinfo...\n";
	freeaddrinfo(result);									//frees information gathered that the getaddrinfo() dynamically allocated into addrinfo structures
	closesocket(AcceptedSocket);
	WSACleanup();
}







//make sure void* instance is of type connection. (the class)
//void connection::clientReceiveThread(void* instance)
//{
//	while (clientReceive(instance)); //as long as it returns true
//}

void connection::clientReceive(void *instance)
{
	int iResult;
	if (instance == NULL)
		return;
	

	//class instance pointer called self is assigned the value of the thing pointed to by instance. instance is a structure/class pointer
	connection* self = (connection*)instance;	//c style cast. now must use ->'s instead of * because self is a POINTER to a class(or struct)

												//receive messages from the targetipaddress
	if (self->AcceptedSocket != INVALID_SOCKET)
	{
		self->inUseSocket = self->AcceptedSocket;
	}
	else
	{
		self->inUseSocket = self->ConnectSocket;
	}

	std::cout << "inUseSocket is currently value: " << self->inUseSocket << "\n";
	do {

		iResult = recv(self->inUseSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			printf("Message received: ");
			//printing the message that is received
			for (int i = 0; i < iResult; i++)
			{
				std::cout << recvbuf[i];
				//at the end of the message, go to a new line.
				if (i == iResult - 1)
				{
					std::cout << "\n";
				}
			}
			printf("Bytes received: %d\n", iResult);
		}
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed: %d\n", WSAGetLastError());

	} while (iResult > 0);
	return;
	//_endthread();
}









//make sure void* instance is of type connection. (the class)
/*
void connection::clientSendThread(void* instance)
{
	HANDLE ghEvents[1];
	ghEvents[0] = (HANDLE)_beginthread(clientSend, 0, this);	//c style typecast    from: uintptr_t    to: HANDLE.
	clientSend(instance);
}

void connection::clientSend(void* instance)
{
	int intResult;
	if (instance == NULL)
		return;
	connection* self = (connection*)instance;	//c style cast. now must use ->'s instead of * because self is a pointer to a class(or struct)

	//make it so client and server can use this function
	if (self->AcceptedSocket != INVALID_SOCKET)
	{
		self->inUseSocket = self->AcceptedSocket;
	}
	else
	{
		self->inUseSocket = self->ConnectSocket;
	}

	const char* sendbuf = "First message sent.";
	std::string message_to_send = "empty_message: user had no input.";

	//send this message once
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	intResult = send(self->inUseSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
	if (intResult == SOCKET_ERROR)
	{
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(self->inUseSocket);
		WSACleanup();
		//return false;
	}
	printf("Message sent: %s\n", sendbuf);
	printf("Bytes Sent: %ld\n", intResult);

	//ask & send the user's input the rest of the time.
	while (1)
	{
		std::getline(std::cin, message_to_send);
		sendbuf = message_to_send.c_str();	//c_str converts from string to char *
		intResult = send(self->inUseSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
		if (intResult == SOCKET_ERROR)
		{
			printf("send failed: %d\n", WSAGetLastError());
			closesocket(self->inUseSocket);
			WSACleanup();
			return;//return 1
		}
		printf("Message sent: %s\n", sendbuf);
		printf("Bytes Sent: %ld\n", intResult);
	}
	//_endthread();
}
*/



bool connection::setIpAndPort(std::string targetIPaddress, std::string port)
{
	//HANDLE ghEvents[2];

	targetIPaddr = targetIPaddress;
	std::cout << "DEFAULT_PORT: " << DEFAULT_PORT << "\n";
	userPort = port;
	std::cout << "USER_PORT: " << userPort << "\n";
	//iUserPort = std::stoi(port);//convert string to int
	return false;
}


void connection::clientSendThread(void* instance)
{
	connection* self = (connection*)instance;
	int intResult;
	const char* sendbuf = "First message sent.";
	std::string message_to_send = "First message from SendThread.";

	if (self->AcceptedSocket != INVALID_SOCKET)
	{
		self->inUseSocket = self->AcceptedSocket;
	}
	else
	{
		self->inUseSocket = self->ConnectSocket;
	}

	//send this message once
	sendbuf = message_to_send.c_str();	//c_str converts from string to char *
	intResult = send(self->inUseSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
	if (intResult == SOCKET_ERROR)
	{
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(self->inUseSocket);
		WSACleanup();
		//return false;
	}
	printf("Message sent: %s\n", sendbuf);
	printf("Bytes Sent: %ld\n", intResult);

	//ask & send the user's input the rest of the time.
	while (1)
	{
		std::getline(std::cin, message_to_send);
		sendbuf = message_to_send.c_str();	//c_str converts from string to char *
		intResult = send(self->inUseSocket, sendbuf, (int)strlen(sendbuf), 0);	//sendbuf is the message being sent. send() returns the total number of bytes sent
		if (intResult == SOCKET_ERROR)
		{
			printf("send failed: %d\n", WSAGetLastError());
			closesocket(self->inUseSocket);
			WSACleanup();
			return;//return 1
		}
		printf("Message sent: %s\n", sendbuf);
		printf("Bytes Sent: %ld\n", intResult);
	}
	//_endthread();
}

//milliseconds only. xplatform windows & linux
void connection::mySleep(int number_in_ms)
{
#ifdef __linux___
	usleep(number_in_ms * 1000);		// normally 1 millionth of a sec, times it by 1000 to make it ms
#endif
#ifdef _WIN32MySleep
	Sleep(number_in_ms);				//in milliseconds
#endif
}











//client source code
/*
bool connection::establish_connection_client(std::string targetIPaddress, std::string port)
{
	targetIPaddr = targetIPaddress;
	userPort = port;

	const char *sendbuf = "this is a test.";	//this is the msg sent.

	// Initialize Winsock 
	//MSDN: After initialization, a SOCKET object must be instantiated for use by the client.
	//initiate use of WS2_32.dll
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);	//MSDN: The MAKEWORD(2,2) parameter of WSAStartup makes a request for version 2.2 of Winsock on the system, and sets the passed version as the highest version of Windows Sockets support that the caller can use.
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}
	
	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;



	//send an initial buffer
//	_beginthread(clientSendThread, 0, this);
	
	//start listening / receiving thread. Passing the instance we are in to client_rcv_thread (aka: this)
	_beginthread(clientReceiveThread, 0, this);//this    is a pointer to the instance we are in.




	//have the buffer[0] be a flag, the rest a message.
	//while loop
	//checkfor msg
	//send msg if one has been put into the buffer
	//how do i continue a while loop for receiving information when the client is waiting for user input (effectively stopping the while loop b/c of getline)?
	//multithreading or message loop. could message loop lock up for too long in a particular spot so to speak if given a big task on part of the loop?
	//or what about, check for a carriage return or a enter key button press in the windows buffer or handler or whatever it is?, and when the windows handle receives a carriage return it sends the whole line before the carriage return into a getline(vectorhere)?
	//is there need to sync threads?


	return false;
	
}
*/