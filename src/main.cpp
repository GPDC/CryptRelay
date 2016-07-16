// main.cpp

// Program name: CryptRelay     version: 0.6.1
// Rough outline for future versions:
// 0.8 == file transfer
// 0.9 == encryption
// 1.0 == polished release

// Formatting guide: Located at the bottom of main.cpp

//solve chat output while typing msg problem by forcing a getline for whatever is currently typed out when
// the user receives()? and then once its done recving and displaying that message, cout the getlined message back to console to make it look like everything
// is normal. I mean an issue would still occur with not being able to delete your msg that got getlined, but this could be a quick band-aid.


// Overview:
// This is where the program starts.

// Warnings:
// This source file expects any input that is given to it has already been checked
//  for safety and validity. For example if a user supplies a port, then
//  this source file will expect the port will be >= 0, and <= 65535.
//  As with an IP address it will expect it to be valid input, however it doesn't
//  expect you to have checked to see if there is a host at that IP address
//  before giving it to this source file.

#ifdef __linux__			//to compile on linux, must set linker library standard library pthreads
							// build-> linker-> libraries->
#include <iostream>
#include <string>
#include <vector>
#include <climits>
#include <sstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <cerrno>
#include <arpa/inet.h>

#include <pthread.h>	//<process.h>

#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "SocketClass.h"
#include "chat_program.h"
#include "port_knock.h"

#include "UPnP.h"
#endif//__linux__

#ifdef _WIN32
#include <iostream>
#include <string>
#include <vector>

#include <WS2tcpip.h>

#include <process.h>	//<pthread.h>

#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "SocketClass.h"
#include "chat_program.h"
#include "port_knock.h"

#include "UPnP.h"
#endif//_WIN32


#ifdef __linux__
#define FALSE 0
#endif//__linux__

bool global_verbose = false;
bool global_debug = true;

// Give Port information, supplied by the user, to the UPnP Class.
void cliGivesPortToUPnP(CommandLineInput* CLI, UPnP* UpnpInstance);

// Gives IP and Port information to the Chat Program.
// /* from */ CommandLineInput* CLI
// /* to */ Connection* ChatServerInstance
// /* to */ Connection* ChatClientInstance
void cliGivesIPAndPortToChatProgram(
		CommandLineInput* CLI,
		Connection* ChatServerInstance,
		Connection* ChatClientInstance
	);

// Gives IP and Port information to the Chat Program.
// /* from */ CommandLineInput* CLI
// /* from */ UPnP* UpnpInstance	//only if the user didn't input anything in the CLI
// /* to */ Connection* ChatServerInstance
// /* to */ Connection* ChatClientInstance
void upnpGivesIPAndPortToChatProgram(
		CommandLineInput* CLI,
		UPnP* UpnpInstance,
		Connection* ChatServerInstance,
		Connection* ChatClientInstance
	);



// Give user supplied port to the UPnP class
void cliGivesPortToUPnP(CommandLineInput* CLI, UPnP* UpnpInstance)
{
	// Give Port that was supplied by the user to the UPnP class
	if (CLI->my_host_port.empty() == false)
	{
		UpnpInstance->my_internal_port = CLI->my_host_port;
		UpnpInstance->my_external_port = CLI->my_host_port;
	}
}

void cliGivesIPAndPortToChatProgram(CommandLineInput* CLI, Connection* ChatServerInstance, Connection* ChatClientInstance)
{
	// If the user inputted values at the command line interface
	// designated for IP and / or port, we will take those values
	// and give them to the chat program.

	// Give IP and port info to the ChatServer instance
	if (CLI->target_ip_address.empty() == false)
		ChatServerInstance->target_external_ip = CLI->target_ip_address;

	if (CLI->target_port.empty() == false)
		ChatServerInstance->target_external_port = CLI->target_port;

	if (CLI->my_ext_ip_address.empty() == false)
		ChatServerInstance->my_external_ip = CLI->my_ext_ip_address;

	if (CLI->my_ip_address.empty() == false)
		ChatServerInstance->my_local_ip = CLI->my_ip_address;

	if (CLI->my_host_port.empty() == false)
		ChatServerInstance->my_local_port = CLI->my_host_port;


	// Give IP and port info to the ChatClient instance
	if (CLI->target_ip_address.empty() == false)
		ChatClientInstance->target_external_ip = CLI->target_ip_address;

	if (CLI->target_port.empty() == false)
		ChatClientInstance->target_external_port = CLI->target_port;

	if (CLI->my_ext_ip_address.empty() == false)
		ChatClientInstance->my_external_ip = CLI->my_ext_ip_address;

	if (CLI->my_ip_address.empty() == false)
		ChatClientInstance->my_local_ip = CLI->my_ip_address;

	if (CLI->my_host_port.empty() == false)
		ChatClientInstance->my_local_port = CLI->my_host_port;
}

// The user's IP and port input will always be used over the IP and port that the UPnP
// class tried to give.
void upnpGivesIPAndPortToChatProgram(CommandLineInput* CLI, UPnP* UpnpInstance, Connection* ChatServerInstance, Connection* ChatClientInstance)
{
	// If the user inputted values at the command line interface
	// designated for IP and / or port, we will take those values
	// and give them to the chat program.
	// Otherwise, we will just take the IP and Port that
	// the UPnP class has gathered / made.

	// Give IP and port info to the ChatServer instance
	if (CLI->target_ip_address.empty() == false)
		ChatServerInstance->target_external_ip = CLI->target_ip_address;

	if (CLI->target_port.empty() == false)
		ChatServerInstance->target_external_port = CLI->target_port;

	if (CLI->my_ext_ip_address.empty() == false)
		ChatServerInstance->my_external_ip = CLI->my_ext_ip_address;
	else
		ChatServerInstance->my_external_ip = UpnpInstance->my_external_ip;

	if (CLI->my_ip_address.empty() == false)
		ChatServerInstance->my_local_ip = CLI->my_ip_address;
	else
		ChatServerInstance->my_local_ip = UpnpInstance->my_local_ip;

	if (CLI->my_host_port.empty() == false)
		ChatServerInstance->my_local_port = CLI->my_host_port;
	else
		ChatServerInstance->my_local_port = UpnpInstance->my_internal_port;


	// Give IP and port info to the ChatClient instance
	if (CLI->target_ip_address.empty() == false)
		ChatClientInstance->target_external_ip = CLI->target_ip_address;

	if (CLI->target_port.empty() == false)
		ChatClientInstance->target_external_port = CLI->target_port;

	if (CLI->my_ext_ip_address.empty() == false)
		ChatClientInstance->my_external_ip = CLI->my_ext_ip_address;
	else
		ChatClientInstance->my_external_ip = UpnpInstance->my_external_ip;

	if (CLI->my_ip_address.empty() == false)
		ChatClientInstance->my_local_ip = CLI->my_ip_address;
	else
		ChatClientInstance->my_local_ip = UpnpInstance->my_local_ip;

	if (CLI->my_host_port.empty() == false)
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
	std::cout << "Welcome to CryptRelay Alpha release 0.8.0\n";

	UPnP* Upnp = nullptr;		// Not sure if the user wants to use UPnP yet, so just preparing with a pointer.
	Connection ChatServer;
	Connection ChatClient;

	if (CLI.show_info_upnp == true)
	{
		Upnp = new UPnP;
		Upnp->standaloneShowInformation();
		delete Upnp;
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
		// The user MUST supply a local ip address when using the lan only option.
		if (CLI.my_ip_address.empty() == true)
		{
			std::cout << "ERROR: User didn't specify his local IP address.\n";
			return EXIT_FAILURE;
		}

		// Give IP and port info to the ChatServer and ChatClient instance
		cliGivesIPAndPortToChatProgram(&CLI, &ChatServer, &ChatClient);
	}
	if (CLI.use_upnp_to_connect_to_peer == true)
	{
		Upnp = new UPnP;	// deltag:9940   (this is just so I can ctrl-f and find where I delete this.
							// If I could make this a plug-in or something of the sort to automatically
							// add tags to everything or keep track of it, that would be pretty cool.

		// Give the user's inputted port to the UPnP Class
		// so that it will port forward what he wanted.
		cliGivesPortToUPnP(&CLI, Upnp);


		// in order to check if port is open (atleast in this upnp related function)
		// these must be done first:
		// 1. UPNP find devices
		// 2. UPNP find valid IGD
		// 
		// Do the port checking here

		Upnp->findUPnPDevices();

		if (Upnp->findValidIGD() == false)
			return EXIT_FAILURE;

		// Checking to see if the user inputted a port number.
		// If he didn't, then he probably doesn't care, and just
		// wants whatever port can be given to him; therefore
		// we will +1 the port each time isLocalPortInUse() returns
		// true, and then try checking again.
		// If the port is not in use, it is assigned as the port
		// that the Connection will use.
		if (CLI.my_host_port.empty() == true)
		{
			PortKnock PortTest;
			const int IN_USE = 1;
			int my_port_int = 0;
			const int ATTEMPT_COUNT = 20;

			// Only checking ATTEMPT_COUNT times to see if the port is in use.
			// Only assigning a new port number ATTEMPT_COUNT times.
			for (int i = 0; i < ATTEMPT_COUNT; ++i)
			{
				if (i == 19)
				{
					std::cout << "Error: After " << i << " attempts, no available port could be found for the purpose of listening.\n";
					std::cout << "Please specify the port on which you wish to listen for incomming connections by using -mP.\n";
					return EXIT_FAILURE;
				}
				if (PortTest.isLocalPortInUse(Upnp->my_internal_port, Upnp->my_local_ip) == IN_USE)
				{
					// Port is in use, lets try again with port++
					std::cout << "Port: " << Upnp->my_internal_port << " is already in use.\n";
					my_port_int = stoi(Upnp->my_internal_port);
					if (my_port_int < USHRT_MAX)
						++my_port_int;
					else
						my_port_int = 30152; // Arbitrary number given b/c the port num was too big.
					Upnp->my_internal_port = std::to_string(my_port_int);
					std::cout << "Trying port: " << Upnp->my_internal_port << " instead.\n\n";
				}
				else
				{
					if (i != 0)// if i != 0, then there must must have assigned a new port number.
					{
						std::cout << "Now using Port: " << Upnp->my_internal_port << " as my local port.\n";
						std::cout << "This is because the default port was already in use\n\n";
					}
					break;// Port is not in use.
				}
			}
		}

		// Add a port forward rule so we can connect to a peer, and if that
		// succeeded, then give info to the Connection.
		if (Upnp->autoAddPortForwardRule() == true)
		{
			// Give IP and port info gathered from the command line and from
			// the UPnP class to the ChatServer and ChatClient instance
			upnpGivesIPAndPortToChatProgram(&CLI, Upnp, &ChatServer, &ChatClient);
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
	Connection::createStartServerThread(&ChatServer);			//<-----------------
	Connection::createStartClientThread(&ChatClient);			//<----------------

	// Wait for ChatServer and ChatClient threads to finish
#ifdef __linux__
	int pret = pthread_join(Connection::thread0, NULL);
            if (pret)
			{
				std::cout << "error";
				DBG_ERR("It failed at ");
			}
	pret = pthread_join(Connection::thread1, NULL);
            if (pret)
			{
				std::cout << "error";
				DBG_ERR("It failed at ");
			}
#endif//__linux__
#ifdef _WIN32
	int r = WaitForMultipleObjects(
			(DWORD)2,	// Number of objects in array
			Connection::ghEvents,	// Array of objects
			TRUE,		// Wait for all objects if it is set to TRUE. FALSE == wait for any one object to finish. Return value indicates the returned thread(s?).
			INFINITE	// Its going to wait this long, OR until all threads are finished, in order to continue.
		);
	if (r == WAIT_FAILED)
	{
		std::cout << "Error " << GetLastError() << ". WaitForMultipleObjects() failed.\n";
		DBG_ERR("It failed at ");
	}
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
//	3. in project settings, C / C++->Preprocessor->Preprocessor Definitions->MINIUPNP_STATICLIB
//	please note we are just adding MINIUPNP_STATICLIB to the list of whatever is currently there. Don't delete everything in there.
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
