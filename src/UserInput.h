// UserInput.h
#ifndef UserInput_h__
#define UserInput_h__

#ifdef __linux__
#include <string>
#endif//__linux__

#ifdef _WIN32
#include <string>
#endif//_WIN32


class UserInput
{
	// Typedef section
public:
	// Send this file to the peer.
	typedef int32_t callback_fn_start_file_transfer(const std::string& file_name_and_path);

	// Send a chat message to the peer.
	typedef int64_t callback_fn_send_chat(std::string& user_input);

	// end the connection with the peer
	typedef int32_t callback_fn_end_connection();

	// If you want to exit the program, call this.
	typedef void callback_fn_exit_program();

public:
	UserInput(
		callback_fn_start_file_transfer * file_xfer_ptr,
		callback_fn_send_chat * chat_ptr,
		callback_fn_end_connection * end_conn_ptr,
		callback_fn_exit_program * exit_program_ptr, // A function that will exit the program, preferably gracefully.
		bool turn_verbose_output_on = false
	);
	~UserInput();

	// Turn on and off verbose output for this class.
	bool verbose_output = false;

	// Continually getline()'s the command prompt to see what the user wants to do.
	void loopedGetUserInput();


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






protected:
private:

	// Prevent someone from copying this class.
	UserInput(UserInput& UserInputInstance) = delete; // disable copy operator
	UserInput& operator=(UserInput& UserInputInstance) = delete; // disable assignment operator

	// If you want to exit the program, set this to true.
	bool exit_now = false;

	// Makes sure we are able to supply the FileTransfer class with reasonably correct input.
	int32_t prepareUserInputForFileXfer(std::string user_input, std::string& prepared_user_input);

	// Callbacks
	callback_fn_start_file_transfer * callbackStartThreadedFileXfer = nullptr; // Start the filetransfer.
	callback_fn_send_chat * callbackSendChatMsg = nullptr; // Send a chat message to the peer.
	callback_fn_end_connection * callbackEndConnection = nullptr; // close() and shutdown() the connected socket.
	callback_fn_exit_program * callbackExitProgram = nullptr; // If you want to exit the program, call this.


public:

	// Accessors

	// If you want this class to proceed to exit, set this to true.
	void setExitNow(bool exit_the_program) { exit_now = exit_the_program; }
};

#endif//UserInput_h__