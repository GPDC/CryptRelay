// UPnP.h

// Overview:
// This class uses the miniupnp library from http://miniupnp.free.fr/
// Handles all things related to UPnP.

// Terminology:
// UPnP: Universal Plug and Play
// IGD: Internet Gateway Device
// External IP addr: the ip address that anyone on the internet will see you connecting from.
// External port: the port that anyone on the internet will see you connecting from.
// Internal / local ip address: the ip address that anyone on your LAN will see you connecting from.
// Internal port: the port that anyone on your LAN will see you connecting from.

// Three important steps in order to doing anything with UPnP:
// 1. Enable socket use on Windows (NOT ANYMORE, I put it in SocketClass's constructor)
//	  SockStuff.myWSAStartup();
//
// 2. Find UPnP devices on the local network
//	  findUPnPDevices();
//
// 3. Find valid IGD based off the list filled out by findUPnPDevices()
//	  findValidIGD();
//
// 4. Now you can do whatever else you want.

#ifndef UPnP_h__
#define UPnP_h__

#include "miniupnpc.h"		// needed in the header file to get the structs going
#include "SocketClass.h"

class UPnP
{
public:
	UPnP();
	~UPnP();

	bool startUPnP();
	void standaloneGetListOfPortForwards();		// Incase user just wants to access the list of port forwards
	void standaloneDeleteThisSpecificPortForward(const char * extern_port, const char* protocol);

	// These are in public because connection class will
	// want to know what the user's IP and ports are.
	char my_local_ip[64] = { 0 };				// findValidIGD() fills this out with your local ip addr
	char my_external_ip[40] = { 0 };		// displayInformation() fills this out
	std::string my_internal_port;					// in the deconstructor, deletePortForwardRule() uses this to delete a port forward.
	std::string my_external_port;					// in the deconstructor, deletePortForwardRule() uses this to delete a port forward.
	
protected:
private:
	void findUPnPDevices();
	void findValidIGD();
	void displayInformation();
	void getListOfPortForwards();
	void displayTimeStarted(u_int uptime);
	void autoAddPortForwardRule();
	void autoDeletePortForwardRule();

	SocketClass SockStuff;

	// findUPnPDevices() stores a list of a devices here as a linked list
	// Must call freeUPNPDevlist() to free allocated memory
	UPNPDev* UpnpDevicesList = nullptr;			

	// findValidIGD() stores the IGD's urls here. Example: a url to control the device.
	// Must call FreeUPNPUrls(Urls) to free allocated memory
	UPNPUrls Urls;

	// findValidIGD() stores data here for the IGD.
	IGDdatas IGDData;					

	// These are assigned in the constructor and used in addPortForwardRule().
	// my_external_port and protocol are used in autoDeletePortForwardRule()
	unsigned short i_internal_port;		// Some devices require that the internal and external ports must be the same.
	unsigned short i_external_port;		// Some devices require that the internal and external ports must be the same.
	const char * protocol = "TCP";		// This is here so autoDeletePortForwardRule() in the deconstructor can delete a port forward.

};

#endif//UPnP_h__



// FYI in the c language, calling something static means that it can't
// be seen outside the file. Think of it like a private section of a class.

