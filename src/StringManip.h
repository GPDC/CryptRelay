// StringManip.h

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
	StringManip(bool turn_verbose_output_on = false);
	virtual ~StringManip();

	// Turn on and off verbose output for this class.
	bool verbose_output = false;

	int32_t split(const std::string& string, char delimiter, std::vector<std::string> &vector);
	std::string duplicateCharacter(std::string& string, char delimiter);
protected:
private:

	// Prevent anyone from copying this class
	StringManip(StringManip& StringManipInstance) = delete; // Delete copy operator
	StringManip& operator=(StringManip& StringManipInstance) = delete; // Delete assignment operator
};

#endif//StringManip_h__