// main.cpp

// Program name: CryptRelay
// Version: 0.8.1
// Rough outline for future versions:
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

#include <thread>
#include <pthread.h>	//<process.h>

#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "SocketClass.h"
#include "Connection.h"
#include "PortKnock.h"
#include "UserInput.h"
#include "ApplicationLayer.h"
#include "FileTransfer.h"

#include "UPnP.h"
#endif//__linux__

#ifdef _WIN32
#include <iostream>
#include <string>
#include <vector>

#include <WS2tcpip.h>
#include <thread>
#include <process.h>	//<pthread.h>

#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "SocketClass.h"
#include "Connection.h"
#include "PortKnock.h"
#include "UserInput.h"
#include "ApplicationLayer.h"
#include "FileTransfer.h"

#include "UPnP.h"
#endif//_WIN32


bool global_verbose = false;
bool global_debug = true;
bool exit_now = false;

// Give Port information, supplied by the user, to the UPnP Class.
void cliGivesPortToUPnP(CommandLineInput* CLI, UPnP* UpnpInstance);

// Gives IP and Port information to the Chat Program.
// /* from */ CommandLineInput* CLI
// /* to */ Connection* ServerConnectInstance
// /* to */ Connection* ServerConnectInstance
void cliGivesIPAndPortToChatProgram(
		const CommandLineInput* CLI,
		Connection* ServerConnectInstance,
		Connection* ClientConnectInstance
	);

// Gives IP and Port information to the Chat Program.
// /* from */ CommandLineInput* CLI
// /* from */ UPnP* UpnpInstance	//only if the user didn't input anything in the CLI
// /* to */ Connection* ServerConnectInstance
// /* to */ Connection* ServerConnectInstance
void upnpGivesIPAndPortToChatProgram(
		const CommandLineInput* CLI,
		const UPnP* UpnpInstance,
		Connection* ServerConnectInstance,
		Connection* ClientConnectInstance
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

void cliGivesIPAndPortToChatProgram(CommandLineInput* CLI, Connection* ServerConnectInstance, Connection* ClientConnectInstance)
{
	// If the user inputted values at the command line interface
	// designated for IP and / or port, we will take those values
	// and give them to the chat program.

	// Give IP and port info to the ServerConnect instance
	if (CLI->getTargetIpAddress().empty() == false)
		ServerConnectInstance->target_external_ip = CLI->getTargetIpAddress();

	if (CLI->getTargetPort().empty() == false)
		ServerConnectInstance->target_external_port = CLI->getTargetPort();

	if (CLI->getMyExtIpAddress().empty() == false)
		ServerConnectInstance->my_external_ip = CLI->getMyExtIpAddress();

	if (CLI->getMyIpAddress().empty() == false)
		ServerConnectInstance->my_local_ip = CLI->getMyIpAddress();

	if (CLI->getMyHostPort().empty() == false)
		ServerConnectInstance->my_local_port = CLI->getMyHostPort();


	// Give IP and port info to the ServerConnect instance
	if (CLI->getTargetIpAddress().empty() == false)
		ClientConnectInstance->target_external_ip = CLI->getTargetIpAddress();

	if (CLI->getTargetPort().empty() == false)
		ClientConnectInstance->target_external_port = CLI->getTargetPort();

	if (CLI->getMyExtIpAddress().empty() == false)
		ClientConnectInstance->my_external_ip = CLI->getMyExtIpAddress();

	if (CLI->getMyIpAddress().empty() == false)
		ClientConnectInstance->my_local_ip = CLI->getMyIpAddress();

	if (CLI->getMyHostPort().empty() == false)
		ClientConnectInstance->my_local_port = CLI->getMyHostPort();
}

// The user's IP and port input will always be used over the IP and port that the UPnP
// class tried to give.
void upnpGivesIPAndPortToChatProgram(CommandLineInput* CLI, UPnP* UpnpInstance, Connection* ServerConnectInstance, Connection* ClientConnectInstance)
{
	// If the user inputted values at the command line interface
	// designated for IP and / or port, we will take those values
	// and give them to the chat program.
	// Otherwise, we will just take the IP and Port that
	// the UPnP class has gathered / made.

	// Give IP and port info to the ServerConnect instance
	if (CLI->getTargetIpAddress().empty() == false)
		ServerConnectInstance->target_external_ip = CLI->getTargetIpAddress();

	if (CLI->getTargetPort().empty() == false)
		ServerConnectInstance->target_external_port = CLI->getTargetPort();

	if (CLI->getMyExtIpAddress().empty() == false)
		ServerConnectInstance->my_external_ip = CLI->getMyExtIpAddress();
	else
		ServerConnectInstance->my_external_ip = UpnpInstance->my_external_ip;

	if (CLI->getMyIpAddress().empty() == false)
		ServerConnectInstance->my_local_ip = CLI->getMyIpAddress();
	else
		ServerConnectInstance->my_local_ip = UpnpInstance->my_local_ip;

	if (CLI->getMyHostPort().empty() == false)
		ServerConnectInstance->my_local_port = CLI->getMyHostPort();
	else
		ServerConnectInstance->my_local_port = UpnpInstance->upnp_my_internal_port;


	// Give IP and port info to the ServerConnect instance
	if (CLI->getTargetIpAddress().empty() == false)
		ClientConnectInstance->target_external_ip = CLI->getTargetIpAddress();

	if (CLI->getTargetPort().empty() == false)
		ClientConnectInstance->target_external_port = CLI->getTargetPort();

	if (CLI->getMyExtIpAddress().empty() == false)
		ClientConnectInstance->my_external_ip = CLI->getMyExtIpAddress();
	else
		ClientConnectInstance->my_external_ip = UpnpInstance->my_external_ip;

	if (CLI->getMyIpAddress().empty() == false)
		ClientConnectInstance->my_local_ip = CLI->getMyIpAddress();
	else
		ClientConnectInstance->my_local_ip = UpnpInstance->my_local_ip;

	if (CLI->getMyHostPort().empty() == false)
		ClientConnectInstance->my_local_port = CLI->getMyHostPort();
	else
		ClientConnectInstance->my_local_port = UpnpInstance->upnp_my_internal_port;
}

FileTransfer* FileXfer = nullptr;
ApplicationLayer* AppLayer = nullptr;




// returns 0 on success.
// returns -1 when a file is still being transfered, and needs to wait for it to finish.
int32_t startThreadedFileXfer(const std::string& file_name_and_path)
{
	// If true, there must be a FileTransfer class instance.
	if (FileXfer != nullptr)
	{
		// Check if the file transfer is done
		if (FileXfer->is_send_file_thread_in_use == false)
		{
			// if the thread exists
			if (FileXfer->send_file_thread.joinable() == true)
			{
				// Wait for it to finish whatever it is doing, and then destroy it.
				FileXfer->send_file_thread.join();

				// Upon creation of a FileTransfer instance, it will
				// create a threaded sendFile() in the constructor.
				bool send_file = true;
				delete(FileXfer); // destroy an old one if there is one.
				FileXfer = new FileTransfer(AppLayer, file_name_and_path, send_file);
				return 0;
			}
			else // A thread must not exist. This shouldn't be possible to get here.
			{
				DBG_DISPLAY_ERROR_LOCATION();
				std::cout << "Error: impossible else statement reached. startThreadedFileXfer()\n";
				return -1;
			}
		}
		else // File transfer is still in progress. Please wait until it is finished.
		{
			return -1;
		}
	}
	else // no FileTransfer class instance, let's create one.
	{
		bool send_file = true;
		delete(FileXfer);
		FileXfer = new FileTransfer(AppLayer, file_name_and_path, send_file);
		return 0;
	}

	return 0;
}

// Returns -1, error
// returns total bytes sent, success
int64_t sendChatMessage(std::string& user_input)
{
	// Prevent the user from sending 'nothing' to the peer.
	if (user_input.length() == 0)
	{
		const int32_t ZERO_BYTES_SENT = 0;
		return ZERO_BYTES_SENT;
	}
	if (AppLayer != nullptr)
		return AppLayer->sendChatStr(user_input);
	else
		return -1;
}

// shutdown() and close() the socket
// returns -1, error
// returns 0, success
int32_t endConnection()
{
	if (AppLayer != nullptr)
	{
		if (AppLayer->endConnection() == 0)
			return 0;
		else
			return -1;
	}
	else
		return -1;
}




int32_t main(int32_t argc, char *argv[])
{
	int32_t errchk = 0;

	CommandLineInput CLI;
	// Check what the user wants to do via command line input
	// Information inputted by the user on startup is stored in CLI
	if ( (errchk = CLI.setVariablesFromArgv(argc, argv)) == true)	
		return EXIT_FAILURE;


	//===================================== Starting Chat Program =====================================
	std::cout << "Welcome to CryptRelay Alpha release 0.8.0\n";

	UPnP* Upnp = nullptr;		// Not sure if the user wants to use UPnP yet, so just preparing with a pointer.

	SocketClass ServerSocket;
	SocketClass ClientSocket;
	Connection ServerConnect(&ServerSocket);
	Connection ClientConnect(&ClientSocket);


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

		// Give IP and port info to the ServerConnect and ServerConnect instance
		cliGivesIPAndPortToChatProgram(&CLI, &ServerConnect, &ClientConnect);
	}
	if (CLI.getUseUpnpToConnectToPeer() == true)
	{
		Upnp = new UPnP;

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
			const int32_t IN_USE = 1;
			int32_t my_port_int = 0;
			const int32_t ATTEMPT_COUNT = 20;

			// Only checking ATTEMPT_COUNT times to see if the port is in use.
			// Only assigning a new port number ATTEMPT_COUNT times.
			for (int32_t i = 0; i < ATTEMPT_COUNT; ++i)
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
			// the UPnP class to the ServerConnect and ServerConnect instance
			upnpGivesIPAndPortToChatProgram(&CLI, Upnp, &ServerConnect, &ClientConnect);
		}
		else
		{
			std::cout << "Fatal: Couldn't port forward via UPnP.\n";
			return EXIT_FAILURE;
		}
	}

	// Being thread race to attempt a connection with the peer.
	std::cout << "Attempting to connect to peer...\n";
	std::thread ServerThread = std::thread(&Connection::serverThread, &ServerConnect);
	std::thread ClientThread = std::thread(&Connection::clientThread, &ClientConnect);

	// Wait for the Server and Client threads to finish.
	if (ServerThread.joinable() == true)
		ServerThread.join();
	if (ClientThread.joinable() == true)
		ClientThread.join();
	
	// Server and Client thread must have finished and set the
	// global_winner variable. Let's see which one won the race.
	// From now on we use the winner's socket that they are connected on.
	SocketClass* WinningSocket = nullptr;
	if (Connection::global_winner == Connection::CLIENT_WON)
	{
		WinningSocket = &ClientSocket;
	}
	else if (Connection::global_winner == Connection::SERVER_WON)
	{
		WinningSocket = &ServerSocket;
	}
	else
	{
		std::cout << "failure.\n";
		std::cout << "Fatal error, nobody won the race.\n";
		return 1;
	}

	// Start up the ApplicationLayer, giving it the SocketClass
	// instance which contains the socket that won the race.
	// This will
	AppLayer = new ApplicationLayer(WinningSocket);

	// UserInput instance must be created after the ApplicationLayer instance,
	// because it needs to be able to send things through the ApplicationLayer
	// using these callbacks in order to have data arrive at the peer.
	UserInput UserInput_o;
	UserInput_o.setCallbackStartFileXfer(&startThreadedFileXfer);
	UserInput_o.setCallbackSendChatMessage(&sendChatMessage);
	UserInput_o.setCallbackEndConnection(endConnection);

	// Start getting the user's input.
	std::thread user_input_thread = std::thread(&UserInput::loopedGetUserInput, &UserInput_o);

	// Start receiving data from peer.
	std::thread recv_messages_thread = std::thread(&ApplicationLayer::loopedReceiveMessages, AppLayer);


	// Wait for all threads to finish.
	if (user_input_thread.joinable() == true)
		user_input_thread.join();
	if (recv_messages_thread.joinable() == true)
		recv_messages_thread.join();
	if (FileXfer != nullptr)
	{
		if (FileXfer->send_file_thread.joinable() == true)
			FileXfer->send_file_thread.join();
	}

	delete(FileXfer);
	delete(AppLayer);
	delete(Upnp);

	return 0;
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
					int32_t i = 5;					// Single lines with no curly braces.
					int32_t truth = 1;
					int32_t lie = 0;
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