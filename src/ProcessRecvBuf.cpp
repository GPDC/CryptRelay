// ProcessRecvBuf.cpp
#ifdef __linux__
#include <iostream>

#include "ProcessRecvBuf.h"
#include "GlobalTypeHeader.h"
#include "ApplicationLayer.h"
#endif//__linux__

#ifdef _WIN32
#include <iostream>

#include "ProcessRecvBuf.h"
#include "GlobalTypeHeader.h"
#include "ApplicationLayer.h"
#endif//_WIN32


#ifdef _WIN32
#pragma warning(disable:4996)		// disable deprecated warning for fopen()
#endif//_WIN32


ProcessRecvBuf::ProcessRecvBuf()
{
	memset(incoming_file_name_from_peer_cstr, 0, INCOMING_FILE_NAME_FROM_PEER_SIZE);
}
ProcessRecvBuf::~ProcessRecvBuf()
{

}

// Returns false when everything is fine, and wants to be given more to process.
bool ProcessRecvBuf::decideActionBasedOnFlag(char * recv_buf, int64_t recv_buf_len, int64_t received_bytes)
{
	position_in_recv_buf = 0;	// the current cursor position inside the buffer.

	// RecvStateMachine
	while (1)
	{
		switch (state)
		{
		case WRITE_FILE_FROM_PEER:
		{
			if (position_in_message < message_size)
			{
				int64_t amount_to_write = message_size - position_in_message;
				if (amount_to_write > received_bytes - position_in_recv_buf)
					amount_to_write = received_bytes - position_in_recv_buf;

				bytes_written = fwrite(recv_buf + position_in_recv_buf, 1, (size_t)(amount_to_write), WriteFile);
				if (!bytes_written)
				{
					perror("Error while writing the file from peer");

					// close file
					if (WriteFile != nullptr)
					{
						if (fclose(WriteFile))
						{
							perror("Error closing file for writing");
						}
					}

					// delete file here? This could be dangerous, and should
					// require user confirmation. Or check to make sure
					// file didn't exist before trying to open it, then
					// it would be safe to delete.

					state = ERROR_STATE;
					break;
				}
				else
				{
					total_bytes_written_to_file += bytes_written;
					position_in_message += bytes_written;
					position_in_recv_buf += bytes_written;

					if (total_bytes_written_to_file >= incoming_file_size_from_peer)
					{
						std::cout << "Finished receiving file from peer.\n";
						std::cout << "Expected " << incoming_file_size_from_peer << ".\n";
						std::cout << "Wrote " << total_bytes_written_to_file << "\n";
						std::cout << "Difference: " << incoming_file_size_from_peer - total_bytes_written_to_file << "\n";
						state = CLOSE_FILE_FOR_WRITE;
						is_file_done_being_written = true;
						break;
					}
				}
			}

			if (position_in_recv_buf >= received_bytes)
			{
				return false; // go recv() again to get more bytes
			}
			else if (position_in_message == message_size)// must have a new message from the peer.
			{
				state = CHECK_FOR_FLAG;
				break;
			}

			// this shouldn't be reached
			std::cout << "Unrecognized message?\n";
			std::cout << "Unreachable area, switchcase DECIDE_ACTION, recv() loop\n";
			std::cout << "Catastrophic failure.\n";
			state = ERROR_STATE;

			break;
		}
		case OUTPUT_CHAT_FROM_PEER:
		{
			// Print out the message to terminal
			std::cout << "\n";
			std::cout << "Peer: ";
			for (; (position_in_recv_buf < received_bytes) && (position_in_message < message_size);
				++position_in_recv_buf, ++position_in_message)
			{
				std::cout << recv_buf[position_in_recv_buf];
			}
			std::cout << "\n";

			if (position_in_recv_buf >= received_bytes)
			{
				if (position_in_message == message_size)// must have a new message from the peer.
					state = CHECK_FOR_FLAG;

				return false; // go recv() again to get more bytes
			}
			else if (position_in_message == message_size)// must have a new message from the peer.
			{
				state = CHECK_FOR_FLAG;
				break;
			}
			// this shouldn't be reached
			std::cout << "Unreachable area, switchcase DECIDE_ACTION, recv() loop\n";
			std::cout << "Catastrophic failure.\n";
			state = ERROR_STATE;
			break;
		}
		case TAKE_FILE_NAME_FROM_PEER:
		{
			// Set the file name variable.
			for (;
				(position_in_recv_buf < received_bytes)
				&& (position_in_message < message_size)
				&& (position_in_message < INCOMING_FILE_NAME_FROM_PEER_SIZE - RESERVED_NULL_CHAR_FOR_FILE_NAME);
				++position_in_recv_buf, ++position_in_message)
			{
				incoming_file_name_from_peer_cstr[position_in_message] = recv_buf[position_in_recv_buf];
			}
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
			std::cout << "# Incoming file name: " << incoming_file_name_from_peer << "\n";


			if (position_in_message == message_size)// must have a new message from the peer.
			{
				state = CHECK_FOR_FLAG;
				break;
			}
			// this shouldn't be reached
			std::cout << "Unreachable area:";
			DBG_DISPLAY_ERROR_LOCATION();
			std::cout << "Catastrophic failure.\n";
			state = ERROR_STATE;
			break;
			// Linux:
			// Max file name length is 255 chars on most filesystems, and max path 4096 chars.
			//
			// Windows:
			// Max file name length + subdirectory path is 255 chars. or MAX_PATH .. 260 chars?
			// 1+2+256+1 or [drive][:][path][null] = 260
			// This is not strictly true as the NTFS filesystem supports paths up to 32k characters.
			// You can use the win32 api and "\\?\" prefix the path to use greater than 260 characters. 
			// However using the long path "\\?\" is not a very good idea.
		}
		case TAKE_FILE_SIZE_FROM_PEER:
		{
			// convert the file size in the buffer from network int64_t to
			// host int64_t. It assigns the variable incoming_file_size_from_peer
			// a value.
			if (assignFileSizeFromPeer(recv_buf, recv_buf_len, received_bytes) != FINISHED_ASSIGNING_FILE_SIZE_FROM_PEER)
			{
				return false;// go recv() again
			}
			else
			{
				std::cout << "Size of Peer's file: " << incoming_file_size_from_peer << "\n";
			}

			state = OPEN_FILE_FOR_WRITE;
			break;
		}
		case CHECK_FOR_FLAG:
		{
			// Because if we are here, that means we currently
			// aren't in a message at the moment.
			position_in_message = 0;

			if (position_in_recv_buf >= received_bytes)
			{
				return false;
			}
			else
			{
				message_flag = (unsigned char)recv_buf[position_in_recv_buf];
				++position_in_recv_buf;// always have to ++ this in order to access the next element in the array.
				state = CHECK_MESSAGE_SIZE_PART_ONE;
				break;
			}
			break;
		}
		case CHECK_MESSAGE_SIZE_PART_ONE:
		{
			// Getting half of the u_short size of the message
			if (position_in_recv_buf >= received_bytes)
			{
				return false;
			}
			else
			{
				message_size_part_one = (unsigned char)recv_buf[position_in_recv_buf];
				message_size_part_one = message_size_part_one << 8;
				++position_in_recv_buf;
				state = CHECK_MESSAGE_SIZE_PART_TWO;
				break;
			}
			break;
		}
		case CHECK_MESSAGE_SIZE_PART_TWO:
		{
			// getting the second half of the u_short size of the message
			if (position_in_recv_buf >= received_bytes)
			{
				return false;
			}
			else
			{
				message_size_part_two = (unsigned char)recv_buf[position_in_recv_buf];
				message_size = message_size_part_one | message_size_part_two;
				++position_in_recv_buf;

				// Now that we have a flag and the size of the message,
				// set the state based on the message flag
				if (message_flag == ApplicationLayer::MsgFlags::FILE_DATA)
					state = WRITE_FILE_FROM_PEER;
				else if (message_flag == ApplicationLayer::MsgFlags::FILE_NAME)
					state = TAKE_FILE_NAME_FROM_PEER;
				else if (message_flag == ApplicationLayer::MsgFlags::FILE_SIZE)
					state = TAKE_FILE_SIZE_FROM_PEER;
				else if (message_flag == ApplicationLayer::MsgFlags::CHAT)
					state = OUTPUT_CHAT_FROM_PEER;
				else
				{
					std::cout << "Fatal Error: Unidentified message has been received.\n";
					state = ERROR_STATE;
					break;
				}

				if (position_in_recv_buf >= received_bytes)
				{
					return false;
				}
				break;
			}

			break;
		}
		case OPEN_FILE_FOR_WRITE:
		{
			if (position_in_recv_buf >= received_bytes)
			{
				return false;//recv again
			}
			WriteFile = new FILE;
			WriteFile = fopen(incoming_file_name_from_peer.c_str(), "wb");
			if (WriteFile == nullptr)
			{
				perror("Error opening file for writing in binary mode.\n");
			}

			is_file_done_being_written = false;

			// must have a new message from the peer if this is true
			if (position_in_message == message_size)
			{
				state = CHECK_FOR_FLAG;
			}
			else // shouldn't be possible if working correctly
			{
				std::cout << "Catastrophic failure. RecvBuf statemachine. Unreachable area.\n";
				state = ERROR_STATE;
			}

			break;
		}
		case CLOSE_FILE_FOR_WRITE:
		{
			if (WriteFile != nullptr)
			{
				if (fclose(WriteFile) != 0)		// 0 == successful close
				{
					perror("Error closing file for writing in binary mode");
				}
			}

			// Set it back to default value
			total_bytes_written_to_file = 0;
			

			if (position_in_message < message_size)
			{
				DBG_TXT("position_in_message < message_size, closefileforwrite");
			}
			if (position_in_message == message_size)
			{
				DBG_TXT("position_in_message == message_size, everything A OK closefileforwrite");
				std::cout << "# File transfer from peer is complete: " << incoming_file_name_from_peer << "\n";
			}
			if (position_in_message > message_size)
			{
				std::cout << "ERROR: position_in_message > message_size , closefileforwrite\n";
			}
			state = CHECK_FOR_FLAG;
			break;
		}
		case ERROR_STATE:
		{
			std::cout << "Exiting with error, state machine for recv().\n";

			if (WriteFile != nullptr)
			{
				if (fclose(WriteFile) != 0)		// 0 == successful close
				{
					perror("Error closing file for writing in binary mode.\n");
					std::cout << "Error occured in RecvStateMachine, case ERROR_STATE:\n";
				}
			}

			return true;
		}
		default:// currently nothing should cause this to execute.
		{
			std::cout << "State machine for recv() got to default. Exiting.\n";
			return true;
		}
		}//end switch
	}

	return false;
}

// For use with RecvBufStateMachine only.
int32_t ProcessRecvBuf::assignFileSizeFromPeer(char * recv_buf, int64_t recv_buf_len, int64_t received_bytes)
{
	// The peer is sending the size of his file as a int64_t.
	// This means we will receive sizeof(int64_t) + CR_RESERVED_BUFFER_SPACE bytes.
	// Those 8 bytes (the size of the file) will be coming to us in network byte order.
	// Referring to each one of those 8 bytes as a 'file_size_fragment'.
	if (file_size_fragment == 0)
	{
		incoming_file_size_from_peer = 0;
	}

	while (file_size_fragment < sizeof(int64_t))
	{
		if (position_in_recv_buf >= received_bytes)
		{
			return RECV_AGAIN;
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

	file_size_fragment = 0;

	return FINISHED_ASSIGNING_FILE_SIZE_FROM_PEER;
}



// Accessors

const std::string& ProcessRecvBuf::getIncomingFileNameFromPeer()
{
	return incoming_file_name_from_peer;
}
const bool& ProcessRecvBuf::getIsFileDoneBeingWritten()
{
	return is_file_done_being_written;
}