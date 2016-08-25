// StringManip.h

// This class is used to manipulate strings.

#ifdef _WIN32
#include <vector>
#endif// _WIN32

#ifdef __linux__
#include <vector>
#endif// __linux__


#ifndef StringManip_h__
#define StringManip_h__

class StringManip
{
public:
	explicit StringManip(bool turn_verbose_output_on = false);
	virtual ~StringManip();

	// Turn on and off verbose output for this class.
	bool verbose_output = false;

	// Split a string based on a given character, aka 'delimiter'.
	// The delimiter is the character that will signal when the string should be split.
	// a delimiter of a ' ' will create a new string every time it encounters a ' '.
	// For example: the quick brown fox
	// is turned into: str1 == the, str2 == quick, str3 == brown, str4 == fox
	// /* OUT */ std::vector<std::string> &vector
	int32_t split(const std::string& string, char delimiter, std::vector<std::string> &vector);

	// Finds the specified character (aka the delimiter), and
	// duplicates it by inserting another one next to it.
	// Returns a std::string that contains the changes.
	void duplicateCharacter(std::string& string, char delimiter);
protected:
private:

	// Prevent anyone from copying this class
	StringManip(StringManip& StringManipInstance) = delete; // Delete copy operator
	StringManip& operator=(StringManip& StringManipInstance) = delete; // Delete assignment operator
};

#endif//StringManip_h__