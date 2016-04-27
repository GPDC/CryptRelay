// file_transfer.h

// Overview:
// This source file is for transfering files from one place to another.
// That means network transfer, hard drive transfer, whatever.

#ifndef file_transfer_h__
#define file_transfer_h__

class FileTransfer
{
public:
	FileTransfer();
	~FileTransfer();

	bool beginFileTransfer(const char * read_file_name_and_location, bool encrypt_the_file = false);
	bool sendFile(const char * file_name, SOCKET s);
	bool copyFile(const char * read_file_name_and_location, const char * write_file_name_and_location);
	bool displayFileSize(const char* file_name_and_location, struct stat * FileStatBuf);

protected:
private:
};

#endif//file_transfer_h__