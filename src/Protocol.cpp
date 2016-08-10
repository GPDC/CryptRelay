#ifdef __linux__
#include <cstdint>
#include "Protocol.h"
#endif//__linux__

#ifdef _WIN32
#include <cstdint>
#include "Protocol.h"
#endif//WIN32

// Flags for send() that indicate what the message is being used for.
// CR == CryptRelay
const int8_t Protocol::MsgFlags::NO_FLAG = 0;
const int8_t Protocol::MsgFlags::SIZE_NOT_ASSIGNED = 0;
const int8_t Protocol::MsgFlags::CHAT = 1;
const int8_t Protocol::MsgFlags::ENCRYPTED_CHAT = 2;
const int8_t Protocol::MsgFlags::FILE_NAME = 30;
const int8_t Protocol::MsgFlags::FILE_SIZE = 31;
const int8_t Protocol::MsgFlags::FILE_DATA = 32;
const int8_t Protocol::MsgFlags::ENCRYPTED_FILE_NAME = 33;
const int8_t Protocol::MsgFlags::ENCRYPTED_FILE_SIZE = 34;
const int8_t Protocol::MsgFlags::ENCRYPTED_FILE_DATA = 35;