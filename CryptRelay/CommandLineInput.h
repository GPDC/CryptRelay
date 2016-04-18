//CommandLineInput.h

// Overview:
// The purpose of this class is to parse command line input.

// Terminology:
//




#ifndef CommandLineInput_h___
#define CommandLineInput_h___
#include <string>


class CommandLineInput
{
public:
	CommandLineInput();
	~CommandLineInput();

	
	int getCommandLineInput(int argc, char* argv[]);

	bool delete_this_specific_port_forward = false;
	std::string delete_this_specific_port_forward_port;
	std::string delete_this_specific_port_forward_protocol;

	bool get_list_of_port_forwards = false;
	bool use_lan_only = false;
	bool use_upnp_to_connect_to_peer = true;	// ChatProgram will always want to use upnp unless the user makes this false

	std::string target_ip_address;
	std::string target_port;
	std::string my_ip_address;
	std::string my_host_port;
	std::string my_ext_ip_address;

	///* currently unused */
	//struct UPnPDeleteSpecificPortForward
	//{
	//	std::string extern_port;
	//	std::string protocol;
	//};

protected:
private:

	void helpAndReadMe();
	void Examples();
};











#endif
