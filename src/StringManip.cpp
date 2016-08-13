// StringManip.cpp

#ifdef _WIN32
#include <string>
#include <limits.h>
#include <sstream>
#include <iostream>

#include "GlobalTypeHeader.h"
#include "StringManip.h"
#endif// _WIN32

#ifdef __linux__
#include <string>
#include <limits.h>
#include <sstream>
#include <iostream>

#include "GlobalTypeHeader.h"
#include "StringManip.h"
#endif// __linux__


StringManip::StringManip(bool turn_verbose_output_on)
{
	if (turn_verbose_output_on == true)
		verbose_output = true;
}
StringManip::~StringManip()
{

}

// The delimiter is the character that will signal when the string should be split.
// a delimiter of a ' ' will create a new string every time it encounters a ' '.
// For example: the quick brown fox
// is turned into: str1 == the, str2 == quick, str3 == brown, str4 == fox
int32_t StringManip::split(const std::string& string, char delimiter, std::vector<std::string> &elements)
{
	std::stringstream ss(string);
	std::string item;

	while (std::getline(ss, item, delimiter))
	{
		elements.push_back(item);
	}

	// Sidenote: if ss.eof() is reached, then it will set the failbit for ss.fail().
	if (ss.eof() == true)
	{
		// End of file reached.
		return 0;
	}
	else // if the failure isn't due to an eof(), then it must be a real failure.
	{
		if (ss.fail())
		{
			ss.clear(); // fail bit will remain set until cleared.
			return -1;
		}
	}


	
	return 0;
}

// Finds the specified character (aka the delimiter), and
// duplicates it by inserting another one next to it.
std::string StringManip::duplicateCharacter(std::string& string, char duplicate_character)
{
	size_t string_length = string.length();
	uint32_t i;
	for (i = 0; i < string_length && i < UINT_MAX; ++i)
	{
		if (string[i] == duplicate_character)
		{
			// arg1 where
			// arg2 how many of arg3
			// arg3 insert this char
			string.insert(i, 1, duplicate_character);
			++i;
		}
	}
	if (i == UINT_MAX)
		std::cout << "WARNING: string length in duplicateCharacter() reached UINT_MAX\n";

	DBG_TXT("string.insert yielded: " << string);
	return string;
}