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
	StringManip();
	virtual ~StringManip();

	bool split(const std::string& string, char delimiter, std::vector<std::string> &vector);
	std::string duplicateCharacter(std::string& string, char delimiter);
protected:
private:
};

#endif//StringManip_h__