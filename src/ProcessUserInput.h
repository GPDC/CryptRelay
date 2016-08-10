// ProcessUserInput.h
#ifndef ProcessUserInput_h__
#define ProcessUserInput_h__

#ifdef __linux__
#include <string>
#include <thread>

#include "FileTransfer.h"
#endif//__linux__

#ifdef _WIN32
#include <string>
#include <thread>

#include "FileTransfer.h"
#endif//_WIN32

class ProcessUserInput
{
public:
	ProcessUserInput();
	~ProcessUserInput();

	// Currently just using the string as a buffer so this shouldn't be needed anymore.
	//const int64_t CHAT_BUF_LEN = 4096;
	//char* chat_buf = new char[CHAT_BUF_LEN];


	enum UserInputStateMachine
	{
		CHECK_IF_USER_WANTS_TO_EXIT,

		SEND_CHAT_MESSAGE,

		CHECK_IF_USER_WANTS_TO_SEND_FILE,
		SEND_FILE,
		SEND_ENCRYPTED_FILE,

		EXIT_GRACEFULLY,
		ERROR_STATE,
	};

	// This is the first step the state machine should take.
	const int32_t BEGINNING_STATE = CHECK_IF_USER_WANTS_TO_EXIT;
	int32_t state = BEGINNING_STATE;

	// Given the user's input that is gathered through whatever
	// method desired, e.g. getline(), it will see what the user
	// might want to do, including just sending a chat message.
	int32_t decideActionBasedOnUserInput(std::string user_input);


	// Used within loopedGetUserInputThread() to check if the user inputted something
	// into the command prompt that would indicate they wanted to send a file.
	bool doesUserWantToSendAFile(std::string& user_msg_from_terminal);
	


	// Typedefs for callbacks:


	typedef int32_t callback_fn_start_file_transfer(const std::string& file_name_and_path); // Start the filetransfer.
	typedef int64_t callback_fn_send_chat(std::string& user_input); // Send a chat message to the peer.
	typedef int32_t callback_fn_end_connection();// close() and shutdown() the connected socket.

protected:
private:


	// Callbacks

	// Starts the file transfer with the given arg
	callback_fn_start_file_transfer * callback_StartFileXfer = nullptr;
	callback_fn_send_chat * callbackSendChatMsg = nullptr;
	callback_fn_end_connection * callbackEndConnection = nullptr;
	

public:
	// Accessors
	void setCallbackStartFileXfer(callback_fn_start_file_transfer * ptr) { callback_StartFileXfer = ptr; }
	void setCallbackSendChatMessage(callback_fn_send_chat * ptr) { callbackSendChatMsg = ptr; }
	void setCallbackEndConnection(callback_fn_end_connection * ptr) { callbackEndConnection = ptr; }

};

#endif//ProcessUserInput_h__