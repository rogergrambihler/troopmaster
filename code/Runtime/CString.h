
// todo: copy/pasted this between both projects. should make a static lib that 
// both can share.

#pragma once

// lightweight CString modeled after MFC
class CString
{
public:
	CString();
	CString(int initialSize);
	~CString();

	int GetLength();
	void TrimEnd(char c);

	const CString& operator =(
		const char *);
	const CString& operator =(
		CString&);

	const CString& operator +=(
		const char *);

	operator char* () const;

private:

	void SetToNull();

	char *m_pBuffer; // pointer to buffer.
	int m_cbBuffer; // size of the buffer
	int m_len;  // length of the string in the buffer Does not include the null.
};

