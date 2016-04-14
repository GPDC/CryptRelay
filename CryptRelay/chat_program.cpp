// chat_program.cpp

#include "chat_program.h"

const std::string ChatProgram::default_port = "30001";

ChatProgram::ChatProgram()
{

}
ChatProgram::~ChatProgram()
{
	// ****IMPORTANT****
	// All addrinfo structures must be freed once they are done being used.
	// Making sure we never freeaddrinfo twice. Ugly bugs other wise.
	// Check comments in the myFreeAddrInfo() to see how its done.
	if (result != nullptr)
		SockStuff.myFreeAddrInfo(result);
}

bool ChatProgram::startChatProgram()
{

}

// This is where the ChatProgram class receives information about IPs and ports.
// /*optional*/ target_port         default value will be assumed
// /*optional*/ my_internal_port    default value will be assumed
void ChatProgram::giveIPandPort(std::string target_extrnl_ip_address, std::string my_ext_ip, std::string my_internal_ip, std::string target_port, std::string my_internal_port)
{
	// If empty == false
	//		user must have desired their own custom stuff. Let's take the
	//		information and store it in the corresponding variables.
	// If empty == true
	//		user must not have desired their own custom
	//		ports / IPs. Therefore, we leave it as is a.k.a. default value.
	if (empty(target_extrnl_ip_address) == false)
		target_external_ip = target_extrnl_ip_address;
	if (empty(target_port) == false)
		target_external_port = target_port;
	if (empty(my_ext_ip) == false)
		my_external_ip = my_ext_ip;
	if (empty(my_internal_ip) == false)
		my_local_ip = my_internal_ip;
	if (empty(my_internal_port) == false)
		my_local_port = my_internal_port;
}

int ChatProgram::startServer()
{
	// Necessary for Windows to do anything with sockets
	SockStuff.myWSAStartup();

	// These are the settings for the connection
	hints.ai_family = AF_INET;			//ipv4
	hints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP
	
	return true;
}


int ChatProgram::startClient()
{
	// Necessary for Windows to do anything with sockets
	SockStuff.myWSAStartup();

	// These are the settings for the connection
	hints.ai_family = AF_INET;			//ipv4
	hints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	if (SockStuff.myGetAddrInfo(target_external_ip, target_external_port, &hints, &result) == false)
		return EXIT_FAILURE;

	return true;

}