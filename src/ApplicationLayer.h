// ApplicationLayer.h
#ifndef ApplicationLayer_h__
#define ApplicationLayer_h__

// The ApplicationLayer is the last step in handling all outbound data before it
// reaches the network and the first step in handling all inbound data after it
// comes from the network. Class ApplicationLayer has all things necessary to send a
// message with the necessary flags, and size of the message.
// Inside the ApplicationLayer class is the ProcessRecvBuf state machine. The function that
// contains the switch case for it is decideActionBasedOnFlag(). It is responsible for
// interpreting incoming messages based on the flag, and size of the message. It will
// decide what to do with the data that it was sent based on the flag.

#ifdef __linux__
#include <sys/types.h>
#include <sys/socket.h>	// <Winsock2.h>
#include <netdb.h>
#include <string>
#include <mutex> // Want to use exceptions and avoid having it never reach unlock? Use std::lock_guard
#endif//__linux__

#ifdef _WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>	// <sys/socket.h>
#include <string>
#include <mutex> // Want to use exceptions and avoid having it never reach unlock? std::lock_guard
#endif//_WIN32


// Forward declaration
class IXBerkeleySockets;

// ApplicationLayer class is for sending and receiving things to the peer.
class ApplicationLayer
{
	// Typedef section
public:
	// Typedefs for callbacks
	typedef void callback_fn_set_exit_now(bool value); // for setting the bool variable if you want the program to exit.
	typedef bool& callback_fn_get_exit_now(); // for viewing the bool variable to see if the program wants to exit.


#ifdef __linux__
	typedef int32_t SOCKET;
#endif//__linux__

public:
	
	ApplicationLayer(
		IXBerkeleySockets* SocketClassInstance, // Simply a cross platform implementation of certain Berkeley Socket functions.
		SOCKET socket, // A socket with an active connection.
		callback_fn_set_exit_now * set_exit_now_ptr,
		callback_fn_get_exit_now * get_exit_now_ptr,
		bool turn_verbose_output_on = false // turn on and off verbose output for this class.
	);
	~ApplicationLayer();

	// Turn on and off verbose output for this class.
	bool verbose_output = false;

	// ----------------------------------------------------------------------
	//                       IMPORTANT INFORMATION
	// 1. Anything in CryptRelay that wants to send information to the peer
	// MUST use ApplicationLayer::send().
	// 2. Anything in CryptRelay that wants to use ApplicationLayer::send()
	// MUST give the ApplicationLayer::send() method a buffer of
	// size > CR_RESERVED_BUFFER_SPACE.
	// 3. That buffer must have the first CR_RESERVED_BUFFER_SPACE characters
	// set in the appropriate manner (see below)
	// 4. Use the setFlagsAndMsgSize() method

	// Amount of bytes to reserve in the buffer for information such as
	// flags, and size of the message that will be sent to the peer.
	// Not to be confused with BUF_LEN.
	// Example buffer with CR_RESERVED_BUFFER_SPACE == 3:
	//    [0]      [1]   [2]  [3][4][5][6][7][8][9][10][11][12][13] etc...
	// [ flag  ][message_size][text that the user is sending to the peer..]
	// [CR_CHAT][     38     ][Hello peer. This is my message to you!]
	static const int8_t CR_RESERVED_BUFFER_SPACE;

	// Flags that indicate what the message is being used for.
	// enum is not used for these b/c it could break compatability when
	// communicating with older version of this program.
	struct MsgFlags
	{
		static const int8_t NO_FLAG;
		static const int8_t SIZE_NOT_ASSIGNED;
		static const int8_t CHAT;
		static const int8_t ENCRYPTED_CHAT;
		static const int8_t FILE_NAME;
		static const int8_t FILE_SIZE;
		static const int8_t FILE_DATA;
		static const int8_t ENCRYPTED_FILE_DATA;
		static const int8_t ENCRYPTED_FILE_NAME;
		static const int8_t ENCRYPTED_FILE_SIZE;
		static const int8_t FILE_TRANSFER_COMPLETE;
	};

	// This artifical limit is to make it so that the send() method does not
	// prevent anything else from being sent for an excessively long period
	// of time.
	// For example if you try sending a 50gb file, send() would
	// normally just be taken up by that file transfer for a very long time,
	// because of the mutex, and thus not allowing any chat message to go
	// through. Therefore limit it to a smaller amount so other things have
	// a chance to access send() method during the file transfer.
	static const int32_t ARTIFICIAL_LENGTH_LIMIT_FOR_SEND;

	//						END IMPORTANT INFORMATION
	// =========================================================================



	// ------------------------------------------------------------------------------------
	// These functions are for doing specific actions with the send() method.
	// They take care of all the flags, and the size of the message for you.
	// For example, sendChat(buf, BUF_LEN, message_length); would
	// assign the MsgFlags::CHAT flag in buf[0], and the message_length
	// would be assigned for you in buf[1] and buf[2].

	// Anyone who calls the functions that take a char * buf arg must make sure they
	// put information into their buffer starting at array index CR_RESERVED_BUFFER_SPACE
	// or else those bytes will be overwritten. This does not apply to std::string bufs.

	// They all return in this fashion:
	// returns total bytes sent, success
	// returns -1, error.

	int64_t sendChatStr(std::string& buf);
	int64_t sendFileName(std::string name_of_file);

	// Writes over the first CR_RESERVED_BUFFER_SPACE bytes of the given buffer.
	// It then writes the size of the file into the next sizeof(file_size) bytes of the given buffer.
	int32_t sendFileSize(char * buf, const int32_t BUF_LEN, int64_t file_size);

	int32_t sendFileData(char * buf, const int32_t BUF_LEN, int32_t message_length);

	// Tell the peer that you have finished sending all the file data to them.
	int32_t sendFileTransferComplete(char * buf, const int32_t BUF_LEN);
	// ====================================================================================



	// Call this if you want the ApplicationLayer to stop the connection with the peer.
	// This will shutdown() the connection on the socket and close() it.
	// This will interrupt recv() if it is currently blocking.
	// returns -1, error
	// returns 0, success
	int32_t endConnection();

	// recv()s data from peer and gives it to decideActionBasedOnFlag() to process the data.
	void loopedReceiveMessages();


protected:
private:

	// Prevent anyone from copying this class
	ApplicationLayer(ApplicationLayer& ApplicationLayerInstance) = delete;			  // disable copy operator
	ApplicationLayer& operator=(ApplicationLayer& ApplicationLayerInstance) = delete; // disable assignment operator
	
	IXBerkeleySockets * Socket;

	// ApplicationLayer expects this socket to have an active connection with the peer.
	SOCKET fd_socket;

	// This method is thread safe.
	// All information that gets sent over the network MUST go through
	// this method. This is to avoid abnormal behavior / problems
	// with multiple threads trying to send() using the same socket.
	// Returns SOCKET_ERROR, error (see ApplicationLayer.cpp for linux #define)
	// Returns total amount of bytes sent, success.
	int32_t send(const char * sendbuf, int32_t amount_to_send);
	// These are for the send() method
	int32_t total_bytes_sent = 0;
	int32_t bytes_sent = 0;
	static std::mutex SendMutex;

	
	// It puts the flag and size of the message into the given buffer.
	// buf: This is where the flags and size of the message will be set.
	// It will use the first CR_RESERVED_BUFFER_SPACE bytes of the buffer
	// for the flags. That means it is up to whoever is giving the char * buf
	// to this method to not use the first CR_RESERVED_BUFFER_SPACE bytes
	// or else they will be overwritten by this method.
	// BUF_LEN: length of the buf.
	// length_of_msg: This is the amount from the buf that you want to send.
	// flag: The type of message that you are sending. Flags are located
	// in the structure ApplicationLayer::MsgFlags
	// Example operation:
	// bytes_read = fread(buf + CR_RESERVED_BUFFER_SPACE, 1, BUF_LEN - CR_RESERVED_BUFFER_SPACE, ReadFile);
	// setFlagsAndMsgSize(buf, BUF_LEN, bytes_read, ApplicationLayer::MsgFlags::CR_FILE);
	// Returns 0, success
	// Returns -1, error
	int32_t setFlagsAndMsgSize(char * buf, const int32_t BUF_LEN, int32_t length_of_message, int8_t flag);
	int32_t setFlagsAndMsgSize(std::string& buf, const int64_t BUF_LEN, int64_t length_of_message, int8_t flag);

	// returns total bytes sent, success
	// returns -1, error
	int64_t sendStrBuf(std::string& str_buf, int64_t message_length);

	// returns total bytes sent, success
	// returns -1, error
	int32_t sendCharBuf(char * buf, int32_t BUF_LEN, int32_t message_length);

	// Used for copying a host byte order LongLong into the given buffer as a network byte order.
	// Using the given variable, it will convert it to network long long and place it into the given buffer.
	int32_t intoBufferHostToNetworkLongLong(char * buf, const int64_t BUF_LEN, int64_t variable_to_convert);

	// Cross-platform WSAStartup();
	// For every WSAStartup() that is called, a WSACleanup() must be called.
	int32_t WSAStartup();

#ifdef _WIN32
	WSADATA wsaData;	// for WSAStartup();
#endif//_WIN32


private:
	callback_fn_set_exit_now * callbackSetExitNow = nullptr; // for setting the bool variable if you want the program to exit.
	callback_fn_get_exit_now * callbackGetExitNow = nullptr; // for viewing the bool variable to see if the program wants to exit.








	// ******************************************************************
	// *             Things related to Recv state machine               *
	// ******************************************************************
public:

	// Here is where all messages received are processed to determine what to do with the message.
	// Some messages might be printed to screen, others are a portion of a file transfer.
	int32_t decideActionBasedOnFlag(char * recv_buf, int64_t buf_len, int64_t byte_count);

	// For writing files in decideActionBasedOnFlag().
	// It is only public because if recv() ever fails inside the ApplicationLayer class,
	// then it needs to close the file that it was writing to.
	FILE * WriteFile = nullptr;

protected:
private:
	
	enum RecvStateMachine
	{
		CHECK_FOR_FLAG,
		CHECK_MESSAGE_SIZE_PART_ONE,
		CHECK_MESSAGE_SIZE_PART_TWO,
		WRITE_FILE_FROM_PEER,
		TAKE_FILE_NAME_FROM_PEER,
		TAKE_FILE_SIZE_FROM_PEER,
		FILE_TRANSFER_COMPLETE,
		OUTPUT_CHAT_FROM_PEER,

		OPEN_FILE_FOR_WRITE,
		CLOSE_FILE_FOR_WRITE,

		ERROR_STATE,
	};

	// Variables necessary for decideActionBasedOnFlag().
	// They are here so that the state can be saved even after exiting the function.
	int64_t position_in_recv_buf = 0;   // the current cursor position inside the buffer.
	int64_t state = CHECK_FOR_FLAG;
	int64_t position_in_message = 0;	// current cursor position inside the imaginary message sent by the peer.
	int8_t message_flag = 0;			// The flag that the peer sent us. (See struct MsgFlags for a list of flags)
	int64_t message_size_part_one = 0;
	int64_t message_size_part_two = 0;
	int64_t message_size = 0;		    // peer told us the size of his message.

	// Variables necessary for decideActionBasedOnFlag().
	// These are related to an incoming file transfer
	static const int64_t RESERVED_NULL_CHAR_FOR_FILE_NAME = 1;
	static const int64_t INCOMING_FILE_NAME_FROM_PEER_SIZE = 200; // size of the cstring char array.
	char incoming_file_name_from_peer_cstr[INCOMING_FILE_NAME_FROM_PEER_SIZE]; // The peer told us his file name, and that file name is initially placed in here.
	std::string incoming_file_name_from_peer; // After the file name was initially placed in the incoming_file_name_from_peer_cstr, it is placed here for ease of use.
	// This is to let the program know that if an error occured while a file was still being written, then it should close the file.
	bool is_file_done_being_written = true; 

	// For use with decideActionBasedOnFlag()
	// Open a file for writing
	// Returns -1, error
	// Returns 0, success
	int32_t openFileForWrite();

	// For use with decideActionBasedOnFlag()
	// Close the file for writing
	void closeFileForWrite();

	// For use with decideActionBasedOnFlag()
	// Write the incoming file from peer.
	// Returns -1, error
	// Returns 0, success
	int32_t writeFileFromPeer(char * recv_buf, int64_t received_bytes);

	// For use with decideActionBasedOnFlag()
	// Assigns the incoming_file_name_from_peer_cstr.
	void assignFileNameFromPeerCStr(char * recv_buf, int64_t received_bytes);

	// For use with decideActionBasedOnFlag()
	void coutFileTransferSuccessOrFail();

	// Variables for decideActionBasedOnFlag()
	// Specifically for the cases that deal with writing an incoming file from the peer.
	int64_t bytes_read = 0;
	int64_t bytes_written = 0;
	int64_t total_bytes_written_to_file = 0;


	// For use with decideActionBasedOnFlag()
	// Using the given buffer, convert the first 8 bytes from Network to Host Long Long.
	// It then assigns the variable incmoing_file_size_from_peer that host byte order value.
	// Returns RECV_AGAIN if it needs more bytes from the peer.
	// Returns FINISHED_ASSIGNING_FILE_SIZE_FROM_PEER if it has completed its task.
	int32_t assignFileSizeFromPeer(char * recv_buf, int64_t recv_buf_len, int64_t received_bytes);
	// Variables necessary for assignFileSizeFromPeer();
	int64_t file_size_fragment = 0;
	int64_t incoming_file_size_from_peer = 0;
	const int32_t RECV_AGAIN = 0;
	const int32_t FINISHED_ASSIGNING_FILE_SIZE_FROM_PEER = 1;

	// For use with decideActionBasedOnFlag()
	void coutChatMsgFromPeer(char * recv_buf, int64_t received_bytes);


	// ******************************************************************
	// *         End of things related to Recv state machine            *
	// ******************************************************************
};
#endif//ApplicationLayer_h__