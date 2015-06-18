//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#ifndef _SAMPLEDEBUG_
#define _SAMPLEDEBUG_

#if _DEBUG

void FnAssert(char* lpstrExpr, char* lpstrMsg, char* lpstrFileName, unsigned int iLine);

#undef Assert
#undef AssertSz
#undef Puts
#undef TRACE

#define Assert(a) { if (!(a)) FnAssert(#a, NULL, __FILE__, __LINE__); }
#define AssertSz(a, b) { if (!(a)) FnAssert(#a, b, __FILE__, __LINE__); }
#define Puts(s) OutputDebugStringA(s)
#define TRACE(s)  OutputDebugStringA(s)

#else // !_DEBUG

#define Assert(a)
#define AssertSz(a, b)
#define Puts(s)
#define TRACE(s)

#endif  // _DEBUG


#endif // _SAMPLEDEBUG_
