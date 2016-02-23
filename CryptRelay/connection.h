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

	sockaddr_in		 *PclientSockaddr_in,			//currently storing ip address and port in this
					 UDPSockaddr_in;
	sockaddr_storage incomingAddr;
	addrinfo		 hints,
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

	//void UDPSpamSetHints();
	//bool UDPSpamGetAddr();
	//int UDPSpamCreateSocket();
	//bool UDPSpamPortsWithSendTo();

	static SOCKET globalSocket;
	static int globalWinner;

protected:	//only the children and their children are aware of these members

private:	//no one but class connection is aware of these members
	SOCKET ListenSocket;	
	SOCKET ConnectSocket;							//since it is NOT static, every instance has its own ConnectSocket
	SOCKET AcceptedSocket;
	SOCKET UDPSpamSocket;

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



/*********** Raw Class ***********/
//notes about RAW sockets https://msdn.microsoft.com/en-us/library/windows/desktop/ms740548%28v=vs.85%29.aspx
class Raw
{
public:
	Raw();
	~Raw();

	bool isLittleEndian();
	bool initializeWinsock();
	SOCKET createSocket(int, int, int);
	bool craftFixedICMPEchoRequestPacket();
	bool sendTheThing();
	bool GetAddress(std::string ip = connection::DEFAULT_IP_TO_LISTEN, std::string port = connection::DEFAULT_PORT_TO_LISTEN);
	void closeThisSocket(SOCKET fd);
	bool shutdownConnection(SOCKET socket);
	void cleanup();
	void getError();

	//addrinfo Hints;
	//addrinfo *PResult;
	//addrinfo *PPtr;
//	sockaddr_storage StorageHints;			// Use this instead of sockaddr b/c it can hold ipv6. cast to sockaddr_in to put stuff in it.
	sockaddr_in SockIn;
	sockaddr_in *PResultSockIn;
protected:
private:

#ifdef _WIN32
	WSADATA wsaData;
#endif

#ifdef _WIN32
	// IP header information stored here

	// Type 11 is time exceeded ICMP, 8 is echo request
	struct iphdr {
		/*
		u_char	oihl : 4;								// Internet_header_length
		u_char	oversion : 4;							// 
		u_char	otos;									// differentiated_services_code_point (originally called Type of Service(ToS) ).
		u_short	otot_len;
		u_short	oid;
		u_short	ofrag_off;
		u_char	ottl;
		u_char	oprotocol;
		u_short	ocheck;
		u_int	osaddr;
		u_int	odaddr;
		*/

		// Mine are below			// https://en.wikipedia.org/wiki/IPv4#Header
		// Warning: little endian ihl comes first, ver comes second. big endian the ver comes first, ihl comes second.
		u_char ihl : 4;				// Internet Header Length. The second field (4 bits) is the number of 32-bit words in the header. Since an IPv4 header may contain a variable number of options, this field specifies the size of the header (this also coincides with the offset to the data). The minimum value for this field is 5 (RFC 791), which is a length of 5×32 = 160 bits = 20 bytes. Being a 4-bit value, the maximum length is 15 words (15×32 bits) or 480 bits = 60 bytes.
		u_char ver : 4;				// The first header field in an IP packet is the four-bit version field. For IPv4, this has a value of 4 (hence the name IPv4).
		u_char dscp : 6;			// differentiated_services_code_point. Originally called Type of Service (ToS) field. 
		u_char ecn : 2;				// explicit_congestion_notification. This field is defined in RFC 3168 and allows end-to-end notification of network congestion without dropping packets. ECN is an optional feature that is only used when both endpoints support it and are willing to use it. It is only effective when supported by the underlying network.
		u_short total_len;			// This 16-bit field defines the entire packet size, including header and data, in bytes. The minimum-length packet is 20 bytes (20-byte header + 0 bytes data) and the maximum is 65,535 bytes — the maximum value of a 16-bit word. All hosts are required to be able to reassemble datagrams of size up to 576 bytes, but most modern hosts handle much larger packets. Sometimes subnetworks impose further restrictions on the packet size, in which case datagrams must be fragmented. Fragmentation is handled in either the host or router in IPv4.
		u_short id;					// Identification. Used for uniquely identifying the group of fragments of a single IP datagram
		u_short flags : 3;			// Used to control or identify fragments. bit 0: reserved, must be 0. bit 1: don't fragment. bit 2: more fragments.
		u_short frag_offset : 13;	// The fragment offset field, measured in units of eight-byte blocks (64 bits), is 13 bits long and specifies the offset of a particular fragment relative to the beginning of the original unfragmented IP datagram. The first fragment has an offset of zero. This allows a maximum offset of (213 – 1) × 8 = 65,528 bytes, which would exceed the maximum IP packet length of 65,535 bytes with the header length included (65,528 + 20 = 65,548 bytes).
		u_char ttl;					// time to live in seconds... orrrrr when it reaches a router it decrements the value by 1. if 0, time exceeded msg is sent back.
		u_char protocol;			// ??
		u_short chksum;				// ipv4 Header checksum. The checksum field is the 16-bit one's complement of the one's complement sum of all 16-bit words in the header. For purposes of computing the checksum, the value of the checksum field is zero.
		u_int src_ip;				// source ip address
		u_int dst_ip;				// destination ip address

		/* If desired, options go here */
		// opts

	}IPV4Header;

	struct icmpheader_echorequest {
		u_char type;
		u_char code;
		u_short checksum;
		u_short id;
		u_short seq;
	}ICMPHeader;
#endif//_WIN32

	SOCKET created_socket;
	std::string target_ip_addr;
	std::string target_port;
	std::string my_ip_addr;
	std::string my_port;

	int iResult;
	int errchk;

	char* payload = nullptr;
	static const int sendbufMAXLENGTH = 400;
	char sendbuf[sendbufMAXLENGTH];

	size_t sendbuf_len;

	//maybe move these back to local in craftFixedICMPEchoRequestPacket();
	int on = 1;
	int on_len = sizeof(int);

#define ICMP_ECHO 8
#define ICMP_ECHO_CODE_ZERO 0	// This is ICMP_ECHO's only code option
#define ICMP_REPLY 0
#define ICMP_REPLY_CODE_ZERO 0	// This is ICMP_REPLY's only code option
};

#endif