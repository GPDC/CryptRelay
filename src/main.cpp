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
//#include <pthread.h>	//<process.h>

#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "XBerkeleySockets.h"
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
#include "XBerkeleySockets.h"
#include "Connection.h"
#include "PortKnock.h"
#include "UserInput.h"
#include "ApplicationLayer.h"
#include "FileTransfer.h"

#include "UPnP.h"
#endif//_WIN32

// If the user wants to exit the program, exit_now == true;
bool exit_now = false;

UPnP* Upnp = nullptr;
FileTransfer* FileXfer = nullptr;
ApplicationLayer* AppLayer = nullptr;
CommandLineInput CLI;
XBerkeleySockets BerkeleySockets;
Connection * ServerConnect = nullptr;
Connection * ClientConnect = nullptr;


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
		UpnpInstance->setMyInternalPort(CLI->getMyHostPort().c_str());
		UpnpInstance->setMyExternalPort(CLI->getMyHostPort().c_str());
	}
}

void cliGivesIPAndPortToChatProgram(CommandLineInput* CLI, Connection* ServerConnectInstance, Connection* ClientConnectInstance)
{
	// If the user inputted values at the command line interface
	// designated for IP and / or port, we will take those values
	// and give them to the chat program.

	// Give IP and port info to the ServerConnect instance
	if (CLI->getTargetIpAddress().empty() == false)
		ServerConnectInstance->setTargetExternalIP(CLI->getTargetIpAddress());

	if (CLI->getTargetPort().empty() == false)
		ServerConnectInstance->setTargetExternalPort(CLI->getTargetPort());

	if (CLI->getMyExtIpAddress().empty() == false)
		ServerConnectInstance->setMyExternalIP(CLI->getMyExtIpAddress());

	if (CLI->getMyIpAddress().empty() == false)
		ServerConnectInstance->setMyLocalIP(CLI->getMyIpAddress());

	if (CLI->getMyHostPort().empty() == false)
		ServerConnectInstance->setMyLocalPort(CLI->getMyHostPort());


	// Give IP and port info to the ClientConnect instance
	if (CLI->getTargetIpAddress().empty() == false)
		ClientConnectInstance->setTargetExternalIP(CLI->getTargetIpAddress());

	if (CLI->getTargetPort().empty() == false)
		ClientConnectInstance->setTargetExternalPort(CLI->getTargetPort());

	if (CLI->getMyExtIpAddress().empty() == false)
		ClientConnectInstance->setMyExternalIP(CLI->getMyExtIpAddress());

	if (CLI->getMyIpAddress().empty() == false)
		ClientConnectInstance->setMyLocalIP(CLI->getMyIpAddress());

	if (CLI->getMyHostPort().empty() == false)
		ClientConnectInstance->setMyLocalPort(CLI->getMyHostPort());
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
		ServerConnectInstance->setTargetExternalIP(CLI->getTargetIpAddress());

	if (CLI->getTargetPort().empty() == false)
		ServerConnectInstance->setTargetExternalPort(CLI->getTargetPort());

	if (CLI->getMyExtIpAddress().empty() == false)
		ServerConnectInstance->setMyExternalIP(CLI->getMyExtIpAddress());
	else
		ServerConnectInstance->setMyExternalIP(UpnpInstance->getMyExternalIP());

	if (CLI->getMyIpAddress().empty() == false)
		ServerConnectInstance->setMyLocalIP(CLI->getMyIpAddress());
	else
		ServerConnectInstance->setMyLocalIP(UpnpInstance->getMyLocalIP());

	if (CLI->getMyHostPort().empty() == false)
		ServerConnectInstance->setMyLocalPort(CLI->getMyHostPort());
	else
		ServerConnectInstance->setMyLocalPort(UpnpInstance->getMyInternalPort());


	// Give IP and port info to the ClientConnect instance
	if (CLI->getTargetIpAddress().empty() == false)
		ClientConnectInstance->setTargetExternalIP(CLI->getTargetIpAddress());

	if (CLI->getTargetPort().empty() == false)
		ClientConnectInstance->setTargetExternalPort(CLI->getTargetPort());

	if (CLI->getMyExtIpAddress().empty() == false)
		ClientConnectInstance->setMyExternalIP(CLI->getMyExtIpAddress());
	else
		ClientConnectInstance->setMyExternalIP(UpnpInstance->getMyExternalIP());

	if (CLI->getMyIpAddress().empty() == false)
		ClientConnectInstance->setMyLocalIP(CLI->getMyIpAddress());
	else
		ClientConnectInstance->setMyLocalIP(UpnpInstance->getMyLocalIP());

	if (CLI->getMyHostPort().empty() == false)
		ClientConnectInstance->setMyLocalPort(CLI->getMyHostPort());
	else
		ClientConnectInstance->setMyLocalPort(UpnpInstance->getMyInternalPort());
}

// +1 the port each time isLocalPortInUse() returns
// true, and then try checking again.
// If the port is not in use, it is assigned as the port
// that the Connection will use.
// Returns "" (aka, empty string) if the port wasn't changed, or if error.
// Returns a new port number if it was changed.
std::string changeLocalPortIfInUse(std::string change_this_port, std::string local_ip)
{
	PortKnock PortTest(&BerkeleySockets, CLI.getVerboseOutput());
	const int32_t IN_USE = 1;
	const int32_t AVAILABLE = 0;
	const int32_t ATTEMPT_COUNT = 20;
	int32_t port_status = -1;
	std::string my_port_str = change_this_port;

	// Only checking ATTEMPT_COUNT times to see if the port is in use.
	// Only assigning a new port number ATTEMPT_COUNT times.
	for (int32_t i = 0; i < ATTEMPT_COUNT; ++i)
	{
		if (i == ATTEMPT_COUNT - 1)
		{
			std::cout << "Error: After " << i << " attempts, no available port could be found for the purpose of listening.\n";
			std::cout << "Please specify the port on which you wish to listen for incoming connections manually.\n";
			my_port_str = "";
			break;
		}
		port_status = PortTest.isLocalPortInUse(my_port_str, local_ip);
		if ( port_status == IN_USE)
		{
			// Port is in use, lets try again with port++
			std::cout << "Port: " << my_port_str << " is already in use.\n";
			int32_t my_port_int = stoi(my_port_str);

			// +1 the port
			if (my_port_int < USHRT_MAX)
				++my_port_int;
			else
				my_port_int = 30152; // Arbitrary number given b/c the port num was too big.

			my_port_str = std::to_string(my_port_int);
			std::cout << "Trying port: " << my_port_str << " instead.\n\n";
		}
		else if (port_status == AVAILABLE)
		{
			// if i != 0, then it must have assigned a new port number.
			if (i != 0)
			{

				std::cout << "Now using Port: " << my_port_str << " as my local port.\n";
				std::cout << "This is because the default port was already in use\n\n";
			}
			break;// Port is not in use.
		}
		else
		{
			// Failed to get the status of the port.
			my_port_str = "";
			break;
		}
	}

	return my_port_str;
}

// Portforward the router using UPnP.
// return 0, success
// return -1, error
int32_t portForwardUsingUPnP()
{
	Upnp = new UPnP(CLI.getVerboseOutput());

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

	if (Upnp->findValidIGD() == -1)
		return -1;

	// Checking to see if the user inputted a local port number.
	// If he didn't, then he probably doesn't care, and just
	// wants whatever port can be given to him.
	if (CLI.getMyHostPort().empty() == true)
	{
		std::string changed_port;
		// If the local port is in use by something else, ++ the local port and try again.
		changed_port = changeLocalPortIfInUse(Upnp->getMyInternalPort(), Upnp->getMyLocalIP());
		if (changed_port != "")
		{
			Upnp->setMyInternalPort(changed_port.c_str());
			Upnp->setMyExternalPort(changed_port.c_str());
		}
	}

	// Add a port forward rule so we can connect to a peer, and if that
	// succeeded, then give info to the Connection.
	if (Upnp->autoAddPortForwardRule() == 0)
	{
		// Give IP and port info gathered from the command line and from
		// the UPnP class to the ServerConnect and ServerConnect instance
		upnpGivesIPAndPortToChatProgram(&CLI, Upnp, ServerConnect, ClientConnect);
		return 0; // successful port forward
	}
	else
	{
		std::cout << "Fatal: Couldn't port forward via UPnP.\n";
		return -1; // failed, no port forward
	}
}

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
				delete FileXfer; // destroy an old one if there is one.
				FileXfer = new FileTransfer(AppLayer, file_name_and_path, send_file, CLI.getVerboseOutput());
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
		delete FileXfer;
		FileXfer = new FileTransfer(AppLayer, file_name_and_path, send_file, CLI.getVerboseOutput());
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

// Callback to set the exit_now variable.
void setExitNow(bool value)
{
	exit_now = value;
}

// Callback to view the exit_now variable.
bool& getExitNow()
{
	return exit_now;
}



int32_t main(int32_t argc, char *argv[])
{
	ClientConnect = new Connection(
		&BerkeleySockets,
		&getExitNow,
		&setExitNow,
		CLI.getVerboseOutput()
	);
	ServerConnect = new Connection(
		&BerkeleySockets,
		&getExitNow,
		&setExitNow,
		CLI.getVerboseOutput()
	);

	int32_t errchk = 0;

	// Check what the user wants to do via command line input
	// Information inputted by the user on startup is stored in CLI
	if ( (errchk = CLI.setVariablesFromArgv(argc, argv)) == -1)	
		return EXIT_FAILURE;


	//===================================== Starting Chat Program =====================================
	std::cout << "Welcome to CryptRelay Alpha release 0.8.0\n";


	if (CLI.getShowInfoUpnp() == true)
	{
		// Show some information related to the router
		Upnp = new UPnP(CLI.getVerboseOutput());
		Upnp->standaloneShowInformation();
		delete Upnp;
		return EXIT_SUCCESS;
	}
	else if (CLI.getRetrieveListOfPortForwards() == true)
	{
		// Show the ports that are currently forwarded on the router
		Upnp = new UPnP(CLI.getVerboseOutput());
		Upnp->standaloneDisplayListOfPortForwards();
		delete Upnp;
		return EXIT_SUCCESS;
	}
	else if (CLI.getDeleteThisSpecificPortForward() == true)
	{
		// Delete the specified port forward rule on the router.
		Upnp = new UPnP(CLI.getVerboseOutput());
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

		// Checking to see if the user inputted a local port number.
		// If he didn't, then he probably doesn't care, and just
		// wants whatever port can be given to him.
		if (CLI.getMyHostPort().empty() == true)
		{
			std::string local_port;
			// If the local port is in use by something else, ++ the local port and try again.
			local_port = changeLocalPortIfInUse(ServerConnect->getMyLocalPort(), CLI.getMyIpAddress());
			if (local_port != "")
			{
				ServerConnect->setMyLocalPort(local_port);
				ClientConnect->setMyLocalPort(local_port);
			}
		}

		// Give IP and port info to the ClientConnect and ServerConnect instance
		cliGivesIPAndPortToChatProgram(&CLI, ServerConnect, ClientConnect);
	}

	if (CLI.getUseUpnpToConnectToPeer() == true)
	{
		// Forward ports on the router.
		if (portForwardUsingUPnP() == -1)
			return EXIT_FAILURE;
	}

	// Being thread race to attempt a connection with the peer.
	std::cout << "Attempting to connect to peer...\n";

	std::thread ServerThread = std::thread(&Connection::server, ServerConnect);
	std::thread ClientThread = std::thread(&Connection::client, ClientConnect);

	// Wait for the Server and Client threads to finish.
	if (ServerThread.joinable() == true)
		ServerThread.join();
	if (ClientThread.joinable() == true)
		ClientThread.join();
	
	// Server and Client thread must have finished and set the
	// connection_race_winner variable. Let's see which one won the race.
	// From now on we use the winner's socket that they are connected on.
	Connection * WinningConnectionClass = nullptr;
	if (Connection::connection_race_winner == Connection::CLIENT_WON)
	{
		WinningConnectionClass = ClientConnect;
	}
	else if (Connection::connection_race_winner == Connection::SERVER_WON)
	{
		WinningConnectionClass = ServerConnect;
	}
	else
	{
		std::cout << "failure.\n";
		std::cout << "Fatal error, nobody won the race.\n";
		return 1;
	}

	// Start up the ApplicationLayer, giving it the SOCKET
	// from the Connection class that won the race.
	AppLayer = new ApplicationLayer(
		&BerkeleySockets,
		WinningConnectionClass->getFdSocket(),
		&setExitNow,
		&getExitNow,
		CLI.getVerboseOutput()
	);

	// UserInput instance must be created after the ApplicationLayer instance,
	// because it needs to be able to send things through the ApplicationLayer
	// using these callbacks in order to have data arrive (correctly) at the peer.
	UserInput UserInput_o(
		&startThreadedFileXfer,
		&sendChatMessage,
		&endConnection,
		&setExitNow,
		&getExitNow,
		CLI.getVerboseOutput()
	);

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

	delete FileXfer;
	delete AppLayer;
	delete ServerConnect;
	delete ClientConnect;
	delete Upnp;

	return EXIT_SUCCESS;
//===================================== End Chat Program =====================================
}



/*

+++++++++++++++++++++++++++++++++++++ Formatting Guide +++++++++++++++++++++++++++++++++++++

Classes:		ThisIsAnExample;		# Capitalize the first letter of every word.
Structures:		ThisIsAnExample;		# Capitalize the first letter of every word.
Functions:		thisIsAnExample();		# Capitalize the first letter of every word except the first.
Variables:		this_is_an_example;		# No capitalization. Underscores to separate words.
Macros:         THIS_IS_AN_EXAMPLE;		# All capitalization. Underscores to separate words.
Constants:      THIS_IS_AN_EXAMPLE;		# All capitalization. Underscores to separate words.

ifdefs must always comment the endif with what it is endifing.
#ifdef _WIN32
#endif //_WIN32



Curly braces:	int32_t thisIsAnExample()		// It is up to you to decide what looks better;
				{								// Single lines with curly braces, or
					int32_t i = 5;				// Single lines with no curly braces.
					int32_t truth = 1;
					int32_t lie = 0;
					bool onepp_theory = false;
					bool bear = true;
					bool big = false;

					if (i == 5)
					{
						if(1 + 1 == 2)
							onepp_theory = true;
						if(truth == lie)
							return -1;
						if(bear == big)
							std::cout << "Oh my!\n";
					}

					if (i == 70)
					{
						return 0; // Success
					}
					else
					{
						return -1; // Failure
					}
				}

+++++++++++++++++++++++++++++++++++ End Formatting Guide +++++++++++++++++++++++++++++++++++

*/
