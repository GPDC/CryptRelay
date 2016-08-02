//CommandLineInput.h

// Overview:
// The purpose of this class is to parse command line input.

#ifndef CommandLineInput_h___
#define CommandLineInput_h___
#include <string>


class CommandLineInput
{
public:
	CommandLineInput();
	virtual ~CommandLineInput();

	// Set the appropriate variables based on the user's argv[] input.
	bool setVariablesFromArgv(int32_t argc, char* argv[]);

	// These are used as a way to get the private member variables
	// while making it clear they can't be changed outside the class.
	const bool& getShowInfoUpnp();
	const bool& getRetrieveListOfPortForwards();
	const bool& getUseLanOnly();
	const bool& getUseUpnpToConnectToPeer();
	const std::string& getTargetIpAddress();
	const std::string& getTargetPort();
	const std::string& getMyIpAddress();
	const std::string& getMyHostPort();
	const std::string& getMyExtIpAddress();
	// Specific to -dpf
	const bool& getDeleteThisSpecificPortForward();
	const std::string& getDeleteThisSpecificPortForwardPort();
	const std::string& getDeleteThisSpecificPortForwardProtocol();


protected:
private:

	void displayHelpAndReadMe();
	void displayExamples();

	// These private member variables are able to be viewed outside this class
	// by using the appropriate corresponding public function.
	bool show_info_upnp = false;
	bool retrieve_list_of_port_forwards = false;
	bool use_lan_only = false;
	bool use_upnp_to_connect_to_peer = true;	// Connection class will always want to use upnp unless the user makes this false
	std::string target_ip_address;
	std::string target_port;
	std::string my_ip_address;
	std::string my_host_port;
	std::string my_ext_ip_address;
	// Specific to -dpf
	bool delete_this_specific_port_forward = false;
	std::string delete_this_specific_port_forward_port;
	std::string delete_this_specific_port_forward_protocol;
};











#endif
