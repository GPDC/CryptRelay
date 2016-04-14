// chat_program.cpp
#ifdef __linux__
#include <iostream>		// cout
#include <pthread.h>	// <process.h>  multithreading

#include "chat_program.h"
#include "GlobalTypeHeader.h"
#endif //__linux__

#ifdef _WIN32
#include <iostream>		// cout
#include <process.h>	// <pthread.h> multithreading

#include "chat_program.h"
#include "GlobalTypeHeader.h"
#endif //_WIN32


#ifdef _WIN32
HANDLE ChatProgram::ghEvents[3]{};
#endif//_WIN32

const std::string ChatProgram::default_port = "30001";
bool ChatProgram::fatal_thread_error = false;


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
	if (global_verbose == true)
		std::cout << "Giving IP and Port information to the chat program.\n";
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

// Thread entrance for startServer();
void ChatProgram::createStartServerThread(void * instance)
{
	if (global_verbose == true)
		std::cout << "Starting server thread.\n";
#ifdef __linux__
	ret0 = pthread_create(&thread0, NULL, startServer, instance);
	if (ret0)
	{
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret0);
		exit(EXIT_FAILURE);
	}
#endif

#ifdef _WIN32
	// If a handle is desired, do something like this:
	// ghEvents[0] = (HANDLE) _beginthread(startServer, 0, instance);
	// or like this because it seems a little better way to deal with error checking
	// because a negative number isn't being casted to a void *   :
	// uintptr_t thread_handle = _beginthread(startServer, 0, instance);
	// ghEvents[0] = (HANDLE)thread_handle;
	//   if (thread_handle == -1L)
	//		error stuff here;
	uintptr_t thread_handle = _beginthread(startServer, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
	ghEvents[0] = (HANDLE)thread_handle;	// i should be using vector of ghEvents instead
	if (thread_handle == -1L)
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		fatal_thread_error = true;	// what should i do with this hmm
		return;
	}

	// previous way of doing it:
	/*
	ghEvents[0] = (HANDLE)_beginthread(startServer, 0, instance);		//c style typecast    from: uintptr_t    to: HANDLE.
	if (ghEvents[0] == (HANDLE)(-1L) )
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		fatal_thread_error = true;
		return;
	} 
	*/

#endif//_WIN32
}

void ChatProgram::startServer(void * instance)
{
	if (instance == NULL)
	{
		std::cout << "Thread instance NULL\n";
		return;
	}

	ChatProgram* self = (ChatProgram*)instance;
	// These are the settings for the connection
	self->hints.ai_family = AF_INET;			//ipv4
	self->hints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	self->hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP
	
	_endthread();
}

void ChatProgram::createClientRaceThread(void * instance)
{
	if (global_verbose == true)
		std::cout << "Starting client thread.\n";
#ifdef __linux__
	ret1 = pthread_create(&thread1, NULL, startServer, instance);
	if (ret1)
	{
		fprintf(stderr, "Error - pthread_create() return code: %d\n", ret1);
		exit(EXIT_FAILURE);
	}
#endif

#ifdef _WIN32
	// If a handle is desired, do something like this:
	// ghEvents[0] = (HANDLE) _beginthread(startServer, 0, instance);
	// or like this because it seems a little better way to deal with error checking
	// because a negative number isn't being casted to a void *   :
	// uintptr_t thread_handle = _beginthread(startServer, 0, instance);
	// ghEvents[0] = (HANDLE)thread_handle;
	//   if (thread_handle == -1L)
	//		error stuff here;
	uintptr_t thread_handle = _beginthread(startClient, 0, instance);	//c style typecast    from: uintptr_t    to: HANDLE.
	ghEvents[1] = (HANDLE)thread_handle;	// i should be using vector of ghEvents instead
	if (thread_handle == -1L)
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		fatal_thread_error = true;	// what should i do with this hmm
		return;
	}

	// previous way of doing it:
	/*
	ghEvents[1] = (HANDLE)_beginthread(startClient, 0, instance);		//c style typecast    from: uintptr_t    to: HANDLE.
	if (ghEvents[1] == (HANDLE)(-1L) )
	{
		int errsv = errno;
		std::cout << "_beginthread() error: " << errsv << "\n";
		fatal_thread_error = true;
		return;
	} 
	*/

#endif//_WIN32
}

void ChatProgram::startClient(void * instance)
{
	if (instance == NULL)
	{
		std::cout << "Thread instance NULL\n";
		_endthread();
	}
	ChatProgram* self = (ChatProgram*)instance;

	//// Necessary for Windows to do anything with sockets
	//SockStuff.myWSAStartup();

	// These are the settings for the connection
	self->hints.ai_family = AF_INET;		//ipv4
	self->hints.ai_socktype = SOCK_STREAM;	// Connect using reliable connection
	self->hints.ai_protocol = IPPROTO_TCP;	// Connect using TCP

	if (self->SockStuff.myGetAddrInfo(self->target_external_ip, self->target_external_port, &self->hints, &self->result) == false)
		_endthread();//_endthread ????


	_endthread();
}