//GlobalTypeHeader.h
#ifndef GlobalTypeHeader_h__
#define GlobalTypeHeader_h__

#ifdef _DEBUG

// Outputs text (x) to console. For use when an error occurs. Includes the
// file name and location, as well as the line number in the source file.
#	define DBG_DISPLAY_ERROR_LOCATION() do {std::cout << "It failed at" << " " << "Line: " << __LINE__ \
								<< ", File: " << __FILE__ << "\n";} while(0)
// Turns on certain areas of code that will output to console.
#   define DBG_OUTPUT

// Simply outputs text (x) to console.
#	define DBG_TXT(x) std::cout << x << "\n"

#else
#	define DBG_DISPLAY_ERROR_LOCATION(){}
#	define DBG_TXT(x)

#endif//_DEBUG


extern bool global_verbose;
extern bool global_debug;

#endif// GlobalTypeHeader_h__