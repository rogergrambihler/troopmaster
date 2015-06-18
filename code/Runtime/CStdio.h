#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "CString.h" // todo: shouldn't need this here. Break out CFiles from here


// base abstract class for a file stream
// todo: may need a seek when get to the nonSequential tables.
class CFile
{
public:
	virtual unsigned int Read(void* lpBuf, unsigned int nCount) = 0;
	virtual unsigned long GetLength() = 0;
	virtual unsigned long GetPosition() = 0;
	virtual unsigned long SeekPosition(unsigned long) = 0; // seek to absolutie position. 
};
// wrapper for IO
// todo: consider an abstract base for a Stream the CFile inherits from. In MFC  objects derive from CFile such a CMemFile 
// so need to decide if want to model MFC or put in a CStream or an interface
// todo: consider if CFile makes sense as a base class or should have CStream as the base class if CFile 
//  consures up a file on disk. use Cfile for now so minimize changes to bootstrap
class CFileDisk : public CFile
{
	CString m_fullPath;
	FILE *m_f;

public:
	CFileDisk();
	~CFileDisk();

	virtual int Open(char *filePath, char *mode);
	virtual unsigned int Read(void* lpBuf, unsigned int nCount);
	virtual unsigned long GetLength();
	virtual unsigned long GetPosition();
	virtual unsigned long SeekPosition(unsigned long);
	// friend class CStdio;
};

// todo: doesn't belong here and need to decide if convention is left to right on base class name to file
// or classes should start with TM such as TMEncryptedCFile

// handles the encryption for a TroopMaster data file such as Scout and Parent
// that are record encrypted. The TMEncrypt is given the record size and then based on
// the record size and seek position determines how to decrypts. This way the code reading
// the file gets unencrypted data even if not reading on record boundaries.
// todo: possibly just make a helper? If wanted to read a stream from a different store would
// then have to wrote another decrypt. Maybe have a Decrypter that takes a CFile or make a CStream base class
// and a stream type that takes a file or file inherits. Check out what MFC does
class CFileTMRecEncrypt : public CFile
{
private:
	unsigned int m_recordSize; // size of each record in the file.
	CFile *m_pFile;

	/// <summary>
	/// Key used by TroopMaster to decrypt data.
	/// </summary>
	static const unsigned char m_decryptDictionary[];

public:
	CFileTMRecEncrypt(CFile *pFile, unsigned int recordSize)
	{
		m_pFile = pFile;
		m_recordSize = recordSize;
	};

	~CFileTMRecEncrypt()
	{
		if (NULL != m_pFile)
		{
			delete m_pFile;
		}
	}

	virtual unsigned int Read(void* lpBuf, unsigned int nCount);
	virtual unsigned long GetLength();
	virtual unsigned long GetPosition();
	virtual unsigned long SeekPosition(unsigned long);
};


class CStdio
{
public://
	CStdio();
	~CStdio();
	
	// const static int MAX_PATH = 260; //  _MAX_PATH;

	// consider if want separate directory of file classes.
	static bool DirectoryExists(char* dirPath);
	static bool FileExists(char* dirPath);
};
