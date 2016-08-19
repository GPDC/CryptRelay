// CommandLineInput.cpp
#ifdef __linux__
#include <string>
#include <iostream>
#include <vector>
#include <climits>

#include "CommandLineInput.h"
#include "GlobalTypeHeader.h"
#include "FormatCheck.h"
#endif//__linux__

#ifdef _WIN32
#include <string>
#include <iostream>
#include <vector>
#include <climits>

#include "CommandLineInput.h"
#include "GlobalTypeHeader.h"
#include "FormatCheck.h"
#endif//_WIN32


CommandLineInput::CommandLineInput()
{

}
CommandLineInput::~CommandLineInput()
{
}

void CommandLineInput::displayHelpAndReadMe()
{
	// 80 chars is width of console
	std::cout << "\n";
	std::cout << "-------------------------------------------------------------------------------\n";	//79 dashes + new line character
	std::cout << "Proper format for a normal connection is: cryptrelay.exe -t 1.2.3.4\n";
	std::cout << "Format for LAN connection: cryptrelay.exe --lan -t 192.168.1.5 -mL 192.168.1.4\n";
	std::cout << "If you wish to specify the ports yourself: cryptrelay.exe --lan -t 192.168.1.5\n";
	std::cout << "       -tP 30001 -mL 192.168.1.4 -mP 30022\n";
	std::cout << "\n";
	std::cout << "-h     Displays this.\n";
	std::cout << "-t     The target's IP address.\n";
	std::cout << "-tP    The target's external port number.\n";
	std::cout << "-mL    My local IP address that I want to listen on.\n";
	std::cout << "-mP    My port number that I want to listen on.\n";
	std::cout << "-v     Turns on verbose output to your terminal.\n";
	std::cout << "--lan     Disables upnp. Still possible to connect to internet, just no upnp.\n";
	std::cout << "-s     Shows the currently forwarded ports \n";
	std::cout << "       Format: cryptrelay.exe -s my_external_port protocol\n";
	std::cout << "-d     Delete a port forward rule.\n";
	std::cout << "       Format: cryptrelay.exe -d protocol my_external_port\n";
	std::cout << "-i     Displays external & local ip, and some UPnP info.\n";
	std::cout << "--examples   Displays a bunch of example usage scenarios.\n";
	std::cout << "Arguments that are able to be used during a chat session:\n";
	std::cout << "-f     Send a file to the peer you are connected to.\n";
	std::cout << "       Format: -f C:\\Users\\JohnDoe\\Downloads\\recipe.txt\n";
	std::cout << "exit() This will exit CryptRelay. Ctrl-c also exits, but not gracefully.\n";
	//std::cout << "\n";
	//std::cout << "NOT YET IMPLEMENTED:\n";
	//std::cout << "Arguments that are able to be used during a chat session:\n";
	//std::cout << "-f     Send a file to the peer you are connected to.\n";
	//std::cout << "       -e Specify the encryption type here.\n";
	//std::cout << "       This copies the file first -> encrypts the copy -> sends it.\n";
	//std::cout << "       Format: -f C:\\Users\\John\\Downloads\\secret_recipe.txt -e RSA-4096\n";
	std::cout << "-------------------------------------------------------------------------------\n";
	std::cout << "\n";
	std::cout << "\n";
}

void CommandLineInput::displayExamples()
{
	std::cout << "\n";
	std::cout << "-------------------------------------------------------------------------------\n";
	std::cout << "# List of various examples:\n";
	std::cout << "cryptrelay.exe -t 192.168.1.5\n";
	std::cout << "cryptrelay.exe -t 192.168.1.5 -tP 50302\n";
	std::cout << "cryptrelay.exe --lan -t 192.168.1.5 -mL 192.168.1.3\n";
	std::cout << "cryptrelay.exe --lan -t 192.168.1.5 -tP 50451 -mL 192.168.1.3 -mP 30456\n";
	std::cout << "cryptrelay.exe -d TCP 30023\n";
	std::cout << "\n";
	std::cout << "# List of various examples for use during a chat session:\n";
	std::cout << "-f C:\\Users\\John\\Downloads\\recipe.txt\n";
	std::cout << "exit()\n";
	//std::cout << "NOT YET IMPLEMENTED:\n";
	//std::cout << "-f C:\\Users\\John\\Downloads\\secret_recipe.txt -e RSA-4096\n";
	std::cout << "-------------------------------------------------------------------------------\n";
	std::cout << "\n";
	std::cout << "\n";
}

int32_t CommandLineInput::setVariablesFromArgv(int32_t argc, char* argv[])
{
	// If necessary, a more thorough checking of command line input's individual chars is in my ParseText program.
	// but for now this is simple and easy to read/understand, so its nice.
	std::vector<std::string> arg;

	FormatCheck IPAdressFormatCheck;

	// Put all argv's into a vector so they can be compared to strings	// could use strcmp ?
	for (int32_t i = 0; i < argc; i++)
	{
		arg.push_back(argv[i]);
	}

	int32_t arg_size;
	if (arg.size() < INT_MAX)
		arg_size = (int32_t)arg.size();
	else
		return -1;

	// If no command line arguments supplied, show the ReadMe
	if (argc <= 1)
	{
		displayHelpAndReadMe();
		return -1;
	}
	// Check all argv inputs to see what the user wants to do
	bool err_chk_bool = 0;
	if (argc >= 2 && arg_size >= 2)
	{
		for (int32_t i = 1; i < argc; ++i)
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
				displayHelpAndReadMe();
				return -1;
			}
			else if (i < arg_size - 1 && arg[i] == "-t")
			{
				err_chk_bool = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk_bool == false)
				{
					std::cout << "Bad IP address format.\n\n";
					return -1;
				}
				else
				{
					target_ip_address = argv[i + 1];
					++i;	// Because we already took the information from i + 1,
					        // there is no need to check what i + 1 is, so we skip it by doing ++i;
				}
			}
			else if (i < arg_size - 1 && arg[i] == "-tP")
			{
				err_chk_bool = IPAdressFormatCheck.isPortFormatCorrect(argv[i + 1]);
				if (err_chk_bool == false)
					return -1;
				else
				{
					target_port = argv[i + 1];
					++i;
				}
			}
			else if (i < arg_size - 1 && arg[i] == "-mL")
			{
				err_chk_bool = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk_bool == false)
					return -1;
				else
				{
					my_ip_address = argv[i + 1];
					++i;
				}
			}
			else if (i < arg_size - 1 && arg[i] == "-mP")
			{
				err_chk_bool = IPAdressFormatCheck.isPortFormatCorrect(argv[i + 1]);
				if (err_chk_bool == false)
					return -1;
				else
				{
					my_local_port = argv[i + 1];
					++i;
				}
			}
			else if (i < arg_size - 1 && arg[i] == "-mE")
			{
				err_chk_bool = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk_bool == false)
					return -1;
				else
				{
					my_ext_ip_address = argv[i + 1];
					++i;
				}
			}
			else if (arg[i] == "--examples")
			{
				displayExamples();
				return -1;
			}
			else if (arg[i] == "-v")
			{
				verbose_output = true;
			}
			else if (arg[i] == "--lan")
			{
				use_lan_only = true;
				use_upnp_to_connect_to_peer = false;
			}
			else if (arg[i] == "-s")
			{
				retrieve_list_of_port_forwards = true;
				return 0;
			}
			else if (i < arg_size - 2 && arg[i] == "-d")
			{
				delete_this_specific_port_forward = true;
				delete_this_specific_port_forward_protocol = arg[i + 1];
				delete_this_specific_port_forward_port = arg[i + 2];

				i += 2;	// Skipping the check for the next 2 argv's b/c we just used those as port and protocol.

				return 0;
			}
			else if (arg[i] == "-i")
			{
				show_info_upnp = true;
			}
			else
			{
				displayHelpAndReadMe();
				return -1;
			}
		}
	}

	// Finished without errors, return success
	return 0;
}
