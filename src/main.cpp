// main.cpp

// Program name: CryptRelay     version: 0.6.1
// Rough outline for future versions:
// 0.8 == file transfer
// 0.9 == encryption
// 1.0 == polished release

// Formatting guide: Located at the bottom of main.cpp

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


bool global_verbose = false;
bool global_debug = true;

// Give Port information, supplied by the user, to the UPnP Class.
void cliGivesPortToUPnP(CommandLineInput* CLI, UPnP* UpnpInstance);

// Gives IP and Port information to the Chat Program.
// /* from */ CommandLineInput* CLI
// /* to */ Connection* ChatServerInstance
// /* to */ Connection* ChatClientInstance
void cliGivesIPAndPortToChatProgram(
		const CommandLineInput* CLI,
		Connection* ChatServerInstance,
		Connection* ChatClientInstance
	);

// Gives IP and Port information to the Chat Program.
// /* from */ CommandLineInput* CLI
// /* from */ UPnP* UpnpInstance	//only if the user didn't input anything in the CLI
// /* to */ Connection* ChatServerInstance
// /* to */ Connection* ChatClientInstance
void upnpGivesIPAndPortToChatProgram(
		const CommandLineInput* CLI,
		const UPnP* UpnpInstance,
		Connection* ChatServerInstance,
		Connection* ChatClientInstance
	);

// Give user supplied port to the UPnP class
void cliGivesPortToUPnP(CommandLineInput* CLI, UPnP* UpnpInstance)
{
	// Give Port that was supplied by the user to the UPnP class
	if (CLI->getMyHostPort().empty() == false)
	{
		UpnpInstance->upnp_my_internal_port = CLI->getMyHostPort();
		UpnpInstance->upnp_my_external_port = CLI->getMyHostPort();
	}
}

void cliGivesIPAndPortToChatProgram(CommandLineInput* CLI, Connection* ChatServerInstance, Connection* ChatClientInstance)
{
	// If the user inputted values at the command line interface
	// designated for IP and / or port, we will take those values
	// and give them to the chat program.

	// Give IP and port info to the ChatServer instance
	if (CLI->getTargetIpAddress().empty() == false)
		ChatServerInstance->target_external_ip = CLI->getTargetIpAddress();

	if (CLI->getTargetPort().empty() == false)
		ChatServerInstance->target_external_port = CLI->getTargetPort();

	if (CLI->getMyExtIpAddress().empty() == false)
		ChatServerInstance->my_external_ip = CLI->getMyExtIpAddress();

	if (CLI->getMyIpAddress().empty() == false)
		ChatServerInstance->my_local_ip = CLI->getMyIpAddress();

	if (CLI->getMyHostPort().empty() == false)
		ChatServerInstance->my_local_port = CLI->getMyHostPort();


	// Give IP and port info to the ChatClient instance
	if (CLI->getTargetIpAddress().empty() == false)
		ChatClientInstance->target_external_ip = CLI->getTargetIpAddress();

	if (CLI->getTargetPort().empty() == false)
		ChatClientInstance->target_external_port = CLI->getTargetPort();

	if (CLI->getMyExtIpAddress().empty() == false)
		ChatClientInstance->my_external_ip = CLI->getMyExtIpAddress();

	if (CLI->getMyIpAddress().empty() == false)
		ChatClientInstance->my_local_ip = CLI->getMyIpAddress();

	if (CLI->getMyHostPort().empty() == false)
		ChatClientInstance->my_local_port = CLI->getMyHostPort();
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
	if (CLI->getTargetIpAddress().empty() == false)
		ChatServerInstance->target_external_ip = CLI->getTargetIpAddress();

	if (CLI->getTargetPort().empty() == false)
		ChatServerInstance->target_external_port = CLI->getTargetPort();

	if (CLI->getMyExtIpAddress().empty() == false)
		ChatServerInstance->my_external_ip = CLI->getMyExtIpAddress();
	else
		ChatServerInstance->my_external_ip = UpnpInstance->my_external_ip;

	if (CLI->getMyIpAddress().empty() == false)
		ChatServerInstance->my_local_ip = CLI->getMyIpAddress();
	else
		ChatServerInstance->my_local_ip = UpnpInstance->my_local_ip;

	if (CLI->getMyHostPort().empty() == false)
		ChatServerInstance->my_local_port = CLI->getMyHostPort();
	else
		ChatServerInstance->my_local_port = UpnpInstance->upnp_my_internal_port;


	// Give IP and port info to the ChatClient instance
	if (CLI->getTargetIpAddress().empty() == false)
		ChatClientInstance->target_external_ip = CLI->getTargetIpAddress();

	if (CLI->getTargetPort().empty() == false)
		ChatClientInstance->target_external_port = CLI->getTargetPort();

	if (CLI->getMyExtIpAddress().empty() == false)
		ChatClientInstance->my_external_ip = CLI->getMyExtIpAddress();
	else
		ChatClientInstance->my_external_ip = UpnpInstance->my_external_ip;

	if (CLI->getMyIpAddress().empty() == false)
		ChatClientInstance->my_local_ip = CLI->getMyIpAddress();
	else
		ChatClientInstance->my_local_ip = UpnpInstance->my_local_ip;

	if (CLI->getMyHostPort().empty() == false)
		ChatClientInstance->my_local_port = CLI->getMyHostPort();
	else
		ChatClientInstance->my_local_port = UpnpInstance->upnp_my_internal_port;
}

int main(int argc, char *argv[])
{
	int errchk = 0;

	CommandLineInput CLI;
	// Check what the user wants to do via command line input
	// Information inputted by the user on startup is stored in CLI
	if ( (errchk = CLI.setVariablesFromArgv(argc, argv)) == true)	
		return EXIT_FAILURE;


	//===================================== Starting Chat Program =====================================
	std::cout << "Welcome to CryptRelay Alpha release 0.8.0\n";

	UPnP* Upnp = nullptr;		// Not sure if the user wants to use UPnP yet, so just preparing with a pointer.
	Connection ChatServer;
	Connection ChatClient;

	if (CLI.getShowInfoUpnp() == true)
	{
		Upnp = new UPnP;
		Upnp->standaloneShowInformation();
		delete Upnp;
		return EXIT_SUCCESS;
	}
	else if (CLI.getRetrieveListOfPortForwards() == true)
	{
		Upnp = new UPnP;
		Upnp->standaloneGetListOfPortForwards();
		delete Upnp;
		return EXIT_SUCCESS;
	}
	else if (CLI.getDeleteThisSpecificPortForward() == true)
	{
		Upnp = new UPnP;
		Upnp->standaloneDeleteThisSpecificPortForward(
				CLI.getDeleteThisSpecificPortForwardPort().c_str(),
				CLI.getDeleteThisSpecificPortForwardProtocol().c_str()
			);
		delete Upnp;
		return EXIT_SUCCESS;
	}
	else if (CLI.getUseLanOnly() == true)
	{
		// The user MUST supply a local ip address when using the lan only option.
		if (CLI.getMyIpAddress().empty() == true)
		{
			std::cout << "ERROR: User didn't specify his local IP address.\n";
			return EXIT_FAILURE;
		}

		// Give IP and port info to the ChatServer and ChatClient instance
		cliGivesIPAndPortToChatProgram(&CLI, &ChatServer, &ChatClient);
	}
	if (CLI.getUseUpnpToConnectToPeer() == true)
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

		if (Upnp->findValidIGD() == true)
			return EXIT_FAILURE;

		// Checking to see if the user inputted a port number.
		// If he didn't, then he probably doesn't care, and just
		// wants whatever port can be given to him; therefore
		// we will +1 the port each time isLocalPortInUse() returns
		// true, and then try checking again.
		// If the port is not in use, it is assigned as the port
		// that the Connection will use.
		if (CLI.getMyHostPort().empty() == true)
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
				if (PortTest.isLocalPortInUse(Upnp->upnp_my_internal_port, Upnp->my_local_ip) == IN_USE)
				{
					// Port is in use, lets try again with port++
					std::cout << "Port: " << Upnp->upnp_my_internal_port << " is already in use.\n";
					my_port_int = stoi(Upnp->upnp_my_internal_port);
					if (my_port_int < USHRT_MAX)
						++my_port_int;
					else
						my_port_int = 30152; // Arbitrary number given b/c the port num was too big.
					Upnp->upnp_my_internal_port = std::to_string(my_port_int);
					Upnp->upnp_my_external_port = Upnp->upnp_my_internal_port;
					std::cout << "Trying port: " << Upnp->upnp_my_internal_port << " instead.\n\n";
				}
				else
				{
					if (i != 0)// if i != 0, then it must have assigned a new port number.
					{
						std::cout << "Now using Port: " << Upnp->upnp_my_internal_port << " as my local port.\n";
						std::cout << "This is because the default port was already in use\n\n";
					}
					break;// Port is not in use.
				}
			}
		}

		// Add a port forward rule so we can connect to a peer, and if that
		// succeeded, then give info to the Connection.
		if (Upnp->autoAddPortForwardRule() == false)
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
				DBG_DISPLAY_ERROR_LOCATION();
			}
	pret = pthread_join(Connection::thread1, NULL);
            if (pret)
			{
				std::cout << "error";
				DBG_DISPLAY_ERROR_LOCATION();
			}
#endif//__linux__
#ifdef _WIN32
	int thread_wait_errchk = WaitForMultipleObjects(
			(DWORD)2,	// Number of objects in array
			Connection::ghEvents,	// Array of objects
			TRUE,		// Wait for all objects if it is set to TRUE. FALSE == wait for any one object to finish. Return value indicates the returned thread(s?).
			INFINITE	// Its going to wait this long, OR until all threads are finished, in order to continue.
		);
	if (thread_wait_errchk == WAIT_FAILED)
	{
		std::cout << "Error " << GetLastError() << ". WaitForMultipleObjects() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
	}
#endif//_WIN32	

	
	delete Upnp;	// deltag:9940

	return 1;
//===================================== End Chat Program =====================================
}



/*

+++++++++++++++++++++++++++++++++++++ Formatting Guide +++++++++++++++++++++++++++++++++++++

Classes:		ThisIsAnExample();		# Capitalize the first letter of every word.
Structures:		ThisIsAnExample();		# Capitalize the first letter of every word.
Functions:		thisIsAnExample();		# Capitalize the first letter of every word except the first.
Variables:		this_is_an_example;		# No capitalization. Underscores to separate words.
Macros:         THIS_IS_AN_EXAMPLE;		# All capitalization. Underscores to separate words.
Globals:								# Put the word global in front of it like so:
	ex variable:	global_this_is_an_example;
	ex function:	globalThisIsAnExample;

ifdefs must always comment the endif with what it is endifing.
#ifdef _WIN32
#endif //_WIN32



Curly braces:	bool thisIsAnExample()			// It is up to you to decide what looks better;
				{								// Single lines with curly braces, or
					int i = 5;					// Single lines with no curly braces.
					int truth = 1;
					int lie = 0;
					bool onepptheory = false;
					bool bear = true;
					bool big = false;

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
							onepptheory = true;
						if(truth == lie)
							return false;
						if(bear == big)
							std::cout << "Oh my!\n";
					}
				}

+++++++++++++++++++++++++++++++++++ End Formatting Guide +++++++++++++++++++++++++++++++++++

*/