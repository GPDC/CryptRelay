// FileTransfer.h
#ifndef FileTransfer_h__
#define FileTransfer_h__

// One instance of this class == 1 file transfer thread.

// With regards to the current setup of the RecvBufStateMachine inside the ApplicationLayer class,
// it is only possible to send 1 file at a time, even if multiple instances of this class are used.
// Multiple files can be sent, its just that one has to be completely done sending before the other
// is to be sent.

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
	FileTransfer(ApplicationLayer* AppLayer,
		std::string file_name_and_path,
		bool send_file_bool,
		bool turn_verbose_output_on = false
	);
	virtual ~FileTransfer();

	// Turn on and off verbose output for this class.
	bool verbose_output = false;

	// Sends a file to the connected peer.
	// Returns -1, error
	// Returns 0, success
	int32_t sendFile(std::string file_name_and_path);
	// This is only for use with sendFile()
	bool is_send_file_thread_in_use = false;

	std::thread send_file_thread;

protected:

private:

	// Prevent anyone from copying this class.
	FileTransfer(FileTransfer& FileTransferInstance) = delete;			   // disable copy operator
	FileTransfer& operator=(FileTransfer& FileTransferInstance) = delete;  // disable assignment operator

	// If you want to exit the program, set this to true.
	bool exit_now = false;

	static const int32_t MAX_FILENAME_LENGTH;

	ApplicationLayer* AppLayer;


	// Copies a file.
	// This function expects the file name and location of the file
	// to be properly formatted already. That means escape characters
	// must be used to make a path valid '\\'.
	bool copyFile(const char * read_file_name_and_location, const char * write_file_name_and_location);

	// Gets some stats about the file. One stat, for example, is the size of the file.
	// Returns 0, success
	// Returns -1, error
	// /* IN */ const char * file_name_and_path
	// /* OUT */ xplatform_struct_stat* FileStats
	static int32_t retrieveFileStats(const char * file_name_and_path, xplatform_struct_stat* FileStats);

	// Displays to console the size of the file.
	// Must call retrieveFileStats() first in order to get the size of the file.
	static bool displayFileSize(xplatform_struct_stat * FileStatBuf);

	// If given a direct path to a file, it will return the file name.
	// Ex: c:\users\me\storage\my_file.txt
	// will return: my_file.txt
	// returns empty string on error.
	static std::string retrieveFileNameFromPath(std::string file_name_and_path);


private:

	// Callbacks



public:

	// Accessors

	// If you want this class to proceed to exit, set this to true.
	void setExitNow(bool exit_the_program) { exit_now = exit_the_program; }
};

#endif//FileTransfer_h__