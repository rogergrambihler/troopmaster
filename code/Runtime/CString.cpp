#include "stdafx.h"
#include <string.h>

CString::CString()
{
	m_pBuffer = NULL;
	m_cbBuffer = 0;
	m_len = 0;
}

// if pass in initializ size preallocate the buffer
CString::CString(int initialSize)
{
	m_pBuffer = new char[initialSize];
	m_cbBuffer = initialSize;
	m_len = 0;
}


CString::~CString()
{
	SetToNull();
}


int CString::GetLength()
{
	return m_len;
}

void CString::TrimEnd(char c)
{
	if (m_len < 1) { return; }

	char *pStart = m_pBuffer + m_len - 1;
	while (*pStart == c)
	{
		*pStart = NULL;
		--m_len;
	}
}


const CString& CString::operator =(
	CString& s) 
{
	AssertSz(NULL != &s, "null CString passed to = operator.")
	return operator=( (char *) s);
}

const CString& CString::operator =(
	const char *pData) // assume zero terminated
{
	if (NULL == pData)
	{
		SetToNull();
	}
	else
	{
		m_len = strlen(pData);

		// check if have a buffer and big enough size else alocate one.
		if (m_cbBuffer < m_len + 1)
		{
			if (NULL != m_pBuffer)
			{
				delete m_pBuffer;
			}

			m_cbBuffer = m_len + 1;
			m_pBuffer = new char[m_cbBuffer];
		}

		// duplicate and store the directory name.
		memcpy(m_pBuffer, pData, m_len + 1);
	}

	return *this;
}

const CString& CString::operator +=(
	const char *pData) // assume zero terminated
{
	int concatLen = strlen(pData);

	// check if need to resize.
	if (concatLen + m_len + 1 > m_cbBuffer)
	{
		char *prevBuffer = m_pBuffer; // stash away the buffer in case have to free.
		m_cbBuffer = concatLen + m_len + 1;
		m_pBuffer = new char[m_cbBuffer]; // allocate the new buffer.

		if (m_len > 0) // if have a previous string copy it over
		{
			memcpy(m_pBuffer, prevBuffer, m_len);
		}

		// if have a previuos buffer remove.
		if (NULL != prevBuffer)
		{
			delete prevBuffer;
		}
	}

	// concat on the new string.
	memcpy(m_pBuffer + m_len, pData, concatLen + 1); // copy over the null.
	
	// now update the lenght member.
	m_len = m_len + concatLen;
	return *this;
}

CString::operator char* () const
{
	return m_pBuffer;
}

// Reset the String buffer to null.
void CString::SetToNull()
{
	if (NULL != m_pBuffer)
	{
		delete m_pBuffer;
		m_pBuffer = NULL;
	}

	m_cbBuffer = 0;
	m_len = 0;
}

/*
friend CString  CString::operator +(
	const CString& string1,
	const CString& string2);
	*/