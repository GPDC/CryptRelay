// FileTransfer.cpp
#ifdef __linux__
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>	// <Winsock2.h>
#include <netdb.h>
#include <sys/stat.h>
#include <thread>
#include <limits.h>

#include "FileTransfer.h"
#include "ApplicationLayer.h"
#include "GlobalTypeHeader.h"
#endif//__linux__

#ifdef _WIN32
#include <iostream>
#include <thread>
#include <limits.h>

#include "FileTransfer.h"
#include "ApplicationLayer.h"
#include "GlobalTypeHeader.h"
#endif//_WIN32


#ifdef _WIN32
#pragma warning(disable:4996)		// disable deprecated warning for fopen()
#endif//_WIN32


const int32_t MAX_FILENAME_LENGTH = 255;

FileTransfer::FileTransfer(ApplicationLayer* AppLayerInstance, std::string file_name_and_path, bool send_the_file)
{
	AppLayer = AppLayerInstance;
	if (send_the_file == true)
	{
		send_file_thread = std::thread(&FileTransfer::sendFile, this, file_name_and_path);
	}
}
FileTransfer::~FileTransfer()
{

}

bool FileTransfer::sendFile(std::string file_name_and_path)
{
	is_send_file_thread_in_use = true;

	// (8 * 1024) == 8,192 aka 8KB, and 8192 * 1024 == 8,388,608 aka 8MB
	// Buffer size must NOT be bigger than 65,535 (u_short).
	const int32_t BUF_LEN = ApplicationLayer::ARTIFICIAL_LENGTH_LIMIT_FOR_SEND;
	char * buf = new char[BUF_LEN];

	// Open the file
	FILE *ReadFile;
	ReadFile = fopen(file_name_and_path.c_str(), "rb");
	if (ReadFile == nullptr)
	{
		delete[]buf;
		std::cout << "Error: File transfer aborted. Couldn't open file.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		perror("Error opening file for reading binary sendFile");
		is_send_file_thread_in_use = false;
		return true;
	}

	// Retrieve the name of the file from the string that contains
	// the path and the name of the file.
	std::string file_name = retrieveFileNameFromPath(file_name_and_path);
	if (file_name.empty() == true)
	{
		delete[]buf;
		std::cout << "Error: couldn't retrieve file name from the given path. Abandoning file transfer.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (fclose(ReadFile))
			perror("Error closing file designated for reading");
		is_send_file_thread_in_use = false;
		return false; //exit please, file name couldn't be found.
	}

	// *** The order in which file name and file size is sent DOES matter! ***
	// Send the file name to peer
	if (AppLayer->sendFileName(file_name) < 0)
	{
		delete[]buf;
		std::cout << "Abandoning file transfer due to an error while sending the file name.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (fclose(ReadFile))
			perror("Error closing file designated for reading");
		is_send_file_thread_in_use = false;
		return true; //exit please
	}


	// Get file statistics and display the size of the file
	int64_t size_of_file = 0;
	xplatform_struct_stat FileStats;
	int err_chk = getFileStats(file_name_and_path.c_str(), &FileStats);
	if (err_chk == -1)
	{
		delete[]buf;
		std::cout << "Error retrieving file statistics. Abandoning file transfer.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (fclose(ReadFile))
			perror("Error closing file designated for reading");
		is_send_file_thread_in_use = false;
		return true; //exit please
	}
	else // success
	{
		// Output to terminal the size of the file in Bytes, KB, MB, GB.
		displayFileSize(file_name_and_path.c_str(), &FileStats);
		size_of_file = FileStats.st_size;
	}


	// Send file size to peer
	if (AppLayer->sendFileSize(buf, BUF_LEN, size_of_file) < 0)
	{
		delete[]buf;
		std::cout << "Abandoning file transfer due to error sending file size.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		if (fclose(ReadFile))
			perror("Error closing file designated for reading");
		is_send_file_thread_in_use = false;
		return true;// exit please
	}


	// Send the file to peer.
	int32_t bytes_read = 0;
	int64_t bytes_sent = 0;
	int64_t total_file_bytes_sent = 0;

	// We are treating bytes_read as a u_short, instead of size_t
	// because we don't want to clog the send() with only file
	// packets. We want the user to still be able to send a message
	// within a reasonable amount of time while transfering a file.
	// File transfer jumps into mutex send() queue.
	// File transfer begins sending.
	// Chat message jumps into mutex send() queue. It is waiting
	//    for file transfer to leave the mutex first.
	// File transfer finishes sending USHRT_MAX bytes, and exits mutex.
	// Chat message now begins sending.
	//
	// This is just to illustrate that if we try sending too much into
	// the mutex send() queue at a time, then a chat message will take
	// forever to be sent out to the peer, causing an uncomfortable delay.
	/*static_assert(BUF_LEN <= ApplicationLayer::ARTIFICIAL_LENGTH_LIMIT_FOR_SEND,
		"Buffer size must NOT be bigger than ApplicationLayer::ARTIFICIAL_LENGTH_LIMIT_FOR_SEND.");*/

	std::cout << "# Sending file: " << file_name << "\n";
	do
	{
		// The first 3 chars of the buffer are reserved for:
		// [0] Flag to tell the peer what kind of packet this is (file, chat)
		// [1] both [1] and [2] combined are considered a u_short.
		// [2] the u_short is to tell the peer what the size of the buffer is.
		//
		// Example: you want to send your peer a 50,000 byte file.
		// So you send() 10,000 bytes at a time. The u_short [1] and [2]
		// will tell the peer to treat the next 10,000 bytes as whatever the flag is set to.
		// In this case the flag would be set to CR_FILE.

		// Making sure bytes_read can always accept the amount being given to it by fread().
		static_assert(sizeof(bytes_read) >= sizeof(BUF_LEN),
			"Error: sizeof(bytes_read) >= sizeof(BUF_LEN) == false");
		bytes_read = fread(
			buf + ApplicationLayer::CR_RESERVED_BUFFER_SPACE,
			1,
			BUF_LEN - ApplicationLayer::CR_RESERVED_BUFFER_SPACE, ReadFile
		);
		if (bytes_read)
		{
			bytes_sent = AppLayer->sendFileData(buf, BUF_LEN, bytes_read);
			if ( bytes_sent < 0)
			{
				std::cout << "Abandoning file transfer due to error. AppLayer->sendFileData();\n";
				DBG_DISPLAY_ERROR_LOCATION();
				break;
			}
			// The CR_RESERVED_BUFFER_SPACE bytes are not part of the file,
			// so do not include them in the total amount of 'file' bytes that were sent.
			total_file_bytes_sent += bytes_sent - ApplicationLayer::CR_RESERVED_BUFFER_SPACE;
		}
		else
			bytes_sent = 0;

		// 0 means error. If they aren't equal to eachother
		// then it didn't write as much as it read for some reason.
	} while (bytes_read > 0 && bytes_sent > 0);

	// Please implement sha hash checking here to make sure the file is legit.

	std::cout << "Total bytes of the file sent: " << total_file_bytes_sent << "\n";
	std::cout << "Expected to send: " << size_of_file << "\n";
	std::cout << "Difference: " << total_file_bytes_sent - size_of_file << "\n";

	// Check if it errored while reading from the file.
	if (ferror(ReadFile))
	{
		DBG_DISPLAY_ERROR_LOCATION();
		perror("Read error copying file");
	}
	else // successful transfer
	{
		std::cout << "# File transfer complete: " << file_name << "\n";
	}

	if (fclose(ReadFile))
	{
		perror("Error closing file designated for reading");
		DBG_DISPLAY_ERROR_LOCATION();
	}

	delete[]buf;
	is_send_file_thread_in_use = false;
	return false;
}

// This function expects the file name and location of the file
// to be properly formatted already. That means escape characters
// must be used to make a path valid '\\'.
bool FileTransfer::copyFile(const char * file_name_and_location_for_reading, const char * file_name_and_location_for_writing)
{
	if (file_name_and_location_for_reading == nullptr || file_name_and_location_for_writing == nullptr)
	{
		std::cout << "Error: copyFile() failed. NULL pointer.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return true;
	}

	FILE *ReadFile;
	FILE *WriteFile;

	ReadFile = fopen(file_name_and_location_for_reading, "rb");
	if (ReadFile == nullptr)
	{
		perror("Error opening file for reading binary");
		DBG_DISPLAY_ERROR_LOCATION();
		return true;
	}
	WriteFile = fopen(file_name_and_location_for_writing, "wb");
	if (WriteFile == nullptr)
	{
		perror("Error opening file for writing binary");
		DBG_DISPLAY_ERROR_LOCATION();
		if (fclose(ReadFile))
		{
			perror("Error closing file designated for reading");
			DBG_DISPLAY_ERROR_LOCATION();
		}
		return true;
	}

	// Get file stastics and cout the size of the file
	int64_t size_of_file_to_be_copied = 0;
	xplatform_struct_stat FileStats;
	int32_t errchk = getFileStats(file_name_and_location_for_reading, &FileStats);
	if (errchk == -1)
	{
		std::cout << "Error retrieving file statistics. Abandoning file transfer.\n";
		DBG_DISPLAY_ERROR_LOCATION();

		if (fclose(ReadFile))
		{
			perror("Error closing file designated for reading");
			DBG_DISPLAY_ERROR_LOCATION();
		}
		if (fclose(WriteFile))
		{
			perror("Error closing file designated for writing");
			DBG_DISPLAY_ERROR_LOCATION();
		}
		
		is_send_file_thread_in_use = false;
		return true; //exit please
	}
	else // success
	{
		// Output to terminal the size of the file in Bytes, KB, MB, GB.
		displayFileSize(file_name_and_location_for_reading, &FileStats);
		size_of_file_to_be_copied = FileStats.st_size;
	}


	// Please make a sha hash of the file here so it can be checked with the
	// hash of the copy later.

	int64_t bytes_read;
	int64_t bytes_written;
	int64_t total_bytes_written_to_file = 0;

	int64_t twenty_five_percent = size_of_file_to_be_copied / 4;	// divide it by 4
	int64_t fifty_percent = size_of_file_to_be_copied / 2;	//divide it by two
	int64_t seventy_five_percent = size_of_file_to_be_copied - twenty_five_percent;

	bool twenty_five_already_spoke = false;
	bool fifty_already_spoke = false;
	bool seventy_five_already_spoke = false;

	// (8 * 1024) == 8,192 aka 8KB, and 8192 * 1024 == 8,388,608 aka 8MB
	const int64_t buffer_size = 8 * 1024 * 1024;
	char* buffer = new char[buffer_size];

	std::cout << "Starting Copy file...\n";
	do
	{
		bytes_read = fread(buffer, 1, buffer_size, ReadFile);
		if (bytes_read)
			bytes_written = fwrite(buffer, 1, (size_t)bytes_read, WriteFile);
		else
			bytes_written = 0;

		total_bytes_written_to_file += bytes_written;

		// If we have some statistics to work with, then display
		// ghetto progress indicators.
		if (size_of_file_to_be_copied >= 0)
		{
			if (total_bytes_written_to_file > twenty_five_percent && total_bytes_written_to_file < fifty_percent && twenty_five_already_spoke == false)
			{
				std::cout << "File copy 25% complete.\n";
				twenty_five_already_spoke = true;
			}
			else if (total_bytes_written_to_file > fifty_percent && total_bytes_written_to_file < seventy_five_percent && fifty_already_spoke == false)
			{
				std::cout << "File copy 50% complete.\n";
				fifty_already_spoke = true;
			}
			else if (total_bytes_written_to_file > seventy_five_percent && seventy_five_already_spoke == false)
			{
				std::cout << "File copy 75% complete.\n";
				seventy_five_already_spoke = true;
			}
		}


		// 0 means error. If they aren't equal to eachother
		// then it didn't write as much as it read for some reason.
	} while ((bytes_read > 0) && (bytes_read == bytes_written));

	if (total_bytes_written_to_file == size_of_file_to_be_copied)
		std::cout << "File copy complete.\n";
	else
		std::cout << "Error, file copy is incomplete.\n";

	// Please implement sha hash checking here to make sure the file is legit.


	if (ferror(WriteFile))
	{
		DBG_DISPLAY_ERROR_LOCATION();
		perror("Write error copying file");
	}
	if (ferror(ReadFile))
	{
		DBG_DISPLAY_ERROR_LOCATION();
		perror("Read error copying file");
	}

	if (fclose(WriteFile))
	{
		perror("Error closing file designated for writing");
		DBG_DISPLAY_ERROR_LOCATION();
	}
	if (fclose(ReadFile))
	{
		perror("Error closing file designated for reading");
		DBG_DISPLAY_ERROR_LOCATION();
	}

	delete[]buffer;
	return false;
}

// Success == 0
// Error == -1
// /* IN */ const char * file_name_and_path
// /* OUT */ xplatform_struct_stat* FileStats
int32_t FileTransfer::getFileStats(const char * file_name_and_path, xplatform_struct_stat* FileStats)
{
	if (FileStats == nullptr || file_name_and_path == nullptr)
	{
		std::cout << "Error: getFileStats() failed. NULL pointer.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return -1;
	}
	// Get some statistics on the file, such as size, time created, etc.
#ifdef _WIN32
	int32_t err_chk = _stat64(file_name_and_path, FileStats);
#endif//_WIN32
#ifdef __linux__
	int32_t err_chk = stat(file_name_and_path, FileStats);
#endif//__linux__

	if (err_chk == -1)
	{
		perror("Error getting file statistics");
		return -1;
	}
	else
	{
		return 0;// success
	}
}

bool FileTransfer::displayFileSize(const char* file_name_and_location, xplatform_struct_stat * FileStatBuf)
{
	if (FileStatBuf == nullptr)
	{
		std::cout << "displayFileSize() failed. NULL pointer.\n";
		DBG_DISPLAY_ERROR_LOCATION();
		return true;
	}
	// Shifting the bits over by 10. This divides it by 2^10 aka 1024
	int64_t KB = FileStatBuf->st_size >> 10;
	int64_t MB = KB >> 10;
	int64_t GB = MB >> 10;
	std::cout << "Displaying file size as Bytes: " << FileStatBuf->st_size << "\n";
	std::cout << "As KB: " << KB << "\n";
	std::cout << "As MB: " << MB << "\n";
	std::cout << "As GB: " << GB << "\n";

	return false;
}

std::string FileTransfer::retrieveFileNameFromPath(std::string file_name_and_path)
{
	std::string error_empty_string;
	// Remove the last \ or / in the name if there is one, so that the name
	// of the file can be separated from the path of the file.
	if (file_name_and_path.back() == '\\' || file_name_and_path.back() == '/')
	{
		perror("ERROR: Found a '\\' or a '/' at the end of the file name.\n");
		std::cout << "Name and location of file that caused the error: " << file_name_and_path << "\n";
		return error_empty_string; //exit please
	}

	const int32_t NO_SLASHES_DETECTED = -1;
	int64_t last_seen_slash_location = NO_SLASHES_DETECTED;
	int64_t name_and_location_of_file_length = file_name_and_path.length();

	if (name_and_location_of_file_length < INT_MAX - 1)
	{
		// iterate backwords looking for a slash
		for (int64_t i = name_and_location_of_file_length; i > 0; --i)
		{
			if (file_name_and_path[(uint32_t)i] == '\\' || file_name_and_path[(uint32_t)i] == '/')
			{
				last_seen_slash_location = i;
				break;
			}
		}

		if (last_seen_slash_location == NO_SLASHES_DETECTED)
		{
			perror("File name and location is invalid. There was not a single \'\\\' or \'/\' found.\n");
			return error_empty_string;
		}
		else if (last_seen_slash_location < name_and_location_of_file_length)
		{
			// Copy the file name from file_name_and_path to file_name.
			std::string file_name(
				file_name_and_path,
				(size_t)last_seen_slash_location + 1,
				(size_t)((last_seen_slash_location + 1) - name_and_location_of_file_length)
			);
			return file_name;
		}
		else
		{
			perror("File name is invalid. Found \'/\' or \'\\\' at the end of the file name.\n");
			return error_empty_string; // exit please
		}
	}
	else
	{
		perror("name_and_location_of_file_length is >= INT_MAX -1");
		return error_empty_string;
	}

	// Shouldn't get here, but if it does, it is an error.
	std::cout << "Impossible location, retrieveFileNameFromPath()\n";
	return error_empty_string;
}
