

#include "stdafx.h"

//+---------------------------------------------------------------------------
//
//  Function:   FnAssert, public
//
//  Synopsis:   Displays the Assert dialog
//
//  Arguments:  [lpstrExptr] - Expression
//      [lpstrMsg] - Msg, if any, to append to the Expression
//      [lpstrFilename] - File Assert Occured in
//      [iLine] - Line Number of Assert
//
//  Returns:    Appropriate status code
//
//  Modifies:
//
//
//----------------------------------------------------------------------------

#if _DEBUG == 1

void FnAssert(char* lpstrExpr, char* lpstrMsg, char* lpstrFileName, unsigned int iLine)
{

	// todo: Need to remove windows specific code
#if todo

	int iResult;
	char lpTemp[] = "";
	char lpBuffer[512];
	char lpLocBuffer[256];
	if (NULL == lpstrMsg)
		lpstrMsg = lpTemp;

	wsprintfA(lpBuffer, "Assertion \"%s\" failed! %s", lpstrExpr, lpstrMsg);
	wsprintfA(lpLocBuffer, "File %s, line %d; (A=exit; R=break; I=continue)",
		lpstrFileName, iLine);
	iResult = MessageBoxA(NULL, lpLocBuffer, lpBuffer,
		MB_ABORTRETRYIGNORE | MB_SYSTEMMODAL);

	if (iResult == IDRETRY)
	{
		DebugBreak();
	}
	else if (iResult == IDABORT)
	{
		FatalAppExitA(0, "Assertion failure");
	}
	return NOERROR;
#endif
}

#endif
