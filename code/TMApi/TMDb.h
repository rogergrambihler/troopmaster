#pragma once
class CTMDb
{
public:
	CTMDb();
	~CTMDb();

	// Connects to the TM Database
	int Connect(const char * szDirectory);

	int Exec(const char *sql,                           /* SQL to be evaluated */
		int(*callback)(void*, int, const char**, const char**),  /* Callback function */
		void *,                                    /* 1st argument to callback */
		char **errmsg                              /* Error msg written here */
		);

#if later
	int GetTable(
		const char *zSql,     /* SQL to be evaluated */
		char ***pazResult,    /* Results of the query */
		int *pnRow,           /* Number of result rows written here */
		int *pnColumn,        /* Number of result columns written here */
		char **pzErrmsg       /* Error msg written here */
		);
#endif // later

private:
	CString m_tmDirectoryPath;
	bool m_fConnected = false; // indicates Connect was called and succesful.

};

