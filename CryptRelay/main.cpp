//main.cpp
//Program name: CryptRelay
//Formatting guide: Located at the bottom of main.cpp

// Windows might be blocking me from sending packets with spoofed source IPs. My google-fu is failing me; hard to find answer.
// I don't think Linux would block it, but if it does, it can be changed!

//TODO:
//nat traversal
//encryption
//add port checking in FormatCheck.cpp
//create a file in the same folder cryptrelay.exe is in and place whatever the user inputted for -m and -mp inside there.
//	^this is so the user will not have to specify their IP and port to listen on _every_single_time.
//when person specifies a port to listen on, maybe should check if that port is currently being used by some other program?
//output what IP and port you are listening on
//output the IP and port of the person you connected to.
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

#include <arpa/inet.h>
#include <signal.h>

#include <pthread.h>	//<process.h>

#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "RawConnection.h"
#endif//__linux__

#ifdef _WIN32
#include <iostream>
#include <string>
#include <vector>
#include <WS2tcpip.h>

#include <process.h>	//<pthread.h>

#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "RawConnection.h"
#endif//_WIN32


bool global_verbose = false;

int main(int argc, char *argv[])
{
	int errchk = 0;

	// Check what the user wants to do via command line input
	CommandLineInput CLI;
	if (errchk = CLI.getCommandLineInput(argc, argv) == false)	// Ip address and port info will be stored in CLI.
		return 0;

	//===================================== Starting Chat Program =====================================

	std::cout << "Welcome to the chat program. Version: 0.5.0\n"; // Somewhat arbitrary version number usage at the moment :)

	
	Raw RawBetterNamePls;
	// Initialize Winsock for everything in the Raw class
	if (RawBetterNamePls.initializeWinsock() == false)
		return 0;
	// Set address information for the Raw class
	RawBetterNamePls.setAddress(
		CLI.target_extrnl_ip_address,
		CLI.target_port,
		CLI.target_local_ip,
		CLI.my_ip_address,
		CLI.my_host_port,
		CLI.my_ext_ip_address
	);

	// SERVER: Start Threaded ICMP Echo Request
	if (RawBetterNamePls.sendICMPEchoRequeest() == false)	// to stop this thread, set RawBetterNamePls.stop_echo_request_loop = true;
		return 0;
	// CLIENT: Start Threaded ICMP Time Exceeded
	if (RawBetterNamePls.sendICMPTimeExceeded() == false)
		return 0;

	std::cout << "PAUSE...";
	std::string pause = "";
	std::getline(std::cin, pause);

/*
	if (no_3rd_party == true) {
		// Initiate the stupid idea (i think it will work though) of spamming ports with UDP to attempt a connection, lol.
		// Unforeseen consequences may occur if sending lots of packets to a router / ports that might have a program on it that doesn't
		// handle random packets very well? IDK.
		// This seems to be the only way to do it w/o admin/root privileges on windows. the ICMP request & time exceeded method is
		// much cleaner, but again, requires admin priv.

		TCPConnection UDPSpamObj;
		//todo: put all send actions in a thread
		UDPSpamObj.clientSetIpAndPort(CLI.target_ip_address, CLI.target_port);
		UDPSpamObj.initializeWinsock();
		UDPSpamObj.UDPSpamCreateSocket();
		UDPSpamObj.UDPSpamPortsWithSendTo();
		//todo: receive UDP messages at the same time here in the main thread.

		// this requires the person you are trying to connect to to be doing the same thing you are doing here ^

		// ROUGH OUTLINE:
		// the idea is to have person A randomly discover the external port of person B.
		// and since B will be sending out messages, as well as trying to listen on a port for messages, it should
		// be able to see/accept the incoming UDP msg that luckily got into the correct external port.
		// now quickly do:
		// 1. send 1 udp msg back to the address to stop A's UDP listening and make it drop into a TCP accept()
		// 2. attempt a TCP connection with A using the external IP and port info that was gathered from the UDP msg.
	}
*/

	// Server startup sequence
	TCPConnection serverObj;
	if (global_verbose == true)
		std::cout << "SERVER::";
	serverObj.serverSetIpAndPort(CLI.my_ip_address, CLI.my_host_port);
	if (serverObj.initializeWinsock() == false)
		return 1;
	serverObj.ServerSetHints();

	// Client startup sequence
	TCPConnection clientObj;
	if (global_verbose == true)
		std::cout << "CLIENT::";
	clientObj.clientSetIpAndPort(CLI.target_extrnl_ip_address, CLI.target_port);
	if (clientObj.initializeWinsock() == false)
		return 1;
	clientObj.ClientSetHints();

	// BEGIN THREAD RACE
	serverObj.createServerRaceThread(&serverObj);			//<-----------------
	clientObj.createClientRaceThread(&clientObj);			//<----------------

	// Wait for 1 thread to finish
#ifdef __linux__
	pthread_join(TCPConnection::thread1, NULL);
	//pthread_join(TCPConnection::thread2, NULL);				//TEMPORARILY IGNORED, not that i want to wait for both anyways, just any 1 thread.
#endif//__linux__
#ifdef _WIN32
	WaitForMultipleObjects(
		2,			//number of objects in array
		TCPConnection::ghEvents,	//array of objects
		FALSE,		//wait for all objects if it is set to TRUE, otherwise FALSE means it waits for any one object to finish. return value indicates the finisher.
		INFINITE);	//its going to wait this long, OR until all threads are finished, in order to continue.
#endif//_WIN32

	// Display the winning thread
	if (global_verbose == true)
	{
		if (TCPConnection::globalWinner == -8)
			std::cout << "MAIN && Client thread is the winner!\n";
		else if (TCPConnection::globalWinner == -7)
			std::cout << "MAIN && Server thread is the winner!\n";
		else
			std::cout << "MAIN && There is no winner.\n";
	}

	int who_won = TCPConnection::globalWinner;
	if (who_won == TCPConnection::NOBODY_WON) 
	{
		std::cout << "Error: Unexpected race result. Exiting\n";
		return 1;
	}

	// Continue running the program for the thread that returned and won.
	while (who_won == TCPConnection::CLIENT_WON || who_won == TCPConnection::SERVER_WON){
		// If server won, then act as a server.
		if (who_won == TCPConnection::SERVER_WON)
		{ 
			std::cout << "Connection established as the server.\n";
			serverObj.closeTheListeningSocket();	// No longer need listening socket since I only want to connect to 1 person at a time.
			serverObj.serverCreateSendThread(&serverObj);
			if (serverObj.receiveUntilShutdown() == false)
				return 1;

			//shutdown
			if (serverObj.shutdownConnection() == false)
				return 1;
			break;
		}
		// If client won, then act as a client
		else if (who_won == TCPConnection::CLIENT_WON)
		{
			std::cout << "Connection established as the client.\n";
			clientObj.clientCreateSendThread(&clientObj);
			if (clientObj.receiveUntilShutdown() == false)
				return 1;

			// Shutdown
			if (clientObj.shutdownConnection() == false)
				return 1;
			break;
		}
		// If nobody won, quit
		else if (who_won == TCPConnection::NOBODY_WON) 
		{
			std::cout << "ERROR: Unexpected doomsday scenario.\n";
			std::cout << "ERROR: Shutting down.\n";
			clientObj.myWSACleanup();
			serverObj.myWSACleanup();
			return 1;
		}
		else 
		{
			std::cout << "ERROR: Shutting down. The impossible is possible.\n";
			clientObj.myWSACleanup();
			serverObj.myWSACleanup();
			return 1;
		}
	}
	std::cout << "beep boop - beep boop. while loop finished or false\n";
	std::cout << "Program exit.\n\n";

	/*

	// Wait until all threads are finished
	WaitForMultipleObjects(
		1,			// Number of objects in array
		ghEvents,	// Array of objects
		TRUE,		// Wait for all objects
		INFINITE);	// Its going to wait this long, OR until all threads are finished, in order to continue.

	*/

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