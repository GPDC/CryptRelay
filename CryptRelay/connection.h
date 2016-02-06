//connection.h
#ifndef connection_h__
#define connection_h__

#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#endif//__linux__

#ifdef _WIN32			//Linux equivalent:
#include <WinSock2.h>	//<sys/socket.h>
#include <string>
#endif//_WIN32

#ifdef __linux__
typedef u_int SOCKET;
#endif



class connection
{
public:	//anyone aware of the class connection will also be aware of these members
	connection();
	~connection();

#ifdef _WIN32
	static HANDLE ghEvents[3];
#endif//_WIN32

#ifdef __linux__
	static pthread_t thread1, thread2, thread3;
	static int ret1, ret2, ret3;
#endif//__linux__

	static const int NOBODY_WON;
	static const int SERVER_WON;
	static const int CLIENT_WON;
	static const std::string DEFAULT_PORT_TO_LISTEN;
	static const std::string DEFAULT_IP_TO_LISTEN;

	sockaddr_in *PclientSockaddr_in;			//currently storing ip address and port in this
	sockaddr_storage incomingAddr;
	addrinfo hints,
					*result,
					*ptr;

	bool clientSetIpAndPort(std::string user_defined_ip_address, std::string user_defined_port);
	bool serverSetIpAndPort(std::string user_defined_ip_address, std::string user_defined_port);
#ifdef __linux__
	static void* PosixThreadEntranceServerCompetitionThread(void* instance);
	static void* PosixThreadEntranceClientCompetitionThread(void* instance);
	static void* PosixThreadSendThread(void* instance);
#endif//__linux__
	static void serverCompetitionThread(void* instance);
	static void clientCompetitionThread(void* instance);
	static void sendThread(void* instance);
	void createClientRaceThread(void* instance);
	void createServerRaceThread(void* instance);
	void serverCreateSendThread(void* instance);
	void clientCreateSendThread(void* instance);
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
	void closeThisSocket(SOCKET fd);
	bool echoReceiveUntilShutdown();				//STATUS: NOT USED
	bool receiveUntilShutdown();
	bool shutdownConnection();
	void cleanup();
	void getError();
	void mySleep(int number_in_ms);

	static SOCKET globalSocket;
	static int globalWinner;

protected:	//only the children and their children are aware of these members

private:	//no one but class connection is aware of these members
	SOCKET ListenSocket;	
	SOCKET ConnectSocket;							//since it is not static, every instance has its own ConnectSocket
	SOCKET AcceptedSocket;
	SOCKET inUseSocket;

#ifdef _WIN32
	WSADATA wsaData;
#endif

	std::string target_ip_addr;
	std::string target_port;
	std::string my_ip_addr;
	std::string my_port;

	static const int DEFAULT_BUFLEN;				//static means shared by all instances.

	int iResult;
	int errchk;
	int iSendResult;
	static int recvbuflen;
	static char recvbuf[];							//has to be static or else intellisense says: incomplete type not allowed. also for recv to work.
};

#endif