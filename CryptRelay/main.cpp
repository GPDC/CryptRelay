//main.cpp
//Program name: CryptRelay
//Formatting guide: Located at the bottom of main.cpp








//get the external ip and port somehow from the NAT by asking it?
//or by brute forcing it while both sides listen?/ set sockopt to have no time to live or something so connection doesn't get clogged waiting for return connections?
//listen for udp connection on a port
//spam eachother's ports with udp packets
//if received a msg, write down the external ip & port of the incoming msg
//quickly attempt to connect via tcp to the given ext ip & port
//profit










//put addrinfo structs into a seperate file
//put it in the constructor of the class
//and deconstructor
//so there can be no mistakes with trying to freeaddrinfo();

//TODO:
//nat traversal
//encryption
//rename ipaddress.cpp to FormatCheck.cpp
//add port checking in FormatCheck.cpp
//rename CommandLineInput.cpp to ProgramInput.cpp
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
#include "ipaddress.h"
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
#include "ipaddress.h"
#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"

#include <iostream>
#include <string>
#include <vector>

#include <process.h>//<pthread.h>
#endif//_WIN32


const int MAX_DIFF_INPUTS = 12; //max possible count of argc to bother reading from
bool global_verbose = false;

//Multiple: make a continuous loop that checks for connection requests using the listen function. if a connection request occurs, call accept and pass the work to another thread to handle it.

int main(int argc, char *argv[])
{
	// Check what the user wants to do via command line input
	CommandLineInput CLI;
	CLI.getCommandLineInput(argc, argv);

	//===================================== Starting Chat Program =====================================

	std::cout << "Welcome to the chat program. Version: 0.5.0\n";//somewhat arbitrary version number usage at the moment :)

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

guide here!




+++++++++++++++++++++++++++++++++++ End Formatting Guide +++++++++++++++++++++++++++++++++++

*/