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
	std::vector<std::string> split(std::string string, char deliminator);
protected:
private:
};

#endif//string_manipulation_h__