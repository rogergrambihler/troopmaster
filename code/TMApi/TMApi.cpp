
// main entry points for TM Api
// todo need some paramater checks and general evaluation.fs
#include "stdafx.h"

extern "C" TM_API int tm_open(
	const char *szDirName,  /* Database directory (UTF-8) */
	tmDb **ppDb          /* OUT: TM db handle */
	)
{
	*ppDb = NULL;

	// create a database object
	CTMDb *pDb = new CTMDb();
	int result = pDb->Connect(szDirName);
	if (TM_OK == result)
	{
		*ppDb = (tmDb *)pDb;
	}
	else
	{
		delete pDb;
	}

	return result;
}

TM_API int tm_close(tmDb* db)
{
	CTMDb *pDb = (CTMDb *) db;
	delete pDb;
	return TM_OK;
}

TM_API int tm_exec(
	tmDb *db,                                  /* An open database */
	const char *sql,                           /* SQL to be evaluated */
	int(*callback)(void*, int, const char**, const char**),  /* Callback function */
	void * blob,                                    /* 1st argument to callback */
	char **errmsg                              /* Error msg written here */
	)
{
	// todo: param validation and proper errors.
	CTMDb *pDb = (CTMDb *)db;
	return pDb->Exec(sql, callback, blob, errmsg);
}

#if later
extern "C" TM_API int tm_get_table(
	tmDb *db,          /* An open database */
	const char *zSql,     /* SQL to be evaluated */
	char ***pazResult,    /* Results of the query */
	int *pnRow,           /* Number of result rows written here */
	int *pnColumn,        /* Number of result columns written here */
	char **pzErrmsg       /* Error msg written here */
	)
{
	CTMDb *pDb = (CTMDb *) db;
	return pDb->GetTable(zSql, pazResult, pnRow, pnColumn, pzErrmsg);
}
#endif