
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
//#include <libupnp>	// Window's <UPnP.h>

#include <arpa/inet.h>
#include <signal.h>

#include <pthread.h>	//<process.h>

#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "chat_program.h"

#include "UPnP.h
#endif//__linux__

#ifdef _WIN32
#include <iostream>
#include <string>
#include <vector>
#include <WS2tcpip.h>
#include <UPnP.h>		// linux's libupnp??

#include <process.h>	//<pthread.h>

#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "SocketClass.h"
#include "chat_program.h"

#include "UPnP.h"
#endif//_WIN32


bool global_verbose = false;

int main(int argc, char *argv[])
{
	int errchk = 0;

	CommandLineInput CLI;
	// Check what the user wants to do via command line input
	// Information inputted by the user on startup is stored in CLI
	if ( (errchk = CLI.getCommandLineInput(argc, argv) ) == FALSE)	
		return EXIT_FAILURE;


	//===================================== Starting Chat Program =====================================

	// Ideally I wouldn't create this Upnp instance unless I am sure I am going to use upnp...
	UPnP Upnp;
	ChatProgram ChatServer;
	ChatProgram ChatClient;

	if (CLI.use_lan_only == true)
	{
		// Giving the TCP class the user specified target ip and port
		// also giving the TCP class the IPs and port gathered by
		// the Upnp class.
		ChatServer.giveIPandPort(CLI.target_ip_address, CLI.my_ext_ip_address, CLI.my_ip_address, CLI.target_port, CLI.my_host_port);
		ChatClient.giveIPandPort(CLI.target_ip_address, CLI.my_ext_ip_address, CLI.my_ip_address, CLI.target_port, CLI.my_host_port);
	}
	else if (CLI.use_upnp == true)	// Checking to make sure user didn't turn off UPnP
	{
		// If UPnP is working, start the chat program using that knowledge.
		if (Upnp.startUPnP() == true)
		{
			// Giving the TCP class the user specified target ip and port
			// also giving the TCP class the IPs and port gathered by
			// the Upnp class.
			// THIS SHOULD BE: if CLI.ipaddr or port w/e has something in it, then give that to Chat instead of the Upnp variable.

			// should be:
			// if (empty(CLI.target_ip_address) == false)
			//     give it to ChatProgram
			// else
			//     take it from upnp

			// Give to the ChatServer instance
			if (empty(CLI.target_ip_address) == false)
				ChatServer.target_external_ip = CLI.target_ip_address;

			if (empty(CLI.target_port) == false)
				ChatServer.target_external_port = CLI.target_port;

			if (empty(CLI.my_ext_ip_address) == false)
				ChatServer.my_external_ip = CLI.my_ext_ip_address;
			else
				ChatServer.my_external_ip = Upnp.my_external_ip;

			if (empty(CLI.my_ip_address) == false)
				ChatServer.my_local_ip = CLI.my_ip_address;
			else
				ChatServer.my_local_ip = Upnp.my_local_ip;

			if (empty(CLI.my_host_port) == false)
				ChatServer.my_local_port = CLI.my_host_port;
			else
				ChatServer.my_local_port = Upnp.my_internal_port;


			// Give to the ChatClient instance
			if (empty(CLI.target_ip_address) == false)
				ChatClient.target_external_ip = CLI.target_ip_address;

			if (empty(CLI.target_port) == false)
				ChatClient.target_external_port = CLI.target_port;

			if (empty(CLI.my_ext_ip_address) == false)
				ChatClient.my_external_ip = CLI.my_ext_ip_address;
			else
				ChatClient.my_external_ip = Upnp.my_external_ip;

			if (empty(CLI.my_ip_address) == false)
				ChatClient.my_local_ip = CLI.my_ip_address;
			else
				ChatClient.my_local_ip = Upnp.my_local_ip;

			if (empty(CLI.my_host_port) == false)
				ChatClient.my_local_port = CLI.my_host_port;
			else
				ChatClient.my_local_port = Upnp.my_internal_port;


			/*ChatServer.giveIPandPort(CLI.target_ip_address, Upnp.my_external_ip, Upnp.my_local_ip, CLI.target_port, Upnp.my_internal_port);
			ChatClient.giveIPandPort(CLI.target_ip_address, Upnp.my_external_ip, Upnp.my_local_ip, CLI.target_port, Upnp.my_internal_port);*/
		}
		else
		{
			std::cout << "Fatal: Couldn't port forward via UPnP.\n";
			return EXIT_FAILURE;
		}

	}

	// Start the chat program.
	
	// BEGIN THREAD RACE
	ChatProgram::createStartServerThread(&ChatServer);			//<-----------------
	ChatProgram::createStartClientThread(&ChatClient);			//<----------------

	// Wait for 1 thread to finish
#ifdef __linux__
	pthread_join(ChatProgram::thread0, NULL);
	pthread_join(ChatProgram::thread1, NULL);
#endif//__linux__
#ifdef _WIN32
	WaitForMultipleObjects(
			2,			// Number of objects in array
			ChatProgram::ghEvents,	// Array of objects
			true,		// Wait for all objects if it is set to TRUE. FALSE == wait for any one object to finish. Return value indicates the returned thread(s?).
			INFINITE	// Its going to wait this long, OR until all threads are finished, in order to continue.
		);	
#endif//_WIN32




	
	
	std::cout << "PAUSE...";
	std::string pause;
	std::getline(std::cin, pause);
	
	/*
	std::cout << "Welcome to the chat program. Version: 0.5.0\n"; // Somewhat arbitrary version number usage at the moment :)

	// Client
	TCPConnection ClientLoopAttack;
	ClientLoopAttack.giveIPandPort(
		CLI.target_ip_address,
		CLI.target_port,
		CLI.my_ext_ip_address,
		CLI.my_ip_address,
		CLI.my_host_port
	);
	ClientLoopAttack.threadEntranceClientLoopAttack(&ClientLoopAttack);

	// Server
	TCPConnection ServerListen;
	ServerListen.giveIPandPort(
		CLI.target_ip_address,
		CLI.target_port,
		CLI.my_ext_ip_address,
		CLI.my_ip_address,
		CLI.my_host_port
		);
	//ServerListen.threadEntranceServer(&ServerListen);

	std::cout << "PAUSE...";
	pause = "";
	std::getline(std::cin, pause);

	*/


	// accept incoming connection
	// get ip address of the accepted connection
	// if it isn't the one you were expecting, close the connection and listen for connections again
	// else continue on as normal, starting the chat system or w/e is desired

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
	clientObj.clientSetIpAndPort(CLI.target_ip_address, CLI.target_port);
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
	int who_won = TCPConnection::global_winner;
	if (global_verbose == true)
	{
		if (who_won == TCPConnection::CLIENT_WON_old)
			std::cout << "MAIN && Client thread is the winner!\n";
		else if (who_won == TCPConnection::SERVER_WON_old)
			std::cout << "MAIN && Server thread is the winner!\n";
		else
			std::cout << "MAIN && There is no winner.\n";
	}
	
	if (who_won == TCPConnection::NOBODY_WON_old) 
	{
		std::cout << "Error: Unexpected race result. Exiting\n";
		return 1;
	}

	// Continue running the program for the thread that returned and won.
	while (who_won == TCPConnection::CLIENT_WON_old || who_won == TCPConnection::SERVER_WON_old){
		// If server won, then act as a server.
		if (who_won == TCPConnection::SERVER_WON_old)
		{ 
			std::cout << "Connection established as the server.\n";
			serverObj.closeThisSocket(serverObj.ListenSocket);	// No longer need listening socket since I only want to connect to 1 person at a time.
			serverObj.serverCreateSendThread(&serverObj);
			if (serverObj.receiveUntilShutdown() == false)
				return 1;

			//shutdown
			if (serverObj.shutdownConnection(serverObj.globalSocket) == false)
				return 1;
			break;
		}
		// If client won, then act as a client
		else if (who_won == TCPConnection::CLIENT_WON_old)
		{
			std::cout << "Connection established as the client.\n";
			clientObj.clientCreateSendThread(&clientObj);
			if (clientObj.receiveUntilShutdown() == false)
				return 1;

			// Shutdown
			if (clientObj.shutdownConnection(clientObj.globalSocket) == false)
				return 1;
			break;
		}
		// If nobody won, quit
		else if (who_won == TCPConnection::NOBODY_WON_old) 
		{
			std::cout << "ERROR: Unexpected doomsday scenario.\n";
			std::cout << "ERROR: Shutting down.\n";
			//clientObj.myWSACleanup();
			//serverObj.myWSACleanup();
			return 1;
		}
		else 
		{
			std::cout << "ERROR: Shutting down. The impossible is possible.\n";
			//clientObj.myWSACleanup();
			//serverObj.myWSACleanup();
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
//	3. in project settings, C / C++->Preprocessor->Preprocessor Definitions->STATICLIB
//	please note we are just adding STATICLIB to the list of whatever is currently there.Don't delete everything in there.
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