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
	virtual ~Connection();

	// Variables for handling threads
#ifdef __linux__
	static pthread_t thread0;	// Server
	static pthread_t thread1;	// Client
	static pthread_t thread2;	// Send()
	static int32_t ret0;	// Server
	static int32_t ret1;	// Client
	static int32_t ret2;	// Send()
#endif //__linux__
#ifdef _WIN32
	static HANDLE ghEvents[2];	// [0] == server
								// [1] == client
#endif //_WIN32

	// Thread entrances.
	static void createStartServerThread(void * instance);
	static void createStartClientThread(void * instance);

	// If you want to give this class IP and port information, call this function.
	void setIPandPort(std::string target_extrnl_ip_address, std::string my_ext_ip, std::string my_internal_ip, std::string target_port = DEFAULT_PORT, std::string my_internal_port = DEFAULT_PORT);

	// IP and port information can be given to the Connection class through these variables.
	std::string target_external_ip;						// If the option to use LAN only == true, this is target's local ip
	std::string target_external_port = DEFAULT_PORT;	// If the option to use LAN only == true, this is target's local port
	std::string my_external_ip;
	std::string my_local_ip;
	std::string my_local_port = DEFAULT_PORT;


protected:
private:

#ifdef _WIN32
	typedef struct _stat64 myStat;
#endif//WIN32
#ifdef __linux__
	typedef struct stat myStat;
#endif//__linux__

	SocketClass* ClientServerSocketClass = nullptr;
	SocketClass ClientSocketClass;
	SocketClass ServerSocketClass;

	static const std::string DEFAULT_PORT;


	//mutex for use in this class' send()
	static std::mutex SendMutex;
	// mutex for use with server and client threads to prevent a race condition.
	static std::mutex RaceMutex;

	// These are called by createStartServerThread() and createStartClientThread()
	// These exist because threads on linux have to return a void*.
	// Conversely on windows it doesn't return anything because threads return void.
	static void* posixStartServerThread(void * instance);
	static void* posixStartClientThread(void * instance);

	// Server and Client threads
	static void serverThread(void * instance);
	static void clientThread(void * instance);

	// Thread used to handle receiving messages.
	void loopedReceiveMessagesThread();

	// Cross platform windows and linux thread exiting
	// Not for use with std::thread
	void exitThread(void* ptr);

	// Variables necessary for determining who won the connection race
	static const int32_t SERVER_WON;
	static const int32_t CLIENT_WON;
	static const int32_t NOBODY_WON;
	static int32_t global_winner;

	// Server and Client thread must use this function to prevent
	// a race condition.
	static int32_t setWinnerMutex(int32_t the_winner);


	// Continually getline()'s the command prompt to see what the user wants to do.
	void loopedGetUserInputThread();

	// Used in loopedGetUserInputThread() to determine if the user wants to exit the program.
	// Eventually might not need to be here in the header file.
	bool exit_now = false;

	// Used within loopedGetUserInputThread() to check if the user inputted something
	// into the command prompt that would indicate they wanted to send a file.
	bool doesUserWantToSendAFile(std::string& user_msg_from_terminal);


	// Hints is used by getaddrinfo()
	// once Hints is given to getaddrinfo() it will return *ConnectionInfo
	addrinfo		 Hints;	

	// after being given to getaddrinfo(), ConnectionInfo now contains all relevant info for
	// ip address, family, protocol, etc.
	// *ConnectionInfo is ONLY used if there is a getaddrinfo().
	addrinfo		 *ConnectionInfo;
	//addrinfo		 *ptr;		// this would only be used if traversing the list of address structures.

	// Used for copying a host byte order LongLong into a buffer as a network byte order.
	bool intoBufferHostToNetworkLongLong(char * buf, const int64_t BUF_LEN, int64_t variable_to_convert);

	// This method is thread safe.
	// Everything in this class should use this instead of the regular ::send(), and
	// the SocketClass.send().
	int32_t send(const char * sendbuf, int32_t amount_to_send);
	// This is for the send() located in this class.
	int32_t total_amount_sent = 0;
	int32_t bytes_sent = 0;


	// Sends a file
	bool sendFileThread(std::string file_name);
	// This is only for use with sendFileThread()
	bool is_send_file_thread_in_use = false;

	// For sending the file size and file name to peer.
	bool sendFileSize(char * buf, const int64_t BUF_LEN, int64_t size_of_file);
	bool sendFileName(char * buf, const int64_t BUF_LEN, const std::string& name_and_location_of_file);

	// Copies a file.
	bool copyFile(const char * read_file_name_and_location, const char * write_file_name_and_location);

	// For use with anything file related
	bool displayFileSize(const char* file_name_and_location, myStat * FileStatBuf);
	int64_t getFileStatsAndDisplaySize(const char * file_name_and_location);

	// If given a direct path to a file, it will return the file name.
	// Ex: c:\users\me\storage\my_file.txt
	// will return: my_file.txt
	std::string retrieveFileNameFromPath(std::string name_and_location_of_file);


	// Flags that indicate what the message is being used for.
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
	int64_t position_in_recv_buf = CR_BEGIN;
	int64_t process_recv_buf_state = CHECK_FOR_FLAG;
	int64_t position_in_message = CR_BEGIN;	// current cursor position inside the imaginary message sent by the peer.
	int8_t type_of_message_flag = CR_NO_FLAG;
	int64_t message_size_part_one = CR_SIZE_NOT_ASSIGNED;
	int64_t message_size_part_two = CR_SIZE_NOT_ASSIGNED;
	int64_t message_size = CR_SIZE_NOT_ASSIGNED;		// peer told us this size

	// More variables necessary for processRecvBuf().
	static const int64_t INCOMING_FILE_NAME_FROM_PEER_SIZE = 200;
	static const int64_t RESERVED_NULL_CHAR_FOR_FILE_NAME = 1;
	char incoming_file_name_from_peer_cstr[INCOMING_FILE_NAME_FROM_PEER_SIZE];
	std::string incoming_file_name_from_peer;
	bool is_file_done_being_written = true;

	// Here is where all messages received are processed to determine what to do with the message.
	// Some messages might be printed to screen, others are a portion of a file transfer.
	bool processRecvBuf(char * recv_buf, int64_t buf_len, int64_t byte_count);

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

	// Variables for processRecvBuf()
	// Specifically for the cases that deal with writing an incoming file from the peer.
	int64_t bytes_read = 0;
	int64_t bytes_written = 0;
	int64_t total_bytes_written_to_file = 0;

	// For writing files in processRecvBuf()
	FILE * WriteFile = nullptr;

	// For use with processRecvBuf()
	// Using the given buffer, convert the first 8 bytes from Network to Host Long Long
	int32_t assignFileSizeFromPeer(char * recv_buf, int64_t recv_buf_len, int64_t received_bytes);
	// Variables necessary for assignFileSizeFromPeer();
	int64_t file_size_fragment = 0;
	int64_t incoming_file_size_from_peer = 0;
	const int32_t RECV_AGAIN = 0;
	const int32_t FINISHED = 1;
};

#endif //chat_program_h__
