// UPnP.cpp
#ifdef __linux__
#include <string.h> //memset
#include <errno.h>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <string>
#include <time.h>	// needed for localtime_s and localtime_r
#include <limits.h>	// macros for int32_t, short, char, etc that are defined as their min/max values.

#include "UPnP.h"
#include "GlobalTypeHeader.h"

//miniupnp library
#include "upnpcommands.h"
#include "upnperrors.h"
#endif//__linux__

#ifdef _WIN32
#include <WinSock2.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <time.h>	// needed for localtime_s and localtime_r
#include <limits.h>	// macros for int32_t, short, char, etc that are defined as their min/max values.

#include "UPnP.h"
#include "GlobalTypeHeader.h"

//miniupnp library
#include "upnpcommands.h"
#include "upnperrors.h"
#endif//_WIN32


#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")      // NEEDED for miniupnp library
#pragma comment(lib, "Iphlpapi.lib")    // NEEDED for miniupnp library
#endif//_WIN32

UPnP::UPnP()
{
	// Making sure there is no arbitrary data in the structs
	memset(&Urls, 0, sizeof(Urls));
	memset(&IGDData, 0, sizeof(IGDData));
}

UPnP::~UPnP()
{
	// Delete port forwarding rule that was created by
	// standaloneAutoAddPortForwardRule() in order to use CryptRelay
	if (port_forward_automatically_added == true)
		autoDeletePortForwardRule();

	// Free everything because we are done with them
	if (global_verbose == true)
		std::cout << "Freeing allocated UPnP memory.\n";
	freeUPNPDevlist(UpnpDevicesList);
	FreeUPNPUrls(&Urls);

	// Done with Sockets, clean it up.
	// This is already done in the SocketClass deconstructor
}


// This is for when the user simply wants a list of all the
// port forwards currently on his router, not starting 
// the chat program or anything else.
void UPnP::standaloneGetListOfPortForwards()
{
	// Enable socket use on Windows
	// This is already done in the SocketClass constructor.

	// Find UPnP devices on the local network
	findUPnPDevices();

	// Find valid IGD based off the list returned by upnpDiscover()
	if (findValidIGD() == true)
		return;

	// Display the list of port forwards
	getListOfPortForwards();
}


// If the user wants to delete a specfic port forward on his router,
// then he can call this, specifying which one to delete.
// With the current setup this means it is done via the
// CommandLineInput class a.k.a. CLI.
// In there the user inputs the options via command line
void UPnP::standaloneDeleteThisSpecificPortForward(const char * extern_port, const char* internet_protocol)
{
	// Enable socket use on Windows
	// This is already done in the SocketClass constructor.

	// Find UPnP devices on the local network
	findUPnPDevices();

	// Find valid IGD based off the list returned by upnpDiscover()
	if (findValidIGD() == true)
		return;

	int32_t errchk = UPNP_DeletePortMapping(
			Urls.controlURL,
			IGDData.first.servicetype,
			extern_port,
			internet_protocol,
			0
		);
	if (errchk != UPNPCOMMAND_SUCCESS)
	{
		if (errchk == 402)
			std::cout << "Error: " << errchk << ". Invalid Arguments supplied to UPNP_DeletePortMapping.\n";
		else if (errchk == 714)
			std::cout << "Error: " << errchk << ". The port forward rule specified for deletion does not exist.\n";
		else
			std::cout << "Error: " << errchk << ". UPNP_DeletePortMapping() failed. Router might not have this feature?\n";
	}
	std::cout << "Port forward rule: " << internet_protocol << " " << extern_port << " successfully deleted.\n";
}


// Finds all UPnP devices on the local network. This is generally the first
// step you need to do for doing anything with UPnP since you need to find
// UPnP enabled devices to do anything with them.
// Stores found devices in struct UPNPDEV* UpnpDevicesList
void UPnP::findUPnPDevices()
{
	int32_t miniupnp_error = 0;		// upnpDiscover() sends error info here.
	const int32_t TTL_DURATION = 2;
	const int32_t DISABLE_IPV6 = 0;
	//const int32_t ENABLE_IPV6 = 1;
	const int32_t WAIT_TIME = 2000; // max time in milliseconds to wait for a response from device
	UpnpDevicesList = upnpDiscover(WAIT_TIME, nullptr, nullptr, UPNP_LOCAL_PORT_ANY, DISABLE_IPV6, TTL_DURATION, &miniupnp_error);

	if (UpnpDevicesList)
	{
		if (global_verbose == true)
		{
			// Output to console the list of devices found. It is a linked list.
			for (UPNPDev* Device = UpnpDevicesList; Device != nullptr; Device = Device->pNext)
			{
				std::cout << "\n# List of UPNP devices found on network:\n";
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
bool UPnP::findValidIGD()
{
	// Looks at the list of UPnP devices returned by upnpDiscover() to determine if one is a valid IGD
	int32_t errchk = UPNP_GetValidIGD(
				UpnpDevicesList,
				&Urls,
				&IGDData,
				my_local_ip,
				sizeof(my_local_ip)
		);
	if (errchk == 0)
	{
		std::cout << "No valid IGD found.\n";
		return true;
	}
	else
	{
		if (global_verbose == true)
		{
			switch (errchk)
			{
			case 1:
				std::cout << "# Valid IGD found: " << Urls.controlURL << "\n\n";
				break;
			case 2:
				std::cout << "Found a (not connected?) IGD: " << Urls.controlURL << "\n";
				std::cout << "attempting to continue\n";
				break;
			case 3:
				std::cout << "UPnP device found idk if it is an IGD: " << Urls.controlURL << "\n";
				std::cout << "continuing regardless...\n";
				break;
			default:
				std::cout << "found device (igd?): " << Urls.controlURL << "\n";
				std::cout << "continuing regardless...\n";
			}
		}
	}

	return false;
}

// Display Information such as:
// External and local IP address
// Connection type
// Connection status, uptime, last connection error
// Time started
// Max bitrates
void UPnP::showInformation()
{
	char connection_type[64];
	char status[64];
	char last_connection_error[64];
	uint32_t uptime;
	uint32_t bitrate_up, bitrate_down;
	int32_t errchk;

	std::cout << "# Displaying various information:\n";

	// Display connection type
	errchk = UPNP_GetConnectionTypeInfo(
			Urls.controlURL,
			IGDData.first.servicetype,
			connection_type
		);
	if (errchk != UPNPCOMMAND_SUCCESS)
		std::cout << "GetConnectionTypeInfo failed.\n";
	else
		std::cout << "Connection Type : " << connection_type << "\n";

	// Display status, uptime, last connection error
	errchk = UPNP_GetStatusInfo(
			Urls.controlURL,
			IGDData.first.servicetype,
			status,
			&uptime,
			last_connection_error
		);
	if (errchk != UPNPCOMMAND_SUCCESS)
		std::cout << "GetStatusInfo failed.\n";
	else
		std::cout << "Status          : " << status << ", uptime: " << uptime << ", LastConnectionError: " << last_connection_error << "\n";


	// Display time started
	displayTimeStarted(uptime);


	// Display Max bit rates
	errchk = UPNP_GetLinkLayerMaxBitRates(
			Urls.controlURL_CIF,
			IGDData.CIF.servicetype,
			&bitrate_down,
			&bitrate_up
		);
	if (errchk != UPNPCOMMAND_SUCCESS)
		std::cout << "GetLinkLayerMaxBitRates failed.\n";
	else
	{
		printf("MaxBitRateDown  : %u bps", bitrate_down);
		if (bitrate_down >= 1000000)
			printf(" (%u.%u Mbps)", bitrate_down / 1000000, (bitrate_down / 100000) % 10);
		else if (bitrate_down >= 1000)
			printf(" (%u Kbps)", bitrate_down / 1000);
		std::cout << "\n";

		printf("MaxBitRateUp    : %u bps", bitrate_up);
		if (bitrate_up >= 1000000)
			printf(" (%u.%u Mbps)", bitrate_up / 1000000, (bitrate_up / 100000) % 10);
		else if (bitrate_up >= 1000)
			printf(" (%u Kbps)", bitrate_up / 1000);
		std::cout << "\n";
	}

	// Get external IP address
	memset(my_external_ip, 0, sizeof(my_external_ip));
	errchk = UPNP_GetExternalIPAddress(
			Urls.controlURL,
			IGDData.first.servicetype,
			my_external_ip
		);
	if (errchk != UPNPCOMMAND_SUCCESS)
		printf("GetExternalIPAddress failed. (errorcode:%d)\n", errchk);
	else
		std::cout << "External IP Addr: " << my_external_ip << "\n";

	// my_local_ip is actually retrieved by findValidIGD()
	if (my_local_ip[0] != 0)
		std::cout << "Your local LAN IP: " << my_local_ip << "\n\n";
}

void UPnP::standaloneShowInformation()
{

	// Find UPnP devices on the local network
	findUPnPDevices();

	// Find valid IGD based off the list returned by upnpDiscover()
	if (findValidIGD() == true)
		return;

	// Output information to the console
	showInformation();
}

// Given the amount of uptime, calculate the time at which
// the connection(?) has started.
void UPnP::displayTimeStarted(uint32_t uptime)
{
#ifdef commentout // disabling this for now
	char am_pm[] = "AM";
	char timebuf[26];
	time_t TimeNow;
	time_t TimeStarted;
	
	// Get and place the current time inside TimeNow
	time(&TimeNow);

	// Calculate time
	TimeStarted = TimeNow - uptime;
	
	// Convert to local time.
	tm StorageForLocalTimeData;	// localtime_s() and localtime_r() store time data in here.
	memset(&StorageForLocalTimeData, 0, sizeof(StorageForLocalTimeData));
#ifdef _WIN32
        errno_t err;
	err = localtime_s(&StorageForLocalTimeData, &TimeNow);
	if (err)
		std::cout << "Invalid argument to localtime_s.";
#endif//_WIN32
#ifdef __linux__
        tm* err;
	err = localtime_r(&TimeNow, &StorageForLocalTimeData);
	if (err)
		std::cout << "Invalid argument to localtime_r.";
#endif//__linux__

	if (StorageForLocalTimeData.tm_hour > 12)				// Set up extension. 
                memcpy(am_pm, "PM", sizeof(am_pm) );
		//strcpy_s(am_pm, sizeof(am_pm), "PM");
	if (StorageForLocalTimeData.tm_hour > 12)				// Convert from 24-hour 
		StorageForLocalTimeData.tm_hour -= 12;				// to 12-hour clock. 
	if (StorageForLocalTimeData.tm_hour == 0)				// Set hour to 12 if midnight.
		StorageForLocalTimeData.tm_hour = 12;

	// Convert to an ASCII representation.
#ifdef _WIN32
	err = asctime_s(timebuf, 26, &StorageForLocalTimeData);
	if (err)
		printf("Invalid argument to asctime_s.");
#endif//_WIN32
        
        /*
#ifdef __linux__
        err = asctime_r(timebuf, &StorageForLocalTimeData);// asctime can be overflowed. do not use.
	if (err)
		printf("Invalid argument to asctime_r.");
#endif//__linux__
        */
        
	// Output to console
	std::cout << "Time started    : ";
	printf("%.19s %s\n", timebuf, am_pm);				// %.19s just means that it will print out a max of 19 chars
#endif
}

// Give it the struct IGDdatas IGDData because that is
// where the data is stored for the IGD.
// And give it struct UPNPUrls Urls because that is
// where urls for controlling the IGD is.
void UPnP::getListOfPortForwards()
{
	int32_t errchk = 1;
	int32_t i = 0;
	char index[6];
	char internal_client[40];
	char internal_port[6];
	char ext_port[6];
	char internet_protocol[4];
	char description[80];
	char enabled[6];
	char remote_host[64];
	char lease_time[16];
	

	std::cout << "# List of all port forwards on the IGD:\n";
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
		errchk = UPNP_GetGenericPortMappingEntry(
			Urls.controlURL,
			IGDData.first.servicetype,
	/*in*/	index,
			ext_port,
			internal_client,
			internal_port,
			internet_protocol,
			description,		// Description of the port forward entry. (this is supplied by the person who made the entry)
			enabled,
			remote_host,
			lease_time
			);

		// If the list was successfully retrieved, print to console
		if (errchk == 0)
		{
			printf("%2d %s %10s->%s:%-5s '%s'     %s         '%s'\n",
				i, internet_protocol, ext_port, internal_client, internal_port,
				description, lease_time, remote_host);
		}
		else
			printf("GetGenericPortMappingEntry() returned %d (%s)\n",
				errchk, strupnperror(errchk));
		++i;
	} while (errchk == 0);

}

// Add a port forwarding rule.
// Must have called findValidIGD() first to get
// your internal IP address, and to have an IGD to talk to.
bool UPnP::autoAddPortForwardRule()
{
	// Information necessary for UPNP_AddPortMapping()
	bool try_again = false;
	const int32_t try_again_count_limit = 20;
	int32_t try_again_count = 0;					// Limiting the number of attempts to make this work in certain fail cases.
	const char * description_of_port_forward_entry = "CryptRelay";	// Describe what the port forward entry is for.

	// Amount of time (seconds) that the ports will be forwarded for. "0" == infinite.
	// Some NATs only allow a lease time of "0".
	const char * lease_duration = "57600";	// 57600 == 16 hrs

	std::cout << "Automatically adding port forward rule... ";
	// Add the port forwarding rule
	do
	{
		// Add port forwarding now
		if (my_local_ip[0] != 0)
		{
			int32_t errchk = UPNP_AddPortMapping(
					Urls.controlURL,
					IGDData.first.servicetype,
					upnp_my_external_port.c_str(),
					upnp_my_internal_port.c_str(),
					my_local_ip,
					description_of_port_forward_entry,
					protocol,
					0,					// Remote host, still not sure what this is really for / doing.
					lease_duration
				);
			if (errchk != UPNPCOMMAND_SUCCESS)
			{
				switch (errchk)
				{
				// 718 == Port forward entry conflicts with one that is in use by another client on the LAN.
				// 501 == Action failed. The router is telling us this error, not the library. It could be that
				// the router doesn't supply detailed error info; it is just reporting that the action failed.
				// Ergo we try again assuming that its just because the port is in use by another client on the LAN.
				case 501:
				{
					// This case will be tried a maximum of try_again_count_limit times.
					// The port will add +1 to itself every time it encounters this case.
					if (global_verbose == true)
						std::cout << "Port forward entry conflicts with one that is in use by another client on the LAN. Improvising...\n";

					// Making it an integer for easy manipulation
					int32_t i_internal_port = stoi(upnp_my_internal_port);
					int32_t i_external_port = stoi(upnp_my_external_port);

					// Making sure we don't ++ over the maximum size of a unsigned short
					if ((i_internal_port < USHRT_MAX && i_external_port < USHRT_MAX)
						&& (i_internal_port > 0 && i_external_port > 0))
					{
						++i_internal_port;
						++i_external_port;
					}
					else // Must have been too big to be a port, let's just give it an arbitrary port number.
					{
						i_internal_port = 30207;
						i_external_port = 30207;
					}

					// Convert it back to string
					upnp_my_internal_port = std::to_string(i_internal_port);
					upnp_my_external_port = std::to_string(i_external_port);
					if (global_verbose == true)
					{
						std::cout << "Now trying with: upnp_my_external_port == " << upnp_my_external_port << "\n";
					}
					++try_again_count;
					try_again = true;

					// Making sure error info is displayed even after that many attempts has failed.
					if (try_again_count == try_again_count_limit)
					{
						std::cout << "case 501: ";
						printf("addPortForwardRule(ext: %s, intern: %s, local_ip: %s) failed with code %d (%s)\n",
							upnp_my_external_port.c_str(), upnp_my_internal_port.c_str(), my_local_ip, errchk, strupnperror(errchk));
						try_again = false;
					}

					break;
				}
				// 718 == Port forward entry conflicts with one that is in use by another client on the LAN.
				case 718:
				{
					// This case will be tried a maximum of try_again_count_limit times.
					// The port will add +1 to itself every time it encounters this case.
					if (global_verbose == true)
						std::cout << "Port forward entry probably conflicts with one that is in use by another client on the LAN. Improvising...\n";

					// Making it an integer for easy manipulation
					int32_t i_internal_port = stoi(upnp_my_internal_port);
					int32_t i_external_port = stoi(upnp_my_external_port);

					// Making sure we don't ++ over the maximum size of a unsigned short
					if ( (i_internal_port < USHRT_MAX && i_external_port < USHRT_MAX)
						&& (i_internal_port > 0 && i_external_port > 0) )
					{
						++i_internal_port;
						++i_external_port;
					}
					else // Must have been too big to be a port, let's just give it an arbitrary port number.
					{
						i_internal_port = 30207;
						i_external_port = 30207;
					}

					// Convert it back to string
					upnp_my_internal_port = std::to_string(i_internal_port);
					upnp_my_external_port = std::to_string(i_external_port);

					if (global_verbose == true)
					{
						std::cout << "Now trying with: upnp_my_external_port == " << upnp_my_external_port << "\n";
					}
					++try_again_count;
					try_again = true;

					// Making sure error info is displayed even after that many attempts has failed.
					if (try_again_count == try_again_count_limit)
					{
						std::cout << "case 718: ";
						printf("addPortForwardRule(ext: %s, intern: %s, local_ip: %s) failed with code %d (%s)\n",
							upnp_my_external_port.c_str(), upnp_my_internal_port.c_str(), my_local_ip, errchk, strupnperror(errchk));
						try_again = false;
					}

					break;
				}
				case 724:	// External and internal ports must match
				{
					if (global_verbose == true)
						std::cout << "External and internal ports must match. Improvising...\n";

					upnp_my_external_port = upnp_my_internal_port;

					++try_again_count;	// Just making extra sure it doesn't get stuck in a loop doing this.
					try_again = true;

					break;
				}
				case 725:	// lease_duration is only supported as "0", aka infinite, on this NAT
				{
					if (global_verbose == true)
						std::cout << "A timed lease duration is not supported on this NAT. Trying with an infinite duration instead.\n";
					lease_duration = "0";

					++try_again_count;	// Just making extra sure it doesn't get stuck in a loop doing this.
					try_again = true;

					break;
				}
				default:	// Non-recoverable error
					try_again = false;
				}//end switch

				 // If we are out of attempts, or if UPNP_AddPortMapping() failed hard
				if ((try_again == false) || (try_again_count == try_again_count_limit))
				{
					std::cout << "UPNP hard fail. try_again_count: " << try_again_count << ", bool try_again: " << try_again << "\n";
					printf("addPortForwardRule(ext: %s, intern: %s, local_ip: %s) failed with code %d (%s)\n",
						upnp_my_external_port.c_str(), upnp_my_internal_port.c_str(), my_local_ip, errchk, strupnperror(errchk));
					return false;
				}
			}
			else//success
			{
				// Set flag to say that the automatic port forward was a success.
				port_forward_automatically_added = true;
				try_again = false;
				std::cout << "Success\n";
			}


			// Should I be using functions for IGD v2 if IGDv2 is available?
			// IGD:2 can be detected by sending out a message looking for InternetGatewayDevice:2
			// but that functionality is currently commented out in the library. I guess I could un-
			// comment it if needed. I'm assuming IGDv2 devices are backwards compatible for now since
			// I have no way to test it.
		}
	} while ((try_again == true) && (try_again_count < try_again_count_limit));


	// Display the port forwarding entry that was just added
	// This should probably be its own function...
	if (global_verbose == true)
	{
		// Things necessary for UPNP_GetSpecific_portMappingEntry() to output information to
		char intClient[40];
		char intPort[6];
		char duration[16];

		// Displays the specific port forward entry to see if it has actually been implemented
		// ... which it should have since it didn't error.
		// Needs some of the same information that was given to UPNP_AddPortMapping()
		int32_t errchk = UPNP_GetSpecificPortMappingEntry(
			Urls.controlURL,
			IGDData.first.servicetype,
			/*in*/	upnp_my_external_port.c_str(),
			/*in*/	protocol,
			/*in*/	NULL/*remoteHost*/,
			intClient,
			intPort,
			NULL/*description*/,
			NULL/*enabled*/,
			duration
			);
		if (errchk != UPNPCOMMAND_SUCCESS)
		{
			printf("GetSpecificPortMappingEntry() failed with code %d (%s)\n",
				errchk, strupnperror(errchk));
		}

		if (intClient[0])
		{
			std::cout << "# Displaying port forward entry that was just added...\n";
			printf("External IP:Port %s:%s %s is redirected to internal IP:Port %s:%s (duration: %s seconds)\n\n",
				my_external_ip, upnp_my_external_port.c_str(), protocol, intClient, intPort, duration);
		}
	}

	return false; // Success
}

// Adds a new port forwarding rule.
// Displays extra information if global_verbose == true.
bool UPnP::standaloneAutoAddPortForwardRule()
{

	// Enable socket use on Windows
	// this is already done in SocketClass constructor

	// Find UPnP devices on the local network
	findUPnPDevices();

	// Find a valid IGD based off the list filled out by findUPnPDevices()
	if (findValidIGD() == true)
		return true;

	// Displays various extra information gathered through UPnP
	if (global_verbose == true)
		showInformation();

	// Get list of currently mapped ports for the IGD and output to console.
	if (global_verbose == true)
		getListOfPortForwards();

	// Add the port forward rule
	if (autoAddPortForwardRule() == true)
		return true;

	return false;// Success
}

// Deletes the port forwarding rule created by
// autoAddPortForwardingRule()
void UPnP::autoDeletePortForwardRule()
{
	std::cout << "Automatically deleting port forward rule... ";

	// Delete the port forward rule
	int32_t errchk = UPNP_DeletePortMapping(
			Urls.controlURL,
			IGDData.first.servicetype,
			upnp_my_external_port.c_str(),
			protocol,
			0				// Remote Host
		);
	if (errchk != UPNPCOMMAND_SUCCESS)
	{
		if (errchk == 402)
			std::cout << "Error: " << errchk << ". Invalid Arguments supplied to UPNP_DeletePortMapping.\n";
		else if (errchk == 714)
		{
			std::cout << "Error: " << errchk << ". The port forward rule specified for deletion does not exist.\n";
			std::cout << "Specified external port: " << upnp_my_external_port << "and Protocol: " << protocol << "\n";
		}
		else
			std::cout << "Error: " << errchk << ". UPNP_DeletePortMapping() failed.\n";
	}
	else
	{
		std::cout << "Success\n";
	}
}
