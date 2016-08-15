// Connection.h  // pls rename to connection

// Overview:
// This is where the program will make a connection with the peer.
// It uses XBerkeleySockets to store SOCKET information.

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
class XBerkeleySockets;

class Connection
{
public:
	// Typedefs for callbacks
	typedef void callback_fn_set_exit_now(bool value);
	typedef bool& callback_fn_get_exit_now();

	Connection(
		XBerkeleySockets* SocketClassInstance, // Simply a cross platform implementation of certain Berkeley Socket functions.
		callback_fn_get_exit_now * get_exit_now_ptr,
		callback_fn_set_exit_now * set_exit_now_ptr,
		bool turn_verbose_output_on = false
	);
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
	std::string target_external_ip;
	std::string target_external_port = DEFAULT_PORT;
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

	XBerkeleySockets * Socket;

	// If a connection has successfully been established, this socket will
	// be the one that it is established on. If it isn't, it will be INVALID_SOCKET.
	SOCKET fd_socket = INVALID_SOCKET;

	// This is the default port for the Connection class.
	static const std::string DEFAULT_PORT;

	// Server and Client thread must use this function to prevent
	// a race condition.
	static int32_t setWinnerMutex(int32_t the_winner);

	// mutex for use in this class' send()
	static std::mutex SendMutex;
	// mutex for use with server and client threads to prevent a race condition.
	static std::mutex RaceMutex;


private:
	callback_fn_set_exit_now * callbackSetExitNow = nullptr; // for setting the bool exit_now variable.
	callback_fn_get_exit_now * callbackGetExitNow = nullptr; // for viewing the bool exit_now variable.

public:

	// Accessors
	SOCKET getFdSocket() { return fd_socket; }
	void setCallbackGetExitNow(callback_fn_get_exit_now * ptr) { callbackGetExitNow = ptr; }
	void setCallbackSetExitNow(callback_fn_set_exit_now * ptr) { callbackSetExitNow = ptr; }
};

#endif//Connection_h__
