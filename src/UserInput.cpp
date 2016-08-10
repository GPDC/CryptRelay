// UserInput.cpp
#ifdef __linux__
#include <iostream>

#include "UserInput.h"
#include "GlobalTypeHeader.h"
#endif//__linux__

#ifdef _WIN32
#include <iostream>

#include "UserInput.h"
#include "GlobalTypeHeader.h"
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
	} while (ProcInput.decideActionBasedOnUserInput(user_input) == 0);

	return;
}