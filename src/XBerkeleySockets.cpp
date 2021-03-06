// XBerkeleySockets.cpp

#ifdef __linux__
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
#include <sys/fcntl.h>

#include "XBerkeleySockets.h"
#include "GlobalTypeHeader.h"
#endif//__linux__

#ifdef _WIN32
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "XBerkeleySockets.h"
#include "GlobalTypeHeader.h"
#endif//_WIN32

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")		//tell the linker that Ws2_32.lib file is needed.
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")
#endif//_WIN32


#ifdef __linux__
#define INVALID_SOCKET	(-1)	// To indicate INVALID_SOCKET, Windows returns (~0) from socket functions, and linux returns -1.
#define SOCKET_ERROR	(-1)	// Linux doesn't have a SOCKET_ERROR macro.
#define SD_RECEIVE      SHUT_RD//0x00			// This is for shutdown(); SD_RECEIVE is the code to shutdown receive operations.
#define SD_SEND         SHUT_WR//0x01			// ^
#define SD_BOTH         SHUT_RDWR//0x02			// ^
#endif//__linux__


const bool XBerkeleySockets::DISABLE_CONSOLE_OUTPUT = false;
const unsigned long XBerkeleySockets::DISABLE_BLOCKING = 1;
const unsigned long XBerkeleySockets::ENABLE_BLOCKING = 0;


XBerkeleySockets::XBerkeleySockets()
{

}
XBerkeleySockets::~XBerkeleySockets()
{

}


int32_t XBerkeleySockets::shutdown(SOCKET socket, int32_t operation)
{
	std::cout << "Shutting down the connection... ";
	// shutdown the connection since we're done
	int32_t errchk = ::shutdown(socket, operation);
	if (errchk == SOCKET_ERROR)
	{
		std::cout << "Failed.\n";
		return -1;
	}
	std::cout << "Success\n";
	return 0;
}

void XBerkeleySockets::closesocket(SOCKET socket)
{
#ifdef __linux__
	::close(socket);
#endif//__linux__

#ifdef _WIN32
	::closesocket(socket);
#endif
}


void XBerkeleySockets::freeaddrinfo(addrinfo** ppAddrInfo)
{
	::freeaddrinfo(*ppAddrInfo);

	*ppAddrInfo = nullptr;	// Set the structure address to nullptr
							// That will just tell us it is no longer in use
							// and it is not available to be freed (because it already has been).
}


int32_t XBerkeleySockets::getError(bool output_to_console)
{
	// If you are getting an error but WSAGetLastError() is saying there is no error, then make sure
	// that WSAStartup() has been performed. WSAGetLastError() doesn't work unless you start WSAStartup();

#ifdef __linux__
	int32_t errsv = errno;	// Quickly saving the error incase it is quickly lost.

	if (output_to_console == true)
	{
		outputSocketErrorToConsole(errsv);
	}

	return errsv;
#endif//__linux__

#ifdef _WIN32
	int32_t errsv = ::WSAGetLastError();

	if (output_to_console == true)
	{
		outputSocketErrorToConsole(errsv);
	}
	
	return errsv;
#endif//_WIN32
}


void XBerkeleySockets::outputSocketErrorToConsole(int32_t error_code)
{
#ifdef __linux__
	// Buffer for strerror_r() to put text into.
	const int32_t STR_BUF_SIZE = 100;
	char str_buf[STR_BUF_SIZE] = { 0 };

	char * str_buf_for_output = nullptr;
	// If strerror_s didn't error, print it out
	str_buf_for_output = strerror_r(error_code, str_buf, STR_BUF_SIZE);
	if (str_buf_for_output != nullptr)
	{
		std::cout << "Errno: " << error_code << ", " << str_buf_for_output << "\n";
	}
	else  // Just print the error code
	{
		std::cout << "Errno: " << error_code << ".\n";
	}
#endif//__linux__
#ifdef _WIN32
	int32_t tchar_count = 0;

	// FORMAT_MESSAGE_ALLOCATE_BUFFER will allocate a buffer on local heap for error text, therefore
	// it needs to be freed with LocalFree();
	// FORMAT_MESSAGE_FROM_SYSTEM tells it to search the system message-table resources for the requested error txt
	// FORMAT_MESSAGE_IGNORE_INSERTS is 100% necessary. It makes it so insert sequences in the message definition
	// are ignored and passed through to the output buffer unchanged.
	// returns 0 on fail.
	// returns the number of TCHARs stored in the output buffer, excluding the terminating null char.
	LPTSTR message_for_console = NULL;
	tchar_count = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,   // Location of message definitions. NULL b/c we told it to FORMAT_MESSAGE_FROM_SYSTEM
		(DWORD)error_code, // Source to look for the error code.
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Use the default language for the output message.
		(LPTSTR)&message_for_console,   // Places the message here so you can cout it.
		0, // number of TCHARs to allocate for an output buffer (b/c we set FORMAT_MESSAGE_ALLOCATE_BUFFER)
		NULL // optional args
	);
	if (tchar_count != 0) // success
	{
		std::cout << "WSAError: " << error_code << ". ";
		fprintf(stderr, "%S\n", message_for_console);
		LocalFree(message_for_console);
		message_for_console = NULL;
	}
	else
	{
		// Still print out the error code even though there is no description of it.
		std::cout << "WSAError: " << error_code << "\n";

		// Now display error information for FormatMessage() failing.
		perror("FormatMessage() failed");
		DBG_DISPLAY_ERROR_LOCATION();
	}
#endif//_WIN32

	return;
}


void XBerkeleySockets::coutPeerIPAndPort(SOCKET connection_with_peer)
{
	sockaddr PeerIPAndPortStorage;
#ifdef _WIN32
	int32_t peer_ip_and_port_storage_len = sizeof(sockaddr);
#endif//_WIN32
#ifdef __linux__
	socklen_t peer_ip_and_port_storage_len = sizeof(sockaddr);
#endif//__linux__
	memset(&PeerIPAndPortStorage, 0, peer_ip_and_port_storage_len);

	// getting the peer's ip and port info and placing it into the PeerIPAndPortStorage sockaddr structure
	int32_t errchk = getpeername(connection_with_peer, &PeerIPAndPortStorage, &peer_ip_and_port_storage_len);
	if (errchk == -1)
	{
		getError();
		std::cout << "getpeername() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		// continuing, b/c this isn't a big problem.
	}


	// If we are here, we must be connected to someone.
	char * remote_host = new char[NI_MAXHOST];
	char * remote_hosts_port = new char[NI_MAXSERV];

	// Let us see the IP:Port we are connecting to. the flag NI_NUMERICSERV
	// will make it return the port instead of the service name.
	errchk = getnameinfo(&PeerIPAndPortStorage, sizeof(sockaddr), remote_host, NI_MAXHOST, remote_hosts_port, NI_MAXSERV, NI_NUMERICHOST);
	if (errchk != 0)
	{
		getError();
		std::cout << "getnameinfo() failed.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		// still going to continue the program, this isn't a big deal
	}
	else
		std::cout << "\n\n\n\n\n\n# Connection established with: " << remote_host << ":" << remote_hosts_port << "\n";

	delete[]remote_host;
	delete[]remote_hosts_port;
}


int32_t XBerkeleySockets::setBlockingSocketOpt(SOCKET socket, const u_long* option)
{

#ifdef _WIN32
	u_long mode = *option;
	int32_t errchk = ioctlsocket(socket, FIONBIO, &mode);
	if (errchk == NO_ERROR)
	{
		return 0;
	}
	else // error
	{
		std::cout << "ioctlsocket() failed.\n";
		getError();
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}
#endif//_WIN32
#ifdef __linux__

	if (*option == DISABLE_BLOCKING)
	{
		int32_t current_flag = fcntl(socket, F_GETFL);
		if (current_flag == -1)
		{
			std::cout << "fcntl() failed getting flag.\n";
			getError();
			DBG_DISPLAY_ERROR_LOCATION();
		}
		if (current_flag == O_NONBLOCK)
		{
			DBG_TXT("Warning: Tried to set non-blocking flag on a socket that is already non-blocking.");
			return 0;
		}
		else
		{
			int32_t errchk = fcntl(socket, F_SETFL, O_NONBLOCK);
			if (errchk < 0)
			{
				std::cout << "fcntl() failed setting non_block flag.\n";
				getError();
				DBG_DISPLAY_ERROR_LOCATION();
				return -1;
			}
			else
				return 0;
		}

	}
	else // *option == ENABLE_BLOCKING
	{
		// Clear the flag, thereby making the mode == ENABLE_BLOCKING
		int32_t enable_blocking = O_NONBLOCK;
		enable_blocking &= ~O_NONBLOCK;

		int32_t current_flag = fcntl(socket, F_GETFL);
		if (current_flag == -1)
		{
			std::cout << "fcntl() failed getting flag.\n";
			getError();
			DBG_DISPLAY_ERROR_LOCATION();
		}
		if (current_flag == O_NONBLOCK)
		{
			DBG_TXT("Warning: Tried to set enable blocking flag on a socket that is already blocking.");
			return 0;
		}
		else
		{
			int32_t errchk = fcntl(socket, F_SETFL, enable_blocking);
			if (errchk < 0)
			{
				std::cout << "fcntl() failed to set the enable blocking flag.\n";
				getError();
				DBG_DISPLAY_ERROR_LOCATION();
				return -1;
			}
			else
				return 0;
		}
	}
#endif//__linux__

	return 0;
}


int32_t XBerkeleySockets::getSockOptError(SOCKET fd_socket)
{
#ifdef _WIN32
	char errorz = 0;
	int32_t len = sizeof(errorz);
#endif//_WIN32
#ifdef __linux__
	int32_t errorz = 0;
	uint32_t len = sizeof(errorz);
#endif//__linux__

	int32_t sock_opt_errorchk = getsockopt(fd_socket, SOL_SOCKET, SO_ERROR, &errorz, &len);
	if (sock_opt_errorchk == SOCKET_ERROR)
	{
		return SOCKET_ERROR;
	}
	
	return errorz;
}
