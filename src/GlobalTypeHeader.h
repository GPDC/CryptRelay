//GlobalTypeHeader.h
#ifndef GlobalTypeHeader_h__
#define GlobalTypeHeader_h__

#define OUTPUT_DEBUG
#ifdef OUTPUT_DEBUG
// Outputs text (x) to console. For use when an error occurs. Includes the
// file name and location, as well as the line number in the source file.
#	define DBG_ERR(x) std::cout << x << " " << "Line: " << __LINE__ \
								<< ", File: " << __FILE__ << "\n";

// Simply outputs text (x) to console.
#	define DBG_TXT(x) std::cout << x << "\n";
#else
#	define DBG_ERR(x)
#endif// OUTPUT_DEBUG

extern bool global_verbose;
extern bool global_debug;

#endif// GlobalTypeHeader_h__