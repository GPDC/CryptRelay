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

public:

	// Accessors:
	const bool& getShowInfoUpnp()	{ return show_info_upnp; }
	const bool& getRetrieveListOfPortForwards() { return retrieve_list_of_port_forwards; }
	const bool& getUseLanOnly() {	return use_lan_only; }
	const bool& getUseUpnpToConnectToPeer() {	return use_upnp_to_connect_to_peer;	}
	const std::string& getTargetIpAddress() {	return target_ip_address; }
	const std::string& getTargetPort() { return target_port; }
	const std::string& getMyIpAddress() {	return my_ip_address; }
	const std::string& getMyHostPort() { return my_host_port; }
	const std::string& getMyExtIpAddress() { return my_ext_ip_address; }
	// Specific to -dpf
	const bool& getDeleteThisSpecificPortForward() { return delete_this_specific_port_forward; }
	const std::string& getDeleteThisSpecificPortForwardPort() { return delete_this_specific_port_forward_port; }
	const std::string& getDeleteThisSpecificPortForwardProtocol() { return delete_this_specific_port_forward_protocol; }
};

#endif
