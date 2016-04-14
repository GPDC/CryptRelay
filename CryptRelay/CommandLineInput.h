//CommandLineInput.h
#ifndef CommandLineInput_h___
#define CommandLineInput_h___
#include <string>


class CommandLineInput
{
public:
	CommandLineInput();
	~CommandLineInput();

	
	int getCommandLineInput(int argc, char* argv[]);

	bool use_lan_only = false;	// default is false;
	bool use_upnp = true;	// default is true;

	std::string target_ip_address;
	std::string target_port;
	std::string my_ip_address;
	std::string my_host_port;
	std::string my_ext_ip_address;

	struct UPnPDeleteSpecificPortForward
	{
		std::string extern_port;
		std::string protocol;
	};

protected:
private:
	void helpAndReadMe();
};











#endif
