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
	if (argc <= 1)
	{
		std::cout << "Proper format is:   cryptrelay.exe -tIP 192.168.27.50 -tP 7172 -mIP 192.168.1.101 -mP 7172\n";
		std::cout << "\n";
		std::cout << "-h			#help			Displays the readme\n";
		std::cout << "-tIP			#targetIP		The target's IP address.\n";
		std::cout << "-tP			#targetPort     The target's port number.\n";
		std::cout << "-mIP			#myIP			Your IP address.\n";
		std::cout << "-mP			#myPort			Your port number.\n";
		return;
	}

	//if (argv[2] > "0" && argv[2] < "65535")	//need to convert to decimal first ********************** stoi();
}