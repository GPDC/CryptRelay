// ProcessRecvBuf.h
#ifndef ProcessRecvBuf_h__
#define ProcessRecvBuf_h__

#ifdef __linux__
#include <string>
#endif//__linux__

#ifdef _WIN32
#include <string>
#endif//_WIN32


// Forward declaration
class ApplicationLayer;

// This class is responsible for 100% of all incoming data sent by the peer.
// It needs to process everything that comes through and identify
// what kind of message the peer sent, and how big it is.
// Example types of messages include: chat message, a piece of a file.
class ProcessRecvBuf
{
public:
	ProcessRecvBuf(ApplicationLayer* ProtocolInstance);
	virtual ~ProcessRecvBuf();

	// Here is where all messages received are processed to determine what to do with the message.
	// Some messages might be printed to screen, others are a portion of a file transfer.
	bool decideActionBasedOnFlag(char * recv_buf, int64_t buf_len, int64_t byte_count);
	
	// For writing files in decideActionBasedOnFlag().
	// It is only public because if recv() ever fails inside the ApplicationLayer class,
	// then it needs to close the file that it was writing to.
	FILE * WriteFile = nullptr;


	// Accessors
	const std::string& getIncomingFileNameFromPeer();
	const bool& getIsFileDoneBeingWritten();


protected:
private:

	// Prevent anyone from copying this class.
	ProcessRecvBuf(ProcessRecvBuf& ProcessInstance) = delete;			    // disable copy operator
	ProcessRecvBuf& operator=(ProcessRecvBuf& ProcessInstance) = delete;  // disable assignment operator


	ApplicationLayer* AppLay;


	enum RecvStateMachine
	{
		CHECK_FOR_FLAG,
		CHECK_MESSAGE_SIZE_PART_ONE,
		CHECK_MESSAGE_SIZE_PART_TWO,
		WRITE_FILE_FROM_PEER,
		TAKE_FILE_NAME_FROM_PEER,
		TAKE_FILE_SIZE_FROM_PEER,
		OUTPUT_CHAT_FROM_PEER,

		OPEN_FILE_FOR_WRITE,
		CLOSE_FILE_FOR_WRITE,

		ERROR_STATE,
	};

	// Variables necessary for decideActionBasedOnFlag().
	// They are here so that the state can be saved even after exiting the function.
	int64_t position_in_recv_buf = 0;
	int64_t state = CHECK_FOR_FLAG;
	int64_t position_in_message = 0;	// current cursor position inside the imaginary message sent by the peer.
	int8_t message_flag = 0;
	int64_t message_size_part_one = 0;
	int64_t message_size_part_two = 0;
	int64_t message_size = 0;		    // peer told us this size

										// More variables necessary for decideActionBasedOnFlag().
										// These are related to an incoming file transfer
	static const int64_t INCOMING_FILE_NAME_FROM_PEER_SIZE = 200;
	static const int64_t RESERVED_NULL_CHAR_FOR_FILE_NAME = 1;
	char incoming_file_name_from_peer_cstr[INCOMING_FILE_NAME_FROM_PEER_SIZE];
	std::string incoming_file_name_from_peer;
	bool is_file_done_being_written = true;


	// Variables for decideActionBasedOnFlag()
	// Specifically for the cases that deal with writing an incoming file from the peer.
	int64_t bytes_read = 0;
	int64_t bytes_written = 0;
	int64_t total_bytes_written_to_file = 0;


	// For use with decideActionBasedOnFlag()
	// Using the given buffer, convert the first 8 bytes from Network to Host Long Long
	int32_t assignFileSizeFromPeer(char * recv_buf, int64_t recv_buf_len, int64_t received_bytes);
	// Variables necessary for assignFileSizeFromPeer();
	int64_t file_size_fragment = 0;
	int64_t incoming_file_size_from_peer = 0;
	const int32_t RECV_AGAIN = 0;
	const int32_t FINISHED_ASSIGNING_FILE_SIZE_FROM_PEER = 1;
};

#endif//ProcessRecvBuf_h__