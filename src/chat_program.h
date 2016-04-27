// chat_program.h  // pls rename to connection

// Overview:
// This is the area where the high level functionality for the chat program is located.
// It relies on SocketClass to perform anything that deals with Sockets or fd's

// Warnings:
// This source file expects any input that is given to it has already been checked
//  for safety and validity. For example if a user supplies a port, then
//  this source file will expect the port will be >= 0, and <= 65535.
//  As with an IP address it will expect it to be valid input, however it doesn't
//  expect you to have checked to see if there is a host at that IP address
//  before giving it to this source file.

// Terminology:
// Socket is an end-point that is defined by an IP-address and port.
//   A socket is just an integer with a number assigned to it.
//   That number can be thought of as a unique ID for the connection
//   that may or may not be established.
// fd is linux's term for a socket. == File Descriptor
// Mutex - mutual exclusions. It is designed so that only 1 thread executes code
// at any given time.

#ifndef chat_program_h__
#define chat_program_h__

#include <string>
#include "SocketClass.h"


class Connection
{
public:
	Connection();
	~Connection();

	// Variables for handling threads
#ifdef __linux__
	static pthread_t thread0;	// Server
	static pthread_t thread1;	// Client
	static pthread_t thread2;	// Send()
	static int ret0;	// Server
	static int ret1;	// Client
	static int ret2;	// Send()
#endif //__linux__
#ifdef _WIN32
	static HANDLE ghEvents[2];	// i should be using vector of ghevents[] instead...
								// [0] == server
								// [1] == client

	// for the loopedSendMessagesThread()
	static HANDLE ghEventsSend[1];// [0] == send()
#endif //_WIN32

	// If something errors in the server thread, sometimes we
	// might want to do something with that information.
	// 0 == no error, 0 == no function was given.
	static int server_thread_error_code;
	static int function_that_errored;

	// A global socket used by threads
	static SOCKET global_socket;

	// Thread entrances.
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

	// Bools to check to see what exactly the user wants to start doing with
	// this client / server connection.
	bool use_chat_program = false;
	bool use_file_transfer_program = false;

protected:
private:
	SocketClass SockStuff;

	// These are called by createStartServerThread() and createStartClientThread()
	// These exist because threads on linux have to return a void*.
	// Conversely on windows it doesn't return anything because threads return void.
	static void* posixStartServerThread(void * instance);
	static void* posixStartClientThread(void * instance);

	// Send Messages thread
	static void createLoopedSendMessagesThread(void * instance);
	static void* posixLoopedSendMessagesThread(void * instance);
	static void loopedSendMessagesThread(void * instance);


	// Server and Client threads
	static void startServerThread(void * instance);
	static void startClientThread(void * instance);

	// Variables necessary for determining who won the connection race
	static const int SERVER_WON;
	static const int CLIENT_WON;
	static const int NOBODY_WON;
	static int global_winner;


	int loopedReceiveMessages(const char* host = "Peer");
	void coutPeerIPAndPort(SOCKET s);

	// Cross platform windows and linux thread exiting
	void exitThread(void* ptr);

	// Hints is used by getaddrinfo()
	// once Hints is given to getaddrinfo() it will return *ConnectionInfo
	addrinfo		 Hints;	

	// after being given to getaddrinfo(), ConnectionInfo now contains all relevant info for
	// ip address, family, protocol, etc.
	// *ConnectionInfo is ONLY used if there is a getaddrinfo().
	addrinfo		 *ConnectionInfo;
	//addrinfo		 *ptr;		// this would only be used if traversing the list of address structures.


	static const std::string default_port;




	// NEW SECTION with threads etc

	// This is the only thread that has access to send().
	// all other threads must send their info to this thread
	// in order to send it over the network.
	int sendThreadTwo(const char * sendbuf, size_t size_of_sendbuf, size_t amount_to_send);

	bool doesUserWantToSendAFile(std::string& user_msg_from_terminal);
	void getUserInputThread();

	static const size_t global_sendbuf_size = 512;
	char global_sendbuf[global_sendbuf_size];



	// Do not touch! This is for sendThreadTwo()  the send() function return value.
	int bytes;
};

#endif //chat_program_h__
