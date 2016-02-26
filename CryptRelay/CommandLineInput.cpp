//CommandLineInput.cpp
#include <string>
#include <iostream>
#include <vector>

#include "CommandLineInput.h"
#include "GlobalTypeHeader.h"
#include "connection.h"
#include "FormatCheck.h"

CommandLineInput::CommandLineInput()
{
}
CommandLineInput::~CommandLineInput()
{
}

void CommandLineInput::helpAndReadMe(int argc)
{

	std::cout << "Proper format is:   cryptrelay.exe -t 192.168.27.50 -tp 7172 -m 192.168.1.101 -mp 7172\n";
	std::cout << "\n";
	std::cout << "-h:	help		Displays the readme\n";
	std::cout << "-t:	target		The target's IP address.\n";
	std::cout << "-tp:	targetport	The target's port number.\n";
	std::cout << "-m:	me		Your IP address that you want to listen on.\n";
	std::cout << "-mp:	myport		Your port number that you want to listen on.\n";
	std::cout << "-v:	verbose		Displays a lot of text output on screen.\n";
	std::cout << "\nNOT YET IMPLEMENTED:\n";
	std::cout << "-f:	file		The file, and location of the file you wish to xfer.\n";
	std::cout << "\nTo exit the program, please type 'cryptrelay.exit' at any time.\n\n\n";
	return;
}

int CommandLineInput::getCommandLineInput(int argc, char* argv[])
{
	// If necessary, a more thorough checking of command line input's individual chars is in my ParseText program.
	//	 but for now this is simple and easy to read/understand, so its nice.
	int err_chk;
	int arg_size;

	target_ip_address = "";
	target_port = "";
	my_ip_address = connection::DEFAULT_IP_TO_LISTEN;
	my_host_port = connection::DEFAULT_PORT_TO_LISTEN;

	std::vector<std::string> arg;

	IPAddress IPAdressFormatCheck;

	// Put all argv's into a vector so they can be compared to strings
	for (int i = 0; i < argc; i++)
	{
		arg.push_back(argv[i]);
	}
	arg_size = arg.size();

	// If command line arguments supplied, show the ReadMe
	if (argc <= 1)
	{
		helpAndReadMe(argc);
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
				|| arg[i] == "readme")
			{
				helpAndReadMe(argc);
				return 0;
			}
			if (arg[i] == "-t" && i < arg_size - 1)
			{
				err_chk = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk == false)
					return 0;
				else
					target_ip_address = argv[i + 1];
			}
			if (arg[i] == "-tp")
			{
				err_chk = IPAdressFormatCheck.isPortFormatCorrect(argv[i + 1]);
				if (err_chk == false)
					return 0;
				else
					target_port = argv[i + 1];
			}
			if (arg[i] == "-m")
			{
				err_chk = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk == false)
					return 0;
				else
					my_ip_address = argv[i + 1];
			}
			if (arg[i] == "-mp")
			{
				err_chk = IPAdressFormatCheck.isPortFormatCorrect(argv[i + 1]);
				if (err_chk == false)
					return 0;
				else
					my_host_port = argv[i + 1];
			}
			if (arg[i] == "-v")
			{
				global_verbose = true;
			}
			if (arg[i] == "-f")
			{
				std::cout << "-f hasn't been implemented yet.\n";
				return 0;
			}
		}
	}

	// Finished without errors, return success
	return 1;
}