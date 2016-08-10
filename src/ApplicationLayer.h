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
#include <string>
#include <mutex> // btw, need to use std::lock_guard if you want to be able to use exceptions and avoid having it never reach the unlock.

#include "ProcessRecvBuf.h"
#endif//__linux__

#ifdef _WIN32
#include <string>
#include <mutex> // btw, need to use std::lock_guard if you want to be able to use exceptions and avoid having it never reach the unlock.

#include "ProcessRecvBuf.h"
#endif//_WIN32

// Forward declaration
class SocketClass;

// ApplicationLayer class is for sending and receiving things to the peer.
class ApplicationLayer
{
public:

	// SocketClass in the constructor because
	ApplicationLayer(SocketClass* SocketClassInstance);
	~ApplicationLayer();

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


protected:
private:

	// Prevent anyone from copying this class
	ApplicationLayer(ApplicationLayer& ApplicationLayerInstance) = delete;			  // disable copy operator
	ApplicationLayer& operator=(ApplicationLayer& ApplicationLayerInstance) = delete; // disable assignment operator

	ProcessRecvBuf ProcRecv;

	// This method is thread safe.
	// Everything in CryptRelay that wants to send information to the peer
	// MUST use this method.
	int32_t send(const char * sendbuf, int32_t amount_to_send);
	// These are for the send() method
	int32_t total_amount_sent = 0;
	int32_t bytes_sent = 0;
	static std::mutex SendMutex;

	SocketClass * Socket;


	// It puts the flag and size of the message into the given buffer.
	int32_t setFlagsAndMsgSize(char * buf, const int32_t BUF_LEN, int32_t length_of_message, int8_t flag);
	int32_t setFlagsAndMsgSize(std::string& buf, const int64_t BUF_LEN, int64_t length_of_message, int8_t flag);
	int64_t sendStrBuf(std::string& str_buf, int64_t message_length);
	int32_t sendCharBuf(char * buf, int32_t BUF_LEN, int32_t message_length);
};
#endif//ApplicationLayer_h__