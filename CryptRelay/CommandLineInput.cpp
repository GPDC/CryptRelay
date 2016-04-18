//CommandLineInput.cpp
#include <string>
#include <iostream>
#include <vector>

#include "CommandLineInput.h"
#include "GlobalTypeHeader.h"
#include "FormatCheck.h"
#include "UPnP.h"

CommandLineInput::CommandLineInput()
{

}
CommandLineInput::~CommandLineInput()
{
}

void CommandLineInput::helpAndReadMe()
{

	std::cout << "Proper format for a normal connection is: cryptrelay.exe -t 1.2.3.4 -tp 30001\n";
	std::cout << "Proper format for a LAN connection is: cryptrelay.exe -lan -t 192.168.1.5 -mL 192.168.1.4\n";
	std::cout << "If you wish to specify the ports yourself: cryptrelay.exe -lan -t 192.168.1.5 -tp 30001 -mL 192.168.1.4 -mp 30001\n";
	std::cout << "\n";
	std::cout << "-h    help        Displays the readme\n";
	std::cout << "-t    target      The target's IP address.\n";
	std::cout << "-tp   targetport  The target's external port number.\n";
	std::cout << "-mL   me          Your local IP address that you want to listen on.\n";
	std::cout << "-mp   myport      Your port number that you want to listen on.\n";
	std::cout << "-v    verbose     Displays a lot of text output on screen.\n";
	std::cout << "-lan  LAN         Don't connect to the internet. Use LAN only.\n";
	std::cout << "-spf  Show Port Forwards Shows the list of current port forwards\n";
	std::cout << "      Format: cryptrelay.exe -spf my_external_port protocol\n";
	std::cout << "-dpf  Delete Port Forward Delete a specific port forward rule.\n";
	std::cout << "      Format: cryptrelay.exe -dpf my_external_port protocol\n";
	std::cout << "--examples        Displays a bunch of example usage scenarios.\n";
	std::cout << "\n";
	std::cout << "NOT YET IMPLEMENTED:\n";
	std::cout << "-f    file       The file, and location of the file you wish to xfer.\n";
	std::cout << "\nTo exit the program, please type 'exit()' at any time.\n\n\n";
}

void CommandLineInput::Examples()
{
	std::cout << "\n# List of various examples:\n";
	std::cout << "cryptrelay.exe -t 192.168.1.5\n";
	std::cout << "cryptrelay.exe -t 192.168.1.5 -tp 50302\n";
	std::cout << "cryptrelay.exe -lan -t 192.168.1.5 -mL 192.168.1.3\n";
	std::cout << "cryptrelay.exe -lan -t 192.168.1.5 -tp 50451 -mL 192.168.1.3 -mp 30456\n";
	std::cout << "\n";

}

int CommandLineInput::getCommandLineInput(int argc, char* argv[])
{
	// If necessary, a more thorough checking of command line input's individual chars is in my ParseText program.
	//	 but for now this is simple and easy to read/understand, so its nice.
	int err_chk = 0;

	std::vector<std::string> arg;

	IPAddress IPAdressFormatCheck;

	// Put all argv's into a vector so they can be compared to strings	// could use strcmp ?
	for (int i = 0; i < argc; i++)
	{
		arg.push_back(argv[i]);
	}
	int arg_size = arg.size();

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
			else if (arg[i] == "-t" && i < arg_size - 1)
			{
				err_chk = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk == false)
					return 0;
				else
				{
					target_ip_address = argv[i + 1];
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
			else if (arg[i] == "--examples")
			{
				Examples();
				return 0;
			}
			else if (arg[i] == "-v")
			{
				global_verbose = true;
			}
			else if (arg[i] == "-lan")
			{
				use_lan_only = true;
				use_upnp_to_connect_to_peer = false;	// This is a wonky way to do this. Think of a better way.
			}
			else if (arg[i] == "-spf")
			{
				get_list_of_port_forwards = true;


				//UPnP Upnp;
				//Upnp.standaloneGetListOfPortForwards();
				return 0; // exit program even though this is not a failure.
			}
			else if (arg[i] == "-dpf" && i < arg_size - 2)
			{
				delete_this_specific_port_forward = true;
				delete_this_specific_port_forward_port = i + 1;
				delete_this_specific_port_forward_protocol = i + 2;

				//UPnP Upnp;
				//Upnp.standaloneDeleteThisSpecificPortForward(argv[i + 1], argv[i + 2]);
				return 0; // exit program even though this is not a failure.

				// pls fix this. I should not be calling functions right here because
				// we might not be done parsing the command line input! This function should
				// only be called after we are done parsing.
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