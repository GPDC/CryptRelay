//main.cpp
//Program name: CryptRelay
//Formatting guide: Located at the bottom of main.cpp

//TODO:
//nat traversal
//encryption
//if bind fails, goes into cleanup(), freeaddrinfo(); causes an unkown error. (linux only?)
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
#include <vector>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <cerrno>

#include <arpa/inet.h>
#include <signal.h>

#include <pthread.h>


#endif//__linux__

#ifdef _WIN32
#include <iostream>
#include <string>
#include "ipaddress.h"
#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include <process.h>//<pthread.h>
#include <vector>
#endif//_WIN32
/*
#ifdef _WIN32
HANDLE ghEvents[3];
#endif//_WIN32

#ifdef __linux__
pthread_t thread1, thread2, thread3;
int ret1, ret2, ret3;
#endif//__linux__
*/
const int MAX_DIFF_INPUTS = 12; //max possible count of argc to bother reading from
bool global_verbose = false;

//Multiple: make a continuous loop that checks for connection requests using the listen function. if a connection request occurs, call accept and pass the work to another thread to handle it.

int main(int argc, char *argv[])
{
	std::string target_ip_address = "";
	std::string target_port = "";
	std::string my_ip_address = connection::DEFAULT_IP_TO_LISTEN;
	std::string my_port = connection::DEFAULT_PORT_TO_LISTEN;
	int err_chk;
	int arg_size;
	std::vector<std::string> arg;
	arg.reserve(MAX_DIFF_INPUTS);

	CommandLineInput CommandLineInputObj;
	ipaddress ipaddress_get;
	
	//----------------------- Check to see what the user wants to do via program input -----------------------


	//put all argv's into a vector so they can be compared to strings
	for (int i = 0; i < argc; i++) {
		arg.push_back(argv[i]);
	}
	arg_size = arg.size();

	//if command line arguments supplied, show the ReadMe
	if (argc <= 1) {
		CommandLineInputObj.helpAndReadMe(argc);
		return 1;
	}
	//check all argv inputs to see what the user wants to do
	if (argc >= 2) {
		for (int i = 1; i < argc; ++i) {

			if (arg[i] == "-h" 
					|| arg[i] == "-H" 
					|| arg[i] == "-help" 
					|| arg[i] == "-Help" 
					|| arg[i] == "help" 
					|| arg[i] == "readme") {
				CommandLineInputObj.helpAndReadMe(argc);
				return 1;
			}
			if (arg[i] == "-t" && i < arg_size -1) {
				err_chk = ipaddress_get.get_target(argv[i + 1]);
				if (err_chk == false)
					return 1;
				else
					target_ip_address = argv[i + 1];
			}
			if (arg[i] == "-tp") {
				//err_chk = ipaddress_get.port(argv[i + 1]);
				//if (err_chk == false)
				//	return 1;
				//else
					target_port = argv[i + 1];
			}
			if (arg[i] == "-m") {
				err_chk = ipaddress_get.get_target(argv[i + 1]);
				if (err_chk == false)
					return 1;
				else
					my_ip_address = argv[i + 1];
			}
			if (arg[i] == "-mp") {
				//err_chk = ipaddress_get.port(argv[i + 1]);
				//if (err_chk == false)
				//	return 1;
				//else
				target_port = argv[i + 1];
			}
			if (arg[i] == "-v") 
				global_verbose = true;
			if (arg[i] == "-f") {
				std::cout << "-f hasn't been implemented yet.\n";
				return 1;
			}
		}
	}


	//===================================== Starting Chat Program =====================================

	std::cout << "Welcome to the chat program. Version: 0.1.1\n";//somewhat arbitrary version number usage at the moment :)

	//Server startup sequence
	connection serverObj;
	if(global_verbose == true)
		std::cout << "SERVER::";
	serverObj.serverSetIpAndPort(my_ip_address, my_port);
	if (serverObj.initializeWinsock() == false) return 1;
	serverObj.ServerSetHints();

	//Client startup sequence
	connection clientObj;
	if (global_verbose == true)
		std::cout << "CLIENT::";
	clientObj.clientSetIpAndPort(target_ip_address, target_port);
	if (clientObj.initializeWinsock() == false) return 1;
	clientObj.ClientSetHints();

	//BEGIN THREAD RACE
	serverObj.createServerRaceThread(&serverObj);			//<-----------------
	clientObj.createClientRaceThread(&clientObj);			//<----------------

#ifdef __linux__
	//Wait for threads to finish
	pthread_join(connection::thread1, NULL);						//currently, on the linux side, the program is practically stuck on connect. idk y.
	//pthread_join(connection::thread2, NULL);						//TEMPORARILY IGNORED, not that i want to wait for both anyways, just any 1 thread.
#endif//__linux__

#ifdef _WIN32
	//Wait for any 1 thread to finish
	WaitForMultipleObjects(
		2,			//number of objects in array
		connection::ghEvents,	//array of objects
		FALSE,		//wait for all objects if it is set to TRUE, otherwise FALSE means it waits for any one object to finish. return value indicates the finisher.
		INFINITE);	//its going to wait this long, OR until all threads are finished, in order to continue.

#endif//_WIN32

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
	while (who_won == connection::CLIENT_WON || who_won == connection::SERVER_WON){
		//if server won, then act as a server.
		if (who_won == connection::SERVER_WON){ 
			std::cout << "Connection established as the server.\n";
			serverObj.closeTheListeningSocket();	// No longer need listening socket since I only want to connect to 1 person at a time.
			//clientObj.cleanup();    here????????
			serverObj.serverCreateSendThread(&serverObj);		//<--------------------
			if (serverObj.receiveUntilShutdown() == false) return 1;

			//shutdown
			if (serverObj.shutdownConnection() == false) return 1;
			serverObj.cleanup();
		}
		//If client won, then act as a client
		else if (who_won == connection::CLIENT_WON){
			std::cout << "Connection established as the client.\n";
			clientObj.clientCreateSendThread(&clientObj);			//<---------------------
			//serverObj.cleanup();    here??????????
			if (clientObj.receiveUntilShutdown() == false) return 1;

			//shutdown
			if (clientObj.shutdownConnection() == false) return 1;
			clientObj.cleanup();
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



/*

+++++++++++++++++++++++++++++++++++++ Formatting Guide +++++++++++++++++++++++++++++++++++++

guide here!




+++++++++++++++++++++++++++++++++++ End Formatting Guide +++++++++++++++++++++++++++++++++++

*/