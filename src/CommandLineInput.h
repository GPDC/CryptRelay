//CommandLineInput.h

// Overview:
// The purpose of this class is to parse command line input.

#ifndef CommandLineInput_h__
#define CommandLineInput_h__
#include <string>


class CommandLineInput
{
public:
	CommandLineInput();
	virtual ~CommandLineInput();

	// Set the appropriate variables based on the user's argv[] input.
	// Returns 0, success
	// Returns -1, please exit the program.
	int32_t setVariablesFromArgv(int32_t argc, char* argv[]);


protected:
private:

	// Prevent anyone from copying this class
	CommandLineInput(CommandLineInput& CommandLineInputInstance) = delete; // Delete copy operator
	CommandLineInput& operator=(CommandLineInput& CommandLineInstance) = delete; // Delete assignment operator

	bool verbose_output = false; // -v 	        // Turn on and off verbose output for this class.
	bool show_info_upnp = false; // -i
	bool retrieve_list_of_port_forwards = false; // -s
	bool use_lan_only = false; // --lan
	bool use_upnp_to_connect_to_peer = true;	// Assume UPnP wants to be used unless the user makes this false.
	std::string target_ip_address;
	std::string target_port;
	std::string my_ip_address;
	std::string my_host_port;
	std::string my_ext_ip_address;

	// Specific to -d
	bool delete_this_specific_port_forward = false;
	std::string delete_this_specific_port_forward_port;
	std::string delete_this_specific_port_forward_protocol;

	void displayHelpAndReadMe();
	void displayExamples();

	
public:

	// Accessors:
	const bool& getShowInfoUpnp() { return show_info_upnp; }
	const bool& getRetrieveListOfPortForwards() { return retrieve_list_of_port_forwards; }
	const bool& getUseLanOnly() {	return use_lan_only; }
	const bool& getUseUpnpToConnectToPeer() {	return use_upnp_to_connect_to_peer;	}
	const std::string& getTargetIpAddress() {	return target_ip_address; }
	const std::string& getTargetPort() { return target_port; }
	const std::string& getMyIpAddress() {	return my_ip_address; }
	const std::string& getMyHostPort() { return my_host_port; }
	const std::string& getMyExtIpAddress() { return my_ext_ip_address; }
	// Specific to -d
	const bool& getDeleteThisSpecificPortForward() { return delete_this_specific_port_forward; }
	const std::string& getDeleteThisSpecificPortForwardPort() { return delete_this_specific_port_forward_port; }
	const std::string& getDeleteThisSpecificPortForwardProtocol() { return delete_this_specific_port_forward_protocol; }
	// Specific to -v
	const bool& getVerboseOutput() { return verbose_output; }
};

#endif// CommandLineInput_h__
