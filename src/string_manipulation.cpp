//string_manipulation.cpp
#include <string>
#include <limits.h>

#include <sstream>

#include <iostream>

#include "string_manipulation.h"

// The deliminiator is the character that will signal when the string should be split.
// a deliminator of a ' ' will create a new string every time it encounters a ' '.
// For example: the quick brown fox
// is turned into: str1 == the, str2 == quick, str3 == brown, str4 == fox
bool StringManip::split(std::string string, char deliminator, std::vector<std::string> &elements)
{
	//std::vector<std::string> elems;
	std::stringstream ss(string);
	std::string item;

	while (std::getline(ss, item, deliminator))
	{
		elements.push_back(item);
	}

	return true;
}

// Finds the specified character (aka the deliminator), and
// duplicates it by adding another one next to it.
std::string StringManip::duplicateCharacter(std::string string, char deliminator)
{

	size_t string_length = string.length();
	for (unsigned long long i = 0; i < string_length && i < ULLONG_MAX; ++i)
	{
		if (string[i] == '\\')
		{
			// arg1 where
			// arg2 how many of arg3
			// arg3 insert this char
			string.insert(i, 1, '\\');
		}
	}
	std::cout << "dbg string.insert yielded: " << string << "\n";
	return string;
}