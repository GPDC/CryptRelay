//CommandLineInput.cpp
#include <string>
#include <iostream>
#include <vector>

#include "CommandLineInput.h"
#include "GlobalTypeHeader.h"
#include "connection.h"
#include "FormatCheck.h"

const std::string CommandLineInput::THE_DEFAULT_PORT = "7001";
const std::string CommandLineInput::THE_DEFAULT_IPADDRESS = "1.2.3.4";

CommandLineInput::CommandLineInput()
{
}
CommandLineInput::~CommandLineInput()
{
}

void CommandLineInput::helpAndReadMe()
{

	std::cout << "Proper format is:   cryptrelay.exe -t 192.168.27.50 -tp 7172 -m 192.168.1.101 -mp 7172\n";
	std::cout << "\n";
	std::cout << "-h	help		Displays the readme\n";
	std::cout << "-tE	target		The target's IP address.\n";
	std::cout << "-tp	targetport	The target's port number.\n";
	std::cout << "-tL	targetLocal	The target's local IP address.\n";
	std::cout << "-mL	me		Your IP address that you want to listen on.\n";
	std::cout << "-mp	myport		Your port number that you want to listen on.\n";
	std::cout << "-mE	myExternal	Your External ip address.\n";
	std::cout << "-v	verbose		Displays a lot of text output on screen.\n";
	std::cout << "\nNOT YET IMPLEMENTED\n";
	std::cout << "-f	file		The file, and location of the file you wish to xfer.\n";
	std::cout << "\nTo exit the program, please type 'cryptrelay.exit' at any time.\n\n\n";
	return;
}

int CommandLineInput::getCommandLineInput(int argc, char* argv[])
{
	// If necessary, a more thorough checking of command line input's individual chars is in my ParseText program.
	//	 but for now this is simple and easy to read/understand, so its nice.
	int err_chk;
	int arg_size;

	target_extrnl_ip_address = "3.3.3.3";
	target_port = "7777";
	target_local_ip = "192.168.1.222";
	my_ip_address = TCPConnection::DEFAULT_IP_TO_LISTEN;
	my_host_port = TCPConnection::DEFAULT_PORT_TO_LISTEN;
	my_ext_ip_address = "2.2.2.2";

	std::vector<std::string> arg;

	IPAddress IPAdressFormatCheck;

	// Put all argv's into a vector so they can be compared to strings	// I think this is a bit silly, could use strcmp?
	for (int i = 0; i < argc; i++)
	{
		arg.push_back(argv[i]);
	}
	arg_size = arg.size();

	// If no command line arguments supplied, show the ReadMe
	if (argc <= 1)
	{
		helpAndReadMe();
		return 0;
	}
	// Check all argv inputs to see what the user wants to do
	if (argc >= 2)
	{
		for (int i = 1; i < argc; ++i)
		{
			if (arg[i] == "-h"
				|| arg[i] == "-H"
				|| arg[i] == "-help"
				|| arg[i] == "-Help"
				|| arg[i] == "help"
				|| arg[i] == "readme"
				|| arg[i] == "--help"
				|| arg[i] == "--Help")
			{
				helpAndReadMe();
				return 0;
			}
			else if (arg[i] == "-tE" && i < arg_size - 1)
			{
				err_chk = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk == false)
					return 0;
				else
				{
					target_extrnl_ip_address = argv[i + 1];
					++i;	// Because we already took the information from i + 1, there is no need to check what i + 1 is, so we skip it by doing ++i;
				}
			}
			else if (arg[i] == "-tp" && i < arg_size - 1)
			{
				err_chk = IPAdressFormatCheck.isPortFormatCorrect(argv[i + 1]);
				if (err_chk == false)
					return 0;
				else
				{
					target_port = argv[i + 1];
					++i;
				}
			}
			else if (arg[i] == "-tL" && i < arg_size - 1)
			{
				err_chk = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk == false)
					return 0;
				else
				{
					target_local_ip = argv[i + 1];
					++i;
				}
			}
			else if (arg[i] == "-mL" && i < arg_size - 1)
			{
				err_chk = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk == false)
					return 0;
				else
				{
					my_ip_address = argv[i + 1];
					++i;
				}
			}
			else if (arg[i] == "-mp" && i < arg_size - 1)
			{
				err_chk = IPAdressFormatCheck.isPortFormatCorrect(argv[i + 1]);
				if (err_chk == false)
					return 0;
				else
				{
					my_host_port = argv[i + 1];
					++i;
				}
			}
			else if (arg[i] == "-mE" && i < arg_size - 1)
			{
				err_chk = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk == false)
					return 0;
				else
				{
					my_ext_ip_address = argv[i + 1];
					++i;
				}
			}
			else if (arg[i] == "-v")
			{
				global_verbose = true;
			}
			else if (arg[i] == "-f")
			{
				std::cout << "-f hasn't been implemented yet.\n";
				return 0;
			}
			else
			{
				helpAndReadMe();
				return 0;
			}
		}
	}

	// Finished without errors, return success
	return 1;
}