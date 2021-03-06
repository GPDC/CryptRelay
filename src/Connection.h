// Connection.h

// Overview:
// This is where the program will make a connection with the peer.

// Terminology:
// Mutex - mutual exclusions. It is designed so that only 1 thread executes code
// at any given time while inside the mutex.

#ifndef Connection_h__
#define Connection_h__

#ifdef __linux__
#include <string>
#include <mutex> // Want to use exceptions and avoid having it never reach unlock? Use std::lock_guard
#endif//__linux__
#ifdef _WIN32
#include <string>
#include <mutex>
#endif//_WIN32



// Forward declaration
class IXBerkeleySockets;

class Connection
{
	// Typedef section

private:
#ifdef __linux__
	typedef int32_t SOCKET;
#endif//__linux__


public:
	Connection(
		IXBerkeleySockets* IXBerkeleySocketsInstance, // Simply a cross platform implementation of certain Berkeley BerkeleySockets functions.
		bool turn_verbose_output_on = false
	);
	virtual ~Connection();

	// Turn on and off verbose output for this class.
	bool verbose_output = false;

	// Server and Client. They will attempt to make a connection with the peer.
	// Only 1 of these methods should be running per Connection class instance.
	// 1 SOCKET per 1 method. Two Methods should not be using the same SOCKET.
	// If you want to start a server thread, make a Connection class instance
	// and then create the thread with server()
	// If you then want to start a client thread, make yet another Connection
	// class instance, and then create the thread with client()
	// The connection_race_winner variable is integral to these member functions.
	void server();
	void client();

	// Variables necessary for determining who won the connection race
	// server() and client() can be both be running at the same time,
	// (as long as they are being run in difference Connection class instances!)
	// and whoever connects to the peer first will set the connection_race_winner variable
	// here, and the loser will return from the function, closing whatever
	// socket it was using.
	static int32_t connection_race_winner;
	static const int32_t SERVER_WON;
	static const int32_t CLIENT_WON;
	static const int32_t NOBODY_WON;


protected:
private:

	// Prevent anyone from copying this class.
	Connection(Connection& ConnectionInstance) = delete;			 // disable copy operator
	Connection& operator=(Connection& ConnectionInstance) = delete;  // disable assignment operator

	// If you want to exit the program, set this to true.
	bool exit_now = false;

	// This is the default port for the Connection class.
	static const std::string DEFAULT_PORT;

	// IP and port information for theConnection class.
	std::string target_external_ip;
	std::string target_external_port = DEFAULT_PORT;
	std::string my_local_ip;
	std::string my_local_port = DEFAULT_PORT;

	IXBerkeleySockets * BerkeleySockets;

	// If a connection has successfully been established, this socket will
	// be the one that it is established on. If it isn't, it will be INVALID_SOCKET.
	SOCKET fd_socket;
	

	// Server and Client thread must use this method, if they are ever
	// running at the same time, to prevent a race condition.
	static int32_t setWinnerMutex(int32_t the_winner);

	// mutex for use with server and client threads setting the
	// connection_race_winner variable to prevent a race condition.
	static std::mutex RaceMutex;

	// For enabling SOCKET usage for windows.
	int32_t WSAStartup();

#ifdef _WIN32
	WSADATA wsaData;	// for WSAStartup();
#endif//_WIN32


private:
	// Callbacks


public:

	// Accessors
	SOCKET getFdSocket() { return fd_socket; }

	void setTargetExternalIP(std::string ip) { target_external_ip = ip; }
	void setTargetExternalPort(std::string port) { target_external_port = port; }
	void setMyLocalIP(std::string ip) { my_local_ip = ip; }
	void setMyLocalPort(std::string port) { my_local_port = port; }
	std::string& getMyLocalPort() { return my_local_port; }

	// If you want this class to proceed to exit, set this to true.
	void setExitNow(bool exit_the_program) { exit_now = exit_the_program; }
};

#endif//Connection_h__
