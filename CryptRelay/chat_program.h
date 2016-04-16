// chat_program.h

// Overview:
// This is the area where the high level functionality for the chat program is located.
// It relies on SocketClass to perform anything that deals with Sockets or fd's

// Terminology:
// Socket is an end-point that is defined by an IP-address and port.
//   A socket is just an integer with a number assigned to it.
//   That number can be thought of as a unique ID for the connection
//   that may or may not be established.
// fd is linux's term for a socket. == File Descriptor

#ifndef chat_program_h__
#define chat_program_h__

#include <string>
#include "SocketClass.h"

class ChatProgram
{
public:
	ChatProgram();
	~ChatProgram();

	// Variables for handling threads
#ifdef __linux__
	static pthread_t thread0;	// Server
	static pthread_t thread1;	// Client
	static pthread_t thread2;	// Send()
	static int ret0;			// Server
	static int ret1;			// Client
	static int ret2;			// Send()
#endif //__linux__
#ifdef _WIN32
	static HANDLE ghEvents[2];	// i should be using vector of ghevents[] instead...
	// [0] == server
	// [1] == client


	// [2] == send()	//cancel this if ghEventsSend[1] is here


	static HANDLE ghEventsSend[1];// [0] == send()
#endif //_WIN32

	// A global socket used by threads
	static SOCKET global_socket;

	// To let whoever created this thread know it errored badly.
	static bool fatal_thread_error;//what am i doin with this

	// Thread entrances
	static void createStartServerThread(void * instance);
	static void createStartClientThread(void * instance);

	// If you want to give this class IP and port information, call this function.
	void giveIPandPort(std::string target_extrnl_ip_address, std::string my_ext_ip, std::string my_internal_ip, std::string target_port = default_port, std::string my_internal_port = default_port);

	// IP and port information can be given to chat_program through these variables.
	std::string target_external_ip;						// If the option to use LAN only == true, this is target's local ip
	std::string target_external_port = default_port;	// If the option to use LAN only == true, this is target's local port
	std::string my_external_ip;
	std::string my_local_ip;
	std::string my_local_port = default_port;

protected:
private:
	SocketClass SockStuff;

	static void createThreadedLoopedSendMessages(void * instance);
	static void threadedLoopedSendMessages(void * instance);
	int loopedReceiveMessages(const char* host = "Peer");

	void coutPeerIPAndPort(SOCKET s);


	// hints is used by getaddrinfo()
	// once hints is given to getaddrinfo() it will return *result
	addrinfo		 hints;	

	// after being given to getaddrinfo(), result now contains all relevant info for
	// ip address, family, protocol, etc.
	// *result is ONLY used if there is a getaddrinfo().
	addrinfo		 *result;
	//addrinfo		 *ptr;		// this would only be used if traversing the list of address structures.


	static const std::string default_port;	

	// Server and Client threads
	static void startServerThread(void * instance);
	static void startClientThread(void * instance);

	// Variables necessary for determining who won the connection race
	static const int SERVER_WON;
	static const int CLIENT_WON;
	static const int NOBODY_WON;
	static int global_winner;
};

#endif //chat_program_h__