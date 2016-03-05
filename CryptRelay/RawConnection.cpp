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
#include <pthread.h>			//<process.h>
#include <iomanip>				// std::setw(2) && std::setfill('0')

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
#include <process.h>			//<pthread.h>
#include <iomanip>				// std::setw(2) && std::setfill('0')

#include "connection.h"
#include "GlobalTypeHeader.h"
#include "CommandLineInput.h"
#include "CrossPlatformSleep.h"
#include "RawConnection.h"
#endif//_WIN32

// Reminder: Carefully read every. single. word. inside documentation for berkley sockets (this applies to everything).
//		Otherwise you might skip the 2 most important words in the gigantic document.

bool Raw::stop_echo_request_loop = false;
bool Raw::stop_time_exceeded_loop = false;

#ifdef _WIN32
HANDLE Raw::ghEvents2[2];
#endif//_WIN32

Raw::Raw()
{
	// int
	iResult = 0;
	errchk = 0;

	// IPHeader
	memset(&IPV4HeaderEchoRequest, 0, sizeof(IPV4HeaderEchoRequest));
	memset(&IPV4HeaderTimeExceeded, 0, sizeof(ICMPHeaderTimeExceeded));
	// ICMPHeader
	memset(&ICMPHeaderEchoRequest, 0, sizeof(ICMPHeaderEchoRequest));
	memset(&ICMPHeaderTimeExceeded, 0, sizeof(ICMPHeaderTimeExceeded));
	// sockaddr_in
	memset(&TargetExternalSockAddrIn, 0, sizeof(TargetExternalSockAddrIn));
	// BufferAndPayloadInfo
	memset(&ER, 0, sizeof(ER));
	memset(&TE, 0, sizeof(TE));

	// SOCKET (The sockets within a struct must come after the memset for the struct)
	TE.s = INVALID_SOCKET;	// s == Socket
	ER.s = INVALID_SOCKET;	// s == Socket
}
Raw::~Raw()
{
}

bool Raw::sendICMPEchoRequeest()
{
	// Create a socket; if successful, craft a packet
	ER.s = createSocket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (ER.s == false)
	{
		return false;
	}
	else
	{
		if (craftFixedICMPEchoRequestPacket(ER.s) == false)
			return false;
	}

	// Threaded 30 sec loop sending Echo Requests
	createThreadLoopEchoRequestToDeadEnd();

	return true;
}

bool Raw::sendICMPTimeExceeded()
{
	TE.s = createSocket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (TE.s == false)
	{
		return false;
	}
	else
	{
		craftICMPTimeExceededPacket(TE.s);
	}

	createThreadLoopTimeExceeded();

	return true;
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


void Raw::setAddress(std::string target_ip, std::string target_port, std::string target_local_ip, std::string my_ip, std::string my_port, std::string my_ext_ip)
{
	if (global_verbose == true)
	{
		std::cout << "Setting info: IP address and port...\n";
	}

	// Retrieving all the info that was gathered by CommandLineInput and checked by FormatCheck.cpp
	TargetExternalSockAddrIn.sin_family = AF_INET;
	TargetExternalSockAddrIn.sin_addr.s_addr = inet_addr(target_ip.c_str());
	TargetExternalSockAddrIn.sin_port = htons(atoi(target_port.c_str()));

	TargetLocalSockAddrIn.sin_family = AF_INET;
	TargetLocalSockAddrIn.sin_addr.s_addr = inet_addr(target_local_ip.c_str());
	TargetLocalSockAddrIn.sin_port = htons(atoi(target_port.c_str()));

	MyExternalSockAddrIn.sin_family = AF_INET;
	MyExternalSockAddrIn.sin_addr.s_addr = inet_addr(my_ext_ip.c_str());
	MyExternalSockAddrIn.sin_port = htons(atoi(my_port.c_str()));

	MyLocalSockAddrIn.sin_family = AF_INET;
	MyLocalSockAddrIn.sin_addr.s_addr = inet_addr(my_ip.c_str());
	MyLocalSockAddrIn.sin_port = htons(atoi(my_port.c_str()));


	if (global_verbose == true)
	{
		std::cout << "t_ext " << target_ip << ", " << "t_p " << target_port << ", " << "t_local " << target_local_ip << ", " << "my_local " << my_ip << ", " << "my_port " << my_port << ", " << "my_ext " << my_ext_ip << "\n";
	}

	return;
}

SOCKET Raw::createSocket(int family, int socktype, int protocol)	// Purely optional return value.
{
	SOCKET created_socket = INVALID_SOCKET;
	if (global_verbose == true)
	{
		std::cout << "Creating RAW Socket...\n";
	}
	// Create a SOCKET handle for connecting to server(no ip address here, that is with bind)
	created_socket = socket(family, socktype, protocol);			// Must be admin / root / SUID 0  in order to open a RAW socket
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
bool Raw::craftFixedICMPEchoRequestPacket(SOCKET fd)
{
	const char on = 1;
	int on_len = sizeof(int);

	// Tell kernel that we are doing our own IP structures
	errchk = setsockopt(fd, IPPROTO_IP, IP_HDRINCL, (char*)&on, on_len);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "setsockopt failed\n";
		closeThisSocket(fd);
		myWSACleanup();
		return false;
	}

	size_t ipv4header_size = sizeof(IPV4HeaderEchoRequest);
	size_t icmpheader_size = sizeof(ICMPHeaderEchoRequest);
	
	std::string dead_end_ip_addr = "3.3.3.3";
	DeadEndSockAddrIn.sin_family = AF_INET;
	DeadEndSockAddrIn.sin_addr.s_addr = inet_addr(dead_end_ip_addr.c_str());
	DeadEndSockAddrIn.sin_port = TargetExternalSockAddrIn.sin_port;

	// ipv4 Header																// https://en.wikipedia.org/wiki/IPv4
	IPV4HeaderEchoRequest.ihl = sizeof(IPV4HeaderEchoRequest) >> 2;				// ihl min value == 5, max == 15; sizeof(IPV4HeaderEchoRequest) >> 2 is just dividing it by 4;
	IPV4HeaderEchoRequest.ver = 4;												// 4 == ipv4, 6 == ipv6
	IPV4HeaderEchoRequest.dscp = 0;
	IPV4HeaderEchoRequest.ecn = 0;
	IPV4HeaderEchoRequest.total_len = (uint16_t)(ipv4header_size + icmpheader_size /* + payload_max_length*/);
	IPV4HeaderEchoRequest.id = htons(256);										//?
	IPV4HeaderEchoRequest.flags = 0;
	IPV4HeaderEchoRequest.frag_offset = htons(0);
	IPV4HeaderEchoRequest.ttl = 64;
	IPV4HeaderEchoRequest.protocol = 1;											// 1 == ICMP
	IPV4HeaderEchoRequest.chksum = 0;
	IPV4HeaderEchoRequest.src_ip.s_addr = MyLocalSockAddrIn.sin_addr.s_addr;	//inet_addr("local ip")
	IPV4HeaderEchoRequest.dst_ip.s_addr = DeadEndSockAddrIn.sin_addr.s_addr;	//inet_addr("3.3.3.3");
	IPV4HeaderEchoRequest.chksum = ICMPChkSum(&IPV4HeaderEchoRequest, ipv4header_size);

	// ICMP header																//https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol
	ICMPHeaderEchoRequest.type = ICMP_ECHO;
	ICMPHeaderEchoRequest.code = ICMP_ECHO_CODE_ZERO;
	ICMPHeaderEchoRequest.checksum = 0;
	ICMPHeaderEchoRequest.id = 0;
	ICMPHeaderEchoRequest.seq = 0;
	ICMPHeaderEchoRequest.checksum = ICMPChkSum(&ICMPHeaderEchoRequest, icmpheader_size);

	package_for_payload = "";
	ER.package_for_payload_size = strlen(package_for_payload.c_str());

	// Need size checking in here for payload !!~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Putting stuff in the payload
	if ((ER.package_for_payload_size <= payload_max_length) && ER.package_for_payload_size > 0)
	{
		memcpy(ER.payload, package_for_payload.c_str(), ER.package_for_payload_size);
	}
	// Copying IPV4HeaderEchoRequest into the sendbuffer starting at the address of sendbuf (that is sendbuf[0])
	memcpy_s(
		ER.sendbuf, sendbuf_max_length,
		&IPV4HeaderEchoRequest,
		ipv4header_size
		);
	ER.current_sendbuf_len = ipv4header_size;

	// Copying the ICMPHeaderEchoRequest into the sendbuffer starting at sendbuf[ER.current_sendbuf_len]
	memcpy_s(
		ER.sendbuf + ER.current_sendbuf_len,
		sendbuf_max_length - ER.current_sendbuf_len,
		&ICMPHeaderEchoRequest,
		icmpheader_size
		);
	ER.current_sendbuf_len += icmpheader_size;

	// Copying payload into the sendbuffer
	/*if (ER.package_for_payload_size > 0)
	{
		memcpy_s(
			ER.sendbuf + ER.current_sendbuf_len,
			sendbuf_max_length - ER.current_sendbuf_len,
			ER.payload,
			ER.package_for_payload_size
			);
		ER.current_sendbuf_len += ER.package_for_payload_size;
	}*/
	


	// Output buffer to screen in hex
	if (global_verbose == true)
	{
		std::cout << "Buffer contents:\n";
		for (u_int i = 0; i < ER.current_sendbuf_len; i++)
		{
			std::cout
				<< " 0x"
				<< std::setfill('0')
				<< std::setw(2)
				<< std::hex
				<< (int)(u_char)ER.sendbuf[i];
		}
		std::cout << "\n";
		std::cout << std::dec;					// Gotta set the stream back to decimal or else it will forever output in hex
	}

	if (global_verbose == true)
	{
		std::cout << "Sizeof IPHeader: " << sizeof(IPV4HeaderEchoRequest) << " \n";
		std::cout << "Sizeof icmphdr: " << sizeof(ICMPHeaderEchoRequest) << " \n";
		std::cout << "current_sendbuf_len: " << ER.current_sendbuf_len << "\n";
	}
	return true;
}


bool Raw::craftICMPTimeExceededPacket(SOCKET fd)
{
	const char on = 1;
	int on_len = sizeof(int);

	// Tell kernel that we are doing our own IP structures for this particular socket
	errchk = setsockopt(fd, IPPROTO_IP, IP_HDRINCL, (char*)&on, on_len);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "setsockopt failed.\n";
		closeThisSocket(fd);
		myWSACleanup();
		return false;
	}

	size_t ipv4header_size = sizeof(IPV4HeaderTimeExceeded);
	size_t icmpheader_size = sizeof(ICMPHeaderTimeExceeded);
	size_t mini_icmpheader_attachment_size = sizeof(TimeExceededAttachment);

	// Copying the IPV4 Echo Request Header so we can change a some variables
	IPV4HeaderEchoRequestCopy = IPV4HeaderEchoRequest;											// sidenote, extra steps need to be taken if there is a pointer in the class/struct (there isn't though, atleast in this one!)
	IPV4HeaderEchoRequestCopy.src_ip.s_addr = TargetExternalSockAddrIn.sin_addr.s_addr;			//inet_addr("external ip");
	IPV4HeaderEchoRequestCopy.ttl = 1;															// This is what would normally be the value if the time exceeded.
	IPV4HeaderEchoRequestCopy.total_len = htons(IPV4HeaderEchoRequestCopy.total_len);			//????? because the kernel did the htons() for me on the original ipv4 packet? whats going on?
	IPV4HeaderEchoRequestCopy.chksum = 0;														// Must set it to 0 before calculating the checksum
	IPV4HeaderEchoRequestCopy.chksum = ICMPChkSum(&IPV4HeaderEchoRequestCopy, ipv4header_size);	// Gotta checksum again b/c things have changed

	// ipv4 Header
	IPV4HeaderTimeExceeded.ihl = sizeof(IPV4HeaderTimeExceeded) >> 2;				// ihl min value == 5, max == 15; sizeof(IPV4HeaderEchoRequest) >> 2 is just dividing it by 4 (shifting to the right 2x);
	IPV4HeaderTimeExceeded.ver = 4;													// 4 == ipv4
	IPV4HeaderTimeExceeded.dscp = 0;												// https://en.wikipedia.org/wiki/Differentiated_Services_Code_Point
	IPV4HeaderTimeExceeded.ecn = 0;													// https://en.wikipedia.org/wiki/Explicit_Congestion_Notification
	IPV4HeaderTimeExceeded.total_len = (uint16_t)(ipv4header_size + icmpheader_size + mini_icmpheader_attachment_size /*+ payload_max_length*/);
	IPV4HeaderTimeExceeded.id = htons(256);											//?
	IPV4HeaderTimeExceeded.flags = 0;
	IPV4HeaderTimeExceeded.frag_offset = htons(0);
	IPV4HeaderTimeExceeded.ttl = 64;
	IPV4HeaderTimeExceeded.protocol = 1;											// 1 == ICMP
	IPV4HeaderTimeExceeded.chksum = 0;												// set checksum to 0 before calculating the checksum (though, i did memset 0 the whole thing already)
	IPV4HeaderTimeExceeded.src_ip.s_addr = MyLocalSockAddrIn.sin_addr.s_addr;		//inet_addr("local ip");
	IPV4HeaderTimeExceeded.dst_ip.s_addr = TargetExternalSockAddrIn.sin_addr.s_addr;//inet_addr("external ip");
	IPV4HeaderTimeExceeded.chksum = ICMPChkSum(&IPV4HeaderTimeExceeded, ipv4header_size);

	// ICMP header																	//https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol
	ICMPHeaderTimeExceeded.type = ICMP_TIME_EXCEEDED;
	ICMPHeaderTimeExceeded.code = ICMP_TIME_EXCEEDED_CODE_TTL_EXPIRED_IN_TRANSIT;
	ICMPHeaderTimeExceeded.checksum = 0;
	ICMPHeaderTimeExceeded.checksum = ICMPChkSum(&ICMPHeaderTimeExceeded, icmpheader_size);

	
	// Copying IPV4HeaderTimeExceeded into the sendbuffer starting at the address of sendbuf (that is sendbuf[0])
	memcpy_s(
		TE.sendbuf, sendbuf_max_length,
		&IPV4HeaderTimeExceeded,
		ipv4header_size
		);
	TE.current_sendbuf_len = ipv4header_size;

	// Copying the ICMPHeaderTimeExceeded into the sendbuffer starting at sendbuf[current_sendbuf_len]
	memcpy_s(
		TE.sendbuf + TE.current_sendbuf_len,
		sendbuf_max_length - TE.current_sendbuf_len,
		&ICMPHeaderTimeExceeded,
		icmpheader_size
		);
	TE.current_sendbuf_len += icmpheader_size;

	// Copying original ip header from Echo Request into the sendbuffer
	memcpy_s(
		TE.sendbuf + TE.current_sendbuf_len,
		sendbuf_max_length - TE.current_sendbuf_len,
		&IPV4HeaderEchoRequestCopy,
		ipv4header_size
		);
	TE.current_sendbuf_len += ipv4header_size;

	// Copying original icmp header from Echo Request into the sendbuffer
	memcpy_s(
		TE.sendbuf + TE.current_sendbuf_len,
		sendbuf_max_length - TE.current_sendbuf_len,
		&ICMPHeaderEchoRequest,
		icmpheader_size
		);
	TE.current_sendbuf_len += icmpheader_size;
	
	//// Copying original payload into the sendbuffer
	//memcpy_s(
	//	TE.sendbuf + TE.current_sendbuf_len,
	//	sendbuf_max_length - TE.current_sendbuf_len,
	//	&ER.payload,								// ER.payload is no mistake. Time Exceeded requires the original Echo Request payload.
	//	ipv4header_size
	//	);
	//TE.current_sendbuf_len += ER.package_for_payload_size;	//ER.package_for_payload_size is not a mistake



	// Output buffer to screen in hex
	if (global_verbose == true)
	{
		std::cout << "Buffer contents:\n";
		for (u_int i = 0; i < TE.current_sendbuf_len; i++)
		{
			std::cout
				<< " 0x"
				<< std::setfill('0')
				<< std::setw(2)
				<< std::hex
				<< (int)(u_char)TE.sendbuf[i];
		}
		std::cout << "\n";
		std::cout << std::dec;					// Gotta set the stream back to decimal or else it will forever output in hex
	}

	if (global_verbose == true)
	{
		std::cout << "Sizeof IPHeader: " << ipv4header_size << " \n";
		std::cout << "Sizeof icmphdr: " << icmpheader_size << " \n";
		std::cout << "current_sendbuf_len: " << TE.current_sendbuf_len << "\n";
	}
	return true;
}

void Raw::createThreadLoopEchoRequestToDeadEnd()
{
#ifdef __linux__

#endif __linux__
#ifdef _WIN32
	ghEvents2[0] = (HANDLE)_beginthread(loopEchoRequestToDeadEnd, 0, this);
	return;
#endif//_WIN32
}

// Send EchoRequest to ip: 3.3.3.3 on a threaded loop
void Raw::loopEchoRequestToDeadEnd(void* instance)
{
	if (instance == NULL)
	{
		return;
	}
	Raw* self = (Raw*)instance;
	int errchk = 0;
	CrossPlatformSleep Sleeping;

	while (stop_echo_request_loop == false)
	{
		errchk = self->sendTheThing(self->ER.s, self->ER.sendbuf, self->ER.current_sendbuf_len, (sockaddr*)&self->DeadEndSockAddrIn, sizeof(self->DeadEndSockAddrIn));
		if (errchk == false)
		{
			exit(0);		// hmmmmm not so sure about using exit() yet...
		}
		Sleeping.mySleep(10'000);			// milliseconds
	}
	return;
}
 
void Raw::createThreadLoopTimeExceeded()
{
#ifdef __linux__

#endif __linux__
#ifdef _WIN32
	ghEvents2[1] = (HANDLE)_beginthread(loopTimeExceeded, 0, this);
	return;
#endif//_WIN32
}
void Raw::loopTimeExceeded(void* instance)
{
	if (instance == NULL)
	{
		return;
	}
	Raw* self = (Raw*)instance;
	int errchk = 0;
	CrossPlatformSleep Sleeping;

	while (stop_time_exceeded_loop == false)
	{
		errchk = self->sendTheThing(self->TE.s, self->TE.sendbuf, self->TE.current_sendbuf_len, (sockaddr*)&self->TargetExternalSockAddrIn, sizeof(self->TargetExternalSockAddrIn));
		if (errchk == false)
		{
			exit(0);
		}
		Sleeping.mySleep(5'000);
	}
	return;
}

bool Raw::sendTheThing(SOCKET fd, char buffer[], size_t buffer_length, const sockaddr* to, int tolen)
{
	errchk = sendto(
		fd,
		buffer,
		buffer_length,
		0,
		to,
		tolen
		);
	if (errchk == SOCKET_ERROR)
	{
		getError();
		std::cout << "Sendto failed.\n";
		closeThisSocket(fd);
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

// Set errno to 0 before every function call that returns errno  // ERR30-C  /  SEI CERT C Coding Standard https://www.securecoding.cert.org/confluence/pages/viewpage.action?pageId=6619179
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

// Pretty sure I must have made one of the slowest checksums out there. Though this program doesn't need the speed, it would be interesting to think of a better way.
uint16_t Raw::ICMPChkSum(void *data, size_t size)
{
	uint16_t* datums = (uint16_t*)data;	// treat it like a 2 byte int so we can add it into sum 2 bytes at a time.
	if (size % 2 == 1)					// checking to see if the data being passed in is an odd number. it needs to be even b/c we are treating it as a 16bit word.
	{
		std::cout << "Checksum error. Data passed into checksum is not an even number.\n";
		std::cout << "Please change Checksum code to accommodate it. Returning 0.\n";
		return 0;
	}
	uint32_t sum = 0;

	for (u_int i = 0; i < size / 2; ++i)
	{
		sum += datums[i];
		if (sum > 0xFFFF)
		{
			sum = sum & 0xFFFF;
			sum += 1;	
		}
	}
	return ~sum;

	// RFC 792 https://tools.ietf.org/html/rfc792
	// The 16 bit one's complement of the one's complement sum of all 16 bit words in the header.

	// My comment: The way this is worded makes this a tad more confusing that it should be.
	// What it means by one's complement is to take the 17th bit (when it exists/created), and add
	// it back in at the least significant bit slot.
	// The other one's complement refers to the ~ operator. Just return ~sum.

	// Special note on Checksums:
	// It appears everything in the packet needs to have a checksum done.
	// If a single checksum is not done, then all the other checksums will be counted as incorrect, even though that isn't true.
}