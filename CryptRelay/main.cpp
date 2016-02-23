//main.cpp
//Program name: CryptRelay
//Formatting guide: Located at the bottom of main.cpp

//put addrinfo structs into a separate file
//put it in the constructor of the class
//and deconstructor
//so there can be no mistakes with trying to freeaddrinfo();

//TODO:
//nat traversal
//encryption
//rename ipaddress.cpp to FormatCheck.cpp
//add port checking in FormatCheck.cpp
//fill out style guide
//clean up current style format to adhere to the guide
//create a file in the same folder cryptrelay.exe is in and place whatever the user inputted for -m and -mp inside there.
//	^this is so the user will not have to specify their IP and port to listen on _every_single_time.
//when person specifies a port to listen on, maybe should check if that port is currently being used by some other program?
//output what IP and port you are listening on
//output the IP and port of the person you connected to.
//fix chat output issue when someone sends you a message while you are typing
//ipv6

#ifdef __linux__			//to compile on linux, must set linker library standard library pthreads
#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"

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

#include <pthread.h>//<process.h>
#endif//__linux__

#ifdef _WIN32
#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"

#include <iostream>
#include <string>
#include <vector>
#include <WS2tcpip.h>

#include <process.h>//<pthread.h>
#endif//_WIN32


const int MAX_DIFF_INPUTS = 12; //max possible count of argc to bother reading from
bool global_verbose = false;
//bool no_3rd_party = false;	// if true, don't use a 3rd party server to initiate the peer-to-peer connection

int main(int argc, char *argv[])
{
	// Check what the user wants to do via command line input
	CommandLineInput CLI;
	CLI.getCommandLineInput(argc, argv);
	// Ip address and port info is stored in CLI.

	//===================================== Starting Chat Program =====================================

	std::cout << "Welcome to the chat program. Version: 0.5.0\n";//somewhat arbitrary version number usage at the moment :)
	
	Raw RawObj;
	RawObj.initializeWinsock();

	// Set 'hints' aka information that is needed when creating a socket or binding.

	RawObj.GetAddress(CLI.target_ip_address, CLI.target_port);
	RawObj.createSocket(AF_INET, SOCK_RAW, IPPROTO_RAW);
//	freeaddrinfo(RawObj.PResult);				// In this case, we aren't using the addrinfo struct anymore; free it.
	RawObj.craftFixedICMPEchoRequestPacket();
	RawObj.sendTheThing();

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

		connection UDPSpamObj;
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

	//Server startup sequence
	connection serverObj;
	if(global_verbose == true)
		std::cout << "SERVER::";
	serverObj.serverSetIpAndPort(CLI.my_ip_address, CLI.my_port);
	if (serverObj.initializeWinsock() == false) return 1;
	serverObj.ServerSetHints();

	//Client startup sequence
	connection clientObj;
	if (global_verbose == true)
		std::cout << "CLIENT::";
	clientObj.clientSetIpAndPort(CLI.target_ip_address, CLI.target_port);
	if (clientObj.initializeWinsock() == false) return 1;
	clientObj.ClientSetHints();

	//BEGIN THREAD RACE
	serverObj.createServerRaceThread(&serverObj);			//<-----------------
	clientObj.createClientRaceThread(&clientObj);			//<----------------

	//Wait for 1 thread to finish
#ifdef __linux__
	pthread_join(connection::thread1, NULL);				//currently, on the linux side, the program is practically stuck on connect. idk y.
	//pthread_join(connection::thread2, NULL);				//TEMPORARILY IGNORED, not that i want to wait for both anyways, just any 1 thread.
#endif//__linux__
#ifdef _WIN32
	WaitForMultipleObjects(
		2,			//number of objects in array
		connection::ghEvents,	//array of objects
		FALSE,		//wait for all objects if it is set to TRUE, otherwise FALSE means it waits for any one object to finish. return value indicates the finisher.
		INFINITE);	//its going to wait this long, OR until all threads are finished, in order to continue.
#endif//_WIN32

	//Display the winning thread
	if (global_verbose == true){
		if (connection::globalWinner == -8)
			std::cout << "MAIN && Client thread is the winner!\n";
		else if (connection::globalWinner == -7)
			std::cout << "MAIN && Server thread is the winner!\n";
		else
			std::cout << "MAIN && There is no winner.\n";
	}

	int who_won = connection::globalWinner;
	if (who_won == connection::NOBODY_WON) {
		std::cout << "Error: Unexpected race result. Exiting\n";
		return 1;
	}

	//Continue running the program for the thread that returned and won.
	while (who_won == connection::CLIENT_WON || who_won == connection::SERVER_WON){
		//if server won, then act as a server.
		if (who_won == connection::SERVER_WON){ 
			std::cout << "Connection established as the server.\n";
			serverObj.closeTheListeningSocket();	// No longer need listening socket since I only want to connect to 1 person at a time.
			serverObj.serverCreateSendThread(&serverObj);
			if (serverObj.receiveUntilShutdown() == false) return 1;

			//shutdown
			if (serverObj.shutdownConnection() == false) return 1;
			break;
		}
		//If client won, then act as a client
		else if (who_won == connection::CLIENT_WON){
			std::cout << "Connection established as the client.\n";
			clientObj.clientCreateSendThread(&clientObj);
			if (clientObj.receiveUntilShutdown() == false) return 1;

			//shutdown
			if (clientObj.shutdownConnection() == false) return 1;
			break;
		}
		//If nobody won, quit
		else if (who_won == connection::NOBODY_WON) {
			std::cout << "ERROR: Unexpected doomsday scenario.\n";
			std::cout << "ERROR: Shutting down.\n";
			clientObj.cleanup();
			serverObj.cleanup();
			return 1;
		}
		else {
			std::cout << "ERROR: Shutting down. The impossible is possible.\n";
			clientObj.cleanup();
			serverObj.cleanup();
			return 1;
		}
	}
	std::cout << "beep boop - beep boop. while loop finished or false\n";
	std::cout << "Program exit.\n\n";

	/*

	//wait until all threads are finished
	WaitForMultipleObjects(
		1,			//number of objects in array
		ghEvents,	//array of objects
		TRUE,		//wait for all objects
		INFINITE);	//its going to wait this long, OR until all threads are finished, in order to continue.

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

+++++++++++++++++++++++++++++++++++ End Formatting Guide +++++++++++++++++++++++++++++++++++

*/