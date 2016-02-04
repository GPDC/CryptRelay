//main.cpp
//Program name: CryptRelay

//Ipv6 not currently implemented.

//Maybe as a chat escape method, put in chat:    cryptrelay.sendfile filenamehere      ...and...       cryptrelay.exit
//What about hitting escape first or another key, then typing a command.

#include <iostream>
#include <string>
#include "ipaddress.h"
#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include <Windows.h>
#include <process.h>
#include <vector>

DWORD dwevent;
HANDLE ghEvents[2];

const int MAX_DIFF_INPUTS = 5;

bool global_verbose = false;

//Multiple: make a continuous loop that checks for connection requests using the listen function. if a connection request occurs, call accept and pass the work to another thread to handle it.

int main(int argc, char *argv[])
{
	std::string targetIPaddress = "";
	std::string userPort = "4545";
	int err_chk;
	CommandLineInput CommandLineInputObj;
	ipaddress ipaddress_get;
	
	//Check to see what the user wants to do via command line input
	std::vector<std::string> arg;
	arg.reserve(MAX_DIFF_INPUTS);
	//put all argv's into a vector so they can be compared to strings
	for (int i = 0; i < argc; i++) {
		arg.push_back(argv[i]);
	}
	//if command line arguments supplied, show the ReadMe
	if (argc <= 1) {
		CommandLineInputObj.helpAndReadMe(argc);
		return 1;
	}
	//check all argv inputs to see what the user wants to do
	if (argc >= 2) {
		for (int i = 0; i < MAX_DIFF_INPUTS; ++i) {

			if (arg[i] == "-h" || arg[i] == "-H" || arg[i] == "-help" || arg[i] == "-Help" || arg[i] == "help" || arg[i] == "readme") {
				CommandLineInputObj.helpAndReadMe(argc);
				return 1;
			}

			//currently taking input from argv[1] and treating that as the target IP address
			if (arg[i] == "-t") {
				err_chk = ipaddress_get.get_target(argv[i + 1]);
				if (err_chk == false)
					return 1;
				else
					return 99;//create a pointer to the thing that has the ip address info!
			}

			//currently taking input from argv[2] and treating that as the target PORT
			if (argc >= 3)
				userPort = (argv[2]);
		}
	}




	//===================================== Starting Chat Program =====================================

	std::cout << "Welcome to the chat program.\n";

	//Server startup sequence
	connection serverObj;
	if(global_verbose == true)
		std::cout << "SERVER::";
	serverObj.setIpAndPort(targetIPaddress, userPort);
	if (serverObj.initializeWinsock() == false) return 1;
	serverObj.ServerSetHints();

	//Client startup sequence
	connection clientObj;
	if (global_verbose == true)
		std::cout << "CLIENT::";
	clientObj.setIpAndPort(targetIPaddress, userPort);
	if (clientObj.initializeWinsock() == false) return 1;
	clientObj.ClientSetHints();


	//BEGIN SERVER COMPETITION THREADS
	ghEvents[0] = (HANDLE)_beginthread(connection::serverCompetitionThread, 0, &serverObj);	//c style typecast    from: uintptr_t    to: HANDLE.
	ghEvents[1] = (HANDLE)_beginthread(connection::clientCompetitionThread, 0, &clientObj);	//c style typecast    from: uintptr_t    to: HANDLE.


	//Wait for any 1 thread to finish
	WaitForMultipleObjects(
		2,			//number of objects in array
		ghEvents,	//array of objects
		FALSE,		//wait for all objects if it is set to TRUE, otherwise FALSE means it waits for any one object to finish. return value indicates the finisher.
		INFINITE);	//its going to wait this long, OR until all threads are finished, in order to continue.

	//Display the winning thread
	if (global_verbose == true){
		if (serverObj.globalWinner == -8)
			std::cout << "MAIN && Client thread is the winner!\n";
		else if (serverObj.globalWinner == -7)
			std::cout << "MAIN && Server thread is the winner!\n";
		else
			std::cout << "MAIN && There is no winner.\n";
	}


	//Continue running the program for the thread that returned and won.
	int who_won = serverObj.globalWinner;
	while (who_won == CLIENT_WON || who_won == SERVER_WON){
		//if server won, then act as a server.
		if (who_won == SERVER_WON){ 
			std::cout << "Connection established as the server.\n";
			serverObj.closeTheListeningSocket();	// No longer need listening socket since I only want to connect to 1 person at a time.
			//clientObj.cleanup();    here????????
			ghEvents[0] = (HANDLE)_beginthread(connection::sendThread, 0, &serverObj);	// c style typecast    from: uintptr_t    to: HANDLE.
			if (serverObj.receiveUntilShutdown() == false) return 1;

			//shutdown
			if (serverObj.shutdownConnection() == false) return 1;
			serverObj.cleanup();
		}
		//If client won, then act as a client
		else if (who_won == CLIENT_WON){
			std::cout << "Connection established as the client.\n";
			ghEvents[0] = (HANDLE)_beginthread(connection::sendThread, 0, &clientObj);
			//serverObj.cleanup();    here??????????
			if (clientObj.receiveUntilShutdown() == false) return 1;

			//shutdown
			if (clientObj.shutdownConnection() == false) return 1;
			clientObj.cleanup();
		}
		//If nobody won, quit
		else if (who_won == NOBODY_WON) {
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
		}
	}
	std::cout << "beep boop - beep boop\n";

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
