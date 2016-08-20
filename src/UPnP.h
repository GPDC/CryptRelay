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
// Internal / local port: the port that anyone on your LAN will see you connecting from.


// Important steps list:
// Three important steps in order to doing anything with UPnP:
// 1. Enable socket use on Windows (This is done for you in the constructor)
//	  WSAStartup();
//
// 2. Find UPnP devices on the local network
//	  findUPnPDevices();
//
// 3. Find valid IGD based off the list filled out by findUPnPDevices()
//	  findValidIGD();
//
// 4. Now you can do whatever else you want.

// What are these functions that have 'standalone' as part of their name?:
// When a function has "standalone" in its name, that is referring to the fact that
//  it is going to do step 2, and 3 from the important steps list, and then do what the function
//  set out to do in the first place. In other words, it will execute all functions
//  necessary to do anything UPnP related first, before doing anything else. Functions that don't have
//  this standalone label will not call the necessery step 2, and 3 functions that
//  are required for it to work.

// List of things that need to be freed after being done with UPnP:
//
// 1. freeUPNPDevlist(struct UPNPDev*)  frees the UPNPDev structure.
// 2. FreeUPNPUrls(struct UPNPUrls)  frees the UPNPUrls structure.
// Please check deconstructor first to see if it is being freed there.


#ifndef UPnP_h__
#define UPnP_h__

#ifdef __linux__
#include "miniupnpc.h"
#endif//__linux__

#ifdef _WIN32
#include <WS2tcpip.h>
#include <WinSock2.h>	// <sys/socket.h>

#include "miniupnpc.h"
#endif//_WIN32



class UPnP
{
public:
	UPnP(bool turn_verbose_output_on = false);
	virtual ~UPnP();

	// Turn on and off verbose output for this class.
	bool verbose_output = false;



	// ----------------- core functions for doing anything in UPnP class -----------------------------
	//								aka the important steps

	// Find UPnP devices on the local network
	// If something responds, that must mean it is a UPnP device,
	// and it stores discovered devices as a chained list inside UPNPDev * UpnpDevicesList.
	void findUPnPDevices();

	// Check for a valid IGD based off the list filled out by findUPnPDevices()
	// After doing that it fills out struct UPNPUrls Urls and struct IGDdatas IGDDatas
	// with information about the device and urls necessary to control it.
	int32_t findValidIGD();

	// =========================================================================================


	// Display Information such as:
	// External and local IP address
	// Connection type
	// Connection status, uptime, last connection error
	// Time started
	// Max bitrates
	void standaloneShowInformation();

	// Get, then display a list of port forwards on the IGD.
	void standaloneDisplayListOfPortForwards();

	// Delete the specified port forward rule.
	void standaloneDeleteThisSpecificPortForward(const char * extern_port, const char* protocol);

	// Add a port forward rule. User does not get to specify anything about the rule.
	int32_t autoAddPortForwardRule();

	// Delete the port forward rule that was added by autoAddPortForwardRule().
	void autoDeletePortForwardRule();

protected:
private:

	// Prevent anyone from copying this class
	UPnP(UPnP& UPnPInstance) = delete; // Delete copy operator
	UPnP& operator=(UPnP& UPnPInstance) = delete; // Delete assignment operator

	static const int32_t MY_LOCAL_IP_LEN = 64;
	char my_local_ip[MY_LOCAL_IP_LEN] = { 0 };		// findValidIGD() fills this out with your local ip addr
	static const int32_t MY_EXTERNAL_IP_LEN = 40;
	char my_external_ip[MY_EXTERNAL_IP_LEN] = { 0 };	// showInformation() fills this out
	const std::string DEFAULT_PORT = "30248";
	std::string my_local_port = DEFAULT_PORT;// in the deconstructor, deletePortForwardRule() uses this to delete a port forward.
	std::string my_external_port = DEFAULT_PORT;// in the deconstructor, deletePortForwardRule() uses this to delete a port forward.

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

	// Display Information such as:
	// External and local IP address
	// Connection type
	// Connection status, uptime, last connection error
	// Time started
	// Max bitrates
	void showInformation();

	// Get, then display a list of port forwards on the IGD.
	void displayListOfPortForwards();

	// Get, then display when the connection(?) with the IGD has started.
	void displayTimeStarted(uint32_t uptime);

	// Add a port forward rule. User does not get to specify anything about the rule.
	int32_t standaloneAutoAddPortForwardRule();

	// Cross-platform WSAStartup();
	// For every WSAStartup() that is called, a WSACleanup() must be called.
	// Necessary to do anything with sockets on Windows
	// Returns 0, success.
	// Returns a WSAERROR code if failed.
	int32_t WSAStartup();

#ifdef _WIN32
	WSADATA wsaData;	// for WSAStartup();
#endif//_WIN32

public:
	// Accessors
	char * getMyLocalIP() { return my_local_ip; }
	char * getMyExternalIP() { return my_external_ip; }
	void setMyLocalPort(const char * ptr) { my_local_port = ptr; }
	void setMyExternalPort(const char * ptr) { my_external_port = ptr; }
	std::string& getMyLocalPort() { return my_local_port; }
	std::string& getMyExternalPort() { return my_external_port; }
};

#endif//UPnP_h__
