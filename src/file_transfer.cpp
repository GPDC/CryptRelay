// file_transfer.cpp
#include <string>
#include <fstream>
#include <iostream>

#ifdef _WIN32
#include <WS2tcpip.h>


#endif//_WIN32

#include "file_transfer.h"

FileTransfer::FileTransfer()
{

}
FileTransfer::~FileTransfer()
{

}

// Set encrypt_the_file to true if you want
// to encrypt the file before transfering it.
// If encryption option is enabled, then it will
// copy the file into the same directory the original
// file is located and encrypt it there.
bool FileTransfer::beginFileTransfer(const char * read_file_name_and_location, bool encrypt_the_file)
{
	std::string name_of_copied_file = read_file_name_and_location;
	name_of_copied_file += ".enc";	// This could be changed to an appropriate extension

	if (encrypt_the_file == true)
	{
		if (copyFile(read_file_name_and_location, name_of_copied_file.c_str()) == false)
		{

			std::cout << "File transfer with the encryption option enable failed at copyFile().\n";
			return false;
		}
		else
		{
			// if (Encrypt the copied file() == true)
			//		read_file_name_and_location = name_of_copied_file;
			// else
			// {
			//		std::cout "File transfer with encryption option enabled failed at encryptThisFile().\n";
			//		return false;
			// }
		}
	}

	if (sendFile(read_file_name_and_location, s) == false)
	{
		std::cout << "File transfer failed at sendFile().\n";
		return false;
	}

	return true;
}

bool FileTransfer::sendFile(const char * file_name_and_location, SOCKET s)
{

	FILE *ReadFile;


	ReadFile = fopen(file_name_and_location, "rb");
	if (ReadFile == NULL)
	{
		perror("Error opening file for reading binary");
		return false;
	}

	int file_stat_error = false;
	struct stat FileStatBuf;

	// Get some statistics on the file, such as size, time created, etc.
	int r = stat(file_name_and_location, &FileStatBuf);
	if (r == -1)
	{
		perror("Error getting file statistics");
		file_stat_error = true;
	}
	else
	{
		// Output to terminal the size of the file in Bytes, KB, MB, GB.
		displayFileSize(file_name_and_location, &FileStatBuf);
	}



	// Please make a sha hash of the file here so it can be checked with the
	// hash of the copy later.

	size_t bytes_read;
	size_t bytes_sent;
	unsigned long long total_bytes_sent = 0;

	unsigned long long twenty_five_percent = FileStatBuf.st_size / 4;	// divide it by 4
	unsigned long long fifty_percent = FileStatBuf.st_size / 2;	//divide it by two
	unsigned long long seventy_five_percent = FileStatBuf.st_size - twenty_five_percent;

	bool twenty_five_already_spoke = false;
	bool fifty_already_spoke = false;
	bool seventy_five_already_spoke = false;

	// (8 * 1024) == 8,192 aka 8KB, and 8192 * 1024 == 8,388,608 aka 8MB
	const size_t buffer_size = 8 * 1024 * 1024;
	unsigned char* buffer = new unsigned char[buffer_size];

	std::cout << "Copying file...\n";
	do
	{
		bytes_read = fread(buffer, 1, buffer_size, ReadFile);
		if (bytes_read)
			bytes_sent = send();
		else
			bytes_sent = 0;

		total_bytes_sent += bytes_sent;

		// If we have some statistics to work with, then display
		// ghetto progress indicators.
		if (file_stat_error == false)
		{
			if (total_bytes_sent > twenty_five_percent && total_bytes_sent < fifty_percent && twenty_five_already_spoke == false)
			{
				std::cout << "File copy 25% complete.\n";
				twenty_five_already_spoke = true;
			}
			if (total_bytes_sent > fifty_percent && total_bytes_sent < seventy_five_percent && fifty_already_spoke == false)
			{
				std::cout << "File copy 50% complete.\n";
				fifty_already_spoke = true;
			}
			if (total_bytes_sent > seventy_five_percent && seventy_five_already_spoke == false)
			{
				std::cout << "File copy 75% complete.\n";
				seventy_five_already_spoke = true;
			}
		}


		// 0 means error. If they aren't equal to eachother
		// then it didn't write as much as it read for some reason.
	} while ((bytes_read > 0) && (bytes_read == bytes_sent));

	if (total_bytes_sent == FileStatBuf.st_size)
		std::cout << "File copy complete.\n";

	// Please implement sha hash checking here to make sure the file is legit.

	if (bytes_sent)
		perror("Error while copying the file");



	if (fclose(WriteFile))
		perror("Error closing file designated for writing");
	if (fclose(ReadFile))
		perror("Error closing file designated for reading");

	delete[]buffer;

	return true;

	/*~~~~~~~ flags ~~~~~~~

	ios::app	Opens the file in append mode

	ios::ate	Opens the file and set the cursor at end of the file

	ios::binary Opens the file in binary mode

	ios::in		Opens the file for reading

	ios::out	Opens the file for writing

	ios::trunc	Opens the file and truncates all the contents from it

	~~~~~~~~~ flags ~~~~~~~~~
	*/
}

// read_file_name_and_location example would be:
// C:\Users\YourName\Downloads\ravioli.txt
// if no location is specified, then the file will be taken from
// or copied to the directory that CryptRelay is located in.
// Example: copyFile("C:\Downloads\foodrecipe.txt", "foodrecipe_copy.txt");
// That example takes reads foodrecipe.txt from your downloads folder and
// copies it to the folder that your CryptRelay program is located with the
// new name: foodrecipe_copy.txt.
bool FileTransfer::copyFile(const char * read_file_name_and_location, const char * write_file_name_and_location)
{

	FILE *ReadFile;
	FILE *WriteFile;

	ReadFile = fopen(read_file_name_and_location, "rb");
	if (ReadFile == NULL)
	{
		perror("Error opening file for reading binary");
		return false;
	}
	WriteFile = fopen(write_file_name_and_location, "wb");
	if (WriteFile == NULL)
	{
		perror("Error opening file for writing binary");
		return false;
	}

	int file_stat_error = false;
	struct stat FileStatBuf;

	// Get some statistics on the file, such as size, time created, etc.
	int r = stat(read_file_name_and_location, &FileStatBuf);
	if (r == -1)
	{
		perror("Error getting file statistics");
		file_stat_error = true;
	}
	else
	{
		// Output to terminal the size of the file in Bytes, KB, MB, GB.
		displayFileSize(read_file_name_and_location, &FileStatBuf);
	}



	// Please make a sha hash of the file here so it can be checked with the
	// hash of the copy later.

	size_t bytes_read;
	size_t bytes_written;
	unsigned long long total_bytes_written = 0;

	unsigned long long twenty_five_percent = FileStatBuf.st_size / 4;	// divide it by 4
	unsigned long long fifty_percent = FileStatBuf.st_size / 2;	//divide it by two
	unsigned long long seventy_five_percent = FileStatBuf.st_size - twenty_five_percent;

	bool twenty_five_already_spoke = false;
	bool fifty_already_spoke = false;
	bool seventy_five_already_spoke = false;

	// (8 * 1024) == 8,192 aka 8KB, and 8192 * 1024 == 8,388,608 aka 8MB
	const size_t buffer_size = 8 * 1024 * 1024;
	unsigned char* buffer = new unsigned char[buffer_size];

	std::cout << "Copying file...\n";
	do
	{
		bytes_read = fread(buffer, 1, buffer_size, ReadFile);
		if (bytes_read)
			bytes_written = fwrite(buffer, 1, bytes_read, WriteFile);
		else
			bytes_written = 0;

		total_bytes_written += bytes_written;

		// If we have some statistics to work with, then display
		// ghetto progress indicators.
		if (file_stat_error == false)
		{
			if (total_bytes_written > twenty_five_percent && total_bytes_written < fifty_percent && twenty_five_already_spoke == false)
			{
				std::cout << "File copy 25% complete.\n";
				twenty_five_already_spoke = true;
			}
			else if (total_bytes_written > fifty_percent && total_bytes_written < seventy_five_percent && fifty_already_spoke == false)
			{
				std::cout << "File copy 50% complete.\n";
				fifty_already_spoke = true;
			}
			else if (total_bytes_written > seventy_five_percent && seventy_five_already_spoke == false)
			{
				std::cout << "File copy 75% complete.\n";
				seventy_five_already_spoke = true;
			}
		}


		// 0 means error. If they aren't equal to eachother
		// then it didn't write as much as it read for some reason.
	} while ((bytes_read > 0) && (bytes_read == bytes_written));

	if (total_bytes_written == FileStatBuf.st_size)
		std::cout << "File copy complete.\n";

	// Please implement sha hash checking here to make sure the file is legit.

	if (bytes_written)
		perror("Error while copying the file");



	if (fclose(WriteFile))
		perror("Error closing file designated for writing");
	if (fclose(ReadFile))
		perror("Error closing file designated for reading");

	delete[]buffer;

	return true;

	//// Give a compiler error if streamsize > sizeof(long long)
	//// This is so we can safely do this: (long long) streamsize
	//static_assert(sizeof(std::streamsize) <= sizeof(long long), "myERROR: streamsize > sizeof(long long)");
}

bool FileTransfer::displayFileSize(const char* file_name_and_location, struct stat * FileStatBuf)
{
	if (FileStatBuf == NULL)
	{
		std::cout << "displayFileSize() failed. NULL pointer.\n";
		return false;
	}
	// Shifting the bits over by 10. This divides it by 2^10 aka 1024
	off_t KB = FileStatBuf->st_size >> 10;
	off_t MB = KB >> 10;
	off_t GB = MB >> 10;
	std::cout << "File size: " << FileStatBuf->st_size << "Bytes\n";
	std::cout << "File size: " << KB << " KB\n";
	std::cout << "File size: " << MB << " MB\n";
	std::cout << "File size: " << GB << " GB\n";

	return true;
}