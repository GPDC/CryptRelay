#ifndef Protocol_h__
#define Protocol_h__

class Protocol
{
public:

	// Flags that indicate what the message is being used for.
	// enum is not used for these b/c it could break compatability when
	// communicating with older version of this program.
	struct MsgFlags
	{
		static const int8_t NO_FLAG;
		static const int8_t SIZE_NOT_ASSIGNED;
		static const int8_t CHAT;
		static const int8_t ENCRYPTED_CHAT;
		static const int8_t FILE_NAME;
		static const int8_t FILE_SIZE;
		static const int8_t FILE_DATA;
		static const int8_t ENCRYPTED_FILE_DATA;
		static const int8_t ENCRYPTED_FILE_NAME;
		static const int8_t ENCRYPTED_FILE_SIZE;
	};
};


#endif//Protocol_h__