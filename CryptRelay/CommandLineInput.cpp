//CommandLineInput.cpp
#include "CommandLineInput.h"
#include "GlobalTypeHeader.h"
#include <string>
#include <iostream>

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
	std::cout << "-h	#help		Displays the readme\n";
	std::cout << "-t	#target		The target's IP address.\n";
	std::cout << "-tp	#targetport	The target's port number.\n";
	std::cout << "-m	#me		Your IP address that you want to listen on.\n";
	std::cout << "-mp	#myport		Your port number that you want to listen on.\n";
	std::cout << "-v	#verbose	Displays a lot of text output on screen.\n";
	std::cout << "\nNOT YET IMPLEMENTED:\n";
	std::cout << "-f	#file		The file, and location of the file you wish to xfer.\n";
	std::cout << "\nTo exit the program, please type 'cryptrelay.exit' at any time.\n\n\n";
	return;

	//if (argv[2] > "0" && argv[2] < "65535")	//need to convert to decimal first ********************** stoi();
}