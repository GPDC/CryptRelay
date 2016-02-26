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

	std::string target_ip_address;
	std::string target_port;
	std::string my_ip_address;
	std::string my_host_port;

protected:
private:
	void helpAndReadMe(int argc);

	static const std::string THE_DEFAULT_PORT;
	static const std::string THE_DEFAULT_IPADDRESS;

};











#endif
