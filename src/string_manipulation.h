//string_manipulation.h

// Overview:
// Things related to string manipulation are here.

// Terminology:
// Deliminator - 

#include <vector>

#ifndef string_manipulation_h__
#define string_manipulation_h__

class StringManip
{
public:
	bool split(std::string string, char delimiter, std::vector<std::string> &vector);
	std::string duplicateCharacter(std::string string, char delimiter);
protected:
private:
};

#endif//string_manipulation_h__