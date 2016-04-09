// DEPRECATED_UPnP.h

// Overview:
// Implements UPnP.
// Provide cross platform (windows, linux) capability to UPnP.
// UPnP class makes extensive use of the SocketClass.h and SocketClass.cpp
//	The error handling is almost entirely done in the SocketClass, not here.
//	Any output to console (for example if verbosity is increased) is also handled in SocketClass.
//	This is to keep UPnP as a high level class that is easy to see what is being done, without clutter.

// Terminology:
// UPnP is Universal Plug and Play. If a router has upnp enabled, it trusts anyone on the local
//	network to change port forwarding settings. Only intended for home network use.

// Warnings:
// This class is not designed for multiple threads to be using it at the same time.

#include <Winsock2.h>
#include <WS2tcpip.h>
#include "SocketClass.h"


#ifndef DEPRECATED_UPnP_h__
#define DEPRECATED_UPnP_h__

class UPnP
{
public:
	UPnP();
	~UPnP();

	// The function that runs other functions. It discovers UPnP router, controls it
	int upnpStart();
	int discoverInternetGatewayDevice();
	int controlInternetGatewayDevice();

	SocketClass SockObj;

	sockaddr_in		 ControlSockAddrUPnP;
	sockaddr_in		 RcvSockAddrUPnP;
	sockaddr_in		 BroadcastSockAddrUPnP;



protected:
private:
};


#endif//UPnPfile_h__