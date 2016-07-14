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
	// 80 chars is width of console
	std::cout << "\n";
	std::cout << "-------------------------------------------------------------------------------\n";	//79 dashes + a new line character
	std::cout << "Proper format for a normal connection is: cryptrelay.exe -t 1.2.3.4\n";
	std::cout << "Format for LAN connection: cryptrelay.exe -lan -t 192.168.1.5 -mL 192.168.1.4\n";
	std::cout << "If you wish to specify the ports yourself: cryptrelay.exe -lan -t 192.168.1.5 -tP 30001 -mP 30022\n";
	std::cout << "\n";
	std::cout << "-h     Displays this.\n";
	std::cout << "-t     The target's IP address.\n";
	std::cout << "-tP    The target's external port number.\n";
	std::cout << "-mL    My local IP address that I want to listen on.\n";
	std::cout << "-mP    My port number that I want to listen on.\n";
	std::cout << "-v     Turns on verbose output to your terminal.\n";
	std::cout << "-lan   Don't connect to the internet. Use LAN only. Currently disables upnp.\n";
	std::cout << "-spf   Shows the currently forwarded ports \n";
	std::cout << "       Format: cryptrelay.exe -spf my_external_port protocol\n";
	std::cout << "-dpf   Delete a port forward rule.\n";
	std::cout << "       Format: cryptrelay.exe -dpf protocol my_external_port\n";
	std::cout << "-si    Show Info   Displays external & local ip, and some UPnP info.\n";
	std::cout << "--examples        Displays a bunch of example usage scenarios.\n";
	std::cout << "Arguments that are able to be used during a chat session:\n";
	std::cout << "-f     Send a file to the peer you are connected to.\n";
	std::cout << "       Format: -f C:\\Users\\JohnDoe\\Downloads\\recipe.txt\n";
	std::cout << "\n";
	std::cout << "NOT YET IMPLEMENTED:\n";
	std::cout << "Arguments that are able to be used during a chat session:\n";
	std::cout << "-f     Send a file to the peer you are connected to.\n";
	std::cout << "       -e Specify the encryption type here.\n";
	std::cout << "       This copies the file first -> encrypts the copy -> sends it.\n";
	std::cout << "       Format: -f C:\\Users\\John\\Downloads\\secret_recipe.txt -e RSA-4096\n";
	std::cout << "exit() This will exit CryptRelay. Another option would pressing ctrl-c.\n";
	std::cout << "-------------------------------------------------------------------------------\n";
	std::cout << "\n";
	std::cout << "\n";
}

void CommandLineInput::Examples()
{
	std::cout << "\n";
	std::cout << "# List of various examples:\n";
	std::cout << "cryptrelay.exe -t 192.168.1.5\n";
	std::cout << "cryptrelay.exe -t 192.168.1.5 -tP 50302\n";
	std::cout << "cryptrelay.exe -lan -t 192.168.1.5 -mL 192.168.1.3\n";
	std::cout << "cryptrelay.exe -lan -t 192.168.1.5 -tP 50451 -mL 192.168.1.3 -mP 30456\n";
	std::cout << "cryptrelay.exe -dpf TCP 30023\n";
	std::cout << "\n";
	std::cout << "# List of various examples for use during a chat session:\n";
	std::cout << "-f C:\\Users\\John\\Downloads\\recipe.txt\n";
	std::cout << "NOT YET IMPLEMENTED:\n";
	std::cout << "-f C:\\Users\\John\\Downloads\\secret_recipe.txt -e RSA-4096\n";
	std::cout << "\n";

}

int CommandLineInput::getCommandLineInput(int argc, char* argv[])
{
	// If necessary, a more thorough checking of command line input's individual chars is in my ParseText program.
	// but for now this is simple and easy to read/understand, so its nice.
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
	bool err_chk_bool = 0;
	if (argc >= 2 && arg_size >= 2)
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
				err_chk_bool = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk_bool == false)
				{
					std::cout << "Bad IP address format.\n\n";
					return 0;
				}
				else
				{
					target_ip_address = argv[i + 1];
					++i;	// Because we already took the information from i + 1, there is no need to check what i + 1 is, so we skip it by doing ++i;
				}
			}
			else if (arg[i] == "-tP" && i < arg_size - 1)
			{
				err_chk_bool = IPAdressFormatCheck.isPortFormatCorrect(argv[i + 1]);
				if (err_chk_bool == false)
					return 0;
				else
				{
					target_port = argv[i + 1];
					++i;
				}
			}
			else if (arg[i] == "-mL" && i < arg_size - 1)
			{
				err_chk_bool = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk_bool == false)
					return 0;
				else
				{
					my_ip_address = argv[i + 1];
					++i;
				}
			}
			else if (arg[i] == "-mP" && i < arg_size - 1)
			{
				err_chk_bool = IPAdressFormatCheck.isPortFormatCorrect(argv[i + 1]);
				if (err_chk_bool == false)
					return 0;
				else
				{
					my_host_port = argv[i + 1];
					++i;
				}
			}
			else if (arg[i] == "-mE" && i < arg_size - 1)
			{
				err_chk_bool = IPAdressFormatCheck.isIPV4FormatCorrect(argv[i + 1]);
				if (err_chk_bool == false)
					return 0;
				else
				{
					my_ext_ip_address = argv[i + 1];
					++i;
				}
			}
			else if (arg[i] == "-f" && i < arg_size - 1)
			{
				transfer_a_file = true;
				file_name_and_location = argv[i + 1];
				++i;
			}
			else if (arg[i] == "-fE" && i < arg_size - 2)
			{
				transfer_an_encrypted_file = true;
				file_name_and_location_to_be_encrypted = argv[i + 1];
				file_encryption_option = argv[i + 2];

				i += 2;
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
				use_upnp_to_connect_to_peer = false;
			}
			else if (arg[i] == "-spf")
			{
				get_list_of_port_forwards = true;
				return 1;
			}
			else if (arg[i] == "-dpf" && i < arg_size - 2)
			{
				delete_this_specific_port_forward = true;
				delete_this_specific_port_forward_protocol = arg[i + 1];
				delete_this_specific_port_forward_port = arg[i + 2];

				i += 2;	// Skipping the check for the next 2 argv's b/c we just used those as port and protocol.

				return 1;
			}
			else if (arg[i] == "-si")
			{
				show_info_upnp = true;
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
