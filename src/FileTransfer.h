// FileTransfer.h
#ifndef FileTransfer_h__
#define FileTransfer_h__

// One instance of this class == 1 file transfer thread.


// With regards to the current setup of the RecvBufStateMachine inside the ApplicationLayer,
// it is only possible to send 1 file at a time, even if multiple instances of this class are used.
// Multiple files can be sent, its just that one has to be completely done sending before the other
// is to be sent.

// Anything that is tagged as a '~CryptRelay specific methods~':
// These methods are specific to CryptRelay as a program, sending data
// to another CryptRelay program, and are not intended for use with any other program.
// Basically, if you want to take this class and use it for some other program,
// do not use any of the methods in this class with the tag '~CryptRelay specific methods~'
// without modifying them first to work with whatever your new program is.
#ifdef __linux__
#include <string>
#endif//__linux__

#ifdef _WIN32
#include <string>
#endif//_WIN32

// Forward declaration
class ApplicationLayer;

class FileTransfer
{
	// Typedef section
private:
#ifdef _WIN32
	typedef struct _stat64 xplatform_struct_stat;
#endif//WIN32
#ifdef __linux__
	typedef struct stat xplatform_struct_stat;
#endif//__linux__

public:
	FileTransfer(ApplicationLayer* AppLayer, std::string file_name_and_path, bool send_file_bool, bool turn_verbose_output_on = false);
	virtual ~FileTransfer();

	// Turn on and off verbose output for this class.
	bool verbose_output = false;

	// Sends a file
	int32_t sendFile(std::string file_name_and_path);
	// This is only for use with sendFile()
	bool is_send_file_thread_in_use = false;

	std::thread send_file_thread;

protected:

private:

	// Prevent anyone from copying this class.
	FileTransfer(FileTransfer& FileTransferInstance) = delete;			   // disable copy operator
	FileTransfer& operator=(FileTransfer& FileTransferInstance) = delete;  // disable assignment operator

	const int32_t MAX_FILENAME_LENGTH = 255;

	ApplicationLayer* AppLayer;


	// Copies a file.
	bool copyFile(const char * read_file_name_and_location, const char * write_file_name_and_location);

	// For use with anything file related
	int32_t getFileStats(const char * file_name_and_path, xplatform_struct_stat* FileStats);
	bool displayFileSize(const char* file_name_and_path, xplatform_struct_stat * FileStatBuf);

	// If given a direct path to a file, it will return the file name.
	// Ex: c:\users\me\storage\my_file.txt
	// will return: my_file.txt
	std::string retrieveFileNameFromPath(std::string file_name_and_path);
};

#endif//FileTransfer_h__