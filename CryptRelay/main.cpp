//main.cpp
//Program name: CryptRelay

//ipv6 not currently implemented.

//maybe as a chat escape method, put in chat:    cryptrelay.sendfile filenamehere      ...and...       cryptrelay.exit
//what about hitting escape first or another key, then typing a command.

//todo: change return true and return false into a number instead for the int functions. should only use true and false in bool functions.
#include <iostream>
#include <string>
#include "ipaddress.h"
#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include <Windows.h>
#include <process.h>

DWORD dwevent;
HANDLE ghEvents[2];

#define NOBODY_WON -30
#define SERVER_WON -7
#define CLIENT_WON -8

bool global_verbose = false;

//multiple: make a continuous loop that checks for connection requests using the listen function. if a connection request occurs, call accept and pass the work to another thread to handle it.

int main(int argc, char *argv[])
{
	std::string targetIPaddress = "";
	std::string userPort = "4545";
	CommandLineInput CommandLineInputObj;
	ipaddress ipaddress_get;

	if (argc <= 1) {
		CommandLineInputObj.helpAndReadMe(argc);
		return 1;
	}
	if (argc >= 2 && argv[1] == "-h" || argv[1] == "-H" || argv[1] == "-help" || argv[1] == "-Help" || argv[1] == "help" || argv[1] == "readme") {
		CommandLineInputObj.helpAndReadMe(argc);
		return 1;
	}
	if (argc >= 2)
		targetIPaddress = ipaddress_get.get_target(argv[1]);
	if (targetIPaddress == "bad IP address format.")
		return 1;
	if (argc >= 3)
		userPort = (argv[2]);


	//=================================== starting program ===========================================

	std::cout << "Welcome to the chat program.\n";

	//server startup sequence
	connection serverObj;
//	if(verbose == true)
//		std::cout << "SERVER::";
	serverObj.setIpAndPort(targetIPaddress, userPort);
	if (serverObj.initializeWinsock() == false) return 1;
	serverObj.ServerSetHints();

	//client startup sequence
	connection clientObj;
	if (global_verbose == true)
		std::cout << "CLIENT::";
	clientObj.setIpAndPort(targetIPaddress, userPort);
	if (clientObj.initializeWinsock() == false) return 1;
	clientObj.ClientSetHints();


	//BEGIN SERVER COMPETITION THREADS
	ghEvents[0] = (HANDLE)_beginthread(connection::serverCompetitionThread, 0, &serverObj);	//c style typecast    from: uintptr_t    to: HANDLE.
	ghEvents[1] = (HANDLE)_beginthread(connection::clientCompetitionThread, 0, &clientObj);	//c style typecast    from: uintptr_t    to: HANDLE.

	//wait for any 1 thread to finish
	WaitForMultipleObjects(
		2,			//number of objects in array
		ghEvents,	//array of objects
		FALSE,		//wait for all objects if it is set to TRUE, otherwise FALSE means it waits for any one object to finish. return value indicates the finisher.
		INFINITE);	//its going to wait this long, OR until all threads are finished, in order to continue.

	//display the winning thread
	if (global_verbose == true){
		if (serverObj.globalWinner == -8)
			std::cout << "MAIN && Client thread is the winner!\n";
		else if (serverObj.globalWinner == -7)
			std::cout << "MAIN && Server thread is the winner!\n";
		else
			std::cout << "MAIN && There is no winner.\n";
	}

		

	//continue running the program for the thread that returned and won.
	int who_won = serverObj.globalWinner;
	while (who_won == CLIENT_WON || who_won == SERVER_WON){
		//if server won, then act as a server.
		if (who_won == SERVER_WON){ 
			std::cout << "Connection established as the server.\n";
			serverObj.closeTheListeningSocket();	// No longer need listening socket since I only want to connect to 1 person at a time.
			ghEvents[0] = (HANDLE)_beginthread(connection::sendThread, 0, &serverObj);	// c style typecast    from: uintptr_t    to: HANDLE.
			if (serverObj.receiveUntilShutdown() == false) return 1;

			//shutdown
			if (serverObj.shutdownConnection() == false) return 1;
			serverObj.cleanup();
		}
		//if client won, then act as a client
		else if (who_won == CLIENT_WON){
			std::cout << "Connection established as the client.\n";
			ghEvents[0] = (HANDLE)_beginthread(connection::sendThread, 0, &clientObj);
			if (clientObj.receiveUntilShutdown() == false) return 1;

			//shutdown
			if (clientObj.shutdownConnection() == false) return 1;
			clientObj.cleanup();
		}
		//if nobody won, quit
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
		INFINITE);		//its going to wait this long, OR until all threads are finished, in order to continue.

	*/
}
