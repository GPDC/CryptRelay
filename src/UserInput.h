// UserInput.h
#ifndef UserInput_h__
#define UserInput_h__

#ifdef __linux__
#include <string>

#include "ProcessUserInput.h"
#endif//__linux__

#ifdef _WIN32
#include <string>

#include "ProcessUserInput.h"
#endif//_WIN32


class UserInput
{
public:
	UserInput();
	~UserInput();

	ProcessUserInput ProcInput;

	// Continually getline()'s the command prompt to see what the user wants to do.
	void loopedGetUserInput();



protected:
private:

public:

	// Accessors

	void setCallbackStartFileXfer(ProcessUserInput::callback_fn_start_file_transfer *ptr) { ProcInput.setCallbackStartFileXfer(ptr); }
	void setCallbackSendChatMessage(ProcessUserInput::callback_fn_send_chat * ptr) { ProcInput.setCallbackSendChatMessage(ptr); }
	void setCallbackEndConnection(ProcessUserInput::callback_fn_end_connection * ptr) { ProcInput.setCallbackEndConnection(ptr); }
};

#endif//UserInput_h__