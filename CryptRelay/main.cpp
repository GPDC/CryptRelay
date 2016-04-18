
//main.cpp

//Program name: CryptRelay
//Formatting guide: Located at the bottom of main.cpp


//TODO:
//encryption
//output what IP and port you are listening on
//fix chat output issue when someone sends you a message while you are typing
//In the FormatCheck.cpp, the port is already changed from string to a number. Change code everywhere to stop taking strings,
//	and to stop changing strings to numbers for ports. Also minor change must be made in FormatCheck to return the port instead of bool.
//ipv6

#ifdef __linux__			//to compile on linux, must set linker library standard library pthreads
#include <iostream>
#include <string.h>
#include <vector>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <cerrno>
//#include <libupnp>	// Window's <UPnP.h>

#include <arpa/inet.h>
#include <signal.h>

#include <pthread.h>	//<process.h>

#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "chat_program.h"

#include "UPnP.h
#endif//__linux__

#ifdef _WIN32
#include <iostream>
#include <string>
#include <vector>
#include <WS2tcpip.h>
#include <UPnP.h>		// linux's libupnp??

#include <process.h>	//<pthread.h>

#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "SocketClass.h"
#include "chat_program.h"

#include "UPnP.h"
#endif//_WIN32


bool global_verbose = false;

// Gives IP and Port information to the Chat Program.
// /* from */ CommandLineInput* CLI
// /* to */ ChatProgram* ChatServerInstance
// /* to */ ChatProgram* ChatClientInstance
void lanGiveIPAndPort(
		CommandLineInput* CLI,
		ChatProgram* ChatServerInstance,
		ChatProgram* ChatClientInstance
	);

// Gives IP and Port information to the Chat Program.
// /* from */ CommandLineInput* CLI
// /* from */ UPnP* UpnpInstance
// /* to */ ChatProgram* ChatServerInstance
// /* to */ ChatProgram* ChatClientInstance
void upnpGiveIPAndPort(
		CommandLineInput* CLI,
		UPnP* UpnpInstance,
		ChatProgram* ChatServerInstance,
		ChatProgram* ChatClientInstance
	);

void lanGiveIPAndPort(CommandLineInput* CLI, ChatProgram* ChatServerInstance, ChatProgram* ChatClientInstance)
{
	// If the user inputted values at the command line interface
	// designated for IP and / or port, we will take those values
	// and give them to the chat program.

	// Give IP and port info to the ChatServer instance
	if (empty(CLI->target_ip_address) == false)
		ChatServerInstance->target_external_ip = CLI->target_ip_address;

	if (empty(CLI->target_port) == false)
		ChatServerInstance->target_external_port = CLI->target_port;

	if (empty(CLI->my_ext_ip_address) == false)
		ChatServerInstance->my_external_ip = CLI->my_ext_ip_address;

	if (empty(CLI->my_ip_address) == false)
		ChatServerInstance->my_local_ip = CLI->my_ip_address;

	if (empty(CLI->my_host_port) == false)
		ChatServerInstance->my_local_port = CLI->my_host_port;


	// Give IP and port info to the ChatClient instance
	if (empty(CLI->target_ip_address) == false)
		ChatClientInstance->target_external_ip = CLI->target_ip_address;

	if (empty(CLI->target_port) == false)
		ChatClientInstance->target_external_port = CLI->target_port;

	if (empty(CLI->my_ext_ip_address) == false)
		ChatClientInstance->my_external_ip = CLI->my_ext_ip_address;

	if (empty(CLI->my_ip_address) == false)
		ChatClientInstance->my_local_ip = CLI->my_ip_address;

	if (empty(CLI->my_host_port) == false)
		ChatClientInstance->my_local_port = CLI->my_host_port;
}

// The user's IP and port input will always be used over the IP and port that the UPnP
// class tried to give.
void upnpGiveIPAndPort(CommandLineInput* CLI, UPnP* UpnpInstance, ChatProgram* ChatServerInstance, ChatProgram* ChatClientInstance)
{
	// If the user inputted values at the command line interface
	// designated for IP and / or port, we will take those values
	// and give them to the chat program.
	// Otherwise, we will just take the IP and Port that
	// the UPnP class has gathered / made.

	// Give IP and port info to the ChatServer instance
	if (empty(CLI->target_ip_address) == false)
		ChatServerInstance->target_external_ip = CLI->target_ip_address;

	if (empty(CLI->target_port) == false)
		ChatServerInstance->target_external_port = CLI->target_port;

	if (empty(CLI->my_ext_ip_address) == false)
		ChatServerInstance->my_external_ip = CLI->my_ext_ip_address;
	else
		ChatServerInstance->my_external_ip = UpnpInstance->my_external_ip;

	if (empty(CLI->my_ip_address) == false)
		ChatServerInstance->my_local_ip = CLI->my_ip_address;
	else
		ChatServerInstance->my_local_ip = UpnpInstance->my_local_ip;

	if (empty(CLI->my_host_port) == false)
		ChatServerInstance->my_local_port = CLI->my_host_port;
	else
		ChatServerInstance->my_local_port = UpnpInstance->my_internal_port;


	// Give IP and port info to the ChatClient instance
	if (empty(CLI->target_ip_address) == false)
		ChatClientInstance->target_external_ip = CLI->target_ip_address;

	if (empty(CLI->target_port) == false)
		ChatClientInstance->target_external_port = CLI->target_port;

	if (empty(CLI->my_ext_ip_address) == false)
		ChatClientInstance->my_external_ip = CLI->my_ext_ip_address;
	else
		ChatClientInstance->my_external_ip = UpnpInstance->my_external_ip;

	if (empty(CLI->my_ip_address) == false)
		ChatClientInstance->my_local_ip = CLI->my_ip_address;
	else
		ChatClientInstance->my_local_ip = UpnpInstance->my_local_ip;

	if (empty(CLI->my_host_port) == false)
		ChatClientInstance->my_local_port = CLI->my_host_port;
	else
		ChatClientInstance->my_local_port = UpnpInstance->my_internal_port;
}

int main(int argc, char *argv[])
{
	int errchk = 0;

	CommandLineInput CLI;
	// Check what the user wants to do via command line input
	// Information inputted by the user on startup is stored in CLI
	if ( (errchk = CLI.getCommandLineInput(argc, argv) ) == FALSE)	
		return EXIT_FAILURE;


	//===================================== Starting Chat Program =====================================

	UPnP* Upnp = nullptr;		// Not sure if the user wants to use UPnP yet, so just preparing with a pointer.
	ChatProgram ChatServer;
	ChatProgram ChatClient;

	if (CLI.show_info_upnp == true)
	{
		Upnp = new UPnP;
		Upnp->standaloneShowInformation();
		return EXIT_SUCCESS;
	}
	else if (CLI.get_list_of_port_forwards == true)
	{
		Upnp = new UPnP;
		Upnp->standaloneGetListOfPortForwards();
		delete Upnp;
		return EXIT_SUCCESS;
	}
	else if (CLI.delete_this_specific_port_forward == true)
	{
		Upnp = new UPnP;
		Upnp->standaloneDeleteThisSpecificPortForward(
				CLI.delete_this_specific_port_forward_port.c_str(),
				CLI.delete_this_specific_port_forward_protocol.c_str()
			);
		delete Upnp;
		return EXIT_SUCCESS;
	}
	else if (CLI.use_lan_only == true)
	{
		// Give IP and port info to the ChatServer and ChatClient instance
		lanGiveIPAndPort(&CLI, &ChatServer, &ChatClient);
	}
	else if (CLI.use_upnp_to_connect_to_peer == true)
	{
		Upnp = new UPnP;	// deltag:9940   (this is just so I can ctrl-f and find where I delete this.

		// If UPnP is working, add a port forward rule so we can connect to a peer
		if (Upnp->autoAddPortForwardRule() == true)
		{
			// Give IP and port info gathered from the command line and from
			// the UPnP class to the ChatServer and ChatClient instance
			upnpGiveIPAndPort(&CLI, Upnp, &ChatServer, &ChatClient);
		}
		else
		{
			std::cout << "Fatal: Couldn't port forward via UPnP.\n";
			return EXIT_FAILURE;
		}
	}

	// Actually starting the chat program now that we have all of the
	// needed information gathered and stored.
	
	// BEGIN THREAD RACE
	ChatProgram::createStartServerThread(&ChatServer);			//<-----------------
	ChatProgram::createStartClientThread(&ChatClient);			//<----------------

	// Wait for 2 ChatServer and ChatClient threads to finish
#ifdef __linux__
	pthread_join(ChatProgram::thread0, NULL);
	pthread_join(ChatProgram::thread1, NULL);
#endif//__linux__
#ifdef _WIN32
	int r = WaitForMultipleObjects(
			(DWORD)2,	// Number of objects in array
			ChatProgram::ghEvents,	// Array of objects
			TRUE,		// Wait for all objects if it is set to TRUE. FALSE == wait for any one object to finish. Return value indicates the returned thread(s?).
			INFINITE	// Its going to wait this long, OR until all threads are finished, in order to continue.
		);	
#endif//_WIN32	

	
	delete Upnp;	// deltag:9940

	return 1;
//===================================== End Chat Program =====================================





//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Starting File Transfer Program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	//if argv[1] == "-f"
	//bleep bloop

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End File Transfer Program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}



/*

+++++++++++++++++++++++++++++++++++++ Formatting Guide +++++++++++++++++++++++++++++++++++++

Classes:		ThisIsAnExample();		# Capitalize the first letter of every word.
Structures:		ThisIsAnExample();		# Capitalize the first letter of every word.
Functions:		thisIsAnExample();		# Capitalize the first letter of every word except the first.
Variables:		this_is_an_example;		# No capitalization. Underscores to separate words.
#define:		THIS_IS_AN_EXAMPLE;		# All capitalization. Underscores to separate words.
Globals:								# Put the word global in front of it like so:
	ex variable:	global_this_is_an_example;
	ex function:	globalThisIsAnExample;

ifdefs must always comment the endif with what it is endifing.
#ifdef _WIN32
#endif //_WIN32



Curly braces:	bool thisIsAnExample()			// It is up to you to decide what looks better;
				{								// Single lines with curly braces, or
					int i = 5;					// Single lines with no curly braces.
					if (i == 70)
					{
						return true;
					}
					else
					{
						return false;
					}

					if (i == 5)
					{
						if(1 + 1 == 2)
							1pptheory = true;
						if(truth == lie)
							return false;
						if(bear == big)
							std::cout << "Oh my!\n";
					}
				}

+++++++++++++++++++++++++++++++++++ End Formatting Guide +++++++++++++++++++++++++++++++++++

*/






// Imporant note on building PJNATH library:
// must limit parallel project builds to 1 at a time. More than this can cause it to fail in odd ways. (rly odd)









// How to build and setup miniupnp library:

//********** Building Microsoft Visual Studio 2015 * ************
//
//1. create a txt file and rename it to miniupnpcstrings.h
//	 This is because that file can't be built in windows for some reason.
//
//2. paste this into it :

///* Project: miniupnp
//* http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
//* Author: Thomas Bernard
//* Copyright (c) 2005-2009 Thomas Bernard
//* This software is subjects to the conditions detailed
//* in the LICENCE file provided within this distribution */
//#ifndef __MINIUPNPCSTRINGS_H__
//#define __MINIUPNPCSTRINGS_H__
//
//#define OS_STRING "Windows/7.0.0000"
//#define MINIUPNPC_VERSION_STRING "1.9"
//
//#endif 

//
//3. in the miniupnpcstrings.h file you will see the line : #define MINIUPNPC_VERSION_STRING "1.9" 
//	 I think this is the version of the library you are using. Who knows. I set it to 1.9. A forum post from long ago had it at 1.4
//
//4. open up visual studio solution in the msvc folder
//
//5. set to release mode and build miniupnp.c PROJECT
//
//6. set to debug mode and build miniupnp.c PROJECT
//
//
//If you get some wierd errors, try re downloading and extract it again, do the whole process over
//I actually had corrupted files a couple times amazingly enough!
//
//
//********** Including / Linking Visual Studio 2015 * ***************
//Steps to take for including it / linking it in your program(visual studio 2015)
//
//
//In your program put these 2 lines b / c they are needed by the miniupnp library to work :
//#pragma comment(lib, "Ws2_32.lib")
//#pragma comment(lib, "Iphlpapi.lib")
//If you were getting linker errors the cause might have been the two missing #pragma comment
//
//1. Set your configuration to Release and then follow the instructions :
//
//2. in project settings, Linker->Input->Additional Dependencies->miniupnpc - 1.9\msvc\Release\miniupnpc.lib
//please note that it needs to be a full path to the miniupnpc.lib.Feel free to use macros.Macros not shown in example :
//c : \dev\workspace\myproject\miniupnpc - 1.9\msvc\Release\miniupnpc.lib
//
//	3. in project settings, C / C++->Preprocessor->Preprocessor Definitions->STATICLIB
//	please note we are just adding STATICLIB to the list of whatever is currently there.Don't delete everything in there.
//
//	4. in project settings, VC++ Directories->Include Directories -> \miniupnpc - 1.9
//	please note that it needs to be a full path to the folder.Feel free to use macros.Macros not shown in example :
//	C : \dev\workspace\myproject\miniupnpc - 1.9
//
//	5. in project settings, VC++ Directories->Include Directories -> \miniupnpc - 1.9
//	please note that it needs to be a full path to the folder.Feel free to use macros.Macros not shown in example :
//	C : \dev\workspace\myproject\miniupnpc - 1.9\msvc\Release
//
//	6. Set your configuration to Debug and do steps 1 through 5 all over again, but replaces any mention of "Release" with "debug".
//	Make sure you changed the additional dependencies directory paths to have debug in it too.
//
//
//	-----------unfinished section---------
//	If you want to static link the library with your program :
//
//	1. project settings->c / c++->code generation->Runtime Library->change it to Multi - Threaded(/ MT) if you are on Release, and Multi - Threaded Debug(/ MTd) if you are on debug
//