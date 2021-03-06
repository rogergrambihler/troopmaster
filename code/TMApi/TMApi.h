
// public header file for TroopMaster format reader
// modeled after sqllite.

// todo: MFC seems to have support for an ODBC driver. I thought Win8 dropped support? If not maybe an ODBC driver
// or something else that is fairly standard and load into Excel would be good but also may be >> of workd for
// just displaying some tables.

// todo: i'm pretty sure use to have some code to slam the vTable when object disposed which is handy
// for catching deleted objects.

#ifndef _TM_H_
#define _TM_H_

/*
** Make sure we can call this stuff from C++.
*/
#ifdef __cplusplus
extern "C" {
#endif

#ifndef TM_API
# define TM_API
#endif

#define TM_VERSION "1.0.0.0"

typedef struct tmDb tmDb;

// TM directory. Typically "C:\Troopmaster Software\TM4\DATA";
// %SystemDrive%\Troopmaster Software\TM4\DATA
TM_API int tm_open(
	const char *szDirName,  /* Database filename (UTF-8) */
	tmDb **ppDb          /* OUT: TM db handle */
	);

TM_API int tm_close(tmDb*);

// todo need
// sqlite3_free(error);
// sqlite3_errmsg(db));

// todo: NoteI change the char* to const char* which is different from SQLLite but want to be clear
// that these a consts. Review if what want or don't want any divergence.
TM_API int tm_exec(
	tmDb *db,                                  /* An open database */
	const char *sql,                           /* SQL to be evaluated */
	int(*callback)(void*, int, const char**, const char**),  /* Callback function */
	void *,                                    /* 1st argument to callback */
	char **errmsg                              /* Error msg written here */
	);

#if later
// do I want this? it is every string in every table.
// Select a table.
// on szSql currently supported is a "select * from [table]"
// ScoutData
TM_API int tm_get_table(
	tmDb *db,          /* An open database */
	const char *zSql,     /* SQL to be evaluated */
	char ***pazResult,    /* Results of the query */
	int *pnRow,           /* Number of result rows written here */
	int *pnColumn,        /* Number of result columns written here */
	char **pzErrmsg       /* Error msg written here */
	);
TM_API void tm_free_table(char **result);
#endif

// Error Codes
// Leave as SQLLite unless use than change to the TM prefix 
/* beginning-of-error-codes */
#define TM_OK           0   /* Successful result */
#define TM_CANTOPEN    14   /* Unable to open the database file */

#if errors
#define SQLITE_ERROR        1   /* SQL error or missing database */
#define SQLITE_INTERNAL     2   /* Internal logic error in SQLite */
#define SQLITE_PERM         3   /* Access permission denied */
#define SQLITE_ABORT        4   /* Callback routine requested an abort */
#define SQLITE_BUSY         5   /* The database file is locked */
#define SQLITE_LOCKED       6   /* A table in the database is locked */
#define SQLITE_NOMEM        7   /* A malloc() failed */
#define SQLITE_READONLY     8   /* Attempt to write a readonly database */
#define SQLITE_INTERRUPT    9   /* Operation terminated by sqlite3_interrupt()*/
#define SQLITE_IOERR       10   /* Some kind of disk I/O error occurred */
#define SQLITE_CORRUPT     11   /* The database disk image is malformed */
#define SQLITE_NOTFOUND    12   /* Unknown opcode in sqlite3_file_control() */
#define SQLITE_FULL        13   /* Insertion failed because database is full */
#define SQLITE_PROTOCOL    15   /* Database lock protocol error */
#define SQLITE_EMPTY       16   /* Database is empty */
#define SQLITE_SCHEMA      17   /* The database schema changed */
#define SQLITE_TOOBIG      18   /* String or BLOB exceeds size limit */
#define SQLITE_CONSTRAINT  19   /* Abort due to constraint violation */
#define SQLITE_MISMATCH    20   /* Data type mismatch */
#define SQLITE_MISUSE      21   /* Library used incorrectly */
#define SQLITE_NOLFS       22   /* Uses OS features not supported on host */
#define SQLITE_AUTH        23   /* Authorization denied */
#define SQLITE_FORMAT      24   /* Auxiliary database format error */
#define SQLITE_RANGE       25   /* 2nd parameter to sqlite3_bind out of range */
#define SQLITE_NOTADB      26   /* File opened that is not a database file */
#define SQLITE_NOTICE      27   /* Notifications from sqlite3_log() */
#define SQLITE_WARNING     28   /* Warnings from sqlite3_log() */
#define SQLITE_ROW         100  /* sqlite3_step() has another row ready */
#define SQLITE_DONE        101  /* sqlite3_step() has finished executing */
/* end-of-error-codes */
#endif

#ifdef __cplusplus
}  /* end of the 'extern "C"' block */
#endif

#endif  /* ifndef _TM_H_ */
