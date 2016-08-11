// UPnP.h

// Overview:
// This class uses the miniupnp library from http://miniupnp.free.fr/
// Handles all things related to UPnP.

// Warnings:
// This source file does not do any input validation.

// Terminology:
// UPnP: Universal Plug and Play
// IGD: Internet Gateway Device
// External IP addr: the ip address that anyone on the internet will see you connecting from.
// External port: the port that anyone on the internet will see you connecting from.
// Internal / local ip address: the ip address that anyone on your LAN will see you connecting from.
// Internal / local port: the port that anyone on your LAN will see you connecting from.


// Important steps list:
// Three important steps in order to doing anything with UPnP:
// 1. Enable socket use on Windows (NOT ANYMORE, I put it in SocketClass's constructor)
//	  SocketClass.WSAStartup();
//
// 2. Find UPnP devices on the local network
//	  findUPnPDevices();
//
// 3. Find valid IGD based off the list filled out by findUPnPDevices()
//	  findValidIGD();
//
// 4. Now you can do whatever else you want.

// Now that the important steps have been discussed:
// When a function has "standalone" in its name, that is referring to the fact that
//  it is going to do step 2, and 3 from the important steps list, and then do what the function
//  set out to do in the first place. In other words, it will execute all functions
//  necessary to do anything UPnP related first, before doing anything else. Functions that don't have
//  this standalone label will not call the necessery step 2, and 3 functions that
//  are required for it to work.

// List of things that need to be freed after being done with UPnP:
//
// 1. freeUPNPDevlist(struct UPNPDev*)  frees the UPNPDev structure.
//
// 2. FreeUPNPUrls(struct UPNPUrls)  frees the UPNPUrls structure.


#ifndef UPnP_h__
#define UPnP_h__

#include "miniupnpc.h"		// needed in the header file to get the structs going
#include "SocketClass.h"

class UPnP
{
public:
	UPnP();
	virtual ~UPnP();

	void findUPnPDevices();					// core function for doing anything in upnp
	bool findValidIGD();					// core function for doing anything in upnp

	void standaloneShowInformation();
	void standaloneGetListOfPortForwards();
	void standaloneDeleteThisSpecificPortForward(const char * extern_port, const char* protocol);
	bool autoAddPortForwardRule();
	void autoDeletePortForwardRule();
	bool standaloneAutoAddPortForwardRule();

	// These are in public because connection class will
	// want to know what the user's IP and ports are.
	char my_local_ip[64] = { 0 };		// findValidIGD() fills this out with your local ip addr
	char my_external_ip[40] = { 0 };	// showInformation() fills this out
	const std::string DEFAULT_PORT = "30248";
	std::string upnp_my_internal_port = DEFAULT_PORT;// in the deconstructor, deletePortForwardRule() uses this to delete a port forward.
	std::string upnp_my_external_port = DEFAULT_PORT;// in the deconstructor, deletePortForwardRule() uses this to delete a port forward.
	
protected:
private:

	// Prevent anyone from copying this class
	UPnP(UPnP& UPnPInstance) = delete; // Delete copy operator
	UPnP& operator=(UPnP& UPnPInstance) = delete; // Delete assignment operator

	void showInformation();
	void getListOfPortForwards();
	void displayTimeStarted(u_int uptime);

	// findUPnPDevices() stores a list of a devices here as a linked list
	// Must call freeUPNPDevlist(UpnpDevicesList) to free allocated memory
	UPNPDev* UpnpDevicesList = nullptr;			

	// findValidIGD() stores the IGD's urls here. Example: a url to control the device.
	// Must call FreeUPNPUrls(Urls) to free allocated memory
	UPNPUrls Urls;

	// findValidIGD() stores data here for the IGD.
	IGDdatas IGDData;					

	// This is here so autoDeletePortForwardRule() in the deconstructor can delete a port forward.
	const char * protocol = "TCP";

	// If this is true, then the deconstructor will delete the port forward
	// entry that was automatically added using autoAddPortForwardRule()
	bool port_forward_automatically_added = false;
};

#endif//UPnP_h__
