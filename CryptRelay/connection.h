//connection.h
#pragma once
#include <string>
#include <WinSock2.h>


class connection
{
public://anyone aware of the class connection will also be aware of these members
	connection();
	~connection();
	//static void connection::srv_snd_thread(void* number);
	struct sockaddr serverAddrRetreive,
					*PserverAddrRetreive;
	struct sockaddr_in serverAcceptedAddr,
					   *PserverAcceptedAddr;

	struct sockaddr clientSockaddr,//to put something in here, use the _in variation of the struct
					*PclientSockaddr;
	struct sockaddr_in clientSockaddr_in,//currently storing ip address and port in this
				  	*PclientSockaddr_in;

	struct sockaddr_storage incomingAddr,
							*PincomingAddr;

	struct addrinfo hints,
					*result,
					*ptr;
	static void serverCompetitionThread(void* instance);
	static void clientCompetitionThread(void* instance);
	bool initializeWinsock();
	void ServerSetHints();
	void ClientSetHints();
	bool createSocket();
	bool serverGetAddress();
	bool clientGetAddress();
	bool bindToListeningSocket();
	void putSocketInFdSet();
	int connectToTarget();
	bool listenToListeningSocket();
	int acceptClient();
	void closeTheListeningSocket();
	bool echoReceiveUntilShutdown();
	bool clientReceiveUntilShutdown();
	bool shutdownConnection();
	void cleanup();

	static void clientSendThread(void* instance);

	void mySleep(int number_in_ms);


	//static void clientReceiveThread(void* instance);
	static void clientReceive(void *instance);
	//void clientSendThread(void* instance);
	//static void clientSend(void* instance);
	//bool establish_connection_client(std::string targetIPaddress, std::string port);
	bool setIpAndPort(std::string targetIPaddress, std::string port);

	static SOCKET globalSocket;
	static int globalWinner;

protected://only the children and their children are aware of these members

private://no one but class connection is aware of these members
	SOCKET ListenSocket;//since it is not static, every instance has its own ConnectSocket
	SOCKET ConnectSocket;
	SOCKET AcceptedSocket;
	SOCKET inUseSocket;
	WSADATA wsaData;

	std::string targetIPaddr;
	std::string userPort;
	//std::string *strIpPtr;
	//std::string *strPortPtr;
	static const std::string DEFAULT_PORT;
	static const int DEFAULT_BUFLEN;	//static means shared by all instances. constant makes it not changeable by anything or anyone.
	static const std::string DEFAULT_IP;
	int iResult;
	int errchk;
	int iSendResult;
	//int iUserPort;
	int  addr_size;
	static int recvbuflen;
	static char recvbuf[]; //has to be static or else intellisense says: incomplete type not allowed. also for recv to work.


};


