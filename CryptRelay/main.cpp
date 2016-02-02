//ipv4 format checking is complete.
//ipv6 not currently implemented.

//maybe as a chat escape method, put in chat:    cryptrelay.sendfile filenamehere      ...and...       cryptrelay.exit
//but would that be excessive chat parsing? what about hitting escape first or another key, then typing a command. then it wouldn't have to constantly check literally everything you type at all times.

//todo: change return true and return false into a number instead for the int functions. should only use true and false in bool functions.
#include <iostream>
#include <string>
#include "ipaddress.h"
#include "connection.h"
#include <Windows.h>
#include <process.h>

DWORD dwevent;
HANDLE ghEvents[2];
std::string targetIPstring = "";
std::string userPort = "4242";
#define SERVER_WON -7
#define CLIENT_WON -8

/*
void sendThread(void* number)
{
	while (1)
	{
		connection connection_server;
		connection_server.clientSend();
	}

	//_endthread();
}
*/
/*

void client_thread(void* number)
{
	while (1)
	{
		connection connection_client;
		connection_client.establish_connection_client(targetIPstring, default_port);
	}

	//_endthread();
}

*/


//make a continuous loop that checks for connection requests using the listen function. if a connection request occurs, call accept and pass the work to another thread to handle it.

int main(int argc, char *argv[])
{
	if (argc <= 1)
	{
		std::cout << "Proper format is:   cryptrelay.exe 192.168.27.50 7172\n";
		std::cout << "Proper format is:   filename   IP address  PORT\n";
		return 1;
	}
	//connection connectionObj;
	std::string targetIPaddress = "";
	ipaddress ipaddress_get;

	//connection *PconnectionObj = &connectionObj;

	std::cout << "Welcome to the chat program.\n";

	//check userinput to see if it is correct
	//if (sizeof(argv[1]) < sizeof(std::string))	//this work?
	targetIPaddress = ipaddress_get.get_target(argv[1]);
	if (targetIPaddress == "bad IP address format.")
	{
		return 1;
	}
	//if (sizeof(argv[2]) < sizeof(std::string))	//this work?
		//if (argv[2] > "0" && argv[2] < "65535")	//need to convert to decimal first **********************

	//if argv[2] isn't NULL then assign check it, then assign it. ****needs to be done*******
	userPort = (argv[2]);

	//=================================== starting program ===========================================

	connection serverObj;
	std::cout << "SERVER::";
	serverObj.setIpAndPort(targetIPaddress, userPort);
	if (serverObj.initializeWinsock() == false) return 1;
	serverObj.ServerSetHints();


	connection clientObj;
	std::cout << "CLIENT::";
	clientObj.setIpAndPort(targetIPaddress, userPort);
	if (clientObj.initializeWinsock() == false) return 1;
	clientObj.ClientSetHints();


	//BEGIN SERVER COMPETITION THREADS
	ghEvents[0] = (HANDLE)_beginthread(connection::serverCompetitionThread, 0, &serverObj);	//c style typecast    from: uintptr_t    to: HANDLE.
	ghEvents[1] = (HANDLE)_beginthread(connection::clientCompetitionThread, 0, &clientObj);	//c style typecast    from: uintptr_t    to: HANDLE.

	//if one of them succeeds, let the program continue while waiting for the other one to finish.
	//this is mostly important for when the server wins, but the client is still hanging waiting for a response from the target.
	//but a problem arises... if client succeeds also, after the server wins, then it needs to check the winner status FIRST
	// and then destroy the socket it created.

	//and if the client wins yet the server succeeded , it needs to close the server socket and only do client stuff.

	WaitForMultipleObjects(
		2,			//number of objects in array
		ghEvents,	//array of objects
		TRUE,		//wait for all objects
		INFINITE);	//its going to wait this long, OR until all threads are finished, in order to continue.


	if (serverObj.globalWinner == -8)
		std::cout << "MAIN && Client thread is the winner!\n";
	else if (serverObj.globalWinner == -7)
		std::cout << "MAIN && Server thread is the winner!\n";
	else
		std::cout << "MAIN && There is no winner.\n";


	int who_won = serverObj.globalWinner;
	while (who_won == CLIENT_WON || who_won == SERVER_WON)
	{
		if (who_won == -30) return 1;
		if (who_won == SERVER_WON) //if server won, then act as a server.
		{
			//if ((iFeedback = serverObj.acceptClient()) == false) return 1;			//creates a new socket to communicate with the person on
			serverObj.closeTheListeningSocket();	// No longer need listening socket since I only want to connect to 1 person at a time.
			ghEvents[0] = (HANDLE)_beginthread(connection::clientSendThread, 0, &serverObj);	//c style typecast    from: uintptr_t    to: HANDLE.
			if (serverObj.clientReceiveUntilShutdown() == false) return 1;	//snd and recv
																				//connectionObj.regularClientSend();
																				//connectionObj.receiveUntilShutdown();		//receive until either person exits.
			if (serverObj.shutdownConnection() == false) return 1;			//shutting down everything
			serverObj.cleanup();
		}
		if (who_won == CLIENT_WON)
		{
			ghEvents[0] = (HANDLE)_beginthread(connection::clientSendThread, 0, &clientObj);	//c style typecast    from: uintptr_t    to: HANDLE.
			if (clientObj.clientReceiveUntilShutdown() == false) return 1;


			if (clientObj.shutdownConnection() == false) return 1;
			clientObj.cleanup();
			//client has already accepted, begin send and receive?
			//clientObj.receive();
		}
	}
	std::cout << "Waka waka waka\n";

	/*

	//wait until all threads are finished
	WaitForMultipleObjects(
		1,			//number of objects in array
		ghEvents,	//array of objects
		TRUE,		//wait for all objects
		INFINITE);		//its going to wait this long, OR until all threads are finished, in order to continue.

	*/
}
