// ApplicationLayer.h
#ifndef ApplicationLayer_h__
#define ApplicationLayer_h__

// The ApplicationLayer is the last step in handling all outbound data before it
// reaches the network and the first step in handling all inbound data after it
// comes from the network. Class ApplicationLayer has all things necessary to send a
// message with the necessary flags, and size of the message included in the message.
// Inside the ApplicationLayer class is another class called ProcessRecvBuf.
// The ProcessRecvBuf class is responsible for interpreting incoming messages
// based on the flag, and size of the message. It will decide what to do
// with the data that it was sent based on the flag.

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
class XBerkeleySockets;

// ApplicationLayer class is for sending and receiving things to the peer.
class ApplicationLayer
{
public:
	
	ApplicationLayer(
		XBerkeleySockets* SocketClassInstance, // Simply a cross platform implementation of certain Berkeley Socket functions.
		SOCKET socket, // A socket with an active connection.
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
	};

	// These functions are for doing specific actions with the send() method.
	// They take care of all the flags, and the size of the message for you.
	// For example, sendChat(buf, BUF_LEN, message_length); would
	// assign the MsgFlags::CHAT flag in buf[0], and the message_length
	// would be assigned for you in buf[1] and buf[2].

	// Anyone who calls this function must make sure they put information into their
	// buffer starting at array index CR_RESERVED_BUFFER_SPACE or else those bytes
	// will be overwritten.
	//int64_t sendChat(char * buf, const int32_t BUF_LEN, int32_t message_length);
	int64_t sendChatStr(std::string& buf);
	int64_t sendFileName(std::string name_of_file);
	int32_t sendFileSize(char * buf, const int32_t buf_len, int64_t file_size);
	int32_t sendFileData(char * buf, const int32_t buf_len, int32_t message_length);

	// SHOULD THIS BE PUBLIC OR PRIVATE?
	// Used for copying a host byte order LongLong into the given buffer as a network byte order.
	int32_t intoBufferHostToNetworkLongLong(char * buf, const int64_t BUF_LEN, int64_t variable_to_convert);

	// This will shutdown() the connection on the socket and close() it.
	// This will interrupt recv() if it is currently blocking.
	int32_t endConnection();

	// ApplicationLayer class will do the receiving and then send the information to ProcessRecvBuf class
	// Thread used to handle receiving messages.
	void loopedReceiveMessages();	


	// This artifical limit is to make it so that the send() method does not
	// prevent anything else from being sent for an excessively long period
	// of time. 
	// For example if you try sending a 50gb file, send() would
	// normally just be taken up by that file transfer for a very long time,
	// because of the mutex, and thus not allowing any chat message to go
	// through. Therefore limit it to a smaller amount so other things have
	// a chance to access send() method during the file transfer.
	static const int32_t ARTIFICIAL_LENGTH_LIMIT_FOR_SEND;

	// ----------------------------------------------------------------------


protected:
private:

	// Prevent anyone from copying this class
	ApplicationLayer(ApplicationLayer& ApplicationLayerInstance) = delete;			  // disable copy operator
	ApplicationLayer& operator=(ApplicationLayer& ApplicationLayerInstance) = delete; // disable assignment operator

	// This method is thread safe.
	// Everything in CryptRelay that wants to send information to the peer
	// MUST use this method.
	int32_t send(const char * sendbuf, int32_t amount_to_send);
	// These are for the send() method
	int32_t total_amount_sent = 0;
	int32_t bytes_sent = 0;
	static std::mutex SendMutex;

	XBerkeleySockets * Socket;

	SOCKET fd_socket;


	// It puts the flag and size of the message into the given buffer.
	int32_t setFlagsAndMsgSize(char * buf, const int32_t BUF_LEN, int32_t length_of_message, int8_t flag);
	int32_t setFlagsAndMsgSize(std::string& buf, const int64_t BUF_LEN, int64_t length_of_message, int8_t flag);

	int64_t sendStrBuf(std::string& str_buf, int64_t message_length);
	int32_t sendCharBuf(char * buf, int32_t BUF_LEN, int32_t message_length);

	// Cross-platform WSAStartup();
	// For every WSAStartup() that is called, a WSACleanup() must be called.
	int32_t WSAStartup();

#ifdef _WIN32
	WSADATA wsaData;	// for WSAStartup();
#endif//_WIN32



public:
	// Typedefs for callbacks
	typedef void callback_fn_set_exit_now(bool value);
	typedef bool& callback_fn_get_exit_now();

private:
	callback_fn_set_exit_now * callbackSetExitNow = nullptr; // for setting the bool exit_now variable.
	callback_fn_get_exit_now * callbackGetExitNow = nullptr; // for viewing the bool exit_now variable.

public:

	// Accessors
	void setCallbackGetExitNow(callback_fn_get_exit_now * ptr) { callbackGetExitNow = ptr; }
	void setCallbackSetExitNow(callback_fn_set_exit_now * ptr) { callbackSetExitNow = ptr; }








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
		OUTPUT_CHAT_FROM_PEER,

		OPEN_FILE_FOR_WRITE,
		CLOSE_FILE_FOR_WRITE,

		ERROR_STATE,
	};

	// Variables necessary for decideActionBasedOnFlag().
	// They are here so that the state can be saved even after exiting the function.
	int64_t position_in_recv_buf = 0;
	int64_t state = CHECK_FOR_FLAG;
	int64_t position_in_message = 0;	// current cursor position inside the imaginary message sent by the peer.
	int8_t message_flag = 0;
	int64_t message_size_part_one = 0;
	int64_t message_size_part_two = 0;
	int64_t message_size = 0;		    // peer told us this size

										// More variables necessary for decideActionBasedOnFlag().
										// These are related to an incoming file transfer
	static const int64_t INCOMING_FILE_NAME_FROM_PEER_SIZE = 200;
	static const int64_t RESERVED_NULL_CHAR_FOR_FILE_NAME = 1;
	char incoming_file_name_from_peer_cstr[INCOMING_FILE_NAME_FROM_PEER_SIZE];
	std::string incoming_file_name_from_peer;
	bool is_file_done_being_written = true;


	// Variables for decideActionBasedOnFlag()
	// Specifically for the cases that deal with writing an incoming file from the peer.
	int64_t bytes_read = 0;
	int64_t bytes_written = 0;
	int64_t total_bytes_written_to_file = 0;


	// For use with decideActionBasedOnFlag()
	// Using the given buffer, convert the first 8 bytes from Network to Host Long Long
	int32_t assignFileSizeFromPeer(char * recv_buf, int64_t recv_buf_len, int64_t received_bytes);
	// Variables necessary for assignFileSizeFromPeer();
	int64_t file_size_fragment = 0;
	int64_t incoming_file_size_from_peer = 0;
	const int32_t RECV_AGAIN = 0;
	const int32_t FINISHED_ASSIGNING_FILE_SIZE_FROM_PEER = 1;
};
#endif//ApplicationLayer_h__