#include "stdafx.h"
#include "CStdio.h"

#include <sys\stat.h>
#include <stdio.h>
#include <string.h>

CStdio::CStdio()
{
}


CStdio::~CStdio()
{
}

/* static */ bool CStdio::DirectoryExists(char* dirPath)
{
	// todo: stat returns an error if \\ is passed in the end. Review if should handle or make caller check and pull
	// hair out why the directory doesn't exist if pass in c:\dirname\ but c:\dirname works.
	struct stat statbuf;
	int statResult = stat(dirPath, &statbuf);
	return (0 == statResult) && (statbuf.st_mode & S_IFDIR) ? true : false;
}

/* static */ bool CStdio::FileExists(char* filePath)
{
	struct stat statbuf;
	int statResult = stat(filePath, &statbuf);
	return (0 == statResult) && (statbuf.st_mode & S_IFMT) ? true : false;
}

// file Classes
CFileDisk::CFileDisk()
{
	m_f = NULL; // todo: review are c++ classes garanteed zero'd out?
}

CFileDisk::~CFileDisk()
{
	if (NULL != m_f)
	{
		fclose(m_f);
	}
}

int CFileDisk::Open(char *filePath, char *mode)
{
	errno_t r = fopen_s(&m_f, filePath, mode);
	if (0 == r)
	{
		m_fullPath = filePath; // remember the file path.
		setvbuf(m_f, new char[30000], _IOFBF, 30000);
	}
	else
	{
		m_f = NULL; // don't trust fopen to not set m_f on error.
	}

	return r;
}

// int returned is the number of bytes read.
unsigned int CFileDisk::Read(void* lpBuf, unsigned int nCount)
{
	size_t cbRead =  fread(lpBuf, sizeof(char), nCount, m_f);
	return cbRead;
}

unsigned long CFileDisk::GetPosition()
{
	long curPos = ftell(m_f); // get current file pointer
	return curPos;
}

unsigned long CFileDisk::SeekPosition(unsigned long pos)
{
	fseek(m_f, pos, SEEK_SET); 
	return GetPosition();
}

unsigned long CFileDisk::GetLength()
{
	// get length of file
	long origPos = GetPosition();
	fseek(m_f, 0, SEEK_END); // seek to end of file
	long size = GetPosition();
	fseek(m_f, origPos, SEEK_SET); // seek back
	return size;
}

/* hash for decrypt bytes at least for this one is
0:000> db 100c3858
100c3858  2f 52 66 7a 20 31 6b 4f-35 38 33 59 4c 4d 76 26  /Rfz 1kO583YLMv&
100c3868  32 63 64 65 45 6e 70 57-54 36 50 23 55 49 62 6f  2cdeEnpWT6P#UIbo
100c3878  41 51 47 58 73 5a 61 44-74 79 69 42 48 6c 4e 37  AQGXsZaDtyiBHlN7
100c3888  34 71 72 43 2d 6d 75 53-30 4a 68 6a 56 77 4b 46  4qrC-muS0JhjVwKF
100c3898  39 67 78 00 00 00 00 00-00 00 00 00 e8 35 25 00  9gx..........5%.
100c38a8  30 32 78 6f 6c 72 52 4e-57 35 4b 62 6f 50 30 00  02xolrRNW5KboP0.
100c38b8  00 00 00 00 00 00 00 00-00 00 00 00 00 00 00 00  ................
100c38c8  00 00 00 00 00 00 00 00-00 00 00 00 00 00 00 00  ................
*/
// edi,offset TSI!EncodeString (100c3858)

unsigned long CFileTMRecEncrypt::GetLength()
{
	return m_pFile->GetLength();
}
unsigned long CFileTMRecEncrypt::GetPosition()
{
	return m_pFile->GetPosition();
}


unsigned long CFileTMRecEncrypt::SeekPosition(unsigned long pos)
{
	return m_pFile->SeekPosition(pos);
}

const unsigned char CFileTMRecEncrypt::m_decryptDictionary[] =
{ 0x2f, 0x52, 0x66, 0x7a, 0x20, 0x31, 0x6b, 0x4f, 0x35, 0x38, 0x33, 0x59, 0x4c, 0x4d, 0x76, 0x26,
0x32, 0x63, 0x64, 0x65, 0x45, 0x6e, 0x70, 0x57, 0x54, 0x36, 0x50, 0x23, 0x55, 0x49, 0x62, 0x6f,
0x41, 0x51, 0x47, 0x58, 0x73, 0x5a, 0x61, 0x44, 0x74, 0x79, 0x69, 0x42, 0x48, 0x6c, 0x4e, 0x37,
0x34, 0x71, 0x72, 0x43, 0x2d, 0x6d, 0x75, 0x53, 0x30, 0x4a, 0x68, 0x6a, 0x56, 0x77, 0x4b, 0x46,
0x39, 0x67, 0x78 };

unsigned int CFileTMRecEncrypt::Read(void* lpBuf, unsigned int nCount)
{
	// Get the current  seek position before reading to calculate record position.
	unsigned long curPos = GetPosition();
	unsigned int cbRead = m_pFile->Read(lpBuf, nCount);

	// calculate the record and index within a record
	// todo: rename these since not curPos when used in logic
	unsigned int curRecord = curPos / m_recordSize; // zero based.
	unsigned int curPosInRecord = curPos % m_recordSize; // zero based.

	// loop through the bytes read and fix it 
	for (unsigned int index = 0; index < cbRead; index++)
	{
		unsigned char *pBuf = ((unsigned char *) lpBuf) + index;
		unsigned char b = *pBuf;
		if (0 != b)
		{
			// calculate the offset into the decrypt dictionary
			unsigned int decryptDictIndex = (curRecord + curPosInRecord + index) % sizeof(m_decryptDictionary);
			unsigned char dictValue = m_decryptDictionary[decryptDictIndex];
			b = b - dictValue; // todo: will unsigned work properly with this???? forget overflow logic
			*pBuf = b;
		}
	}

	/*
	C## reference code
	           byte[] decryptDict = this.DecryptDictionary;
            byte[] decoded = new byte[length];
            for (int index = 0; index < length; index++)
            {
                byte curByte = data[index];
                if (0 != curByte)
                {
                    byte decryptBytePost = (byte)((index + decryptOffset) % decryptDict.Length);
                    byte unEncryptByte = (byte)(curByte - decryptDict[decryptBytePost]);
                    decoded[index] = unEncryptByte;
                }
            }
*/

	return cbRead;
}