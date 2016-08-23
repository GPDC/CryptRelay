// ApplicationLayer.cpp
#ifdef __linux__
#include <iostream>
#include <sys/types.h>  // <Winsock2.h>, but not required on linux as of 2001
#include <sys/socket.h>	// <Winsock2.h>
#include <netdb.h>
#include <thread>
#include <limits.h>
#include <string.h>

#include "ApplicationLayer.h"
#include "IXBerkeleySockets.h"
#include "GlobalTypeHeader.h"
#endif//__linux__

#ifdef _WIN32
#include <iostream>
#include <WS2tcpip.h>
#include <thread>
#include <limits.h>

#include "ApplicationLayer.h"
#include "IXBerkeleySockets.h"
#include "GlobalTypeHeader.h"
#endif//_WIN32


#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")		//tell the linker that Ws2_32.lib file is needed.
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#endif//_WIN32


#ifdef __linux__
#define INVALID_SOCKET	(-1)	// To indicate INVALID_SOCKET, Windows returns (~0) from socket functions, and linux returns -1.
#define SOCKET_ERROR	(-1)	// Linux doesn't have a SOCKET_ERROR macro.
#define SD_RECEIVE      SHUT_RD//0x00			// This is for shutdown(); SD_RECEIVE is the code to shutdown receive operations.
#define SD_SEND         SHUT_WR//0x01			// ^
#define SD_BOTH         SHUT_RDWR//0x02			// ^
#endif//__linux__


#ifdef _WIN32
#pragma warning(disable:4996)			// disable deprecated warning for fopen()
#endif//_WIN32


// How many characters at the beginning of the buffer that should be
// reserved for usage of a flag, (1) and size of the message (2). (1 + 2 == 3)
const int8_t ApplicationLayer::CR_RESERVED_BUFFER_SPACE = 3;

// Flags for send() that indicate what the message is being used for.
// CR == CryptRelay
const int8_t ApplicationLayer::MsgFlags::NO_FLAG = 0;
const int8_t ApplicationLayer::MsgFlags::SIZE_NOT_ASSIGNED = 0;
const int8_t ApplicationLayer::MsgFlags::CHAT = 1;
const int8_t ApplicationLayer::MsgFlags::ENCRYPTED_CHAT = 2;
const int8_t ApplicationLayer::MsgFlags::FILE_NAME = 30;
const int8_t ApplicationLayer::MsgFlags::FILE_SIZE = 31;
const int8_t ApplicationLayer::MsgFlags::FILE_DATA = 32;
const int8_t ApplicationLayer::MsgFlags::ENCRYPTED_FILE_NAME = 33;
const int8_t ApplicationLayer::MsgFlags::ENCRYPTED_FILE_SIZE = 34;
const int8_t ApplicationLayer::MsgFlags::ENCRYPTED_FILE_DATA = 35;
const int8_t ApplicationLayer::MsgFlags::FILE_TRANSFER_COMPLETE = 36;

const int32_t ApplicationLayer::ARTIFICIAL_LENGTH_LIMIT_FOR_SEND = USHRT_MAX;

//mutex for the send() method.
std::mutex ApplicationLayer::SendMutex;


ApplicationLayer::ApplicationLayer(
	IXBerkeleySockets* IXBerkeleySocketsInstance,
	SOCKET socket_z,
	callback_fn_function_terminating * function_terminating_ptr,
	bool turn_verbose_output_on)
{
	// Checking for nullptrs on callbacks
	if (IXBerkeleySocketsInstance == nullptr
		|| function_terminating_ptr == nullptr)
	{
		throw "Exception thrown: nullptr in ApplicationLayer constructor";
	}

	if (turn_verbose_output_on == true)
		verbose_output = true;

	// Enable socket use on windows.
	WSAStartup();

	BerkeleySockets = IXBerkeleySocketsInstance;

	fd_socket = socket_z;

	// Setting the callbacks
	callbackTerminateLoopedReceiveMessages = function_terminating_ptr;

	// Specific to the ProcessRecvBuf state machine
	memset(incoming_file_name_from_peer_cstr, 0, INCOMING_FILE_NAME_FROM_PEER_SIZE);
}
ApplicationLayer::~ApplicationLayer()
{
	// Done with sockets.
#ifdef _WIN32
	WSACleanup();
#endif//_WIN32
}

// Returns 0, success
// Returns -1, error
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

// Returns 0, success
// Returns -1, error
int32_t ApplicationLayer::setFlagsAndMsgSize(std::string& buf, const int64_t BUF_LEN, int64_t length_of_msg, int8_t flag)
{
	const int64_t MAX_STRING_LENGTH = buf.max_size();

	// Making sure length_of_msg is not too big.
	if (length_of_msg > ApplicationLayer::ARTIFICIAL_LENGTH_LIMIT_FOR_SEND - CR_RESERVED_BUFFER_SPACE
		|| length_of_msg >= MAX_STRING_LENGTH - CR_RESERVED_BUFFER_SPACE)
	{
		std::cout << "Error: Message too long: setFlagsAndMsgSize()'s message_size was > ApplicationLayer::ARTIFICIAL_LENGTH_LIMIT_FOR_SEND - RESERVED_BUFFER_SPACE.\n";
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


int32_t ApplicationLayer::send(const char * sendbuf, int32_t amount_to_send)
{
	// Whatever thread gets here first, will lock
	// the door so that nobody else can come in.
	// That means all other threads will form a queue here,
	// and won't be able to go past this point until the
	// thread that got here first unlocks it.
	SendMutex.lock();
	total_bytes_sent = amount_to_send;
	
	do
	{
		bytes_sent = ::send(fd_socket, sendbuf, amount_to_send, 0);
		if (bytes_sent == SOCKET_ERROR)
		{
			BerkeleySockets->getError();
			perror("ERROR: send() failed.");
			DBG_DISPLAY_ERROR_LOCATION();
			BerkeleySockets->closesocket(fd_socket);
			SendMutex.unlock();
			return SOCKET_ERROR;
		}

		amount_to_send -= bytes_sent;

	} while (amount_to_send > 0);

	total_bytes_sent -= amount_to_send;

	// Unlock the door now that the thread is done with this function.
	SendMutex.unlock();

	return total_bytes_sent;// returning the amount of bytes sent.
}

// This method writes over the first CR_RESERVED_BUFFER_SPACE bytes of the given buffer.
// returns total bytes sent, success
// returns -1, error.
int64_t ApplicationLayer::sendChatStr(std::string& buf)
{
	int64_t message_length = buf.length();

	if (setFlagsAndMsgSize(buf, message_length, message_length, ApplicationLayer::MsgFlags::CHAT) == -1)
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

// This method writes over the first CR_RESERVED_BUFFER_SPACE bytes of the given buffer.
// returns total bytes sent, success
// returns -1, error.
int64_t ApplicationLayer::sendFileName(std::string name_of_file)
{
	int64_t message_length = name_of_file.length();

	// Put flag and message size into the first CR_RESERVED_BUFFER_SPACE chars.
	if (setFlagsAndMsgSize(name_of_file, message_length, message_length, ApplicationLayer::MsgFlags::FILE_NAME) == -1)
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

	if (setFlagsAndMsgSize(buf, BUF_LEN, message_length, ApplicationLayer::MsgFlags::FILE_SIZE) == -1)
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
	if (message_length > INT_MAX - CR_RESERVED_BUFFER_SPACE)
	{
		std::cout << "Error: sendFileData() was given too large of a message.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}

	if (setFlagsAndMsgSize(buf, BUF_LEN, message_length, ApplicationLayer::MsgFlags::FILE_DATA) == -1)
	{
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}
	
	// Send it
	int32_t total_bytes_sent = sendCharBuf(buf, BUF_LEN, message_length);
	return total_bytes_sent;
}

// This method writes over the first CR_RESERVED_BUFFER_SPACE bytes of the given buffer.
// returns total bytes sent, success
// returns -1, error.
int32_t ApplicationLayer::sendFileTransferComplete(char * buf, const int32_t BUF_LEN)
{
	if (buf == nullptr)
	{
		std::cout << "Error: sendFileSize() given nullptr.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}

	// The peer will know that the file transfer is complete based purely off of the
	// flag we are giving it, so there is no point in sending a message also.
	const int32_t FILE_TRANSFER_COMPLETE_MESSAGE_LENGTH = 0;
	int32_t message_length = FILE_TRANSFER_COMPLETE_MESSAGE_LENGTH;

	// Put the flag and size of the message into the buffer.
	if (setFlagsAndMsgSize(buf, BUF_LEN, message_length, ApplicationLayer::MsgFlags::FILE_TRANSFER_COMPLETE) == -1)
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
	if (BUF_LEN > (int64_t)sizeof(variable_to_convert) + CR_RESERVED_BUFFER_SPACE)
	{
		// Since the first CR_RESERVED_BUFFER_SPACE bytes shouldn't be touched,
		// put the variable_to_convert into the buffer starting at CR_RESERVED_BUFFER_SPACE.
		buf[CR_RESERVED_BUFFER_SPACE] = (char)(variable_to_convert >> 56);
		buf[CR_RESERVED_BUFFER_SPACE + 1] = (char)(variable_to_convert >> 48);
		buf[CR_RESERVED_BUFFER_SPACE + 2] = (char)(variable_to_convert >> 40);
		buf[CR_RESERVED_BUFFER_SPACE + 3] = (char)(variable_to_convert >> 32);
		buf[CR_RESERVED_BUFFER_SPACE + 4] = (char)(variable_to_convert >> 24);
		buf[CR_RESERVED_BUFFER_SPACE + 5] = (char)(variable_to_convert >> 16);
		buf[CR_RESERVED_BUFFER_SPACE + 6] = (char)(variable_to_convert >> 8);
		buf[CR_RESERVED_BUFFER_SPACE + 7] = (char)variable_to_convert;
		return 0;
	}

	DBG_DISPLAY_ERROR_LOCATION();
	return -1;
}

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
// of his message that he sent us. The CR_RESERVED_BUFFER_SPACE characters do not
// count as part of the message.
void ApplicationLayer::loopedReceiveMessages()
{
	// Receive until the peer shuts down the connection
	if (verbose_output == true)
		std::cout << "Recv loop started...\n";

	// Buffer for receiving data from peer
	static const int64_t recv_buf_len = 512;
	char recv_buf[recv_buf_len];

	const int32_t CONNECTION_GRACEFULLY_CLOSED = 0; // when recv() returns 0, it means gracefully closed.
	int32_t bytes = 0;
	while (1)
	{
		bytes = recv(fd_socket, (char *)recv_buf, recv_buf_len, 0);
		if (bytes > 0)
		{
			// State machine that processes recv_buf and decides what to do
			// based on the information in the buffer.
			if (decideActionBasedOnFlag(recv_buf, recv_buf_len, bytes) == -1)
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

			int32_t errchk = BerkeleySockets->getError(BerkeleySockets->getDisableConsoleOutput());

			// If errchk == BLOCKING_OPERATION_CANCELED, don't report the error.
			// else, report whatever error happened.
			if (errchk != BLOCKING_OPERATION_CANCELED)
			{
				std::cout << "recv() failed.\n";
				BerkeleySockets->outputSocketErrorToConsole(errchk);
				DBG_DISPLAY_ERROR_LOCATION();
				if (errchk == CONNECTION_RESET)
				{
					std::cout << "Peer might have hit ctrl-c.\n";
				}
				if (is_file_done_being_written == false)
				{
					// Close the file that was being written inside the decideActionBasedOnFlag() state machine.
					if (WriteFile != nullptr)
					{
						if (fclose(WriteFile) != 0)
						{
							DBG_DISPLAY_ERROR_LOCATION();
							perror("Error closing file for writing in binary mode.");
						}
					}
					std::cout << "File transfer was interrupted. File name: " << incoming_file_name_from_peer << "\n";
				}
			}

			break;
		}
		else if (bytes == CONNECTION_GRACEFULLY_CLOSED)
		{
			if (is_file_done_being_written == false)
			{
				// Close the file that was being written inside the decideActionBasedOnFlag() state machine.
				if (WriteFile != nullptr)
				{
					if (fclose(WriteFile) != 0)
					{
						DBG_DISPLAY_ERROR_LOCATION();
						perror("Error closing file for writing in binary mode.");
					}
				}
				std::cout << "File transfer was interrupted. File name: " << incoming_file_name_from_peer << "\n";
			}

			std::cout << "Connection with peer has been gracefully closed.\n";
			break;
		}
	}

	// Say that this function is exiting.
	callbackTerminateLoopedReceiveMessages();

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
	if (BerkeleySockets->shutdown(fd_socket, SD_BOTH) == -1)	// SD_BOTH == shutdown both send and receive on the socket.
	{
		BerkeleySockets->getError();
		std::cout << "Error: shutdown() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		BerkeleySockets->closesocket(fd_socket);
		return -1;
	}
	BerkeleySockets->closesocket(fd_socket);

	return 0;
}

// returns total bytes sent, success
// returns -1, error
int32_t ApplicationLayer::sendCharBuf(char * buf, const int32_t BUF_LEN, int32_t message_length)
{
	int32_t total_amount_to_send = 0;
	if (message_length > INT_MAX - CR_RESERVED_BUFFER_SPACE)
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
			BerkeleySockets->getError();
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
			BerkeleySockets->getError();
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







// ******************************************************************
// *             Things related to Recv state machine               *
// ******************************************************************

// Returns 0, success, and wants to recv() more bytes.
// Returns -1, error
int32_t ApplicationLayer::decideActionBasedOnFlag(char * recv_buf, int64_t recv_buf_len, int64_t received_bytes)
{
	// the current cursor position inside the buffer.
	position_in_recv_buf = 0;	

	bool done = false;
	bool error = false;

	// RecvStateMachine
	while (done == false)
	{
		done = true;

		switch (state)
		{
		case WRITE_FILE_FROM_PEER:
		{
			if ( (position_in_recv_buf < received_bytes) && (position_in_message < message_size) )
			{
				if (writeFileFromPeer(recv_buf, received_bytes) == -1)
				{
					error = true;
					done = true;
					break;
				}
			}

			if (position_in_message == message_size)// must have a new message from the peer.
			{
				state = CHECK_FOR_FLAG;
				done = false;
			}

			break;
		}
		case OUTPUT_CHAT_FROM_PEER:
		{
			// Print out the message to terminal
			coutChatMsgFromPeer(recv_buf, received_bytes);

			if (position_in_recv_buf >= received_bytes)
			{
				if (position_in_message == message_size)// must have a new message from the peer.
				{
					state = CHECK_FOR_FLAG;
					done = false;
				}

				return 0; // go recv() again to get more bytes
			}
			else if (position_in_message == message_size)// must have a new message from the peer.
			{
				state = CHECK_FOR_FLAG;
				done = false;
				break;
			}

			// this shouldn't be reached
			error = true;
			done = true;
			break;
		}
		case TAKE_FILE_NAME_FROM_PEER:
		{
			// Set the file name variable.
			if (assignFileNameFromPeerCStr(recv_buf, received_bytes) == FINISHED_ASSIGNING_FILE_NAME_FROM_PEER)
			{
				// If the file name was too big, then say so, but don't error.
				if (position_in_message >= INCOMING_FILE_NAME_FROM_PEER_SIZE - RESERVED_NULL_CHAR_FOR_FILE_NAME)
				{
					std::cout << "Receive File: WARNING: Peer's file name is too long. Exceeded " << INCOMING_FILE_NAME_FROM_PEER_SIZE << " characters.\n";
					std::cout << "Receive File: File name will be incorrect on your computer.\n";
				}

				// Null terminate it.
				if (position_in_message <= INCOMING_FILE_NAME_FROM_PEER_SIZE)
				{
					incoming_file_name_from_peer_cstr[position_in_message] = '\0';
				}

				// Convert it to a std::string
				std::string temporary_incoming_file_name_from_peer(incoming_file_name_from_peer_cstr);
				incoming_file_name_from_peer = temporary_incoming_file_name_from_peer;
				std::cout << "\n";
				std::cout << "# Incoming file name: " << incoming_file_name_from_peer << "\n";
			}

			// Must have a new message from the peer if this is true
			if (position_in_message == message_size)
			{
				state = CHECK_FOR_FLAG;
				done = false;
			}
			else
			{
				// this shouldn't be reached
				error = true;
				done = true;
			}

			// Linux:
			// Max file name length is 255 chars on most filesystems, and max path 4096 chars.
			//
			// Windows:
			// Max file name length + subdirectory path is 255 chars. or MAX_PATH .. 260 chars?
			// 1+2+256+1 or [drive][:][path][null] = 260
			// This is not strictly true as the NTFS filesystem supports paths up to 32k characters.
			// You can use the win32 api and "\\?\" prefix the path to use greater than 260 characters. 
			// However using the long path "\\?\" is not a very good idea.

			break;
		}
		case TAKE_FILE_SIZE_FROM_PEER:
		{
			// convert the file size in the buffer from network int64_t to
			// host int64_t. It assigns the variable incoming_file_size_from_peer
			// a value.
			if (assignFileSizeFromPeer(recv_buf, recv_buf_len, received_bytes) == FINISHED_ASSIGNING_FILE_SIZE_FROM_PEER)
			{
				std::cout << "# Size of Peer's file: " << incoming_file_size_from_peer << "\n";

				// If the peer gave us a negative file size, error.
				if (incoming_file_size_from_peer < 0)
				{
					error = true;
					done = true;
				}
				else if (openFileForWrite() == -1)// Open the file for writing.
				{
					error = true;
					done = true;
				}
				else // success
				{
					state = CHECK_FOR_FLAG;
					done = false;
				}
			}
			break;
		}
		case FILE_TRANSFER_COMPLETE:
		{
			// Cout to console whether it completed successfully or unsuccessfully.
			coutFileTransferSuccessOrFail();

			closeFileForWrite();
			state = CHECK_FOR_FLAG;
			done = false;

			break;
		}
		case CHECK_FOR_FLAG:
		{
			// Because if we are here, that means we currently
			// aren't in a message at the moment.
			position_in_message = 0;

			if (position_in_recv_buf < received_bytes)
			{
				message_flag = (unsigned char)recv_buf[position_in_recv_buf];
				++position_in_recv_buf;// always have to ++ this in order to access the next element in the array.
				state = CHECK_MESSAGE_SIZE_PART_ONE;
				done = false;
				break;
			}
			break;
		}
		case CHECK_MESSAGE_SIZE_PART_ONE:
		{
			// Getting half of the u_short size of the message
			if (position_in_recv_buf < received_bytes)
			{
				message_size_part_one = (unsigned char)recv_buf[position_in_recv_buf];
				message_size_part_one = message_size_part_one << 8;
				++position_in_recv_buf;
				state = CHECK_MESSAGE_SIZE_PART_TWO;
				done = false;
				break;
			}
			break;
		}
		case CHECK_MESSAGE_SIZE_PART_TWO:
		{
			// getting the second half of the u_short size of the message
			if (position_in_recv_buf < received_bytes)
			{
				message_size_part_two = (unsigned char)recv_buf[position_in_recv_buf];
				message_size = message_size_part_one | message_size_part_two;
				++position_in_recv_buf;

				// Now that we have a flag and the size of the message,
				// set the state based on the message flag
				done = false;
				if (message_flag == ApplicationLayer::MsgFlags::FILE_DATA)
					state = WRITE_FILE_FROM_PEER;
				else if (message_flag == ApplicationLayer::MsgFlags::FILE_NAME)
					state = TAKE_FILE_NAME_FROM_PEER;
				else if (message_flag == ApplicationLayer::MsgFlags::FILE_SIZE)
					state = TAKE_FILE_SIZE_FROM_PEER;
				else if (message_flag == ApplicationLayer::MsgFlags::FILE_TRANSFER_COMPLETE)
					state = FILE_TRANSFER_COMPLETE;
				else if (message_flag == ApplicationLayer::MsgFlags::CHAT)
					state = OUTPUT_CHAT_FROM_PEER;
				else
				{
					done = true;
					error = true;
				}
			}
			break;
		}
		default:// currently nothing should cause this to execute.
		{
			std::cout << "State machine for recv() got to default. Exiting.\n";
			error = true;
			done = true;
		}
		}//end switch
	}

	if (error == true)
	{
		std::cout << "Exiting with error, state machine for recv().\n";

		done = true;

		if (WriteFile != nullptr)
		{
			if (fclose(WriteFile) != 0)		// 0 == successful close
			{
				perror("Error closing file for writing in binary mode.");
				std::cout << "Error occured in RecvStateMachine, case ERROR_STATE:\n";
			}
		}
		return -1;
	}

	return 0; // recv() more data
}

// For use with RecvBufStateMachine only.
int32_t ApplicationLayer::assignFileSizeFromPeer(char * recv_buf, int64_t recv_buf_len, int64_t received_bytes)
{
	// The peer is sending the size of his file as a int64_t.
	// This means we will receive sizeof(int64_t) + CR_RESERVED_BUFFER_SPACE bytes.
	// Those 8 bytes (the size of the file) will be coming to us in network byte order.
	// Referring to each one of those 8 bytes as a 'file_size_fragment'.
	if (file_size_fragment == 0)
	{
		incoming_file_size_from_peer = 0;
	}

	while (file_size_fragment < (int64_t)sizeof(int64_t))
	{
		if (position_in_recv_buf >= received_bytes)
		{
			return RECV_AGAIN;
		}

		// If, for some reason, the peer has sent us a message that is, say, only 4
		// bytes long, even though it should be sizeof(int64_t) bytes long, then
		// it will be caught here.
		if (position_in_message == message_size)
		{
			return FINISHED_ASSIGNING_FILE_SIZE_FROM_PEER;
		}

		// Converting it to host byte order.
		incoming_file_size_from_peer |= (
			(int64_t)(unsigned char)recv_buf[position_in_recv_buf])
			<< (64 - (8 * (file_size_fragment + 1))
				);
		++position_in_recv_buf;
		++position_in_message;
		++file_size_fragment;
	}

	// Just incase the peer sent us, for some unknown reason, extra bytes,
	// then we will ignore them.
	position_in_message = message_size;

	file_size_fragment = 0;

	return FINISHED_ASSIGNING_FILE_SIZE_FROM_PEER;
}

// Necessary to do anything with sockets on Windows
// Returns 0, success.
// Returns a WSAERROR code if failed.
int32_t ApplicationLayer::WSAStartup()
{
#ifdef _WIN32
	int32_t errchk = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (errchk != 0)
	{
		std::cout << "WSAStartup failed, WSAERROR: " << errchk << "\n";
		return errchk;
	}
#endif//_WIN32
	return 0;
}

void ApplicationLayer::coutChatMsgFromPeer(char * recv_buf, int64_t received_bytes)
{
	std::cout << "\n";
	std::cout << "Peer: ";
	for (;
		(position_in_recv_buf < received_bytes) && (position_in_message < message_size);
		++position_in_recv_buf, ++position_in_message)
	{
		std::cout << recv_buf[position_in_recv_buf];
	}
	std::cout << "\n";
}

int32_t ApplicationLayer::assignFileNameFromPeerCStr(char * recv_buf, int64_t received_bytes)
{
	for (;
		(position_in_recv_buf < received_bytes)
		&& (position_in_message < message_size)
		&& (position_in_message < INCOMING_FILE_NAME_FROM_PEER_SIZE - RESERVED_NULL_CHAR_FOR_FILE_NAME);
		++position_in_recv_buf, ++position_in_message)
	{
		incoming_file_name_from_peer_cstr[position_in_message] = recv_buf[position_in_recv_buf];
	}

	// If it turns out that the file name was too long, then ignore the rest of the bytes in the message.
	if (position_in_message == INCOMING_FILE_NAME_FROM_PEER_SIZE - RESERVED_NULL_CHAR_FOR_FILE_NAME)
		position_in_message = message_size;

	if (position_in_message == message_size)
		return FINISHED_ASSIGNING_FILE_NAME_FROM_PEER;
	else
		return RECV_AGAIN;
}

int32_t ApplicationLayer::openFileForWrite()
{
	WriteFile = fopen(incoming_file_name_from_peer.c_str(), "wb");
	if (WriteFile == nullptr)
	{
		DBG_DISPLAY_ERROR_LOCATION();
		perror("Error opening file for writing in binary mode.");
		return -1;
	}

	is_file_done_being_written = false;

	// Set it back to default value
	total_bytes_written_to_file = 0;

	return 0;
}

void ApplicationLayer::closeFileForWrite()
{
	if (fclose(WriteFile) != 0)		// 0 == successful close
	{
		DBG_DISPLAY_ERROR_LOCATION();
		perror("Error closing file for writing in binary mode");
	}

	// Set it back to default value
	total_bytes_written_to_file = 0;

	is_file_done_being_written = true;

#ifdef _DEBUG
	if (position_in_message < message_size)
	{
		DBG_TXT("position_in_message < message_size, closefileforwrite");
	}
	if (position_in_message == message_size)
	{
		DBG_TXT("position_in_message == message_size, everything A OK closefileforwrite");
	}
	if (position_in_message > message_size)
	{
		DBG_DISPLAY_ERROR_LOCATION();
		std::cout << "ERROR: position_in_message > message_size , closefileforwrite\n";
	}
#endif// _DEBUG
}

void ApplicationLayer::coutFileTransferSuccessOrFail()
{
	// Cout the difference in bytes written compared to what the file size should be.
	std::cout << "# Expected " << incoming_file_size_from_peer << ".\n";
	std::cout << "# Wrote " << total_bytes_written_to_file << "\n";
	std::cout << "# Difference: " << incoming_file_size_from_peer - total_bytes_written_to_file << "\n";

	// If it didn't write as many bytes as it thought it should
	if (incoming_file_size_from_peer - total_bytes_written_to_file != 0)
	{
		std::cout << "# File transfer from peer is INCOMPLETE and will not recover: " << incoming_file_name_from_peer << "\n";
		std::cout << "\n";
	}
	else
	{
		// Everything is fine, tell the user it is complete.
		std::cout << "# File transfer from peer is complete: " << incoming_file_name_from_peer << "\n";
		std::cout << "\n";
	}
}

int32_t ApplicationLayer::writeFileFromPeer(char * recv_buf, int64_t received_bytes)
{
	int64_t amount_to_write = message_size - position_in_message;
	if (amount_to_write > received_bytes - position_in_recv_buf)
		amount_to_write = received_bytes - position_in_recv_buf;

	if (WriteFile == nullptr)
	{
		return -1; // set error state please
	}

	bytes_written = fwrite(recv_buf + position_in_recv_buf, 1, (size_t)(amount_to_write), WriteFile);
	if (!bytes_written)
	{
		perror("Error while writing the file from peer");
		if (fclose(WriteFile))
		{
			perror("Error closing file for writing");
		}
		// delete file here? This could be dangerous, and should
		// require user confirmation. Or check to make sure
		// file didn't exist before trying to open it, then
		// it would be safe to delete.

		return -1; // set error state please
	}
	else
	{
		total_bytes_written_to_file += bytes_written;
		position_in_message += bytes_written;
		position_in_recv_buf += bytes_written;
	}

	return 0;
}