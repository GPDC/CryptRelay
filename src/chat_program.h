// chat_program.h  // pls rename to connection

// Overview:
// This is the area where the high level functionality for the chat program is located.
// It relies on SocketClass to perform anything that deals with Sockets or fd's

// Warnings:
// This source file does not do any input validation.

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
#include <mutex> // btw, need to use std::lock_guard if you want to be able to use exceptions and avoid having it never reach the unlock.
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

	// for the loopedSendChatMessagesThread()
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
	void giveIPandPort(std::string target_extrnl_ip_address, std::string my_ext_ip, std::string my_internal_ip, std::string target_port = DEFAULT_PORT, std::string my_internal_port = DEFAULT_PORT);

	// IP and port information can be given to the Connection class through these variables.
	std::string target_external_ip;						// If the option to use LAN only == true, this is target's local ip
	std::string target_external_port = DEFAULT_PORT;	// If the option to use LAN only == true, this is target's local port
	std::string my_external_ip;
	std::string my_local_ip;
	std::string my_local_port = DEFAULT_PORT;

	// Send a file the normal way
	bool does_user_want_to_send_a_file = false;
	std::string file_name_and_loc;

	// Before sending a file, make a copy of it, encrypt that copy, then send it.
	bool does_user_want_to_send_an_encrypted_file = false;
	std::string file_name_and_loc_to_be_encrypted;
	std::string file_encryption_option;

protected:
private:

#ifdef _WIN32
	typedef struct _stat64 myStat;
#endif//WIN32
#ifdef __linux__
	typedef struct stat myStat;
#endif//__linux__

	std::mutex SendMutex;
	std::mutex RaceMutex;

	// These are called by createStartServerThread() and createStartClientThread()
	// These exist because threads on linux have to return a void*.
	// Conversely on windows it doesn't return anything because threads return void.
	static void* posixStartServerThread(void * instance);
	static void* posixStartClientThread(void * instance);

	// Server and Client threads
	static void serverThread(void * instance);
	static void clientThread(void * instance);

	// Variables necessary for determining who won the connection race
	static const int SERVER_WON;
	static const int CLIENT_WON;
	static const int NOBODY_WON;
	static int global_winner;

	// Server and Client thread must use this function to prevent
	// a race condition.
	int setWinnerMutex(int the_winner);

	void loopedReceiveMessagesThread(void * instance);
	bool EXIT_NOW = false;

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


	static const std::string DEFAULT_PORT;


	// This method is thread safe.
	int send(const char * sendbuf, int amount_to_send);
	// This is for send()
	int total_amount_sent = 0;
	int bytes_sent = 0;

	bool doesUserWantToSendAFile(std::string& user_msg_from_terminal);
	void loopedGetUserInput();

	bool displayFileSize(const char* file_name_and_location, myStat * FileStatBuf);
	long long getFileStatsAndDisplaySize(const char * file_name_and_location);
	bool copyFile(const char * read_file_name_and_location, const char * write_file_name_and_location);
	bool sendFileThread(std::string file_name);

	// This is only for use with sendFileThread()
	bool is_send_file_thread_in_use = false;


	// Flags for send() that indicated what the message is being used for.
	// enum is not used for these b/c it could break compatability when
	// communicating with older version of this program.
	static const int8_t CR_NO_FLAG;
	static const int8_t CR_BEGIN;
	static const int8_t CR_SIZE_NOT_ASSIGNED;
	static const int8_t CR_CHAT;
	static const int8_t CR_ENCRYPTED_CHAT_MESSAGE;
	static const int8_t CR_FILE_NAME;
	static const int8_t CR_FILE_SIZE;
	static const int8_t CR_FILE;
	static const int8_t CR_ENCRYPTED_FILE;

	// Amount of space to reserve for information such as
	// flags, and size of the message that will be sent to the peer.
	static const int8_t CR_RESERVED_BUFFER_SPACE;	//not to be confused with the size or length of a buffer


	// Variables necessary for processRecvBuf().
	// They are here so that the state can be saved even after exiting the funciton.
	long long position_in_recv_buf = CR_BEGIN;
	long long process_recv_buf_state = CHECK_FOR_FLAG;
	long long position_in_message = CR_BEGIN;	// current cursor position inside the imaginary message sent by the peer.
	int8_t type_of_message_flag = CR_NO_FLAG;
	long long message_size_part_one = CR_SIZE_NOT_ASSIGNED;
	long long message_size_part_two = CR_SIZE_NOT_ASSIGNED;
	long long message_size = CR_SIZE_NOT_ASSIGNED;		// peer told us this size

	// More variables necessary for processRecvBuf().
	static const long long INCOMING_FILE_NAME_FROM_PEER_SIZE = 200;
	char incoming_file_name_from_peer_cstr[INCOMING_FILE_NAME_FROM_PEER_SIZE];
	std::string incoming_file_name_from_peer;
	static const long long RESERVED_NULL_CHAR_FOR_FILE_NAME = 1;

	// More variables necessary for processRecvBuf().
	bool received_file_name = false;
	bool received_file_size = false;
	bool is_file_done_being_written = true;

	// More variables necessary for processRecvBuf().
	long long file_size_part_one = 0;
	long long file_size_part_two = 0;
	long long file_size_part_three = 0;
	long long file_size_part_four = 0;
	long long file_size_part_five = 0;
	long long file_size_part_six = 0;
	long long file_size_part_seven = 0;
	long long file_size_part_eight = 0;


	bool processRecvBuf(char * recv_buf, long long buf_len, long long byte_count);

	enum RecvStateMachine
	{
		CHECK_FOR_FLAG,
		CHECK_MESSAGE_SIZE_PART_ONE,
		CHECK_MESSAGE_SIZE_PART_TWO,
		WRITE_FILE_FROM_PEER,
		TAKE_FILE_NAME_FROM_PEER,
		TAKE_FILE_SIZE_FROM_PEER,
		OUTPUT_CHAT_FROM_PEER,

		OPEN_FILE_FOR_WRITE,
		CLOSE_FILE_FOR_WRITE,

		ERROR_STATE,
	};

	// Variables for writePartOfTheFileFromPeer()
	long long bytes_read = 0;
	long long bytes_written = 0;
	long long total_bytes_written_to_file = 0;

	FILE * WriteFile = nullptr;

	// using the given buffer, convert the first 8 bytes from Network to Host Long Long
	int assignFileSizeFromPeer(char * recv_buf, long long recv_buf_len, long long received_bytes);
	// Variables necessary for assignFileSizeFromPeer();
	long long file_size_fragment = 0;
	long long incoming_file_size_from_peer = 0;
	const int RECV_AGAIN = 0;
	const int FINISHED = 1;

	bool intoBufferHostToNetworkLongLong(char * buf, const long long BUF_LEN, long long variable_to_convert);

	bool sendFileSize(char * buf, const long long BUF_LEN, long long size_of_file);
	bool sendFileName(char * buf, const long long BUF_LEN, const std::string& name_and_location_of_file);
	std::string getFileNameFromPath(std::string name_and_location_of_file);
};

#endif //chat_program_h__
