//CommandLineInput.cpp
#include "CommandLineInput.h"
#include "GlobalTypeHeader.h"
#include <string>
#include <iostream>
#include "connection.h"
#include "ipaddress.h"
#include <vector>

CommandLineInput::CommandLineInput()
{
}
CommandLineInput::~CommandLineInput()
{
}

void CommandLineInput::helpAndReadMe(int argc)
{

	std::cout << "Proper format is:   cryptrelay.exe -t 192.168.27.50 -tp 7172 -m 192.168.1.101 -mp 7172\n";
	std::cout << "\n";																			//the weight of each one. need to add them up to set MAX_DIFF_INPUTS
	std::cout << "-h:	help		Displays the readme\n";										//1
	std::cout << "-t:	target		The target's IP address.\n";								//2
	std::cout << "-tp:	targetport	The target's port number.\n";								//2
	std::cout << "-m:	me		Your IP address that you want to listen on.\n";					//2
	std::cout << "-mp:	myport		Your port number that you want to listen on.\n";			//2
	std::cout << "-v:	verbose		Displays a lot of text output on screen.\n";					//1
	std::cout << "\nNOT YET IMPLEMENTED:\n";
	std::cout << "-f:	file		The file, and location of the file you wish to xfer.\n";	//2
	std::cout << "\nTo exit the program, please type 'cryptrelay.exit' at any time.\n\n\n";
	return;
}

int CommandLineInput::getCommandLineInput(int argc, char* argv[])
{
	const int MAX_DIFF_INPUTS = 12; //max possible count of argc to bother reading from
	int err_chk;
	int arg_size;

	target_ip_address = "";
	target_port = "";
	my_ip_address = connection::DEFAULT_IP_TO_LISTEN;
	my_port = connection::DEFAULT_PORT_TO_LISTEN;

	std::vector<std::string> arg;
	arg.reserve(MAX_DIFF_INPUTS);

	ipaddress ipaddress_get;

	//put all argv's into a vector so they can be compared to strings
	for (int i = 0; i < argc; i++) {
		arg.push_back(argv[i]);
	}
	arg_size = arg.size();

	//if command line arguments supplied, show the ReadMe
	if (argc <= 1) {
		helpAndReadMe(argc);
		return 1;
	}
	//check all argv inputs to see what the user wants to do
	if (argc >= 2) {
		for (int i = 1; i < argc; ++i) {

			if (arg[i] == "-h"
				|| arg[i] == "-H"
				|| arg[i] == "-help"
				|| arg[i] == "-Help"
				|| arg[i] == "help"
				|| arg[i] == "readme") {
				helpAndReadMe(argc);
				return 1;
			}
			if (arg[i] == "-t" && i < arg_size - 1) {
				err_chk = ipaddress_get.get_target(argv[i + 1]);
				if (err_chk == false)
					return 1;
				else
					target_ip_address = argv[i + 1];
			}
			if (arg[i] == "-tp") {
				//err_chk = ipaddress_get.port(argv[i + 1]);
				//if (err_chk == false)
				//	return 1;
				//else
				target_port = argv[i + 1];
			}
			if (arg[i] == "-m") {
				err_chk = ipaddress_get.get_target(argv[i + 1]);
				if (err_chk == false)
					return 1;
				else
					my_ip_address = argv[i + 1];
			}
			if (arg[i] == "-mp") {
				//err_chk = ipaddress_get.port(argv[i + 1]);
				//if (err_chk == false)
				//	return 1;
				//else
				my_port = argv[i + 1];
			}
			if (arg[i] == "-v")
				global_verbose = true;
			if (arg[i] == "-f") {
				std::cout << "-f hasn't been implemented yet.\n";
				return 1;
			}
		}
	}
}