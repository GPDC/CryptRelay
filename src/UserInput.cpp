// UserInput.cpp
#ifdef __linux__
#include <iostream>

#include "UserInput.h"
#include "GlobalTypeHeader.h"
#include "StringManip.h"
#endif//__linux__

#ifdef _WIN32
#include <iostream>

#include "UserInput.h"
#include "GlobalTypeHeader.h"
#include "StringManip.h"
#endif//_WIN32

UserInput::UserInput()
{

}
UserInput::~UserInput()
{

}

// The user's input is getlined here and checked for things
// that the user might want to do.
void UserInput::loopedGetUserInput()
{
	std::string user_input;

	do
	{
		std::getline(std::cin, user_input);
	} while (decideActionBasedOnUserInput(user_input) == 0);

	return;
}

// Return -2, exiting the program gracefully, don't retrieve any more use input.
// Return -1, error.
// Return 0, success.
int32_t UserInput::decideActionBasedOnUserInput(std::string user_input)
{
	const int GRACEFUL_EXIT = -2;

	while (1)
	{
		switch (state)
		{
		case CHECK_IF_USER_WANTS_TO_EXIT:
		{
			if (user_input == "exit()")
				state = EXIT_GRACEFULLY;
			else if (exit_now == true)
				state = EXIT_GRACEFULLY;
			else
				state = CHECK_IF_USER_WANTS_TO_SEND_FILE;

			break;
		}
		case CHECK_IF_USER_WANTS_TO_SEND_FILE:
		{
			if (doesUserWantToSendAFile(user_input) == true)
			{
				state = SEND_FILE;
				break;
			}
			else
			{
				// User dosn't want to exit, nor send a file. It must be a chat message then.
				state = SEND_CHAT_MESSAGE;
				break;
			}
		}
		case SEND_CHAT_MESSAGE:
		{
			int64_t user_input_length = user_input.length();

			// Send the message
			if (callbackSendChatMsg != nullptr)
			{
				if (callbackSendChatMsg(user_input) < 0)
				{
					state = ERROR_STATE;
					break;
				}
				else // success
				{
					state = BEGINNING_STATE;
					return 0;
				}
			}
			else
			{
				std::cout << "Error: Programmer error. callbackSendChatMsg == nullptr.\n";
				state = ERROR_STATE;
				break;
			}
		}
		case SEND_FILE:
		{
			// prepareUserInputForFileXfer() assigns this.
			std::string file_name_and_path;

			// Adds any necessary escape characters for '\'s,
			// Makes sure user typed something after "-f", Ex: "-f something_like_a_path_to_a_file"
			if (prepareUserInputForFileXfer(user_input, file_name_and_path) == -1)
			{
				state = BEGINNING_STATE;
				return 0;
			}

			// Try to start the file transfer.
			if (callback_StartThreadedFileXfer != nullptr)
			{
				if (callback_StartThreadedFileXfer(file_name_and_path) == -1)
				{
					// Couldn't start the file transfer
					std::cout << "Error: A file transfer is already in progress. Please wait until it is finished.\n";
				}
			}
			else // Programmer error, never set the callback
			{
				std::cout << "Error: File transfer never started. Programmer error. callback_StartThreadedFileXfer == nullptr.\n";
				DBG_DISPLAY_ERROR_LOCATION();
			}

			state = BEGINNING_STATE;
			return 0;
		}
		case EXIT_GRACEFULLY:
		{
			// Proceeding to exit program. shutdown() and close() the socket.
			if (callbackEndConnection != nullptr)
				callbackEndConnection();
			else
			{
				std::cout << "ERROR: callbackEndConnection == nullptr.\n";
				DBG_DISPLAY_ERROR_LOCATION();
			}
			exit_now = true;
			state = BEGINNING_STATE;
			return GRACEFUL_EXIT;
		}
		case ERROR_STATE:
		{
			// Reset the state and tell whoever called this method that an error occured.
			state = BEGINNING_STATE;
			return -1;
		}
		default:// Should be possible to reach this with the current design
		{
			// Treating it like an error.
			std::cout << "FATAL ERROR: UserInput's state machine reached default.\n";
			state = BEGINNING_STATE;
			return -1;
		}
		}//end switch()
	}//end while()

	 // Shouldn't be possible to reach here with the current design.
	return -1;
}

// Check the user's message that he put into the terminal to see if he
// used the flag -f to indicate he wants to send a file.
bool UserInput::doesUserWantToSendAFile(std::string& user_msg_from_terminal)
{
	size_t user_msg_from_terminal_len = user_msg_from_terminal.length();
	// pls split into multiple strings based on spaces
	// make a function that takes a string as input, and then return a vector filled with the strings that it split up.

	if (user_msg_from_terminal_len >= 2)
	{
		if (user_msg_from_terminal[0] == '-' && user_msg_from_terminal[1] == 'f')
			return true;
	}

	return false;
}

// /* IN */ std::string& user_input is the source string that will be prepared for file transfer.
// /* OUT */ std::string& prepared_user_input is the modified file name and path
// Returns -1, error.
// Returns 0, success.
int32_t UserInput::prepareUserInputForFileXfer(std::string user_input, std::string& prepared_user_input)
{
	// Split the string into multiple strings for every space.
	StringManip StrManip;
	std::vector <std::string> split_strings_vector;
	if (StrManip.split(user_input, ' ', split_strings_vector) == true)
	{
		std::cout << "Unexpected error: split(). File transfer never started due to error.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}

	// get the size of the array
	int64_t split_strings_size = split_strings_vector.size();

	// There should be at least two strings in the vector.
	// [0] should be -f
	// [1] should be the file name and path
	// [2] and onwards could exist if there were spaces in the file name or path
	if (split_strings_size < 2)
	{
		std::cout << "Error: not enough arguments given.\n";
		return -1;
	}


	if (split_strings_vector[0] == "-f" && split_strings_size >= 2)
	{
		// Incase the file name and path had spaces in it,
		// concatenate everything after [1], since [0]
		// should just be "-f".
		for (int32_t b = 2; b < split_strings_size; ++b)
		{
			split_strings_vector[1] += ' ' + split_strings_vector[b];
		}

		// There should now only be two strings in the vector.
		// [0] should be -f
		// [1] should be the file name
		if (split_strings_size >= 2)
		{
			prepared_user_input = split_strings_vector[1];

			// Fix the user's input to add an escape character to every '\'
			StrManip.duplicateCharacter(prepared_user_input, '\\');
		}
	}
	
	return 0;
}














	//// Split the string into multiple strings for every space.
	//StringManip StrManip;
	//std::vector <std::string> split_strings_vector;
	//if (StrManip.split(user_input, ' ', split_strings_vector) == true)
	//{
	//	std::cout << "Unexpected error: split()";
	//	DBG_DISPLAY_ERROR_LOCATION();
	//}

	//// get the size of the array
	//int64_t split_strings_size = split_strings_vector.size();

	//// There should be two strings in the vector.
	//// [0] should be -f
	//// [1] should be the file name and path
	//if (split_strings_size < 2)
	//{
	//	std::cout << "Error: not enough arguments given.\n"; // Non-fatal error.
	//	state = BEGINNING_STATE;
	//	return 0;
	//}

	//std::string file_name_and_path;
	//// Determine if user wants to send a file or Encrypt & send a file.
	//for (int64_t i = 0; i < split_strings_size; ++i)
	//{
	//	if (split_strings_vector[(uint32_t)i] == "-f" && i < split_strings_size - 1)
	//	{
	//		// The first string will always be -f. All strings after that
	//		// will be concatenated, and then have the spaces re-added after
	//		// to prevent issues with spaces in file names and paths.
	//		for (int32_t b = 2; b < split_strings_size; ++b)
	//		{
	//			split_strings_vector[1] += ' ' + split_strings_vector[b];
	//		}

	//		// There should be two strings in the vector.
	//		// [0] should be -f
	//		// [1] should be the file name
	//		if (split_strings_size >= 2)
	//		{
	//			file_name_and_path = split_strings_vector[1];

	//			// Fix the user's input to add an escape character to every '\'
	//			StrManip.duplicateCharacter(file_name_and_path, '\\');
	//		}

	//		// create a file xfer instance, which will make
	//		// it start a threaded sendFile().
	//		if (callback_StartThreadedFileXfer != nullptr)
	//		{
	//			if (callback_StartThreadedFileXfer(file_name_and_path) == -1)
	//			{
	//				// Couldn't start the file transfer
	//				std::cout << "Error: A file transfer is already in progress. Please wait until it is finished.\n";
	//			}
	//		}
	//		else // Programmer error, never set the callback
	//		{
	//			std::cout << "Error: File transfer never started. Programmer error. callback_StartThreadedFileXfer == nullptr.\n";
	//			DBG_DISPLAY_ERROR_LOCATION();
	//		}

	//		state = BEGINNING_STATE;
	//		return 0;
	//	}