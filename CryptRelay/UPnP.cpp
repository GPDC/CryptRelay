#define _CRT_SECURE_NO_WARNINGS
// Terminology:
// IGD: Internet Gateway Device (your router).

#include <iostream>
#include <stdio.h>
#include <string>
#include <WinSock2.h>
#include <time.h>	// example code displayinfos()
#include <limits.h>	// macros for int, short, char, etc that are defined as their max values.

#include "UPnP.h"
#include "SocketClass.h"
#include "GlobalTypeHeader.h"

//miniupnp
#include "minixml.h"
#include "minissdpc.h"
#include "miniwget.h"
#include "minisoap.h"
#include "minixml.h"
#include "upnpcommands.h"
#include "upnperrors.h"
#include "connecthostport.h"
#include "receivedata.h"
#include "portlistingparse.h"
#include "upnpreplyparse.h"

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")		// NEEDED for miniupnp library!
#pragma comment(lib, "Iphlpapi.lib")	// NEEDED for miniupnp library!!!
#endif//_WIN32

UPnP::UPnP()
{
	UpnpDevicesList = nullptr;
	memset(&Urls, 0, sizeof(Urls));
	memset(&IGDData, 0, sizeof(IGDData));
}

UPnP::~UPnP()
{
	// Delete ALL port forwarding settings with description CryptRelay


	// Free everything because we are done with them
	if (global_verbose == true)
		std::cout << "Freeing allocated UPnP memory.\n";
	freeUPNPDevlist(UpnpDevicesList);
	FreeUPNPUrls(&Urls);
}


int UPnP::startUPnP()
{
	// Enable socket use on Windows
	SocketClass SockStuff;
	SockStuff.myWSAStartup();

	// Find UPnP devices on the local network
	findUPnPDevices();

	// Find valid IGD based off the list returned by upnpDiscover()
	findValidIGD(UpnpDevicesList, &Urls, &IGDData);

	// Displays extra information
	if (global_verbose == true)
		displayInformation(&Urls, &IGDData);
		
	// Get list of currently mapped ports for the IGD.
	getListOfPortForwards(&Urls, &IGDData);

	// Add a port forwarding rule.
	// Must have called findValidIGD() first to get
	// internal IP address.
	addPortForwardRule(&Urls, &IGDData,	my_local_ip);

	return 1;// Success
}





// Finds all UPnP devices on the local network. This is generally the first
// step you need to do for doing anything with UPnP since you need to find
// UPnP enabled devices to do anything with them.
// Stores found devices in struct UPNPDEV* UpnpDevicesList
void UPnP::findUPnPDevices()
{
	int miniupnp_error = 0;		// upnpDiscover() sends error info here.
	UpnpDevicesList = upnpDiscover(2000, nullptr, nullptr, 0, 0, &miniupnp_error);

	if (UpnpDevicesList)
	{
		if (global_verbose == true)
		{
			// Output to console the list of devices found. It is a linked list.
			for (UPNPDev* Device = UpnpDevicesList; Device != nullptr; Device = Device->pNext)
			{
				std::cout << "\nList of UPNP devices found on network:\n";
				std::cout << "description: " << Device->descURL << "\n";
				std::cout << "st: " << Device->st << "\n\n";
			}
		}
	}
	else
	{
		std::cout << "upnpDiscover() error: " << miniupnp_error << "\n";
	}
}

// Looks at the list of UPnP devices inside struct UPNPDEV* UpnpDeviceList after
// it has been filled out by findUPnPDevices() to determine if there is a valid IGD.
// After doing that it fills out struct UPNPUrls Urls and struct IGDdatas IGDDatas
// with information about the device and urls necessary to control it.
// This is generally the second step of the 2 important steps. The first step is to findUPnPDevices().
void UPnP::findValidIGD(struct UPNPDev* ListOfUPnPDevices, struct UPNPUrls* IGDUrls, struct IGDdatas* IGDDatas)
{
	// Looks at the list of UPnP devices returned by upnpDiscover() to determine if one is a valid IGD
	int value_check = UPNP_GetValidIGD(
				ListOfUPnPDevices,
				IGDUrls,
				IGDDatas,
				my_local_ip,
				sizeof(my_local_ip)
		);
	if (value_check)
	{
		if (global_verbose == true)
		{
			switch (value_check)
			{
			case 1:
				std::cout << "Valid IGD found : " << IGDUrls->controlURL << "\n";
				break;
			case 2:
				std::cout << "Found a (not connected?) IGD: " << IGDUrls->controlURL << "\n";
				std::cout << "attempting to continue\n";
				break;
			case 3:
				std::cout << "UPnP device found idk if it is an IGD: " << IGDUrls->controlURL << "\n";
				std::cout << "continuing regardless...\n";
				break;
			default:
				std::cout << "found device (igd?): " << IGDUrls->controlURL << "\n";
				std::cout << "continuing regardless...\n";
			}
		}
	}
}

// Display Information such as:
// External and local IP address
// Connection type
// Connection status, uptime, last connection error
// Time started
// Max bitrates
void UPnP::displayInformation(struct UPNPUrls * Urls, struct IGDdatas * Data)
{
	char connectionType[64];
	char status[64];
	char lastconnerr[64];
	unsigned int uptime;
	unsigned int brUp, brDown;
	time_t timenow, timestarted;
	int r;
	if (UPNP_GetConnectionTypeInfo(Urls->controlURL,
		Data->first.servicetype,
		connectionType) != UPNPCOMMAND_SUCCESS)
		printf("GetConnectionTypeInfo failed.\n");
	else
		printf("Connection Type : %s\n", connectionType);
	if (UPNP_GetStatusInfo(Urls->controlURL, Data->first.servicetype,
		status, &uptime, lastconnerr) != UPNPCOMMAND_SUCCESS)
		printf("GetStatusInfo failed.\n");
	else
		printf("Status          : %s, uptime=%us, LastConnectionError : %s\n",
			status, uptime, lastconnerr);
	timenow = time(nullptr);
	timestarted = timenow - uptime;
	printf("Time started    : %s", ctime(&timestarted));
	if (UPNP_GetLinkLayerMaxBitRates(Urls->controlURL_CIF, Data->CIF.servicetype,
		&brDown, &brUp) != UPNPCOMMAND_SUCCESS) {
		printf("GetLinkLayerMaxBitRates failed.\n");
	}
	else {
		printf("MaxBitRateDown  : %u bps", brDown);
		if (brDown >= 1000000) {
			printf(" (%u.%u Mbps)", brDown / 1000000, (brDown / 100000) % 10);
		}
		else if (brDown >= 1000) {
			printf(" (%u Kbps)", brDown / 1000);
		}
		printf("   MaxBitRateUp %u bps", brUp);
		if (brUp >= 1000000) {
			printf(" (%u.%u Mbps)", brUp / 1000000, (brUp / 100000) % 10);
		}
		else if (brUp >= 1000) {
			printf(" (%u Kbps)", brUp / 1000);
		}
		printf("\n");
	}
	r = UPNP_GetExternalIPAddress(Urls->controlURL,
		Data->first.servicetype,
		externalIPAddress);
	if (r != UPNPCOMMAND_SUCCESS) {
		printf("GetExternalIPAddress failed. (errorcode=%d)\n", r);
	}
	else {
		printf("External IP Addr : %s\n", externalIPAddress);
	}

	// my_local_ip is actually retrieved by findValidIGD()
	if (my_local_ip[0] != 0)
		std::cout << "Your local LAN IP: " << my_local_ip << "\n\n";
}


// Give it the struct IGDdatas IGDData because that is
// where the data is stored for the IGD.
// And give it struct UPNPUrls Urls because that is
// where urls for controlling the IGD is.
void UPnP::getListOfPortForwards(struct UPNPUrls * Urls, struct IGDdatas * IGDData)
{
	int r = 1;
	int i = 0;
	char index[6];
	char internal_client[40];
	char internal_port[6];
	char ext_port[6];
	char protocol[4];
	char description[80];
	char enabled[6];
	char remote_host[64];
	char lease_time[16];
	
	std::cout << "List of all port forwards on the IGD:\n";
	std::cout << " i protocol exPort  inAddr       inPort description leaseTime Remote_Host\n";
	do
	{
		// These are required idk why. Otherwise it will error 501.
		snprintf(index, 6, "%d", i);	// It is putting i in each index char slot up to a max of 6? so max 6 devices or what?
		remote_host[0] = '\0'; enabled[0] = '\0';
		lease_time[0] = '\0'; description[0] = '\0';
		ext_port[0] = '\0'; internal_port[0] = '\0'; internal_client[0] = '\0';

		// Retrieve the list of current Port Forwards
		// index is the only thing that requires your INput. Everything else is for the function to OUTput to.
		// What is the difference between this function and this one: UPNP_GetSpecificPortMappingEntry()
		r = UPNP_GetGenericPortMappingEntry(
			Urls->controlURL,
			IGDData->first.servicetype,
	/*in*/	index,
			ext_port,
			internal_client,
			internal_port,
			protocol,
			description,		// Description of the port forward entry. (this is supplied by the person who made the entry)
			enabled,
			remote_host,
			lease_time
			);

		// If the list was successfully retrieved, print to console
		if (r == 0)
		{
			if (global_verbose == true)
			{
				printf("%2d %s %10s->%s:%-5s '%s'     %s         '%s'\n",
					i, protocol, ext_port, internal_client, internal_port,
					description, lease_time, remote_host);
			}
		}
		else
			printf("GetGenericPortMappingEntry() returned %d (%s)\n",
				r, strupnperror(r));
		++i;
	} while (r == 0);

}


// Adds a new port forwarding rule.
// All traffick sent to the given external port will be forwarded to
// the specified host.
// Conceptually it looks something like this:
// internet -> ext_port 5783 -> internal_ip:internal_port 192.168.1.110:2405
void UPnP::addPortForwardRule(struct UPNPUrls * urls, struct IGDdatas * data, const char * my_local_ip)
{
	// Information necessary for UPNP_AddPortMapping()
	bool try_again = false;
	const int try_again_count_limit = 20;
	int try_again_count = try_again_count_limit;				// Limiting the number of attempts to make this work in certain fail cases.
	unsigned short i_internal_port = 0;
	unsigned short i_external_port = 0;
	char * internal_port = "7419";								// Some devices require that the internal and external ports must be the same.
	char * external_port = "7419";								// Some devices require that the internal and external ports must be the same.
	char * description_of_port_forward_entry = "CryptRelay";	// Describe what the entry is for.
	char * protocol = "TCP";									// TCP, UDP?
	char * leaseDuration = "0";								// Amount of time that the ports will be forwarded for. "0" == infinite. Time is in seconds.
																// Some NATs only allow a lease time of "0".

	do
	{
		if (global_verbose == true)
			std::cout << "Adding port forward entry...\n";
		// Add port forwarding now
		if (my_local_ip[0] != 0)
		{
			int r = UPNP_AddPortMapping(
				urls->controlURL,
				data->first.servicetype,
				external_port,
				internal_port,
				my_local_ip,
				description_of_port_forward_entry,
				protocol,
				0,					// Remote host, still not sure what this is really for / doing.
				leaseDuration
				);
			if (r != UPNPCOMMAND_SUCCESS)
			{
				switch (r)
				{
				case 718:	// Port forward entry conflicts with one that is in use by another client on the LAN.
				{
					if (global_verbose == true)
						std::cout << "Port forward entry conflicts with one that is in use by another client on the LAN. Improvising...\n";
					// It is safe to roll over the max since it is unsigned and it is handled correctly;
					if ( (i_internal_port == USHRT_MAX) && (i_external_port == USHRT_MAX) )
					{
						i_internal_port += 30001;	// arbitrary number to start trying again with.
						i_external_port += 30001;	// arbitrary number to start trying again with.
					}
					if (try_again_count < try_again_count_limit)
					{
						++i_internal_port;
						++i_external_port;
						++try_again_count;
						try_again = true;
					}
					else
					{
						try_again = false;
					}

					break;
				}
				case 724:	// External and internal ports must match
				{
					if (global_verbose == true)
						std::cout << "External and internal ports must match. Improvising...\n";
					i_internal_port = 30001;
					i_external_port = 30001;
					++try_again_count;	// Just making sure it doesn't get stuck in a loop doing this.
					try_again = true;

					break;
				}
				default:
					try_again = false;
				}

				// If we are out of attempts, or if UPNP_AddPortMapping() failed hard
				if (try_again == false)
				{
					printf("addPortForwardRule(ext: %s, intern: %s, local_ip: %s) failed with code %d (%s)\n",
						external_port, internal_port, my_local_ip, r, strupnperror(r));
				}
			}
			// Should I be using functions for IGD v2 if IGDv2 is available?
			// IGD:2 can be detected by sending out a message looking for InternetGatewayDevice:2
			// but that functionality is currently commented out in the library. I guess I could un-
			// comment it if needed. Assuming IGDv2 devices are backwards compatible for now since
			// I have no way to test it.
		}
	} while (try_again == true);
	

	// Things necessary for UPNP_GetSpecific_portMappingEntry() to output information to
	char intClient[40];
	char intPort[6];
	char duration[16];

	// Look at the specific port forward entry to see if it has actually been implemented
	// ... which it should have since it didn't error.
	// Needs the some of the same information that was given to UPNP_AddPortMapping()
	int r = UPNP_GetSpecificPortMappingEntry(
			urls->controlURL,
			data->first.servicetype,
			external_port,
			protocol,
			NULL/*remoteHost*/,
			intClient,
			intPort,
			NULL/*description*/,
			NULL/*enabled*/,
			duration
		);
	if (r != UPNPCOMMAND_SUCCESS)
	{
		printf("GetSpecificPortMappingEntry() failed with code %d (%s)\n",
			r, strupnperror(r));
	}

	if (intClient[0])
	{
		std::cout << "\n This port forward entry created and verified:\n";
		printf("InternalIP:Port = %s:%s\n", intClient, intPort);
		printf("external %s:%s %s is redirected to internal %s:%s (duration=%s)\n",
			externalIPAddress, external_port, protocol, intClient, intPort, duration);
	}
}