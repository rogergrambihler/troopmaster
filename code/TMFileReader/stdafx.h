// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <tchar.h> // todo: something in this for the winmain crt. If remove crt I think can remove this


// TODO: reference additional headers your program requires here
// todo: references from TMApi. If kept need to cleanup how to have a public .h
// or standard way to do this.
// #include "..\TMApi\CString.h"
#include "..\Runtime\Runtime.h"
#include  "..\dependencies\SqlLite\sqlite3.h"
#include "..\TMApi\TMApi.h"
