// ApplicationLayer.cpp
#ifdef __linux__
#include <iostream>
#include <sys/types.h>  // <Winsock2.h>, but not required on linux as of 2001
#include <sys/socket.h>	// <Winsock2.h>
#include <netdb.h>
#include <thread>
#include <limits>

#include "ApplicationLayer.h"
#include "SocketClass.h"
#include "GlobalTypeHeader.h"
#include "Protocol.h"
#endif//__linux__

#ifdef _WIN32
#include <iostream>
#include <WS2tcpip.h>
#include <thread>
#include <limits>

#include "ApplicationLayer.h"
#include "SocketClass.h"
#include "GlobalTypeHeader.h"
#include "Protocol.h"
#endif//_WIN32


#ifdef __linux__

#ifndef INVALID_SOCKET
#define INVALID_SOCKET	((SOCKET)(~0))	// To indicate INVALID_SOCKET, Windows returns (~0) from socket functions, and linux returns -1.
#endif//INVALID_SOCKET

#ifndef SOCKET_ERROR
#define SOCKET_ERROR	(-1)			// To indicate SOCKET_ERROR, Windows returns -1 from socket functions, and linux returns -1.
										// Linux doesn't distinguish between INVALID_SOCKET and SOCKET_ERROR. It just returns -1 on error.
#endif//SOCKET_ERROR

#endif // __linux__


// How many characters at the beginning of the buffer that should be
// reserved for usage of a flag, (1) and size of the message (2). (1 + 2 == 3)
const int8_t ApplicationLayer::CR_RESERVED_BUFFER_SPACE = 3;

//
const int32_t ApplicationLayer::ARTIFICIAL_LENGTH_LIMIT_FOR_SEND = USHRT_MAX;

//mutex for the send() method.
std::mutex ApplicationLayer::SendMutex;


ApplicationLayer::ApplicationLayer(SocketClass* SocketClassInstance)
{
	Socket = SocketClassInstance;
}
ApplicationLayer::~ApplicationLayer()
{

}

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
// Returns 0 on success, -1 on error
int32_t ApplicationLayer::setFlagsAndMsgSize(char * buf, const int32_t BUF_LEN, int32_t length_of_msg, int8_t flag)
{
	if (buf == nullptr)
	{
		std::cout << "Error: setFlagsAndMsgSize() nullptr.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}

	// Making sure length_of_msg is not too big.
	if (length_of_msg > ApplicationLayer::ARTIFICIAL_LENGTH_LIMIT_FOR_SEND - CR_RESERVED_BUFFER_SPACE)
	{
		std::cout << "Error: setFlagsAndMsgSize()'s message_size was > ApplicationLayer::ARTIFICIAL_LENGTH_LIMIT_FOR_SEND - RESERVED_BUFFER_SPACE.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}

	// Preventing someone from unknowingly breaking the program if they change
	// the CR_RESERVED_BUFFER_SPACE variable and don't check everything carefully.
	static_assert(CR_RESERVED_BUFFER_SPACE >= 3, "Error: CR_RESERVED_BUFFER_SPACE < 3.\n");

	// Set the flag and size of the message
	if (BUF_LEN >= CR_RESERVED_BUFFER_SPACE)
	{
		// Copy the type of message flag into the buf
		buf[0] = flag;
		// Copy the size of the message into the buf as big endian.
		buf[1] = (char)(length_of_msg >> 8);
		buf[2] = (char)length_of_msg;
		return 0;
	}
	else
	{
		std::cout << "Programmer error. BUF_LEN < 3.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}
}

// Returns 0 on success, -1 on error
int32_t ApplicationLayer::setFlagsAndMsgSize(std::string& buf, const int64_t BUF_LEN, int64_t length_of_msg, int8_t flag)
{
	// Making sure length_of_msg is not too big.
	if (length_of_msg > ApplicationLayer::ARTIFICIAL_LENGTH_LIMIT_FOR_SEND - CR_RESERVED_BUFFER_SPACE)
	{
		std::cout << "Error: setFlagsAndMsgSize()'s message_size was > ApplicationLayer::ARTIFICIAL_LENGTH_LIMIT_FOR_SEND - RESERVED_BUFFER_SPACE.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}

	// Preventing someone from unknowingly breaking the program if they change
	// the CR_RESERVED_BUFFER_SPACE variable and don't check everything carefully.
	static_assert(CR_RESERVED_BUFFER_SPACE >= 3, "Error: CR_RESERVED_BUFFER_SPACE < 3.\n");

	// Copy the type of message flag into the buf
	// Format: str.insert(position, how_many, char_to_insert);
	buf.insert(0, 1, flag);
	// Copy the size of the message into the buf as big endian.
	buf.insert(1, 1, (char)(length_of_msg >> 8));
	buf.insert(2, 1, (char)length_of_msg);

	return 0;
}


// All information that gets sent over the network MUST go through
// this method. This is to avoid abnormal behavior / problems
// with multiple threads trying to send() using the same socket.
// The flag argument is to tell the receiver of these messages
// how to interpret the incoming message. For example as a file,
// or as a chat message.
// To see a list of flags, look in the header file.
// Returns SOCKET_ERROR on error, and total amount of bytes sent on success.
int32_t ApplicationLayer::send(const char * sendbuf, int32_t amount_to_send)
{
	// Whatever thread gets here first, will lock
	// the door so that nobody else can come in.
	// That means all other threads will form a queue here,
	// and won't be able to go past this point until the
	// thread that got here first unlocks it.
	SendMutex.lock();
	total_amount_sent = amount_to_send;
	
	do
	{
		bytes_sent = ::send(Socket->fd_socket, sendbuf, amount_to_send, 0);
		if (bytes_sent == SOCKET_ERROR)
		{
			Socket->getError();
			perror("ERROR: send() failed.");
			DBG_DISPLAY_ERROR_LOCATION();
			Socket->closesocket(Socket->fd_socket);
			SendMutex.unlock();
			return SOCKET_ERROR;
		}

		amount_to_send -= bytes_sent;

	} while (amount_to_send > 0);

	total_amount_sent -= amount_to_send;

	// Unlock the door now that the thread is done with this function.
	SendMutex.unlock();

	return total_amount_sent;// returning the amount of bytes sent.
}

// returns total bytes sent, succes
// returns -1, error.
int64_t ApplicationLayer::sendChatStr(std::string& buf)
{
	int64_t message_length = buf.length();
	if (setFlagsAndMsgSize(buf, message_length, message_length, Protocol::MsgFlags::CHAT) == -1)
	{
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}

	// Assign the length of the message once again b/c
	// it was modified in setFlagsAndMsgSize()
	message_length = buf.length();

	// Send it
	int64_t total_bytes_sent = sendStrBuf(buf, message_length);
	return total_bytes_sent;
}

// returns total bytes sent, succes
// returns -1, error
int64_t ApplicationLayer::sendFileName(std::string name_of_file)
{
	int64_t message_length = name_of_file.length();

	// Put flag and message size into the first CR_RESERVED_BUFFER_SPACE chars.
	if (setFlagsAndMsgSize(name_of_file, message_length, message_length, Protocol::MsgFlags::FILE_NAME) == -1)
	{
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}

	// Check length again since the length was changed in the setFlagsAndMsgSize()
	message_length = name_of_file.length();

	// Send it
	int64_t total_bytes_sent = sendStrBuf(name_of_file, message_length);
	return total_bytes_sent;
}

// This method writes over the first CR_RESERVED_BUFFER_SPACE bytes of the given buffer.
// returns total bytes sent, success
// returns -1, error.
int32_t ApplicationLayer::sendFileSize(char * buf, const int32_t BUF_LEN, int64_t file_size)
{
	if (buf == nullptr)
	{
		std::cout << "Error: sendFileSize() given nullptr.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}

	int32_t message_length = sizeof(file_size);

	if (setFlagsAndMsgSize(buf, BUF_LEN, message_length, Protocol::MsgFlags::FILE_SIZE) == -1)
	{
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}

	// Put the size of the file into the buffer
	if (intoBufferHostToNetworkLongLong(buf, BUF_LEN, file_size) == -1)
		return -1;

	// Send it
	int32_t total_bytes_sent = sendCharBuf(buf, BUF_LEN, message_length);
	return total_bytes_sent;
}

// This method writes over the first CR_RESERVED_BUFFER_SPACE bytes of the given buffer.
// returns total bytes sent, success
// returns -1, error.
int32_t ApplicationLayer::sendFileData(char * buf, const int32_t BUF_LEN, int32_t message_length)
{
	if (buf == nullptr)
	{
		std::cout << "Error: sendFileData() given nullptr.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}
	if (message_length > MAXINT32 - CR_RESERVED_BUFFER_SPACE)
	{
		std::cout << "Error: sendFileData() was given too large of a message.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}

	if (setFlagsAndMsgSize(buf, BUF_LEN, message_length, Protocol::MsgFlags::FILE_DATA) == -1)
	{
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}
	
	// Send it
	int32_t total_bytes_sent = sendCharBuf(buf, BUF_LEN, message_length);
	return total_bytes_sent;
}

// Using the given variable, it will convert it to network long long and place it into the given buffer.
int32_t ApplicationLayer::intoBufferHostToNetworkLongLong(char * buf, const int64_t BUF_LEN, int64_t variable_to_convert)
{
	if (BUF_LEN > (int64_t)sizeof(variable_to_convert))
	{
		buf[3] = (char)(variable_to_convert >> 56);
		buf[4] = (char)(variable_to_convert >> 48);
		buf[5] = (char)(variable_to_convert >> 40);
		buf[6] = (char)(variable_to_convert >> 32);
		buf[7] = (char)(variable_to_convert >> 24);
		buf[8] = (char)(variable_to_convert >> 16);
		buf[9] = (char)(variable_to_convert >> 8);
		buf[10] = (char)variable_to_convert;
		return 0;
	}

	DBG_DISPLAY_ERROR_LOCATION();
	return -1;
}


// remote_host is /* optional */    default == "Peer".
// remote_host is the IP that we are connected to.
// To find out who we were connected to, use getnameinfo()
void ApplicationLayer::loopedReceiveMessages()
{
	// Helpful Information:
	// Any time a 'message' is mentioned, it is referring to the idea of
	// a whole message. That message may be broken up into packets and sent over the
	// network, and when it gets to this recv() loop, it will read in as much as it can
	// at a time. The entirety of a message is not always contained in the recv_buf,
	// because the message could be so large that you will have to recv() multiple times
	// in order to get the whole message. 
	// References to 'message' have nothing to do with the recv_buf.
	// the size and type of a message is determined by the peer, and the peer tells us
	// the type and size of the message in the first CR_RESERVED_BUFFER_SPACE characters
	// of his message that he sent us.

	// Receive until the peer shuts down the connection
	if (global_verbose == true)
		std::cout << "Recv loop started...\n";

	// Buffer for receiving data from peer
	static const int64_t recv_buf_len = 512;
	char recv_buf[recv_buf_len];

	const int32_t CONNECTION_GRACEFULLY_CLOSED = 0; // when recv() returns 0, it means gracefully closed.
	int32_t bytes = 0;
	while (1)
	{
		bytes = ::recv(Socket->fd_socket, (char *)recv_buf, recv_buf_len, 0);
		if (bytes > 0)
		{
			// State machine that processes recv_buf and decides what to do
			// based on the information in the buffer.
			if (ProcRecv.decideActionBasedOnFlag(recv_buf, recv_buf_len, bytes) == true)
				break;
		}
		else if (bytes == SOCKET_ERROR)
		{
#ifdef __linux__
			const int32_t CONNECTION_RESET = ECONNRESET;
			const int32_t BLOCKING_OPERATION_CANCELED = EINTR;
#endif// __linux__
#ifdef _WIN32
			const int32_t CONNECTION_RESET = WSAECONNRESET;
			const int32_t BLOCKING_OPERATION_CANCELED = WSAEINTR;
#endif// _WIN32

			int32_t errchk = Socket->getError(Socket->DISABLE_CONSOLE_OUTPUT);

			// If errchk == BLOCKING_OPERATION_CANCELED, don't report the error.
			// else, report whatever error happened.
			if (errchk != BLOCKING_OPERATION_CANCELED)
			{
				std::cout << "recv() failed.\n";
				Socket->outputSocketErrorToConsole(errchk);
				DBG_DISPLAY_ERROR_LOCATION();
				if (errchk == CONNECTION_RESET)
				{
					std::cout << "Peer might have hit ctrl-c.\n";
				}
				if (ProcRecv.getIsFileDoneBeingWritten() == false)
				{
					// Close the file that was being written inside the decideActionBasedOnFlag() state machine.
					if (ProcRecv.WriteFile != nullptr)
					{
						if (fclose(ProcRecv.WriteFile) != 0)
						{
							perror("Error closing file for writing in binary mode.\n");
						}
					}
					std::cout << "File transfer was interrupted. File name: " << ProcRecv.getIncomingFileNameFromPeer() << "\n";
				}
			}

			break;
		}
		else if (bytes == CONNECTION_GRACEFULLY_CLOSED)
		{
			std::cout << "Connection with peer has been gracefully closed.\n";
			break;
		}
	}
	exit_now = true;
	std::cout << "\n";
	std::cout << "# Press 'Enter' to exit CryptRelay.\n";

	return;
}
// returns -1, error
// returns 0, success
int32_t ApplicationLayer::endConnection()
{
	// This will shutdown the connection on the socket.
	// closesocket() will interrupt recv() if it is currently blocking.
	// Done communicating with peer. Proceeding to exit.
	if (Socket->shutdown(Socket->fd_socket, SD_BOTH) == true)	// SD_BOTH == shutdown both send and receive on the socket.
	{
		Socket->getError();
		std::cout << "Error: shutdown() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		Socket->closesocket(Socket->fd_socket);
		return -1;
	}
	Socket->closesocket(Socket->fd_socket);

	return 0;
}

// returns total bytes sent, success
// returns -1, error
int32_t ApplicationLayer::sendCharBuf(char * buf, const int32_t BUF_LEN, int32_t message_length)
{
	int32_t total_amount_to_send = 0;
	if (message_length > MAXINT32 - CR_RESERVED_BUFFER_SPACE)
	{
		std::cout << "Message too big.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}
	else
	{
		// Since we added CR_RESERVED_BUFFER_SPACE characters to the buf,
		// add CR_RESERVED_BUFFER_SPACE to the message length.
		total_amount_to_send = message_length + CR_RESERVED_BUFFER_SPACE;
	}

	int32_t total_bytes_sent = 0;
	int32_t amount_left_to_send = total_amount_to_send;
	int32_t send_this_amount = 0;
	int32_t bytes_sent = 0;
	
	// Making sure there is no data loss when doing: send_this_amount = amount_left_to_send
	static_assert(sizeof(ARTIFICIAL_LENGTH_LIMIT_FOR_SEND) <= sizeof(amount_left_to_send),
		"Error: sizeof(ARTIFICIAL_LENGTH_LIMIT_FOR_SEND) <= sizeof(amount_left_to_send).\n");
	do
	{
		// Making sure to not hog the mutex'd send() with just this communication.
		// In order to do that we limit the amount we give to send().
		if (amount_left_to_send > ARTIFICIAL_LENGTH_LIMIT_FOR_SEND)
		{
			send_this_amount = ARTIFICIAL_LENGTH_LIMIT_FOR_SEND;
		}
		else
		{
			send_this_amount = amount_left_to_send;
		}

		// Send it
		bytes_sent = send(buf, send_this_amount);
		if (bytes_sent == SOCKET_ERROR)
		{
			Socket->getError();
			return -1;
		}
		else
		{
			total_bytes_sent += bytes_sent;
			amount_left_to_send -= bytes_sent;
		}
	} while (bytes_sent > 0 && amount_left_to_send > 0);


	return total_bytes_sent;
}

// returns total bytes sent, success
// returns -1, error
int64_t ApplicationLayer::sendStrBuf(std::string& str_buf, int64_t message_length)
{
	int64_t total_amount_to_send = message_length;
	int64_t total_bytes_sent = 0;
	int64_t amount_left_to_send = total_amount_to_send;
	int32_t send_this_amount = 0;
	int32_t bytes_sent = 0;

	// Making sure there is no data loss when doing: send_this_amount = amount_left_to_send
	static_assert(sizeof(ARTIFICIAL_LENGTH_LIMIT_FOR_SEND) <= sizeof(amount_left_to_send),
		"Error: sizeof(ARTIFICIAL_LENGTH_LIMIT_FOR_SEND) <= sizeof(amount_left_to_send).\n");
	do
	{
		// Making sure to not hog the mutex'd send() with just this communication.
		// In order to do that we limit the amount we give to send().
		if (amount_left_to_send > ARTIFICIAL_LENGTH_LIMIT_FOR_SEND)
		{
			send_this_amount = ARTIFICIAL_LENGTH_LIMIT_FOR_SEND;
		}
		else
		{
			send_this_amount = (int32_t)amount_left_to_send;
		}

		bytes_sent = send(str_buf.c_str(), send_this_amount);
		if (bytes_sent == SOCKET_ERROR)
		{
			Socket->getError();
			return -1;
		}
		else
		{
			total_bytes_sent += bytes_sent;
			amount_left_to_send -= bytes_sent;
		}
	} while (bytes_sent > 0 && amount_left_to_send > 0);


	return total_bytes_sent;
}