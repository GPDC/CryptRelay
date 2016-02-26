#define _WINSOCK_DEPRECATED_NO_WARNINGS
// RawConnection.cpp
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
#include <pthread.h>	//<process.h>
#include <iomanip>	// std::setw(2) && std::setfill('0')

#include <arpa/inet.h>
#include <signal.h>

#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "CrossPlatformSleep.h"
#include "RawConnection.h"
#endif//__linux__

#ifdef _WIN32
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <process.h>	//<pthread.h>
#include <iomanip>	// std::setw(2) && std::setfill('0')

#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "CrossPlatformSleep.h"
#include "RawConnection.h"
#endif//_WIN32


// Reminder: Carefully read every. single. word. inside documentation for berkley sockets (this applies to everything).
//		Otherwise you might skip the 2 most important words in the gigantic document.
bool Raw::stop_echo_request_loop = false;
#ifdef _WIN32
HANDLE Raw::ghEvents2[1];
#endif//_WIN32
Raw::Raw()
{
	// SOCKET
	created_socket = INVALID_SOCKET;

	// int
	iResult = 0;
	errchk = 0;

	// size_t
	current_sendbuf_len = 0;

	// Gotta make those structs don't have random values in them!
	//memset(&Hints, 0, sizeof(Hints));					// addrinfo
	//memset(&StorageHints, 0, sizeof(StorageHints));	// SOCKADDR_STORAGE
	memset(&IPV4Header, 0, sizeof(IPV4Header));			// iphdr, and the 2 structs inside iphd
	memset(&ICMPHeader, 0, sizeof(ICMPHeader));			// icmpheader_echorequest;
	memset(&TargetSockAddrIn, 0, sizeof(TargetSockAddrIn));					// SOCKADDR_IN
}
Raw::~Raw()
{
}

// Decided not to use this. In reality, not necessary for this program. Could use boost library, or look up every compiler's info myself.
bool Raw::isLittleEndian()	// Figuring out where to store this since i'm not using it, but its cool.
{
	// A union is only big enough to hold the largest data member that it contains.
	// Don't read from something unless it was just written; it probably won't give back the value you are thinking it will.
	union
	{
		std::uint32_t i;
		std::uint8_t b[4];	// Please treat as a 8 bit int. (if you wish to cout this number, cast it to (int) )
	}iEndians;
	iEndians.i = 0x01020304;// If the machine is little endian, then iEndians.i will show up as:
							//		0x//  01 02 03 04
							//		dec// 16,909,060
							//		0b//  0000 0001 0000 0010 0000 0011 0000 0100
							// So what we're checking for is, as a decimal number, b[0] == 4. If true, then it's == little endian.

							// If the machine is big endian, then iEndians.i will show up as:
							//		0x//  04 03 02 01
							//		dec// 1,086,341,248
							//		0b//  0010 0000 1100 0000 0100 0000 1000 0000.
							// So what we're checking for is, as a decimal number, b[0] == 1. If true, then it's == big endian

	if (iEndians.b[0] == 4)			// Little endian
	{
		if (global_verbose == true)
		{
			std::cout << "Machine is detected as little endian. " << "b[0] == " << (int)iEndians.b[0] << "\n";
		}
		return true;
	}
	else if (iEndians.b[0] == 1)	// Must be big endian, and b[0] == 1
	{
		if (global_verbose == true)
		{
			std::cout << "Machine is detected as big endian. " << "b[0] == " << (int)iEndians.b[0] << "\n";
		}
		return false;
	}
	else							// Shouldn't ever happen.
	{
		std::cout << "Major catastrophical problem determining byte order. Exiting.\n";
		exit(0);
	}


}

// ihl min is 5, max is 15;
//std::uint8_t Raw::setIHLAndVer(u_int ihl, u_int ver)
//{
//	return (ver << 4) | ihl;
//}		
//std::uint8_t Raw::setDSCPAndECN(u_int dscp, u_int ecn)
//{
//	return (dscp << 2) | ecn;	//dscp takes up 6 bits, ecn 2.
//
//
//	// For example if dscp = 4 and ecn = 1
//	//00010000	dscp |		//after having been shifted already
//	//00000001 ecn  =
//	//---------
//	//00010001
//}

// Unsure if this is working correctly or if wireshark is displaying it incorrectly?
std::uint16_t Raw::setFlagsAndFragOffset(uint16_t flags, uint16_t frag_offset)
{
	return  (flags << 5) | htons(frag_offset);

	// This means max frag_offset is 65,528, and theoretical flags max is 7 (but not really, see next line below)
	// bit 0 is reserved. bit 1 == don't fragment. bit 2 == more fragments. This concludes the flags available to be set.

	//573 = 0000'0010   0011'1101
	//1   = 0000'0000   0010'0000


	//htons573 = 0011'1101   0000'0010
	//1   = 0000'0000   0010'0000			//has already been shifted
	//			----------------------
	//			 0011'1101   0010'0010
	//htons	   = 0010'0010   0011'1101



	// 0000'1111   1110'0001


	//		 0000'1111   0011'1100   1111'0000
	//htons  1111'0000   0011'1100   0000'1111


	//bitmask
	// htons(frag) & 0b10000000'00000000
	//.....................................................................


	// Explanation:
	// Let the compiler figure out if the machine we are in is big endian or little endian by using bitwise shift operators.
	// << shifts in the direction of the most significant bit, and >> shifts in the dir. of the least significant bit.
	// This is great because the compiler knows the direction of the most significant bit. It could be big endian or
	//		little endian. We don't know which it is, but the compiler does, therefore we let the compiler handle the direction to shift.	

	// This is what it looks like:	
	// ver == 0b0000'0100  |
	// ihl == 0b1111'0000  =
	// ------------------
	// spb == 0b1111'0100

	// I COULD instead do either:
	// A. Look through all the compiler specific stuff and things for all the compilers ever... OR
	// B. Create 2 structures. One is for little endian. The other is for big endian.
	//		Create function for determining endianness.
}

uint16_t Raw::roL(uint16_t unsigned_number_to_shift, uint16_t unsigned_shift_count)
{

	uint16_t most_significant_bitmask = 1 << ((sizeof(unsigned_number_to_shift) * 8 - 1));		// multiply the size of x by 8.. because sizeof returns the num of bytes.
																								// multiply by 8 to get the bit count. Then -1 so we don't shift it off into space.
	uint16_t least_significant_bitmask = 1;	// the least_significant_bitmask			

	for (; unsigned_shift_count > 0; unsigned_shift_count--)
	{
		if (unsigned_number_to_shift & most_significant_bitmask)
		{																	// Checking to see if the most significant bit is set ... bool checks for nonzero. if it is nonzero, true, else if it is 0, it is false.		
			unsigned_number_to_shift <<= 1;									// If it is set, then shift left 1, and set the least significant bit
			unsigned_number_to_shift |= least_significant_bitmask;			// to 1 (b/c they come in as a 0) by bitwise ORing the number to
		}
		else
		{
			unsigned_number_to_shift <<= 1;
		}
	}
	return unsigned_number_to_shift;


	//in regards to rotating right on signed int's:
	// it can retain its signededness  after shifting to the right (aka most significant bit is still set to 1)
	// this could be unexpected since a 0 should be there. so this needs to be accounted for.
}


//uint16_t Raw::rolVersionTwo(uint16_t number, uint16_t shift_count)
//{
//	if ((shift_count &= 31) == 0)
//		return number;
//	return (number << shift_count) | (number >> (32 - shift_count));
//	// This is Bertrand Marron's code from stack overflow. It is much more concise and maybe even faster?
//}


bool Raw::initializeWinsock()
{
#ifdef _WIN32
	if (global_verbose == true)
	{
		std::cout << "Initializing Winsock...\n";
	}
	errchk = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (errchk != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return false;
	}
#endif//_WIN32
	return true;
}


void Raw::setAddress(std::string target_ip, std::string target_port, std::string my_ip, std::string my_host_port)
{
	if (global_verbose == true)
	{
		std::cout << "Setting info: IP address and port...\n";
	}

	// Retrieving all the info that was gathered by CommandLineInput and checked by FormatCheck.cpp
	TargetSockAddrIn.sin_family = AF_INET;
	TargetSockAddrIn.sin_addr.s_addr = inet_addr(target_ip.c_str());
	TargetSockAddrIn.sin_port = htons(atoi(target_port.c_str()));
	my_host_ip_addr = my_ip;
	my_host_port = my_host_port;

	return;
}

SOCKET Raw::createSocket(int family, int socktype, int protocol)	// Purely optional return value.
{
	if (global_verbose == true)
	{
		std::cout << "Creating RAW Socket...\n";
	}
	// Create a SOCKET handle for connecting to server(no ip address here, that is with bind)
	created_socket = socket(family, socktype, protocol);	// Must be admin / root / SUID 0  in order to open a RAW socket
	if (created_socket == INVALID_SOCKET)
	{
		getError();
		std::cout << "Socket failed.\n";
		closeThisSocket(created_socket);
		myWSACleanup();
		return false;
	}
	return created_socket;
}



// This echo request is going to a dead end IP address at 3.3.3.3
bool Raw::craftFixedICMPEchoRequestPacket()
{
	const char on = 1;
	int on_len = sizeof(int);

	// Tell kernel that we are doing our own IP structures
	errchk = setsockopt(created_socket, IPPROTO_IP, IP_HDRINCL, (char*)&on, on_len);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "setsockopt failed\n";
		closeThisSocket(created_socket);
		myWSACleanup();
		return false;
	}

	size_t ipv4header_size = sizeof(IPV4Header);
	size_t icmpheader_size = sizeof(ICMPHeader);


	// ipv4 Header
	std::string dead_end_ip_addr = "3.3.3.3";

	IPV4Header.ihl = sizeof(IPV4Header) >> 2;							// ihl min value == 5, max == 15; sizeof(IPV4Header) >> 2 is just dividing it by 4 (shifting to the right 2x);
	IPV4Header.ver = 4;													// 4 == ipv4
	IPV4Header.dscp = 0;												// https://en.wikipedia.org/wiki/Differentiated_Services_Code_Point
	IPV4Header.ecn = 0;													// https://en.wikipedia.org/wiki/Explicit_Congestion_Notification
	IPV4Header.total_len = ipv4header_size + icmpheader_size + payload_max_length;
	IPV4Header.id = htons(55155);										//?
																		//IPV4Header.flags_and_frag_offset = setFlagsAndFragOffset((uint16_t)0, (uint16_t)0);	// POSSIBLY BROKEN. leave it set to 0.
	IPV4Header.flags = 0;
	IPV4Header.frag_offset = htons(0);
	IPV4Header.ttl = 64;
	IPV4Header.protocol = 1;											// 1 == ICMP
	IPV4Header.chksum = /*datchecksum*/ 0;								// need a checksum
	IPV4Header.src_ip.s_addr = inet_addr(my_host_ip_addr.c_str());	// needs conversion to big endian
	IPV4Header.dst_ip.s_addr = inet_addr(dead_end_ip_addr.c_str());	// do not convert to big endian if assigning it something located inside a sockaddr_in structure
																	// TargetSockAddrIn.sin_addr.s_addr;	// this is one that is set to the target instead of a dead end


																	// ICMP header														//https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol
	ICMPHeader.type = ICMP_ECHO;
	ICMPHeader.code = ICMP_ECHO_CODE_ZERO;
	ICMPHeader.checksum = htons(~(ICMP_ECHO << 8));						// This trick only works with the most basic icmpheader; no payload. do not use.
																		//ICMPHeader.id = htons(12345);
																		//ICMPHeader.seq = 1;


	std::string package_for_payload = "OneTwoSkyTree";
	size_t package_for_payload_size = strlen(package_for_payload.c_str());

	// Putting stuff in the payload
	if ((package_for_payload_size <= payload_max_length) && package_for_payload_size > 0)
	{
		memcpy(payload, package_for_payload.c_str(), package_for_payload_size);
	}
	// Copying IPV4Header into the sendbuffer starting at the address of sendbuf (that is sendbuf[0])
	memcpy_s(
		sendbuf, sendbuf_max_length,
		&IPV4Header,
		ipv4header_size
		);
	current_sendbuf_len = ipv4header_size;

	// Copying the ICMPHeader into the sendbuffer starting at sendbuf[current_sendbuf_len]
	memcpy_s(
		sendbuf + current_sendbuf_len,
		sendbuf_max_length - current_sendbuf_len,
		&ICMPHeader,
		icmpheader_size
		);
	current_sendbuf_len += icmpheader_size;

	// Copying payload into the sendbuffer
	memcpy_s(
		sendbuf + current_sendbuf_len,
		sendbuf_max_length - current_sendbuf_len,
		payload,
		package_for_payload_size
		);
	current_sendbuf_len += package_for_payload_size;


	// Output buffer to screen in hex
	if (global_verbose == true)
	{
		std::cout << "Buffer contents:\n";
		for (int i = 0; i < sendbuf_max_length; i++)
		{
			std::cout
				<< " 0x"
				<< std::setfill('0')
				<< std::setw(2)
				<< std::hex
				<< (int)(u_char)sendbuf[i];
		}
		std::cout << std::dec;					// Gotta set the stream back to decimal or else it will forever output in hex
	}

	if (global_verbose == true)
	{
		std::cout << "Sizeof iphdr: " << sizeof(IPV4Header) << " \n";
		std::cout << "sizeof icmphdr: " << sizeof(ICMPHeader) << " \n";
		std::cout << "current_sendbuf_len: " << current_sendbuf_len << "\n";
	}
	return true;
}

void Raw::createThreadLoopEchoRequestToDeadEnd(void *instance)
{
#ifdef __linux__

#endif __linux__
#ifdef _WIN32
	if (instance == NULL)
	{
		return;
	}
	Raw* self = (Raw*)instance;
	self->ghEvents2[0] = (HANDLE)_beginthread(loopEchoRequestToDeadEnd, 0, instance);
	return;
#endif//_WIN32
}

// Send EchoRequest to ip: 3.3.3.3 every 3 seconds
void Raw::loopEchoRequestToDeadEnd(void* instance)
{
	if (instance == NULL)
	{
		return;
	}
	Raw* self = (Raw*)instance;		// gotta make sure we are using this thread on the correct object instance!

	CrossPlatformSleep Sleeping;
	while (self->stop_echo_request_loop == false)
	{
		if (self->stop_echo_request_loop == true)
		{
			return;
		}
		self->errchk = self->sendTheThing();
		if (self->errchk == false)
		{
			exit(0);		// hmmmmm not so sure about using exit() yet...
		}
		Sleeping.mySleep(3000);			// milliseconds
	}
	return;
}

bool Raw::sendTheThing()
{
	errchk = sendto(
		created_socket,
		sendbuf,
		current_sendbuf_len,
		0,
		(sockaddr*)&TargetSockAddrIn,
		sizeof(TargetSockAddrIn)
		);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "Sendto failed.\n";
		closeThisSocket(created_socket);
		myWSACleanup();
		return false;
	}
	return true;
}

void Raw::closeThisSocket(SOCKET fd)
{
#ifdef __linux__
	close(fd);
#endif//__linux__

#ifdef _WIN32
	closesocket(fd);
#endif
}

bool Raw::shutdownConnection(SOCKET socket)
{
	std::cout << "Shutting down the connection...\n";
	// shutdown the connection since we're done
	errchk = shutdown(socket, SD_SEND);
	if (errchk == SOCKET_ERROR)
	{
		std::cout << "shutdown failed.\n";
		closeThisSocket(socket);
		myWSACleanup();
		return false;
	}
	return true;
}

void Raw::myWSACleanup()
{
#ifdef _WIN32
	if (global_verbose == true)
	{
		std::cout << "Performing WSACleanup...\n";
	}
	WSACleanup();
#endif//_WIN32
}

// errno needs to be set to 0 before every function call that returns errno  // ERR30-C  /  SEI CERT C Coding Standard https://www.securecoding.cert.org/confluence/pages/viewpage.action?pageId=6619179
void Raw::getError()
{
#ifdef __linux__
	int errsv = errno;
	std::cout << "ERROR " << errsv << ". ";
	if (errsv == 10013)
	{
		std::cout << "Permission Denied. ";
	}
	return;
#endif//__linux__

#ifdef _WIN32
	int errsv = WSAGetLastError();
	std::cout << "ERROR: " << errsv << ". ";
	if (errsv == 10013)
	{
		std::cout << "Permission Denied. ";
	}
	return;
#endif//_WIN32
}