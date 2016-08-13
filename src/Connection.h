// Connection.h  // pls rename to connection

// Overview:
// This is where the program will make a connection with the peer.
// It uses SocketClass to store SOCKET information.

// Warnings:
// This source file does not do any input validation.

// Terminology:
// Socket is an end-point that is defined by an IP-address and port.
//   A socket is just an integer with a number assigned to it.
//   That number can be thought of as a unique ID for the connection
//   that may or may not be established.
// fd is linux's term for a socket. == File Descriptor
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
class SocketClass;

class Connection
{
public:
	Connection(SocketClass* SocketClassInstance, bool turn_verbose_output_on = false);
	virtual ~Connection();

	// Turn on and off verbose output for this class.
	bool verbose_output = false;

	// Server and Client threads
	// Only 1 of these methods should be running per Connection class instance.
	// Basically, 1 SOCKET per 1 method. Two Methods should not be using the same SOCKET.
	void serverThread();
	void clientThread();

	// If you want to give this class IP and port information, call this function.
	void setIPandPort(std::string target_extrnl_ip_address, std::string my_ext_ip, std::string my_internal_ip, std::string target_port = DEFAULT_PORT, std::string my_internal_port = DEFAULT_PORT);

	// Variables necessary for determining who won the connection race
	static int32_t global_winner;
	static const int32_t SERVER_WON;
	static const int32_t CLIENT_WON;
	static const int32_t NOBODY_WON;

	// IP and port information can be given to the Connection class through these variables.
	std::string target_external_ip;						// If the option to use LAN only == true, this is target's local ip
	std::string target_external_port = DEFAULT_PORT;	// If the option to use LAN only == true, this is target's local port
	std::string my_external_ip;
	std::string my_local_ip;
	std::string my_local_port = DEFAULT_PORT;

protected:
private:

	// Prevent anyone from copying this class.
	Connection(Connection& ConnectionInstance) = delete;			 // disable copy operator
	Connection& operator=(Connection& ConnectionInstance) = delete;  // disable assignment operator


#ifdef _WIN32
	typedef struct _stat64 xplatform_struct_stat;
#endif//WIN32
#ifdef __linux__
	typedef struct stat xplatform_struct_stat;
#endif//__linux__

	SocketClass * Socket;

	static const std::string DEFAULT_PORT;

	// Server and Client thread must use this function to prevent
	// a race condition.
	static int32_t setWinnerMutex(int32_t the_winner);

	// mutex for use in this class' send()
	static std::mutex SendMutex;
	// mutex for use with server and client threads to prevent a race condition.
	static std::mutex RaceMutex;
};

#endif//Connection_h__
