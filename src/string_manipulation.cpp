//string_manipulation.cpp
#include <string>

#include <sstream>

#include "string_manipulation.h"

// The deliminiator is the character that will signal when the string should be split.
// a deliminator of a ' ' will create a new string every time it encounters a ' '.
// For example: the quick brown fox
// is turned into: str1 == the, str2 == quick, str3 == brown, str4 == fox
std::vector<std::string> StringManip::split(std::string string, char deliminator)
{
	std::vector<std::string> elems;
	std::stringstream ss(string);
	std::string item;

	while (std::getline(ss, item, deliminator))
	{
		elems.push_back(item);
	}

	return elems;
}