//connection.h
#ifndef connection_h__
#define connection_h__
#include <string>
#include <WinSock2.h>

class connection
{
public:	//anyone aware of the class connection will also be aware of these members
	connection();
	~connection();

	static const int NOBODY_WON;
	static const int SERVER_WON;
	static const int CLIENT_WON;
	static const std::string DEFAULT_PORT_TO_LISTEN;
	static const std::string DEFAULT_IP_TO_LISTEN;

	struct sockaddr_in *PclientSockaddr_in;			//currently storing ip address and port in this
	struct sockaddr_storage incomingAddr;
	struct addrinfo hints,
					*result,
					*ptr;

	bool clientSetIpAndPort(std::string user_defined_ip_address, std::string user_defined_port);
	bool serverSetIpAndPort(std::string user_defined_ip_address, std::string user_defined_port);
	static void serverCompetitionThread(void* instance);
	static void clientCompetitionThread(void* instance);
	static void sendThread(void* instance);
	bool initializeWinsock();
	void ServerSetHints();
	void ClientSetHints();
	bool createSocket();
	bool serverGetAddress();
	bool clientGetAddress();
	bool bindToListeningSocket();
	void putSocketInFdSet();						//STATUS: NOT USED, might have something somewhere I need to put into it
	int connectToTarget();
	bool listenToListeningSocket();
	int acceptClient();
	void closeTheListeningSocket();
	bool echoReceiveUntilShutdown();				//STATUS: NOT USED
	bool receiveUntilShutdown();
	bool shutdownConnection();
	void cleanup();
	void mySleep(int number_in_ms);

	static SOCKET globalSocket;
	static int globalWinner;

protected:	//only the children and their children are aware of these members

private:	//no one but class connection is aware of these members
	SOCKET ListenSocket;	
	SOCKET ConnectSocket;							//since it is not static, every instance has its own ConnectSocket
	SOCKET AcceptedSocket;
	SOCKET inUseSocket;
	WSADATA wsaData;

	std::string target_ip_addr;
	std::string target_port;
	std::string my_ip_addr;
	std::string my_port;

	static const int DEFAULT_BUFLEN;				//static means shared by all instances.

	int iResult;
	int errchk;
	int iSendResult;
	int addr_size;
	static int recvbuflen;
	static char recvbuf[];							//has to be static or else intellisense says: incomplete type not allowed. also for recv to work.
};

#endif