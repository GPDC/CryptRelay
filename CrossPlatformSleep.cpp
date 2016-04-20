#include "CrossPlatformSleep.h"

#ifdef _WIN32
#include <ws2tcpip.h>	// So an int can be passed to Sleep()
#endif//_WIN32



// Milliseconds only. xplatform windows & linux
void CrossPlatformSleep::mySleep(int number_in_ms)
{
#ifdef __linux___

	usleep(number_in_ms * 1000);		// Takes input in microseconds; times it by 1000 to turn it into ms.

#endif//__linux__
#ifdef _WIN32

	Sleep(number_in_ms);				// In milliseconds

#endif//_WIN32
}