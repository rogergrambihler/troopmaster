// tm2sqllite.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>

#include "..\Runtime\getopt.h"


#if example


#endif


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
	return 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
	// Setup the default TroopMaster data location
	CString tmDirectory;

	char *sysDrive = getenv("SystemDrive");
	if (NULL == sysDrive)
	{
		tmDirectory = "c:"; // use c as the default if environment not set.
	}
	else
	{
		tmDirectory = sysDrive;
	}

	tmDirectory += "\\Troopmaster Software\\TM4\\DATA";

	CString sqlLiteOutputFile;
	sqlLiteOutputFile  = "tmsqllite.db";

	char opt;
	int flags = 0;
	bool fPrintHelp = false;
	bool fPrintHelpIsFromParseError = false;
	bool fOverWriteExistingFile = false;

	// item for value is argv[optind]
	while ((opt = ::getopt(argc, argv, "i:o:fh?")) != -1) {
		switch (opt) {
		case 'i':
			tmDirectory = optarg;
			break;
		case 'o':
			sqlLiteOutputFile = optarg;
			break;
		case 'f':
			fOverWriteExistingFile = true;
			break;
		case 'h':
		case '?':
			fPrintHelp = true;
			break;
		default: /* '?' */
			fPrintHelpIsFromParseError = true;
			fPrintHelp = true;
			break;
		}
	}

	// check optarg for NULL to know if have any non command line items.
	if (optind < argc)
	{
		fPrintHelp = true;
	}

	if (fPrintHelp)
	{
		FILE* stdHelpOutput = fPrintHelpIsFromParseError ? stderr : stdout;
		fprintf(stdHelpOutput,
			"Usage: %s [-i troopMasterDataDirectory] [-o outputFileName] [-f forceOverWrite]\n", argv[0]);

		fprintf(stdHelpOutput, " -i : Directory that contains the TroopMaster data files. Default is %s.\n", (char *) tmDirectory);
		fprintf(stdHelpOutput, " -o : Name of SqlLite database to create. Default is %s.\n", (char *) sqlLiteOutputFile);
		fprintf(stdHelpOutput, " -f : Will create a new SqlLite database even if one already exits at the -o location.\n");

		exit(EXIT_FAILURE);

	}

	// Check if TroopMaster directory is present.
	tmDb *ptmDb;
	int tmOpenResult = tm_open(tmDirectory, &ptmDb);
	if (0 != tmOpenResult)
	{
		fprintf(stderr, "Did not find a TroopMaster data directory located at %s\n",
			(char *) tmDirectory);
		exit(EXIT_FAILURE);
	}
	
	// Check if output file already exists.
	if (!fOverWriteExistingFile && CStdio::FileExists(sqlLiteOutputFile))
	{
		fprintf(stderr, "The output file %s already exists. Use -f option if want to force an overwrite the existing file.\n",
			(char*) sqlLiteOutputFile);
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "Converting TroopMaster Data at '%s' to SqlLite Database '%s'.\n",
		(char *)tmDirectory, (char *)sqlLiteOutputFile);

	// Make sure the previous output file is removed.
	remove(sqlLiteOutputFile);

	// hack location while bootstrapping items. just to call into the api
	// Make some sql lite calls. review if cleanup properly on error.
	sqlite3 *pDb;

	int result = sqlite3_open(
		sqlLiteOutputFile,   /* Database filename (UTF-8) */
		&pDb          /* OUT: SQLite db handle */
		);

	if (0 != result)
	{
		fprintf(stderr, "An error occured openning sql file %s. Errorcode %d\n",
			(char *) sqlLiteOutputFile, result);
		exit(EXIT_FAILURE);
	}

	char *zErrMsg;
	CallbackData *pCallbackData = new CallbackData();
	pCallbackData->pDb = pDb;

	// FUTURE: implement way in TMAPI to get table names but for now just know the list
	const int TMNUMTABLES = 8;
	char* tableName[TMNUMTABLES] =
	{
		"activity",
		"adultdata",
		"advancement",
		"attend",
		"mbcdata",
		"parentdata",
		"pocdata",
		"scoutdata",
	};

	for (int tableIndex = 0; tableIndex < TMNUMTABLES; tableIndex++)
	{
		fprintf(stdout, "Creating table: '%s'.\n",
			tableName[tableIndex]);

		CString selectStatement;
		selectStatement = "select * from ";
		selectStatement += tableName[tableIndex];

		sqlite3_exec(pDb, "BEGIN TRANSACTION;", NULL, NULL, NULL);
		pCallbackData->createdTabled = false;
		pCallbackData->pTableName = tableName[tableIndex];
		pCallbackData->recordCount = 0;
		int execResult = tm_exec(ptmDb, selectStatement, callback, (void*)pCallbackData/* data */, &zErrMsg);
		if (0 != execResult)
		{
			// out the error but keep going
			fprintf(stderr, "An error occured getting the %s table. %s.\n",
				tableName[tableIndex], zErrMsg);
			continue;
		}

		execResult = sqlite3_exec(pDb, "END TRANSACTION;", NULL, NULL, NULL);
		if (0 != execResult)
		{
			// print out the error but keep going
			fprintf(stderr, "An error occured in sqlLite transaction. %s.\n",
				execResult);
			continue;
		}
	}

	sqlite3_close(pDb);

	return 0;
}

