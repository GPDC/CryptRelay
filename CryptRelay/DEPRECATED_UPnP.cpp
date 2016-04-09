// DEPRECATED_UPnP.cpp
// Explanation: after looking at the documentation for UPnP there seems to be a lot of things
//  to do and a lot of different circumstances that need handling so I think using a library
//  would be such an immense time saver that it outweighs any learning benefit of UPnP.
// Besides, creating my own implementation of UPnP isn't going to teach me anything useful
//  other than just general programming stuff, since making your own UPnP implementation
//  isn't exactly a thing people do all the time(from what I have gathered!).

// See header file for information about UPnP.cpp

// http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.1.pdf
// Steps:
// discover internet gateway device
// gather returned LOCATION:  (port and IP)
// this location link ... example: LOCATION: http://192.168.1.1:80/IGD.xml
//  can allow us to control it

#include <string>
#include <iostream>

#include "DEPRECATED_UPnP.h"
#include "SocketClass.h"	// Currently retrieving all necessary includes for dealing with sockets from this.

#define SUCCESS 1
#define FAIL 0

UPnP::UPnP()
{
	// I'm beginng to think I shouldn't memset everything in here, but rather
	//  memset the respective things right before they are used the first time.
	//  that way nobody new will have to remember / adjust to memsetting in here.
	memset(&BroadcastSockAddrUPnP, 0, sizeof(BroadcastSockAddrUPnP));
	memset(&RcvSockAddrUPnP, 0, sizeof(BroadcastSockAddrUPnP));
	memset(&ControlSockAddrUPnP, 0, sizeof(BroadcastSockAddrUPnP));
}
UPnP::~UPnP()
{

}

int UPnP::upnpStart()
{
	// Must start WSA if you want to do anything with sockets on Windows.
	if (SockObj.myWSAStartup() == false)
		return 0;

	if (discoverInternetGatewayDevice() == FAIL)
	{
		std::cout << "Couldn't find a router with UPnP enabled.\n";
		return FAIL;
	}
	if (controlInternetGatewayDevice() == FAIL)
		return FAIL;//success


	return SUCCESS;
}

int UPnP::discoverInternetGatewayDevice()
{
	// https://en.wikipedia.org/wiki/Internet_Gateway_Device_Protocol
	// UPnP multicast address is: 239.255.255.250 and port 1900
	// MX: max time in seconds to wait for response
	// ST: Search Target. URN value of service to search
	// Man: not sure what it stands for?
	// USER-AGENT: optional for UPnP. OS/versionUDAP/2.0product/version
	// Yes, \r\n needs to be put at the end. I believe the reason for this is
	//	if you don't want to include USER-AGENT (it is optional with UPnP)
	//	then the \r\n that would follow it is still necessary. Just a guess.
	const char* broadcast_discover_msg = "M-SEARCH * HTTP/1.1\r\nHost:239.255.255.250:1900\r\nST:urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\nMan:\"ssdp:discover\"\r\nMX:3\r\n\r\n";
	int broadcast_discover_msg_len = strlen(broadcast_discover_msg);

	// Create socket
	SOCKET sock_discover = SockObj.mySocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock_discover == INVALID_SOCKET)
		return FAIL;

	int success = FALSE;

	// Enabling broadcast on this socket.
	const bool broadcast_bool = true;
	success = SockObj.mySetSockOpt(
		sock_discover,
		SOL_SOCKET,
		SO_BROADCAST,
		(char*)&broadcast_bool,
		sizeof(broadcast_bool)
		);
	if (success == FALSE)
		return FAIL;

	// Setting recvfrom() to stop listening after x number of milliseconds
	const DWORD receive_timeout_time = 2000;
	success = SockObj.mySetSockOpt(
		sock_discover,
		SOL_SOCKET,
		SO_RCVTIMEO,
		(char*)&receive_timeout_time,
		sizeof(receive_timeout_time)
		);
	if (success == FALSE)
		return FAIL;

	// Settings ip addr, port, and ipv4 or ipv6 to send on.
	BroadcastSockAddrUPnP.sin_family = AF_INET;
	BroadcastSockAddrUPnP.sin_port = htons(1900);				// Port must be 1900 to talk to UPnP
	if (SockObj.myinet_pton(BroadcastSockAddrUPnP.sin_family, "239.255.255.250", &(BroadcastSockAddrUPnP.sin_addr)) == FAIL)
		return FAIL;

	int message_count = 0;		// Number of messages received.
	int z = 0;
	// Try 5 times to find a UPnP enabled router (UDP unreliable) if no msg received
	while (message_count == 0 && z < 5)
	{
		// Send the broadcast message to look for a UPnP enabled router
		success = SockObj.mySendTo(
			sock_discover,
			broadcast_discover_msg,
			broadcast_discover_msg_len,
			0,
			(sockaddr*)&BroadcastSockAddrUPnP,
			sizeof(BroadcastSockAddrUPnP)
			);
		if (success == SOCKET_ERROR)
			return FAIL;

		// Stuff necessary for the receive loop
		int size = sizeof(RcvSockAddrUPnP);
		int bytes = 0;							// Byte count of the message that was received.

		const int recv_buf_len = 512;
		char recv_buf[recv_buf_len];								// recvfrom() puts the received msg here

		const int psaved_msg_array_len = 20;
		char* psaved_msg_array[psaved_msg_array_len] = { nullptr };	// pointers to the saved message arrays
		char saved_msg[recv_buf_len];								// Location that the messages are saved.

		// Making sure there is no random data in the arrays
		memset(recv_buf, 0, recv_buf_len);
		memset(saved_msg, 0, recv_buf_len);

		// This doesn't seem to be needed, but here it is.
		RcvSockAddrUPnP.sin_family = AF_INET;
		RcvSockAddrUPnP.sin_addr.s_addr = INADDR_ANY;
		RcvSockAddrUPnP.sin_port = htons(1900);

		// Receive messages loop
		do
		{
			bytes = SockObj.myRecvFrom(
				sock_discover,
				recv_buf,
				recv_buf_len,
				0,
				(sockaddr*)&RcvSockAddrUPnP,
				&size
				);
			if (bytes == SOCKET_ERROR)
				return FAIL;
			else if (bytes == WSAETIMEDOUT)		// Time limit for checking for messages has been reached. Leave the loop.
				break;
			else								// Must have received a response. That means we have +1 UPnP Internet Gateway Device we can control.
				message_count++;

			// Outputting received msg to console
			for (int i = 0; i < bytes; ++i)
			{
				std::cout << recv_buf[i];
			}
			std::cout << "\n";

			// Save the message here so it can be checked later for specific information.
			std::cout << "Message saved:\n";
			for (int i = 0; i < message_count && i < psaved_msg_array_len; ++i)
			{
				// Save the msg
				memcpy(saved_msg, recv_buf, recv_buf_len);
				// Give the saved_msg array a pointer to its location
				psaved_msg_array[i] = saved_msg;
				// Output to console the saved msg
				if (psaved_msg_array[i] != nullptr)
				{
					for (int b = 0; b < bytes; ++b)
						std::cout << psaved_msg_array[i][b];
				}
			}

		} while (bytes > 0);	// if 0, that means graceful shutdown of connection occured.
	}
	return SUCCESS;
}

int UPnP::controlInternetGatewayDevice()
{
	std::string external_port = "5005";
	std::string protocol = "UDP";
	std::string internal_port = "7474";
	std::string internal_ip = "192.168.1.116";
	std::string entry_description = "wombo combo";

	const char* payload_upnp_control_msg =
		"<?xml version=\"1.0\"?>"
		"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
		"<SOAP-ENV:Body>"
		"<m:AddPortMapping xmlns:m=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewRemoteHost>"
		""
		"</NewRemoteHost>"
		"<NewExternalPort>"
		"5005"//
		"</NewExternalPort>"
		"<NewProtocol>"
		"UDP"//
		"</NewProtocol>"
		"<NewInternalPort>"
		"7474"//
		"</NewInternalPort>"
		"<NewInternalClient>"
		"192.168.1.116"//
		"</NewInternalClient>"
		"<NewEnabled>"
		"1"
		"</NewEnabled>"
		"<NewPortMappingDescription>"
		"wombocombo"//
		"</NewPortMappingDescription>"
		"<NewLeaseDuration>"
		"0"
		"</NewLeaseDuration>"
		"</m:AddPortMapping>"
		"</SOAP-ENV:Body>"
		"</SOAP-ENV:Envelope>\r\n\r\n";

	char* the_string = nullptr;
	

	const char* header_upnp_control_msg =
		"POST /UD/?3 HTTP/1.1\r\n"
		"Content-Type: text/xml; charset=\"utf-8\"\r\n"
		"SOAPAction: \"urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping\"\r\n"
		"User-Agent: Mozilla/4.0 (compatible; UPnP/1.0; Windows 9x)\r\n"
		"Host: " "192.168.1.1" "\r\n"
		"Content-Length: " "636" "\r\n" //636 is strlen as of right now ... change this to an actual variable
		"Connection: Close\r\n"
		"Cache-Control: no-cache\r\n"
		"Pragma: no-cache\r\n\r\n";

	// Create TCP socket
	SOCKET sock_control = SockObj.mySocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_control == INVALID_SOCKET)
		return FAIL;

	// Setting ip addr, port, and ipv4 or ipv6
	ControlSockAddrUPnP.sin_family = AF_INET;
	ControlSockAddrUPnP.sin_port = htons(49152);
	if (SockObj.myinet_pton(AF_INET, "192.168.1.1", &ControlSockAddrUPnP.sin_addr) == FAIL) // temp ip addr, its supposed to be what is returned by discovery
		return FAIL;

	// Connect to the device
	int errchk = SockObj.myConnect(sock_control, (sockaddr*)&ControlSockAddrUPnP, sizeof(ControlSockAddrUPnP));
	if (errchk == SOCKET_ERROR || errchk == INVALID_SOCKET)
		return FAIL;

	// Stuff for send()
	const int send_buf_len = 1024;
	char send_buf[send_buf_len];

	int header_upnp_control_msg_len = strlen(header_upnp_control_msg);
	int payload_upnp_control_msg_len = strlen(payload_upnp_control_msg);

	int size_of_payload = payload_upnp_control_msg_len;
	size_of_payload += header_upnp_control_msg_len;

	// Copy header to buffer
	memcpy(send_buf, header_upnp_control_msg, header_upnp_control_msg_len);
	int current_length = header_upnp_control_msg_len;
	
	// Copy payload to buffer
	memcpy(send_buf + header_upnp_control_msg_len, payload_upnp_control_msg, payload_upnp_control_msg_len);
	current_length += payload_upnp_control_msg_len;

	// Send buffer with the current length of the buffer (not maximum length)
	errchk = SockObj.mySend(sock_control, send_buf, current_length, 0);
	if (errchk == SOCKET_ERROR)
		return FAIL;

	// Things necessary for receive loop
	const int large_recv_buf_len = 2000;
	char large_recv_buf[large_recv_buf_len];
	int bytes = 0;

	memset(large_recv_buf, 0, large_recv_buf_len);

	do
	{
		bytes = SockObj.myRecv(sock_control, large_recv_buf, large_recv_buf_len, 0);
		if (bytes == SOCKET_ERROR)
			return FAIL;
		else if (bytes == 0)	// Connection gracefully closed
		{
			break;
		}
		else
		{
			for (int i = 0; i < bytes; ++i)
			{
				std::cout << large_recv_buf[i];
			}
			std::cout << "\n";
		}

	} while (bytes > 0);



	return SUCCESS;
}