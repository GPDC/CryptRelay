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


	WaitForMultipleObjects(
		2,			//number of objects in array
		ghEvents,	//array of objects
		TRUE,		//wait for all objects
		INFINITE);		//its going to wait this long, OR until all threads are finished, in order to continue.


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
	//START CHAT PROGRAM
	connectionObj.setIpAndPort(targetIPaddress, userPort);
	connectionObj.initializeWinsock();
	connectionObj.setHints();
	connectionObj.getAddressPort();//puts the local port info in the addrinfo structure
	connectionObj.createSocket();//creates listening socket to listen on that local port
	connectionObj.bindToListeningSocket();
	connectionObj.connectToTarget();

	//start client snd thread
	//ghEvents[0] = (HANDLE)_beginthread(clientSendThread, 0, NULL);	//c style typecast    from: uintptr_t    to: HANDLE.

	connectionObj.listenToListeningSocket();	//thread waits here until someone connects or it errors.
	connectionObj.acceptClient();				//creates a new socket to communicate with the person on
	connectionObj.closeTheListeningSocket();	// No longer need listening socket
	connectionObj.receiveUntilShutdown();		//receive until either person exits.
	connectionObj.shutdownConnection();		//shutting down everything
	connectionObj.cleanup();



	//start threads
	ghEvents[0] = (HANDLE)_beginthread(server_thread, 0, NULL);	//c style typecast    from: uintptr_t    to: HANDLE.      threads must be passed a static function if it is in a class.
	//ghEvents[0] = (HANDLE)_beginthread(client_thread, 0, NULL);	//c style typecast    from: uintptr_t    to: HANDLE.

	*/

	/*
	//wait until all threads are finished
	WaitForMultipleObjects(
		1,			//number of objects in array
		ghEvents,	//array of objects
		TRUE,		//wait for all objects
		INFINITE);		//its going to wait this long, OR until all threads are finished, in order to continue.
		*/
}







//create socket
//bind socket to address
//listen for clients
//accept client
//send and receive
//shutdown, close socket for client
//cleanup WSA




//server start
//wsa
//create socket
//bind socket
//THREAD LISTEN
	//if accept = true
	//shutdown connect attempts
	//stop listening
//try to connect to given address
//if connect failed, try again until user inputs a command
//if succeeded, shutdown listening thread

//2 competing threads, whichever one wins, pass socket to main thread, close both threads
//now start the real program with the socket and
//createa  a send thread
//receive normally.


//run as a server
//START CHAT PROGRAM
