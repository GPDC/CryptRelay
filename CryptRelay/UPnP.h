// UPnP.h

// Overview:
// This class uses the miniupnp library from http://miniupnp.free.fr/
// Handles all things related to UPnP.

// Terminology:
// UPnP: Universal Plug and Play
// IGD: Internet Gateway Device
// External IP addr: the ip address that anyone on the internet will see you connecting from.
// internal / local ip address: the ip address that anyhone on your LAN will see you connecting from.

#ifndef UPnP_h__
#define UPnP_h__

#include "miniupnpc.h"//using this one atleast... needed in the header file to get the structs going

class UPnP
{
public:
	UPnP();
	~UPnP();

	int startUPnP();
	
protected:
private:
	void findUPnPDevices();
	void findValidIGD(struct UPNPDev* ListOfUPnPDevices, struct UPNPUrls* IGDUrls, struct IGDdatas* IGDDatas);
	void displayInformation(struct UPNPUrls * Urls, struct IGDdatas * IGDData);
	void getListOfPortForwards(struct UPNPUrls * Urls, struct IGDdatas * IGDData);
	void addPortForwardRule(struct UPNPUrls * Urls,	struct IGDdatas * IGDData,	const char * my_local_ip);

	// findUPnPDevices() stores a list of a devices here as a linked list
	// Must call freeUPNPDevlist() to free allocated memory
	UPNPDev* UpnpDevicesList;			

	// findValidIGD() stores the IGD's urls here. Example: a url to control the device.
	// Must call FreeUPNPUrls(Urls) to free allocated memory
	UPNPUrls Urls;
	
	IGDdatas IGDData;					// findValidIGD() stores data here for the IGD.

	char my_local_ip[64] = { 0 };		// findValidIGD() fills this out with your local ip addr
	char externalIPAddress[40] = { 0 };	// displayInformation() fills this out
};

#endif//UPnP_h__





