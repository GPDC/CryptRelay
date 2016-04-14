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

#ifdef __linux__
	static pthread_t thread0;	// Server
	static pthread_t thread1;	// Client
	static pthread_t thread2;
	static int ret0;			// Server
	static int ret1;			// Client
	static int ret2;
#endif //__linux__

#ifdef _WIN32
	static HANDLE ghEvents[3];	// For threads // i should be using vector of ghevents instead
#endif //_WIN32

	// To let whoever created this thread know it errored badly.
	static bool fatal_thread_error;

	static void createStartServerThread(void * instance);
	static void createClientRaceThread(void * instance);

	void giveIPandPort(std::string target_extrnl_ip_address, std::string my_ext_ip, std::string my_internal_ip, std::string target_port = default_port, std::string my_internal_port = default_port);
	bool startChatProgram();
	static void startServer(void * instance);
	static void startClient(void * instance);


protected:
private:
	SocketClass SockStuff;

	addrinfo		 hints;		// once hints is given to getaddrinfo() it will return *result

	// after being given to getaddrinfo(), result now contains all relevant info for
	// ip address, family, protocol, etc.
	// *result is ONLY used if there is a getaddrinfo().
	addrinfo		 *result;
	//addrinfo		 *ptr;

	static const std::string default_port;
	std::string target_external_ip;						// If the option to use LAN only == true, this is target's local ip
	std::string target_external_port = default_port;					// If the option to use LAN only == true, this is target's local port
	std::string my_external_ip;
	std::string my_local_ip;
	std::string my_local_port = default_port;

};

#endif //chat_program_h__