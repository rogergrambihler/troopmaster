// TMFileReader.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "TMFileReader.h"
#include <stdio.h> // todo: abstract or remove just here for testin sqlLite output

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


typedef struct tag_callBackData
{
	sqlite3 *pDb;
	bool createdTabled;
	char *pTableName;
	unsigned int recordCount;
} CallbackData;


// sql lite callback.
static int callback(void *data, int argc, const char **argv, const char **azColName){

	CallbackData *pCallback = (CallbackData *)data;

	if (!pCallback->createdTabled)
	{
		pCallback->createdTabled = true;

		CString psql(90);

		// table needs to be created so setup it up.
		// Hack I'm not sure if way to sell SQlLite to just use the row as the primary 
		// but create an ID int and then always write along with the records.s
		psql += "CREATE TABLE ";
		psql += pCallback->pTableName;
		psql += "(ID INT PRIMARY KEY  NOT NULL";

		for (int i = 0; i < argc; i++)
		{
			// add in the other tables
			psql += ", "; // add a comma
			psql += azColName[i];
			psql += " TEXT ";
		}

		psql += ");"; // close the statement

		/* Execute SQL statement */

		char *zErrMsg;
		int rc = SQLITE_OK;
		rc = sqlite3_exec(pCallback->pDb, psql, NULL /* callback */, 0, &zErrMsg);
		if (rc != SQLITE_OK){
			// bad
			rc = 0;
		}
	}

#ifdef insertexample
	sql = "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
		"VALUES (1, 'Paul', 32, 'California', 20000.00 ); " \

#endif

	CString sql(3000);
	sql += "INSERT INTO ";
	sql += pCallback->pTableName;
	sql += " (ID";

	// make a items for each column
	for (int i = 0; i < argc; i++)
	{
		// add in the other tables
		sql += ", "; // add a comma
		sql += azColName[i];
	}
	sql += ") Values (";

	// todo: could probably do inplace but not sure if c++ handles.
	unsigned int recordCount = pCallback->recordCount; // try recordCount  of zero for lining up data.

	char *pOutput = new char[500]; // todo: obviously this is too large but need to figure out how to set.
	_itoa_s(recordCount, pOutput, 500, 10);

	sql += pOutput;

	// write value for each column
	for (int i = 0; i < argc; i++)
	{
		// add in the other tables
		sql += ", "; // add a comma

		if (NULL != argv[i])
		{
		char *zSQL = sqlite3_mprintf("'%q'", argv[i]);
		sql += zSQL; // todo: review if way to do an escape since text could have the quote.
		sqlite3_free(zSQL);
		}
		else{
			sql += "NULL";
		}
	}
	
	sql += ");";
	
	// give it a try
	char *zErrMsg;
	int rc2 = sqlite3_exec(pCallback->pDb, sql, NULL /* callback */, 0, &zErrMsg);
	if (rc2 != SQLITE_OK){
		// bad
		rc2 = 0;
	}

	++recordCount;
	pCallback->recordCount = recordCount;


#if loop
	int i;
//	fprintf(stderr, "%s: ", (const char*)data);
	for (i = 0; i<argc; i++){
		// create an Insert record.

	//	printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	//printf("\n");
#endif
	return 0;
}


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TMFILEREADER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TMFILEREADER));


	// hack location while bootstrapping items. just to call into the api
	// Make some sql lite calls.
	sqlite3 *pDb;

	char sqlLiteOutputFile[] = "testdb.db"; //  If f5 in Visual studio this will get created in the TMFileReader folder;
	remove(sqlLiteOutputFile);

	int result = sqlite3_open(
		sqlLiteOutputFile,   /* Database filename (UTF-8) */
		&pDb          /* OUT: SQLite db handle */
		);

	if (0 != result)
	{
		// todo: test file not found behavior and error message handling.
		// bad
	}


	// Troop master intersperse
	tmDb *ptmDb;
	tm_open("C:\\Troopmaster Software\\TM4\\DATA", &ptmDb); // todo: enforce slash or add??

	// TODO: check open error code and for NULL == ptmDb

	// select with the callback
	/* Create SQL statement */
	const char *sql = "SELECT * from test";

	// hack put table names in lower case for now since that is what recognize.
	/* Execute SQL statement */
	char *zErrMsg;
	// sqlite3_exec(pDb, sql, callback, (void*)3 /* data */, &zErrMsg);

	CallbackData *pCallbackData = new CallbackData();

	// global 
	pCallbackData->pDb = pDb;

	sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	pCallbackData->createdTabled = false;
	pCallbackData->pTableName = "mbcdata";
	pCallbackData->recordCount = 0;
	tm_exec(ptmDb, "select * from mbcdata", callback, (void*)pCallbackData/* data */, &zErrMsg);
	sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);



	sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	pCallbackData->createdTabled = false;
	pCallbackData->pTableName = "attend";
	pCallbackData->recordCount = 0;
	tm_exec(ptmDb, "select * from attend", callback, (void*)pCallbackData /* data */, &zErrMsg);
	sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);

	sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	pCallbackData->createdTabled = false;
	pCallbackData->pTableName = "parentdata";
	pCallbackData->recordCount = 0;
	tm_exec(ptmDb, "select * from parentdata", callback, (void*)pCallbackData/* data */, &zErrMsg);
	sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);


	sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	pCallbackData->createdTabled = false;
	pCallbackData->pTableName = "scoutdata";
	pCallbackData->recordCount = 0;
	tm_exec(ptmDb, "select * from scoutdata", callback, (void*)pCallbackData /* data */, &zErrMsg);
	sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);

	sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	pCallbackData->createdTabled = false;
	pCallbackData->pTableName = "activity";
	pCallbackData->recordCount = 0;
	tm_exec(ptmDb, "select * from activity", callback, (void*)pCallbackData/* data */, &zErrMsg);
	sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);


	sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	pCallbackData->createdTabled = false;
	pCallbackData->pTableName = "adultdata";
	pCallbackData->recordCount = 0;
	tm_exec(ptmDb, "select * from adultdata", callback, (void*)pCallbackData/* data */, &zErrMsg);
	sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);

	sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	pCallbackData->createdTabled = false;
	pCallbackData->pTableName = "advancement";
	pCallbackData->recordCount = 0;
	tm_exec(ptmDb, "select * from advancement", callback, (void*)pCallbackData/* data */, &zErrMsg);
	sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);

	sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
	pCallbackData->createdTabled = false;
	pCallbackData->pTableName = "pocdata";
	pCallbackData->recordCount = 0;
	tm_exec(ptmDb, "select * from pocdata", callback, (void*)pCallbackData/* data */, &zErrMsg);
	sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);


	// display a table
	const char *sqlSelect = "SELECT * FROM Test;";

	char **results = NULL;
	int rows, columns;
	char *error; // language??
	int rc = sqlite3_get_table(pDb, sqlSelect, &results, &rows, &columns, &error);
	if (rc)
	{
		//cerr << "Error executing SQLite3 query: " << sqlite3_errmsg(db) << endl << endl;
		sqlite3_free(error);
	}
	else
	{
		// Display Table
		for (int rowCtr = 0; rowCtr <= rows; ++rowCtr)
		{
			for (int colCtr = 0; colCtr < columns; ++colCtr) // first row is the Column Names???
			{
				// Determine Cell Position
				int cellPosition = (rowCtr * columns) + colCtr;
				char *pResultText = results[cellPosition]; // todo how does this read different DataType.

				// Display Cell Value
			//	cout.width(12);
			//	cout.setf(ios::left);
			//	cout << results[cellPosition] << " ";
			}

			// End Line
			//cout << endl;

			// Display Separator For Header
			if (0 == rowCtr)
			{
				for (int colCtr = 0; colCtr < columns; ++colCtr)
				{
				//	cout.width(12);
				//	cout.setf(ios::left);
				//	cout << "~~~~~~~~~~~~ ";
				}
				// cout << endl;
			}
		}
	}
	sqlite3_free_table(results);

	// query the table.

	sqlite3_close(pDb);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TMFILEREADER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TMFILEREADER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
