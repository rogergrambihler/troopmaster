#include "stdafx.h"
#include "TMDb.h"

// todo NULLABLE vs. empty string, or zero value on fields
// todo: Merit Badge counselors? I though I had started decoding but don't see in the TroopMaster OneDrive
// todo: may as well make sure the SOAR export decrypt algorith is in this project so keep all together.
// TODO: review if shoul have the RowPersiste data as Offsets from the start so don't rely on record before and/or if make metadata more consolidated

#include <string.h> // todo: bug bug, should be wrapped in CString.

// DataTypes for persisting data.
// todo: consider a Date as a type and convert to a standard string data
// no base date in c++ so more of a string conversion so caller could just as easily convert if really cared.
enum TMDbDataType {
	TEXT, // char * (not sure how TM handles this as Ascii, UTF-8, or other encoding.
	SMALLINT, // 2 bytes
	INTEGER, // 4 bytes
};



// Helper class for converting between raw persisted bytes to char* 
// todo: eventually maybe return other things change char* and/or from char* to a persisted format.
class TMDbDataTypeConverter
{
public:
	// convert the pBuf of dataType to a char*
	// for a char* don't really have to do anything and could save memory
	// by just returning the buf* but go ahead and alloc for now for simplicity
	// for caller to always know have a ptr to memory the caller owns and must free.
	static char* Convert(TMDbDataType dataType, const unsigned int cbBuf, const char *pBuf)
	{
		char *pOutput = NULL;
		switch (dataType)
		{
		case TMDbDataType::INTEGER: // 4 bytes
		{
			// todo: bug bug, review int is proper type to use for 4 byte signed.
			// todo: should add some asserts or error handling for now just crash if 
			AssertSz(4 == cbBuf, "integer should be 4 bytes");
			unsigned char *pbufUnsigned = (unsigned char *) pBuf; // want unsigned so get proper byte values.
			int value = (pbufUnsigned[0]) | (pbufUnsigned[1] << 8) | (pbufUnsigned[2] << 16) | (pbufUnsigned[3] << 24);

			// okay, now convert back to a string representation. 
			pOutput = new char[500]; // todo: obviously this is too large but need to figure out how to set.
			_itoa_s(value, pOutput, 500, 10);
		}
		break;
		case TMDbDataType::SMALLINT: // 2  bytes
		{
			// todo: get the proper type for 2 byted signed value.
			unsigned char *pbufUnsigned = (unsigned char *)pBuf; // want unsigned so get proper byte values.
			int value = (pbufUnsigned[0]) | (pbufUnsigned[1] << 8);
			pOutput = new char[20];
			_itoa_s(value, pOutput, 20, 10);
		}
		break;
		case TMDbDataType::TEXT:
		default:
			// text just allocate the same size as the cbBuf and memcpy
			// todo: should be wraped in stdio
			pOutput = new char[cbBuf];
			memcpy(pOutput, pBuf, cbBuf); // todo: bug bug, memcpy should be wrapped.
			break;
		}

		return pOutput;
	}
};

// helper class to try to figure out a tables metadata and other quick conversions
// of a file.
class DatabaseMetaDataDecoder
{

	struct columnInfo
	{
		char *pdata; // pointer to start of column
		unsigned long len;
		unsigned long recOffset; // 
	};

public:
	// decodes metadata for a file with record based
	void DecodeRecordFile(char *pFilePath, bool encrypted, unsigned long recordSize, unsigned int readRecNum)
	{
		CFile *pFileToDecode = GetFile(pFilePath, encrypted, recordSize);
		unsigned long recAbsoluteFilePosition;

		// read in first record and use that to decode. could be more advanced and wallk all the records
		// need to make that fill in the first record in the file which may != the first one shown in the UI 
		// or even be a first record depending on how the file is authored.
		char *pbuf = new char[recordSize];

		// read number of records to skip if not start at start.
		while (readRecNum--)
		{
			pFileToDecode->Read(pbuf, recordSize);
		}

		recAbsoluteFilePosition = pFileToDecode->GetPosition();
		pFileToDecode->Read(pbuf, recordSize);

		columnInfo *pcolumnInfo = new columnInfo[recordSize]; // for simplicity just allocate if every byte was a column
		columnInfo *pcurColumn = pcolumnInfo;
		unsigned long totalColumns = 0;
		// make some state var
		char *pcurPos = pbuf;
		unsigned long curColumnLen = 0;
		unsigned long bytesLeft = recordSize;

		pcurColumn->pdata = pcurPos;
		pcurColumn->recOffset = 0;
		while (bytesLeft > 0) // be easier to just calc where record ends byte and run pcurPos until over.
		{
			// check if found a non null and consider end of the last record
			if (*pcurPos != NULL)
			{
				++totalColumns;
				// write data about the cur column
				pcurColumn->len = curColumnLen;

				// reset to the next one
				++pcurColumn;
				pcurColumn->pdata = pcurPos;
				pcurColumn->recOffset = (unsigned long) (pcurPos - pbuf);
				curColumnLen = 0; 

				// move ahead to a NULL. 
				// todo: this assums as least one NULL between data and data doesn't start with null
				// we'll see how accurate that is
				while (*pcurPos != NULL)
				{
					// todo bug bug whould check bytesLeft, or could run past if don't find NULL before end of file.
					++curColumnLen; // todo: can calc this from pStart and where cursor is when done.
					++pcurPos;
					--bytesLeft;
				}
			}

			--bytesLeft;
			++curColumnLen;
			++pcurPos;
		}
		// close down the last column
		++totalColumns;
		pcurColumn->len = curColumnLen;

		CString *ptotalMeta = new CString();

		// okay, do some analysis.
		for (unsigned long columnIndex = 0; columnIndex < totalColumns; ++columnIndex)
		{
			columnInfo *pcurColumnInfo = pcolumnInfo + columnIndex;
			CString *pMeta = new CString();
 
			char indexChar[30];		
			_itoa_s((columnIndex + 1), indexChar, 30, 10);
			*pMeta += "{ \"column";
			*pMeta += indexChar;
			*pMeta += "\", ";
			

			// do some checks to see if the type could possibly be and int 
#if types
			enum TMDbDataType {
				TEXT, // char * (not sure how TM handles this as Ascii, UTF-8, or other encoding.
				SMALLINT, // 2 bytes
				INTEGER, // 4 bytes
			};
#endif
			bool foundType = false;
#define TM_COLUMNDDATA_MAX 1024 // Not sure really what the max is but pick something.
			char commentText[TM_COLUMNDDATA_MAX]; // assumes long enough but could crash
			commentText[0] = NULL;
			if (pcurColumnInfo->len == 4)
			{
				// if not an asciie printable assubme INTEGER.
				// todo: figure out where numbers are
				if (pcurColumnInfo->pdata[0] < ' ' || pcurColumnInfo->pdata[0] > 'Z')
				{
					*pMeta += "TMDbDataType::INTEGER";
					char *pc = TMDbDataTypeConverter::Convert(TMDbDataType::INTEGER, 4, pcurColumnInfo->pdata);
					strcpy_s(commentText, TM_COLUMNDDATA_MAX, pc);
					delete pc;
					foundType = true;
				}

			}
			else if(pcurColumnInfo->len == 2)
			{
				// if not an asciie printable assubme INTEGER.
				// todo: figure out where numbers are
				if (pcurColumnInfo->pdata[0] < ' ' || pcurColumnInfo->pdata[0] > 'Z')
				{
					*pMeta += "TMDbDataType::SMALLINT";
					char *pc = TMDbDataTypeConverter::Convert(TMDbDataType::SMALLINT, 2, pcurColumnInfo->pdata);
					strcpy_s(commentText, TM_COLUMNDDATA_MAX, pc);
					delete pc;
					foundType = true;
				}

			}
			
			// if haven't found a type yet then add text
			if (!foundType)
			{
				*pMeta += "TMDbDataType::TEXT";
				strcpy_s(commentText, TM_COLUMNDDATA_MAX, pcurColumnInfo->pdata);
			}

			*pMeta += ", ";
			_itoa_s(pcurColumnInfo->len, indexChar, 30, 10);
			*pMeta += indexChar;

			// make a commend with the value for mapping column name.
			*pMeta += ", 1}, // ";

			// relative record offset.
			_itoa_s(pcurColumnInfo->recOffset, indexChar, 30, 10);
			*pMeta += indexChar;
			*pMeta += ", ";

			// absolute file position
			_itoa_s(recAbsoluteFilePosition + pcurColumnInfo->recOffset, indexChar, 30, 16);
			*pMeta += indexChar;
			*pMeta += ", ";

			*pMeta += commentText;
			*pMeta += "\r\n";

		//	{ "column", TMDbDataType::TEXT, 16, 1 }, // pData

			*ptotalMeta += *pMeta;
			delete pMeta;
		}

		delete pFileToDecode; // delete file first so closed and can compare with TM data
		delete ptotalMeta;
		delete pcolumnInfo;
	}

private:
	CFile *GetFile(char *pFilePath, bool encrypted, unsigned long recordSize)
	{
		// create a file.
		CFile *pFileToDecode;
		CFileDisk *pfile = new CFileDisk();
		int result = pfile->Open(pFilePath, "rb");
		if (encrypted)
		{
			pFileToDecode = new CFileTMRecEncrypt(pfile, recordSize);
		}
		else
		{
			pFileToDecode = pfile;
		}

		return pFileToDecode;
	}
};

CTMDb::CTMDb()
{
}


CTMDb::~CTMDb()
{
}

// Connects to the TM Database
int CTMDb::Connect(const char *szDirectory)
{
	m_tmDirectoryPath = szDirectory;

	// Check if ends with path separator and if so remove it.
	m_tmDirectoryPath.TrimEnd('\\');

	// verify the directory exists which is enough at least for now.
	if (!CStdio::DirectoryExists(m_tmDirectoryPath))
	{
		// reset the directory path
		m_tmDirectoryPath = NULL;

		// directory does not exist. 
		// better if some doesn't exister or something api but sqllite I tried just creates one.
		// todo: call sqllite method that would fail if doesn't exist and see error code
		// todo: should we set an error string or log somethng here
		return TM_CANTOPEN;
	}

	m_fConnected = true;
	return TM_OK;
}



// todo: need some uber table collection so someone either just knows the mappings or can determi

// todo: review what left to right right to left naming convention makes sense.
// currently general to more specific
// todo should et all metadata storege off of the Table Readers so doesn't matter 
// if metadata is changed to from from an imput file vs. hard coded in c++ file.

// todo: review if want any type/schema information on this
typedef struct tag_TableDataRecordOrientedColumn {
	const char *name; // column Name
	const TMDbDataType dataType;
	const unsigned int size; // size of a record in the column
	const unsigned int repeatCount; // amount of times records is repeated (Scout1, Scout2, see Attend table for reason).
} TableDataRecordOrientedColumn ;

// Record oriented are stored with the records written sequentially in the file system (ScoutData, Adult Data)
typedef struct tag_TableDataRecordOriented
{
	const char *tableName;
	const unsigned long recordSize; // size of each record
	const bool isEncrypted;
	const unsigned int numColumns;
	const TableDataRecordOrientedColumn *pmetaData;

} TableDataRecordOriented;





// integer value can come back as one of the follow
// if positive the value is a number divided by 10 give the value. for example 32 = 3.2
// if the value is negative the absolute value is the character code for the character (-88) == X == ascii 88
const TableDataRecordOrientedColumn TableDataRecordOrientedColumnAttend[] =
{
	{ "Scout", TMDbDataType::INTEGER, 4, 250 }, // value is index into the Scout and Adult data respectively??
	{ "Adult", TMDbDataType::INTEGER, 4, 500 },
};

// todo: consider a total columns entry so don't have to calculate as well as in debug
// can verify the entries match expected.
const TableDataRecordOriented TableDataRecordOrientedAttend =
{
	"Attend",
	0xbb8, // record size.
	false, //  isEncrypted
	sizeof(TableDataRecordOrientedColumnAttend) / sizeof(*TableDataRecordOrientedColumnAttend), // number of  columns
	TableDataRecordOrientedColumnAttend, // column metadata
};


// Raw output for advancement filled in needs cleanup and proper column names.
const TableDataRecordOrientedColumn TableDataRecordOrientedColumnAdvancement[] =
{
	{ "column2", TMDbDataType::INTEGER, 4, 1 }, // 0, 0, 2 // Seems to be a 2 if a record 
	{ "column3", TMDbDataType::TEXT, 8, 1 }, // 4, 4, 
	{ "column4", TMDbDataType::TEXT, 16, 1 }, // 12, c, 

	// Partials
	{ "partialMbAdvancementId", TMDbDataType::INTEGER, 4, 15 }, // 28, 1c, 17
	// Each Merit badge can hav 50 requirements and a byte is set if the 
	// requirement is complete. There are 50 consecutive bytes for each record
	// just leaving as a text run for now. Note!!! doesn't have to end in NULL which could
	// cause some garbage records but also don't want a new Type just for this? really a BYTE run.
	{ "partialMbReqsCompletedBytes", TMDbDataType::TEXT, 50, 15 }, // 88, 58, 
	{ "partialMbReqs", TMDbDataType::TEXT, 300, 15 }, // 838, 346, 1a,1b,1c,1d,1e,2,3,4,5
	{ "partialMbReqYr", TMDbDataType::TEXT, 5, 15 }, // 5338, 14da, 2006
	{ "partialMbRemark", TMDbDataType::TEXT, 50, 15 }, // 5413, 1525, AMERICAN CULTURES PARTIAL REMARK
	{ "partialMbStartDate", TMDbDataType::TEXT, 9, 15 }, // 6163, 1813, 20020202 [Not sure if StartDate, LastProgress order)
	{ "partialMbDateOfLastProgress", TMDbDataType::TEXT, 9, 15 }, // 6298, 189a, 20020202

	// Unknown.
	{ "column26", TMDbDataType::TEXT, 903, 1 }, // 6307, 18a3, 20040404 (originally 1029). Now is empty

	// todo: need to find bit fields for R/P check boxes.

	// Scout Advancement
	{ "ScoutAgeRequirement", TMDbDataType::TEXT, 9, 1 }, // 7336, 1ca8, 20010101
	{ "ScoutFindATroop", TMDbDataType::TEXT, 9, 1 }, // 7345, 1cb1, 20020202
	{ "ScoutApplicationForms", TMDbDataType::TEXT, 9, 1 }, // 7354, 1cba, 20030303
	{ "ScoutPledgeOfAllegiance", TMDbDataType::TEXT, 9, 1 }, // 7363, 1cc3, 20040404
	{ "ScoutScoutSign", TMDbDataType::TEXT, 9, 1 }, // 7372, 1ccc, 20050505
	{ "ScoutSquareKnot", TMDbDataType::TEXT, 9, 1 }, // 7381, 1cd5, 20060606
	{ "ScoutOathLaw", TMDbDataType::TEXT, 9, 1 }, // 7390, 1cde, 20070707
	{ "ScoutDescribeBadge", TMDbDataType::TEXT, 9, 1 }, // 7399, 1ce7, 20080808
	{ "ScoutPamphletExcercises", TMDbDataType::TEXT, 9, 1 }, // 7408, 1cf0, 20090909
	{ "ScoutScoumasterConference", TMDbDataType::TEXT, 9, 1 }, // 7417, 1cf9, 20101010

	// Tenderfoot Advancement
	{ "TenderfootPrepareToCamp", TMDbDataType::TEXT, 9, 1 }, // 7426, 1d02, 20010101
	{ "TenderfootCampAndPitchTent", TMDbDataType::TEXT, 9, 1 }, // 7435, 1d0b, 20020202
	{ "TenderfootPrepareCookMeal", TMDbDataType::TEXT, 9, 1 }, // 7444, 1d14, 20030303
	{ "TenderfootWhipfuseRope", TMDbDataType::TEXT, 9, 1 }, // 7453, 1d1d, 20040404
	{ "TenderfootHitchKnots", TMDbDataType::TEXT, 9, 1 }, // 7462, 1d26, 20050505
	{ "TenderfootSquareKnot", TMDbDataType::TEXT, 9, 1 }, // 7471, 1d2f, 20060606
	{ "TenderfootHikingRules", TMDbDataType::TEXT, 9, 1 }, // 7480, 1d38, 20070707
	{ "TenderfootFlagCare", TMDbDataType::TEXT, 9, 1 }, // 7489, 1d41, 20080808
	{ "TenderfootScoutingPrinciples", TMDbDataType::TEXT, 9, 1 }, // 7498, 1d4a, 20090909
	{ "TenderfootPatrolKnowledge", TMDbDataType::TEXT, 9, 1 }, // 7507, 1d53, 20101010
	{ "TenderfootBuddySystem", TMDbDataType::TEXT, 9, 1 }, // 7516, 1d5c, 20111111
	{ "TenderfootPhysicalFitness", TMDbDataType::TEXT, 9, 1 }, // 7525, 1d65, 20020101
	{ "TenderfootShowImprovement", TMDbDataType::TEXT, 9, 1 }, // 7534, 1d6e, 20030101
	{ "TenderfootIdentifyPoisonousPlants", TMDbDataType::TEXT, 9, 1 }, // 7543, 1d77, 20040101
	{ "TenderfootProcedureForChoking", TMDbDataType::TEXT, 9, 1 }, // 7552, 1d80, 20150115
	{ "TenderfootFirstAid", TMDbDataType::TEXT, 9, 1 }, // 7561, 1d89, 20150116
	{ "TenderfootScoutSpirit", TMDbDataType::TEXT, 9, 1 }, // 7570, 1d92, 20070101
	{ "TenderfootScoutmasterConference", TMDbDataType::TEXT, 9, 1 }, // 7579, 1d9b, 20080101
	{ "TenderfootBoardOfReview", TMDbDataType::TEXT, 9, 1 }, // 7588, 1da4, 20101011

	// Second Class
	{ "SecondClassMapAndCompassUse", TMDbDataType::TEXT, 9, 1 }, // 7597, 1dad, 20111215
	{ "SecondClassMapandCompassHike", TMDbDataType::TEXT, 9, 1 }, // 7606, 1db6, 20120819
	{ "SecondClassDiscussLNT", TMDbDataType::TEXT, 9, 1 }, // 7615, 1dbf, N/A
	{ "SecondClassActivity", TMDbDataType::TEXT, 9, 1 }, // 7624, 1dc8, 20120910
	{ "SecondClassSelectCampSite", TMDbDataType::TEXT, 9, 1 }, // 7633, 1dd1, 20121128
	{ "SecondClassKnifeSawAxe", TMDbDataType::TEXT, 9, 1 }, // 7642, 1dda, 20130115
	{ "SecondClassPrepareCookFire", TMDbDataType::TEXT, 9, 1 }, // 7651, 1de3, 20120630
	{ "SecondClassCookingFire", TMDbDataType::TEXT, 9, 1 }, // 7660, 1dec, 20130128
	{ "SecondClassPropaneStove", TMDbDataType::TEXT, 9, 1 }, // 7669, 1df5, 20121218
	{ "SecondClassPlanAndCookMeal", TMDbDataType::TEXT, 9, 1 }, // 7678, 1dfe, 20121210
	{ "SecondClassFlagCeremony", TMDbDataType::TEXT, 9, 1 }, // 7687, 1e07, 20121010
	{ "SecondClassServiceProject", TMDbDataType::TEXT, 9, 1 }, // 7696, 1e10, 20120910
	{ "SecondClassIdentifyWildAnimals", TMDbDataType::TEXT, 9, 1 }, // 7705, 1e19, 20121230
	{ "SecondClassHandleHurryCase", TMDbDataType::TEXT, 9, 1 }, // 7714, 1e22, 20130112
	{ "SecondClassFirstAidKit", TMDbDataType::TEXT, 9, 1 }, // 7723, 1e2b, 20120812
	{ "SecondClassFirstAid", TMDbDataType::TEXT, 9, 1 }, // 7732, 1e34, 20121212
	{ "SecondClassSwimmingPrecautions", TMDbDataType::TEXT, 9, 1 }, // 7741, 1e3d, 20120630
	{ "SecondClassSwimAbility", TMDbDataType::TEXT, 9, 1 }, // 7750, 1e46, 20130110
	{ "SecondClassWaterRescue", TMDbDataType::TEXT, 9, 1 }, // 7759, 1e4f, 20130125
	{ "SecondClassDrugAlcoholProgram", TMDbDataType::TEXT, 9, 1 }, // 7768, 1e58, 20120715
	{ "SecondClassSafetyProtection", TMDbDataType::TEXT, 9, 1 }, // 7777, 1e61, N/A
	{ "SecondClassEarnAndSaveMoney", TMDbDataType::TEXT, 9, 1 }, // 7786, 1e6a, N/A
	{ "SecondClassScoutSpirit", TMDbDataType::TEXT, 9, 1 }, // 7795, 1e73, 20120910
	{ "SecondClassScoutmasterConference", TMDbDataType::TEXT, 9, 1 }, // 7804, 1e7c, 20130201
	{ "SecondClassBoardOfReview", TMDbDataType::TEXT, 9, 1 }, // 7813, 1e85, 20130210

	// First Class
	{ "FirstClassWayWithCompass", TMDbDataType::TEXT, 9, 1 }, // 7822, 1e8e, 20130310
	{ "FirstClassOrienteeringCourse", TMDbDataType::TEXT, 9, 1 }, // 7831, 1e97, 20130419
	{ "FirstClassActivity", TMDbDataType::TEXT, 9, 1 }, // 7840, 1ea0, 20130520
	{ "FirstClassPatrolMenu", TMDbDataType::TEXT, 9, 1 }, // 7849, 1ea9, 20130710
	{ "FirstClassFoodList", TMDbDataType::TEXT, 9, 1 }, // 7858, 1eb2, 20130425
	{ "FirstClassCookingUtensils", TMDbDataType::TEXT, 9, 1 }, // 7867, 1ebb, 20130507
	{ "FirstClassSafeFoodHandling", TMDbDataType::TEXT, 9, 1 }, // 7876, 1ec4, 20130610
	{ "FirstClassServeAsPatrolCook", TMDbDataType::TEXT, 9, 1 }, // 7885, 1ecd, 20130627
	{ "FirstClassCivicLeader", TMDbDataType::TEXT, 9, 1 }, // 7894, 1ed6, 20130718
	{ "FirstClassNativePlants", TMDbDataType::TEXT, 9, 1 }, // 7903, 1edf, 20121208
	{ "FirstClassLashings", TMDbDataType::TEXT, 9, 1 }, // 7912, 1ee8, 20130828
	{ "FirstClassCampGadget", TMDbDataType::TEXT, 9, 1 }, // 7921, 1ef1, 20121208
	{ "FirstClassRescueKnot", TMDbDataType::TEXT, 9, 1 }, // 7930, 1efa, 20130918
	{ "FirstClassBandages", TMDbDataType::TEXT, 9, 1 }, // 7939, 1f03, 20131019
	{ "FirstClassMovingInjured", TMDbDataType::TEXT, 9, 1 }, // 7948, 1f0c, 20130317
	{ "FirstClassCPR", TMDbDataType::TEXT, 9, 1 }, // 7957, 1f15, 20130407
	{ "FirstClassSafetyAfloat", TMDbDataType::TEXT, 9, 1 }, // 7966, 1f1e, 20130801
	{ "FirstClassSwimTest", TMDbDataType::TEXT, 9, 1 }, // 7975, 1f27, 20130603
	{ "FirstClassLineRescue", TMDbDataType::TEXT, 9, 1 }, // 7984, 1f30, 20130510
	{ "FirstClassInviteFriend", TMDbDataType::TEXT, 9, 1 }, // 7993, 1f39, N/A
	{ "FirstClassInternet", TMDbDataType::TEXT, 9, 1 }, // 8002, 1f42, N/A
	{ "FirstClassScoutSpirit", TMDbDataType::TEXT, 9, 1 }, // 8011, 1f4b, 20131029
	{ "FirstClassScoutmasterConference", TMDbDataType::TEXT, 9, 1 }, // 8020, 1f54, 20131101
	{ "FirstClassBoardOfReview", TMDbDataType::TEXT, 9, 1 }, // 8029, 1f5d, 20131110

	// Star
	{ "StarParticipation", TMDbDataType::TEXT, 9, 1 }, // 8038, 1f66, 20140310
	{ "StarScoutSpirit", TMDbDataType::TEXT, 9, 1 }, // 8047, 1f6f, 20140415
	{ "StarService", TMDbDataType::TEXT, 9, 1 }, // 8056, 1f78, 20140518
	{ "StarPositionOfResponsibility", TMDbDataType::TEXT, 9, 1 }, // 8065, 1f81, 20131201
	{ "StarScoutmasterConference", TMDbDataType::TEXT, 9, 1 }, // 8074, 1f8a, 20140715
	{ "StarBoardOfReview", TMDbDataType::TEXT, 9, 1 }, // 8083, 1f93, 20140715

	// Life
	{ "LifeParticipation", TMDbDataType::TEXT, 9, 1 }, // 8092, 1f9c, 20150115
	{ "LifeScoutSpirit", TMDbDataType::TEXT, 9, 1 }, // 8101, 1fa5, 20140404
	{ "LifeService", TMDbDataType::TEXT, 9, 1 }, // 8110, 1fae, 20140910
	{ "LifePositionOfResponsibility", TMDbDataType::TEXT, 9, 1 }, // 8119, 1fb7, 20150115
	{ "LifeTeachEdge", TMDbDataType::TEXT, 9, 1 }, // 8128, 1fc0, 20150116
	{ "LifeScoutmasterConference", TMDbDataType::TEXT, 9, 1 }, // 8137, 1fc9, 20150117
	{ "LifeBoardOfReview", TMDbDataType::TEXT, 9, 1 }, // 8146, 1fd2, 20150118

	// Eagle
	{ "EagleParticipation", TMDbDataType::TEXT, 9, 1 }, // 8155, 1fdb, 20010101
	{ "EagleScoutSpirit", TMDbDataType::TEXT, 9, 1 }, // 8164, 1fe4, 20150222
	{ "EaglePositionOfResponsibility", TMDbDataType::TEXT, 9, 1 }, // 8173, 1fed, 20030303
	{ "EagleProject", TMDbDataType::TEXT, 9, 1 }, // 8182, 1ff6, 20040404
	{ "EagleScoutmasterConference", TMDbDataType::TEXT, 9, 1 }, // 8191, 1fff, 20050505
	{ "EagleBoardOfReview", TMDbDataType::TEXT, 9, 1 }, // 8200, 2008, 20150118

	{ "column123", TMDbDataType::TEXT, 1219, 1 }, // 
	{ "column124", TMDbDataType::TEXT, 8, 1 }, // 9428, 24d4, 
	{ "column125", TMDbDataType::INTEGER, 4, 1 }, // 9436, 24dc, 1
	{ "column126", TMDbDataType::INTEGER, 4, 1 }, // 9440, 24e0, 1
	{ "column127", TMDbDataType::INTEGER, 4, 1 }, // 9444, 24e4, 1
	{ "column128", TMDbDataType::INTEGER, 4, 1 }, // 9448, 24e8, 1
	{ "column129", TMDbDataType::TEXT, 116, 1 }, // 9452, 24ec, 
	{ "column130", TMDbDataType::TEXT, 8, 1 }, // 9568, 2560, 
	{ "column131", TMDbDataType::INTEGER, 4, 1 }, // 9576, 2568, 1
	{ "column132", TMDbDataType::INTEGER, 4, 1 }, // 9580, 256c, 1
	{ "column133", TMDbDataType::INTEGER, 4, 1 }, // 9584, 2570, 1
	{ "column134", TMDbDataType::INTEGER, 4, 1 }, // 9588, 2574, 1
	{ "column135", TMDbDataType::TEXT, 9312, 1 }, // 9592, 2578, 

	// Troopmaster say 136 merit badges which leaves som room after the MB
	// bulk of these must be Merit badge advancement ids
	// http://meritbadge.org/wiki/index.php/BSA%27s_Merit_Badge_Numbering_System
	{ "meritBadgeEarnedId", TMDbDataType::INTEGER, 4, 136 }, // 18904, 49d8, ;
	{ "column158", TMDbDataType::TEXT, 80, 1 }, // 18992, 4a30, 

	{ "meritBadgeEarnedDate", TMDbDataType::TEXT, 9, 136 }, // 19528, 4c48, 20140403 // MB Earned date (TM says limit of 136)
	{ "column181", TMDbDataType::TEXT, 248, 1 }, // 19726, 4d0e, 20020202 // leaves 248 if 136 limit is correct and start of mb counselor info is correct

	// Here is what my guess is. there is a 68 bit entry for each Merit badge counselor data 
	// for the badges earned.
	// First two integers seem to always be set?  maybe set if  Scout has the badge??
	{ "mb1Uknown1", TMDbDataType::INTEGER, 4, 1 }, // 21000, 5208, 1  (Earned??) 
	{ "mb1Unknown2", TMDbDataType::INTEGER, 4, 1 }, // 21000, 5208, 1  (CounselorData??)
	{ "mb1Counselor", TMDbDataType::TEXT, 40, 1 }, 
	{ "mb1CounselorBSAId", TMDbDataType::TEXT, 20, 1 }, 

	{ "column184", TMDbDataType::INTEGER, 4, 1 }, // 21068, 524c, 1
	{ "column185", TMDbDataType::TEXT, 64, 1 }, // 21072, 5250, 

	{ "column186", TMDbDataType::INTEGER, 4, 1 }, // 21136, 5290, 1 // this is the third.
	{ "column187", TMDbDataType::INTEGER, 4, 1 }, // 21140, 5294, 1
	{ "column188", TMDbDataType::TEXT, 40, 1 }, // 21144, 5298, Edwards,   Samuel (badge counselor)
	{ "column189", TMDbDataType::TEXT, 20, 1 }, // 21184, 52c0, bsaidof counselor

	{ "column190", TMDbDataType::INTEGER, 4, 1 }, // 21204, 52d4, 1
	{ "column191", TMDbDataType::TEXT, 64, 1 }, // 21208, 52d8, 

	{ "column192", TMDbDataType::INTEGER, 4, 1 }, // 21272, 5318, 1
	{ "column193", TMDbDataType::TEXT, 64, 1 }, // 21276, 531c, 

	{ "column194", TMDbDataType::INTEGER, 4, 1 }, // 21340, 535c, 1
	{ "column195", TMDbDataType::TEXT, 64, 1 }, // 21344, 5360, 
	{ "column196", TMDbDataType::INTEGER, 4, 1 }, // 21408, 53a0, 1
	{ "column197", TMDbDataType::TEXT, 64, 1 }, // 21412, 53a4, 
	{ "column198", TMDbDataType::INTEGER, 4, 1 }, // 21476, 53e4, 1
	{ "column199", TMDbDataType::TEXT, 132, 1 }, // 21480, 53e8, 
	{ "column200", TMDbDataType::INTEGER, 4, 1 }, // 21612, 546c, 1
	{ "column201", TMDbDataType::TEXT, 64, 1 }, // 21616, 5470, 
	{ "column202", TMDbDataType::INTEGER, 4, 1 }, // 21680, 54b0, 1
	{ "column203", TMDbDataType::TEXT, 472, 1 }, // 21684, 54b4, 
	{ "column204", TMDbDataType::INTEGER, 4, 1 }, // 22156, 568c, 1
	{ "column205", TMDbDataType::TEXT, 64, 1 }, // 22160, 5690, 
	{ "column206", TMDbDataType::INTEGER, 4, 1 }, // 22224, 56d0, 1
	{ "column207", TMDbDataType::INTEGER, 4, 1 }, // 22228, 56d4, 1
	{ "column208", TMDbDataType::TEXT, 40, 1 }, // 22232, 56d8, Neuben,   Neil
	{ "column209", TMDbDataType::TEXT, 156, 1 }, // 22272, 5700, 111
	{ "column210", TMDbDataType::INTEGER, 4, 1 }, // 22428, 579c, 1
	{ "column211", TMDbDataType::TEXT, 64, 1 }, // 22432, 57a0, 
	{ "column212", TMDbDataType::INTEGER, 4, 1 }, // 22496, 57e0, 1
	{ "column213", TMDbDataType::TEXT, 200, 1 }, // 22500, 57e4, 
	{ "column214", TMDbDataType::INTEGER, 4, 1 }, // 22700, 58ac, 1
	{ "column215", TMDbDataType::INTEGER, 4, 1 }, // 22704, 58b0, 1
	{ "column216", TMDbDataType::TEXT, 40, 1 }, // 22708, 58b4, Scouter,   John
	{ "column217", TMDbDataType::TEXT, 20, 1 }, // 22748, 58dc, 2222
	{ "column218", TMDbDataType::INTEGER, 4, 1 }, // 22768, 58f0, 1
	{ "column219", TMDbDataType::TEXT, 744, 1 }, // 22772, 58f4, 
	{ "column220", TMDbDataType::INTEGER, 4, 1 }, // 23516, 5bdc, 1
	{ "column221", TMDbDataType::TEXT, 132, 1 }, // 23520, 5be0, 
	{ "column222", TMDbDataType::INTEGER, 4, 1 }, // 23652, 5c64, 1
	{ "column223", TMDbDataType::TEXT, 1288, 1 }, // 23656, 5c68, 
	{ "column224", TMDbDataType::INTEGER, 4, 1 }, // 24944, 6170, 1
	{ "column225", TMDbDataType::TEXT, 132, 1 }, // 24948, 6174, 
	{ "column226", TMDbDataType::INTEGER, 4, 1 }, // 25080, 61f8, 1
	{ "column227", TMDbDataType::TEXT, 336, 1 }, // 25084, 61fc, 
	{ "column228", TMDbDataType::INTEGER, 4, 1 }, // 25420, 634c, 1
	{ "column229", TMDbDataType::TEXT, 4280, 1 }, // 25424, 6350, 
	{ "column230", TMDbDataType::INTEGER, 4, 1 }, // 29704, 7408, 1
	{ "column231", TMDbDataType::TEXT, 4824, 1 }, // 29708, 740c, 
	
    // Training Information (100)
	{ "trainingName", TMDbDataType::TEXT, 30, 100 }, // 34532, 86e4, CustomTraining
	{ "trainingDate", TMDbDataType::TEXT, 9, 100}, // 37532, 929c, 20010101
	{ "trainingUnknown", TMDbDataType::INTEGER, 4, 100 }, // 38432, 9620, 

	// Special Awards (100)
	{ "specialAwardName", TMDbDataType::TEXT, 30, 100 }, // 38832, 97b0, Mile Swim
	{ "specialAwardDate", TMDbDataType::TEXT, 9, 100 }, // 41832, a368, 20120614

	{ "advancementremarks", TMDbDataType::TEXT, 1064, 1 }, // 42732, a6ec, Thes are some advancement remarks (size??)
	{ "column247", TMDbDataType::TEXT, 3, 1 }, // 43796, ab14, 5
	{ "column248", TMDbDataType::TEXT, 17, 1 }, // 43799, ab17, 50209
};

const TableDataRecordOriented TableDataRecordOrientedAdvancement =
{
	"advancement",
	0xab28, // record size.
	false, //  isEncrypted
	sizeof(TableDataRecordOrientedColumnAdvancement) / sizeof(*TableDataRecordOrientedColumnAdvancement), // number of  columns
	TableDataRecordOrientedColumnAdvancement, // column metadata
};

// totally bogus metadata as placeholder for POC
const TableDataRecordOrientedColumn TableDataRecordOrientedColumnPOC[] =
{
	{ "column1", TMDbDataType::INTEGER, 4, 1 }, // 0, a3c, 0
	{ "title", TMDbDataType::TEXT, 5, 1 }, // 4, a40, titl
	{ "firstname", TMDbDataType::TEXT, 32, 1 }, // 9, a45, FirstNameaaaaaa
	{ "initial", TMDbDataType::TEXT, 2, 1 }, // 41, a65, I
	{ "lastname", TMDbDataType::TEXT, 25, 1 }, // 43, a67, LastName
	{ "nickname", TMDbDataType::TEXT, 16, 1 }, // 68, a80, Mary
	{ "spouse", TMDbDataType::TEXT, 16, 1 }, // 84, a90, John
	{ "addressline1", TMDbDataType::TEXT, 30, 1 }, // 100, aa0, Address1Line1sfasfdsadfsadfsa
	{ "addressline2", TMDbDataType::TEXT, 30, 1 }, // 130, abe, Address1Line2adsfasdfasfdsafd
	{ "city", TMDbDataType::TEXT, 20, 1 }, // 160, adc, Cityasdfasdfasdfsad
	{ "state", TMDbDataType::TEXT, 3, 1 }, // 180, af0, WA
	{ "zip", TMDbDataType::TEXT, 11, 1 }, // 183, af3, 98077
	{ "country", TMDbDataType::TEXT, 4, 1 }, // 194, afe, AI
	{ "phone1type", TMDbDataType::SMALLINT, 2, 1 }, // 198, b02, 7
	{ "phone1arecode", TMDbDataType::TEXT, 5, 1 }, // 200, b04, 425
	{ "phone1number", TMDbDataType::TEXT, 14, 1 }, // 205, b09, 869-8304
	{ "phone1extension", TMDbDataType::TEXT, 7, 1 }, // 219, b17, 1
	{ "phone2type", TMDbDataType::SMALLINT, 2, 1 }, // 226, b1e, 1
	{ "phone2areacode", TMDbDataType::TEXT, 5, 1 }, // 228, b20, 425
	{ "phone2number", TMDbDataType::TEXT, 14, 1 }, // 233, b25, 555-1212
	{ "phone2extension", TMDbDataType::TEXT, 7, 1 }, // 247, b33, 2
	{ "phone3type", TMDbDataType::SMALLINT, 2, 1 }, // 254, b3a, 3
	{ "phone3areacode", TMDbDataType::TEXT, 5, 1 }, // 256, b3c, 555
	{ "phone3number", TMDbDataType::TEXT, 14, 1 }, // 261, b41, 123-4562
	{ "phone3extension", TMDbDataType::TEXT, 7, 1 }, // 275, b4f, 3
	{ "phone4type", TMDbDataType::SMALLINT, 2, 1 }, // 282, b56, 6
	{ "phone4areacode", TMDbDataType::TEXT, 5, 1 }, // 284, b58, 111
	{ "phone4number", TMDbDataType::TEXT, 14, 1 }, // 289, b5d, 444-3322
	{ "phone4extension", TMDbDataType::TEXT, 7, 1 }, // 303, b6b, 4
	{ "email1", TMDbDataType::TEXT, 60, 1 }, // 310, b72, email1@firemail.com
	{ "email2", TMDbDataType::TEXT, 62, 1 }, // 370, bae, email2@something.com
	{ "email1reports", TMDbDataType::INTEGER, 4, 1 }, // 432, bec, 1
	{ "email2reports", TMDbDataType::INTEGER, 4, 1 }, // 436, bf0, 1
	{ "position", TMDbDataType::TEXT, 30, 1 }, // 440, bf4, Position
	{ "occupation", TMDbDataType::TEXT, 30, 1 }, // 470, c12, Interior Designer
	{ "emmployer", TMDbDataType::TEXT, 30, 1 }, // 500, c30, Insight, Inc. // todo: part of occupation
	{ "website", TMDbDataType::TEXT, 70, 1 }, // 530, c4e, websiteurl
	{ "remarks", TMDbDataType::TEXT, 400, 1 }, // 600, c94, Remarks can have 20 skills/interest
	{ "column39", TMDbDataType::INTEGER, 4, 1 }, // 1000, e24, 3
	{ "skillsandinterests", TMDbDataType::TEXT, 30, 20 }, // 1004, e28, Backpacking // todo: foget the anem
	{ "linkadult", TMDbDataType::INTEGER, 4, 1 }, // todo: not sure if these are in proper order
	{ "linkparent", TMDbDataType::INTEGER, 4, 1 },
	{ "linkmbc", TMDbDataType::INTEGER, 4, 1 },
	{ "column45", TMDbDataType::TEXT, 1004, 1 }, // 1604, 1080, ������������

	// adult, parent, mbc
};

// bogus data for POC (Point of Contact)
const TableDataRecordOriented TableDataRecordOrientedPOC =
{
	"pocdata",
	0xA3C, // (2620d), // record size.
	true, //  isEncrypted
	sizeof(TableDataRecordOrientedColumnPOC) / sizeof(*TableDataRecordOrientedColumnPOC), // number of  columns
	TableDataRecordOrientedColumnPOC, // column metadata
};
// 

#if csvFiles
// todo: Sex (m/f=1) Actiive and However links are done is missing.
// number of badges that can have is unknown just made the run badges.
,  Active(Y / N),  Remarks (need proper length), Badges Taught (final size and if unsigned???)
#endif
// todo:  data just copied from Parent Data so need to verify fields
const TableDataRecordOrientedColumn TableDataRecordOrientedColumnMBCData[] =
{
	{ "column1", TMDbDataType::TEXT, 9, 1 }, // 0, 2a44, 
	{ "firstname", TMDbDataType::TEXT, 34, 1 }, // 9, 2a4d, FirstNameaaaaaa
	{ "lastname", TMDbDataType::TEXT, 25, 1 }, // 43, 2a6f, LastName
	{ "nickname", TMDbDataType::TEXT, 16, 1 }, // 68, 2a88, Mary
	{ "homeaddressline1", TMDbDataType::TEXT, 30, 1 }, // 84, 2a98, Address1Line1sfasfdsadfsadfsa
	{ "homeaddressline2", TMDbDataType::TEXT, 30, 1 }, // 114, 2ab6, Address1Line2adsfasdfasfdsafd
	{ "homecity", TMDbDataType::TEXT, 20, 1 }, // 144, 2ad4, Cityasdfasdfasdfsad
	{ "homestate", TMDbDataType::TEXT, 3, 1 }, // 164, 2ae8, WA
	{ "homezip", TMDbDataType::TEXT, 11, 1 }, // 167, 2aeb, 98077
	{ "homecountry", TMDbDataType::TEXT, 4, 1 }, // 178, 2af6, AI
	{ "phonetype1", TMDbDataType::SMALLINT, 2, 1 }, // 182, 2afa, 7
	{ "phoneareacode1", TMDbDataType::TEXT, 5, 1 }, // 184, 2afc, 425
	{ "phonenumber1", TMDbDataType::TEXT, 14, 1 }, // 189, 2b01, 869-8304
	{ "phoneextension1", TMDbDataType::TEXT, 7, 1 }, // 203, 2b0f, 1
	{ "phonetype2", TMDbDataType::SMALLINT, 2, 1 }, // 210, 2b16, 1
	{ "phoneareacode2", TMDbDataType::TEXT, 5, 1 }, // 212, 2b18, 425
	{ "phonenumber2", TMDbDataType::TEXT, 14, 1 }, // 217, 2b1d, 555-1212
	{ "phoneextension2", TMDbDataType::TEXT, 7, 1 }, // 231, 2b2b, 2
	{ "phonetype3", TMDbDataType::SMALLINT, 2, 1 }, // 238, 2b32, 3
	{ "phoneareacode3", TMDbDataType::TEXT, 5, 1 }, // 240, 2b34, 555
	{ "phonenumber3", TMDbDataType::TEXT, 14, 1 }, // 245, 2b39, 123-4562
	{ "phoneextension3", TMDbDataType::TEXT, 7, 1 }, // 259, 2b47, 3
	{ "phonetype4", TMDbDataType::SMALLINT, 2, 1 }, // 266, 2b4e, 6
	{ "phoneareacode4", TMDbDataType::TEXT, 5, 1 }, // 268, 2b50, 111
	{ "phonenumber4", TMDbDataType::TEXT, 14, 1 }, // 273, 2b55, 444-3322
	{ "phoneextension4", TMDbDataType::TEXT, 7, 1 }, // 287, 2b63, 4
	{ "email1", TMDbDataType::TEXT, 60, 1 }, // 294, 2b6a, email1@firemail.com
	{ "email2", TMDbDataType::TEXT, 62, 1 }, // 354, 2ba6, email2@something.com
	{ "emailreports1", TMDbDataType::INTEGER, 4, 1 }, // 416, 2be4, 1
	{ "emailreports2", TMDbDataType::INTEGER, 4, 1 }, // 420, 2be8, 1
	{ "mb", TMDbDataType::INTEGER, 4, 136 }, // 424, 2bec, 120
	// todo: possibly something between mb and LinkToParent. Need to add as many mbs as can and see where the limit is.
	{ "linktoparent", TMDbDataType::INTEGER, 4, 1 },
	{ "linktoadult", TMDbDataType::INTEGER, 4, 1 },
	{ "linktopoc", TMDbDataType::INTEGER, 4, 1 },
	{ "active", TMDbDataType::INTEGER, 4, 1 }, // No=0, Yes=1
	{ "gender", TMDbDataType::INTEGER, 4, 1 }, // M=0, f =1 
	{ "bsaid", TMDbDataType::TEXT, 41, 1 }, // 988, 2e20, BSAID
	{ "mbcorientation", TMDbDataType::TEXT, 9, 1 }, // 1029, 2e49, 19440607
	{ "youthprotectiontraining", TMDbDataType::TEXT, 27, 1 }, // 1038, 2e52, 20180801
	{ "originalcertification", TMDbDataType::TEXT, 9, 1 }, // 1065, 2e6d, 20230505
	{ "recertification", TMDbDataType::TEXT, 9, 1 }, // 1074, 2e76, 20210505
	{ "remarks", TMDbDataType::TEXT, 1081, 1 }, // 1083, 2e7f, Remarkssaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
	// todo:  remarks isn't 1081 in length, should get proper size and then make a reserved column
};

// Record size is Merit Badge Counselors is 0x874 record Size
const TableDataRecordOriented TableDataRecordOrientedMBCData =
{
	"mbcdata",
	0x874, // record size.
	true, //  isEncrypted
	sizeof(TableDataRecordOrientedColumnMBCData) / sizeof(*TableDataRecordOrientedColumnMBCData), // number of  columns
	TableDataRecordOrientedColumnMBCData, // column metadata
};

#if columnExportedFromTM
needToFind
link to: POC, Parent, MBC fields
Sex(M / F)
Unit Leader(Y / N),
work code
highest rank
link vehicles to parent record
Boys' Life (Y/N)
social security
Swimming Level,Swimming Date
special awards presented (should be 100)
 
// OA i THINK IS IN ADUL OA FILE SO NOT HERE.
 O / A Member(Y / N), O / A Active(Y / N), O / A Election / Recommended Date, O / A Call Out Date, O / A Ordeal Date, O / A Brotherhood Date, O / A Vigil Date, O / A Vigil Name, O / A Leadership Pos, O / A Leadership Pos Date, O / A Remarks, MOS Foxman Date, MOS Brave Date, MOS Warrior Date, MOS Honorary Warrior / Honored Woman, MOS Tribal Council(Y / N), MOS She She Be Council(Y / N), MOS Leadership Pos, MOS Leadership Pos Date, MOS Tribal Name, MOS Remarks
#endif

const TableDataRecordOrientedColumn TableDataRecordOrientedColumnAdult[] =
	{
		{ "column1", TMDbDataType::TEXT, 9, 1 }, // 0, 0, 
		{ "firstname", TMDbDataType::TEXT, 16, 1 }, // 9, 9, firstnamenamena
		{ "middlename", TMDbDataType::TEXT, 16, 1 }, // 25, 19, Middlenammi
		{ "middleinitial", TMDbDataType::TEXT, 2, 1 }, // 41, 29, M
		{ "lastname", TMDbDataType::TEXT, 20, 1 }, // 43, 2b, LASTNAMlastname
		{ "suffix", TMDbDataType::TEXT, 5, 1 }, // 63, 3f, suf
		{ "nickname", TMDbDataType::TEXT, 16, 1 }, // 68, 44, NickName
		{ "bsaid", TMDbDataType::TEXT, 20, 1 }, // 84, 54, BSAID
		{ "spouse", TMDbDataType::TEXT, 16, 1 }, // 104, 68, Spouse
		{ "homeaddressline1", TMDbDataType::TEXT, 30, 1 }, // 120, 78, addressline1
		{ "homeaddressline2", TMDbDataType::TEXT, 30, 1 }, // 150, 96, addressline2
		{ "homecity", TMDbDataType::TEXT, 20, 1 }, // 180, b4, city
		{ "homestate", TMDbDataType::TEXT, 3, 1 }, // 200, c8, WA
		{ "homezip", TMDbDataType::TEXT, 11, 1 }, // 203, cb, 99352
		{ "homecountry", TMDbDataType::TEXT, 3, 1 }, // 214, d6, US
		{ "maileaddressline1", TMDbDataType::TEXT, 30, 1 }, // 217, d9, maddressline1
		{ "maileaddressline2", TMDbDataType::TEXT, 30, 1 }, // 247, f7, maddressline2
		{ "maileaddresscity", TMDbDataType::TEXT, 20, 1 }, // 277, 115, citys2
		{ "maileaddressstate", TMDbDataType::TEXT, 3, 1 }, // 297, 129, OR
		{ "maileaddresszip", TMDbDataType::TEXT, 11, 1 }, // 300, 12c, 99352
		{ "maileaddresscountry", TMDbDataType::TEXT, 3, 1 }, // 311, 137, TN
		{ "phonetype1", TMDbDataType::SMALLINT, 2, 1 }, // 182, 2afa, 7
		{ "phoneareacode1", TMDbDataType::TEXT, 5, 1 }, // 184, 2afc, 425
		{ "phonenumber1", TMDbDataType::TEXT, 14, 1 }, // 189, 2b01, 869-8304
		{ "phoneextension1", TMDbDataType::TEXT, 7, 1 }, // 203, 2b0f, 1
		{ "phonetype2", TMDbDataType::SMALLINT, 2, 1 }, // 210, 2b16, 1
		{ "phoneareacode2", TMDbDataType::TEXT, 5, 1 }, // 212, 2b18, 425
		{ "phonenumber2", TMDbDataType::TEXT, 14, 1 }, // 217, 2b1d, 555-1212
		{ "phoneextension2", TMDbDataType::TEXT, 7, 1 }, // 231, 2b2b, 2
		{ "phonetype3", TMDbDataType::SMALLINT, 2, 1 }, // 238, 2b32, 3
		{ "phoneareacode3", TMDbDataType::TEXT, 5, 1 }, // 240, 2b34, 555
		{ "phonenumber3", TMDbDataType::TEXT, 14, 1 }, // 245, 2b39, 123-4562
		{ "phoneextension3", TMDbDataType::TEXT, 7, 1 }, // 259, 2b47, 3
		{ "phonetype4", TMDbDataType::SMALLINT, 2, 1 }, // 266, 2b4e, 6
		{ "phoneareacode4", TMDbDataType::TEXT, 5, 1 }, // 268, 2b50, 111
		{ "phonenumber4", TMDbDataType::TEXT, 14, 1 }, // 273, 2b55, 444-3322
		{ "phoneextension4", TMDbDataType::TEXT, 7, 1 }, // 287, 2b63, 4
		{ "email1", TMDbDataType::TEXT, 60, 1 }, // 426, 1aa, email1@email.com
		{ "email2", TMDbDataType::TEXT, 62, 1 }, // 486, 1e6, email2@email.com
		{ "emailreports1", TMDbDataType::INTEGER, 4, 1 }, // 548, 224, 1
		{ "emailreports2", TMDbDataType::INTEGER, 4, 1 }, // 552, 228, 1
		{ "dateofbirth", TMDbDataType::TEXT, 21, 1 }, // 556, 22c, 19030303 (todo: if like adult social secuirty is also here.
		{ "joinedunit", TMDbDataType::TEXT, 9, 1 }, // 577, 241, 20010101
		{ "becameleader", TMDbDataType::TEXT, 9, 1 }, // 586, 24a, 20020202
		{ "eagledate", TMDbDataType::TEXT, 9, 1 }, // 595, 253, 20000404
		{ "driverslicense", TMDbDataType::TEXT, 20, 1 }, // 604, 25c, 8538920-drv
		{ "driverslicensestate", TMDbDataType::TEXT, 3, 1 }, // 624, 270, VA
		{ "church", TMDbDataType::TEXT, 30, 1 }, // 627, 273, CHURCH
		{ "occupation", TMDbDataType::TEXT, 30, 1 }, // 657, 291, Ocuppation
		{ "employer", TMDbDataType::TEXT, 33, 1 }, // 687, 2af, employer
		{ "vehiclenumseatbelts1", TMDbDataType::INTEGER, 4, 1 }, // 720, 2d0, 3 
		{ "vehicletrailerhitch1", TMDbDataType::INTEGER, 4, 1 }, // 724, 2d4, 1
		{ "vehicleyear1", TMDbDataType::TEXT, 3, 1 }, // 728, 2d8, 67
		{ "vehiclemake1", TMDbDataType::TEXT, 16, 1 }, // 731, 2db, MAKE1
		{ "vehiclemodel1", TMDbDataType::TEXT, 16, 1 }, // 747, 2eb, MODEL1
		{ "vehicleplate1", TMDbDataType::TEXT, 16, 1 }, // 763, 2fb, PLATE1
		{ "vehicleperinsuranceperperson1", TMDbDataType::TEXT, 5, 1 }, // 779, 30b, 1
		{ "vehicleperinsuranceperaccident1", TMDbDataType::TEXT, 5, 1 }, // 784, 310, 2
		{ "vehicleperinsuranceperproperty1", TMDbDataType::TEXT, 7, 1 }, // 789, 315, 3
		{ "vehiclenumseatbelts2", TMDbDataType::INTEGER, 4, 1 }, // 796, 31c, 4 ???
		{ "vehicletrailerhitch2", TMDbDataType::INTEGER, 4, 1 }, // 800, 320, 1 /
		{ "vehicleyear2", TMDbDataType::TEXT, 3, 1 }, // 804, 324, 68
		{ "vehiclemake2", TMDbDataType::TEXT, 16, 1 }, // 807, 327, MAKE2
		{ "vehiclemodel2", TMDbDataType::TEXT, 16, 1 }, // 823, 337, MODEL2
		{ "vehicleplate2", TMDbDataType::TEXT, 16, 1 }, // 839, 347, PLATE2
		{ "vehicleperinsuranceperperson2", TMDbDataType::TEXT, 5, 1 }, // 855, 357, 4
		{ "vehicleperinsuranceperaccident2", TMDbDataType::TEXT, 5, 1 }, // 860, 35c, 5
		{ "vehicleperinsuranceperproperty2", TMDbDataType::TEXT, 7, 1 }, // 865, 361, 6
		{ "column73", TMDbDataType::INTEGER, 4, 1 }, // 872, 368, 1 /  !! linnk to vehicles parent record?
		{ "column74", TMDbDataType::INTEGER, 4, 1 }, // 876, 36c, 1 //
		{ "column75", TMDbDataType::INTEGER, 4, 1 }, // 880, 370, 7   // todo: work code  and highest rankprobably here somewhere
		{ "column76", TMDbDataType::TEXT, 20, 1 }, // 884, 374, k"
		{ "column77", TMDbDataType::TEXT, 16, 1 }, // 904, 388, ������������
		{ "column78", TMDbDataType::TEXT, 28, 1 }, // 920, 398, 
		{ "priorservicefrom", TMDbDataType::TEXT, 9, 8 }, // 948, 3b4, 20010505 
		{ "priorserviceto", TMDbDataType::TEXT, 9, 8 }, // 1020, 3fc, 20020505
		{ "priorserivceunit", TMDbDataType::TEXT, 5, 8 }, // 1092, 444, 581
		{ "priorservicelevel", TMDbDataType::TEXT, 9, 8 }, // 1132, 46c, Post
		{ "priorservicecouncil", TMDbDataType::TEXT, 4, 8 }, // 1204, 4b4, 1
		{ "column104", TMDbDataType::TEXT, 4, 1 }, // 1236, 4d4, &
		{ "column105", TMDbDataType::INTEGER, 4, 1 }, // 1240, 4d8, 22
		{ "column106", TMDbDataType::INTEGER, 4, 1 }, // 1244, 4dc, 3
		{ "column107", TMDbDataType::INTEGER, 4, 1 }, // 1248, 4e0, 13
		{ "column108", TMDbDataType::INTEGER, 4, 1 }, // 1252, 4e4, 24
		{ "column109", TMDbDataType::INTEGER, 4, 1 }, // 1256, 4e8, 24
		{ "column110", TMDbDataType::INTEGER, 4, 1 }, // 1260, 4ec, 24
		{ "column111", TMDbDataType::INTEGER, 4, 1 }, // 1264, 4f0, 24
		{ "currentposition", TMDbDataType::TEXT, 25, 4 }, // 1268, 4f4, Treasurer
		{ "currentpositiondate", TMDbDataType::TEXT, 9, 4 }, // 1368, 558, 20120612
		{ "leadershiphistoryposition", TMDbDataType::TEXT, 20, 15 }, // 1404, 57c, Crew Advisor // start of Leadership history
		{ "leadershiphistoryfrom", TMDbDataType::TEXT, 9, 15 }, // 1704, 6a8, 20010202 //from 
		{ "leadershiphistoryfromto", TMDbDataType::TEXT, 9, 15 }, // 1713, 6b1, 20010101 // from
		{ "trainingcompleted", TMDbDataType::TEXT, 10, 100 }, // 1974, 7b6, ACPR // Training
		{ "trainingcompleteddate", TMDbDataType::TEXT, 9, 100 }, // 2974, b9e, 20120101 // Training date
		{ "column135", TMDbDataType::TEXT, 402, 1 }, // 3010, bc2, 20121101 Presented??? have an extra 2??
		{ "specialaward", TMDbDataType::TEXT, 30, 100 }, // 4276, 10b4, Scouter's Key // Special Awards
		{ "specialawarddate", TMDbDataType::TEXT, 9, 100 }, // 7276, 1c6c, 20020101
		{ "emergencycontact1", TMDbDataType::TEXT, 30, 1 }, // 8176, 1ff0, EmergencyContact1
		{ "emergencycontact2", TMDbDataType::TEXT, 32, 1 }, // 8206, 200e, EmergencyContact2
		{ "emergencyphoneareacode1", TMDbDataType::TEXT, 5, 1 }, // 8238, 202e, 434
		{ "emergencyphonenumber1", TMDbDataType::TEXT, 14, 1 }, // 8243, 2033, 234-5501
		{ "emergencyphoneext", TMDbDataType::TEXT, 9, 1 }, // 8257, 2041, 1
		{ "emergencyphoneareacode2", TMDbDataType::TEXT, 5, 1 }, // 8266, 204a, 434
		{ "emergencyphonenumber2", TMDbDataType::TEXT, 14, 1 }, // 8271, 204f, 324-5502
		{ "emergencyphonepreext2", TMDbDataType::TEXT, 7, 1 }, // 8285, 205d, 2
		{ "doctor", TMDbDataType::TEXT, 32, 1 }, // 8292, 2064, Doctor Flanagan
		{ "doctorphoneareacode", TMDbDataType::TEXT, 5, 1 }, // 8324, 2084, 434
		{ "doctorphonenumber", TMDbDataType::TEXT, 14, 1 }, // 8329, 2089, 154-5503
		{ "doctorphoneext", TMDbDataType::TEXT, 2, 1 }, // 8343, 2097, 3
		{ "column152", TMDbDataType::TEXT, 5, 1 }, // 8345, 2099, 2 // ????
		{ "insurancecompany", TMDbDataType::TEXT, 32, 1 }, // 8350, 209e, Blue Cross Blue Shield
		{ "insurancecompanyphoneareacode", TMDbDataType::TEXT, 5, 1 }, // 8382, 20be, 800
		{ "insurancecompanyphonenumber", TMDbDataType::TEXT, 14, 1 }, // 8387, 20c3, 432-5504
		{ "insurancecompanyphoneext", TMDbDataType::TEXT, 7, 1 }, // 8401, 20d1, 4
		{ "insurancepolicynumber", TMDbDataType::TEXT, 20, 1 }, // 8408, 20d8, policynum
		{ "insurancepolicygroup", TMDbDataType::TEXT, 20, 1 }, // 8428, 20ec, policygroup
		{ "medications", TMDbDataType::TEXT, 80, 1 }, // 8448, 2100, Medications
		{ "medicalallergies", TMDbDataType::TEXT, 80, 1 }, // 8528, 2150, Allergies
		{ "medicalother", TMDbDataType::TEXT, 26, 1 }, // 8608, 21a0, other medical information
		{ "column162", TMDbDataType::TEXT, 124, 1 }, // 8634, 21ba, todo mut be special needs/description fasdfasdfasdfasdfasdf asdf lasfl; as;dlfj askl;fklas d;fkl ;asld fj;lksdf lasj fj;askldf;klas dfkl; as;df jaskl;df jl;kas d
		{ "medicalparta", TMDbDataType::TEXT, 9, 1 }, // 8758, 2236, 20010225
		{ "medicalpartb", TMDbDataType::TEXT, 9, 1 }, // 8767, 223f, 20020225
		{ "medicalpartc", TMDbDataType::TEXT, 9, 1 }, // 8776, 2248, 20030225
		{ "medicaltetnus", TMDbDataType::TEXT, 1009, 1 }, // 8785, 2251, 20050621 // todo: obviusly some other stuff here
		{ "medicalpartd", TMDbDataType::TEXT, 502, 1 }, // 9794, 2642, 20040225 // todo: obvioulsy more stuff here
		{ "column168", TMDbDataType::INTEGER, 4, 1 }, // 10296, 2838, 1 // todo: Help form on file??
		{ "column169", TMDbDataType::INTEGER, 4, 1 }, // 10300, 283c, 3 // ????
		{ "label", TMDbDataType::TEXT, 30, 4 }, // 10304, 2840, LABEL1
		{ "labelcheck", TMDbDataType::INTEGER, 4, 4 }, // 10424, 28b8, 1
		{ "linktolabel", TMDbDataType::TEXT, 260, 4 }, // 10440, 28c8, linketoLabel1
		{ "remarks", TMDbDataType::TEXT, 1400, 1 }, // 11480, 2cd8, THIS IS A REMAREKS FIELD

		// todo: not sure on size of remarks field or what these remaining items are. 
		{ "column183", TMDbDataType::TEXT, 3, 1 }, // 12880, 3250, 4
		{ "column184", TMDbDataType::TEXT, 17, 1 }, // 12883, 3253, 40326
	};

	// todo: consider a total columns entry so don't have to calculate as well as in debug
	// can verify the entries match expected.
	const TableDataRecordOriented TableDataRecordOrientedAdult =
	{
		"adultdata",
		0x3264, // record size.
		true, //  isEncrypted
		sizeof(TableDataRecordOrientedColumnAdult) / sizeof(*TableDataRecordOrientedColumnAdult), // number of  columns
		TableDataRecordOrientedColumnAdult, // column metadata
	};

	const TableDataRecordOrientedColumn TableDataRecordOrientedColumnParent[] =
	{
		{ "column1", TMDbDataType::TEXT, 9, 1}, // 0, f20, 
		{ "parent1firstname", TMDbDataType::TEXT, 32, 1}, // 9, f29, parentfirst
		{ "parent1initial", TMDbDataType::TEXT, 2, 1}, // 41, f49, F
		{ "parent1lastname", TMDbDataType::TEXT, 20, 1}, // 43, f4b, parentLast
		{ "parent1suffix", TMDbDataType::TEXT, 5, 1}, // 63, f5f, s
		{ "parent1nickname", TMDbDataType::TEXT, 16, 1}, // 68, f64, Cal
		{ "parent1phone1type", TMDbDataType::SMALLINT, 2, 1}, // 84, f74, 1
		{ "parent1phone1areacode", TMDbDataType::TEXT, 5, 1}, // 86, f76, 804
		{ "parent1phone1number", TMDbDataType::TEXT, 14, 1}, // 91, f7b, 580-0334
		{ "parent1phone1extension", TMDbDataType::TEXT, 7, 1}, // 105, f89, 1
		{ "parent1phone2type", TMDbDataType::SMALLINT, 2, 1}, // 112, f90, 3
		{ "parent1phone2areacode", TMDbDataType::TEXT, 5, 1}, // 114, f92, 804
		{ "parent1phone2number", TMDbDataType::TEXT, 14, 1}, // 119, f97, 255-9011
		{ "parent1phone2extension", TMDbDataType::TEXT, 7, 1}, // 133, fa5, 2
		{ "parent1phone3type", TMDbDataType::SMALLINT, 2, 1}, // 140, fac, 4
		{ "parent1phone3areacode", TMDbDataType::TEXT, 5, 1}, // 142, fae, 222
		{ "parent1phone3number", TMDbDataType::TEXT, 14, 1}, // 147, fb3, 234-2922
		{ "parent1phone3extension", TMDbDataType::TEXT, 7, 1}, // 161, fc1, 3
		{ "parent1email1", TMDbDataType::TEXT, 60, 1}, // 168, fc8, CalsCamping@outdoors.com
		{ "parent1email2", TMDbDataType::TEXT, 60, 1}, // 228, 1004, parent1email2@somefile.com
		{ "parent1socialsecurity", TMDbDataType::TEXT, 12, 1}, // 288, 1040, 123-45-6789
		{ "parent1dateofbirth", TMDbDataType::TEXT, 9, 1}, // 300, 104c, 19740914
		{ "parent1driverslicense", TMDbDataType::TEXT, 20, 1}, // 309, 1055, 457558888
		{ "parent1driverslicensestate", TMDbDataType::TEXT, 3, 1}, // 329, 1069, VA
		{ "parent1occupation", TMDbDataType::TEXT, 30, 1}, // 332, 106c, Bull Rider
		{ "parent1employer", TMDbDataType::TEXT, 30, 1 }, // 362, 108a, Bulldozers (this should be 30)
		{ "column28", TMDbDataType::INTEGER, 4, 1 }, // 373, 1095,todo: seems unused
		{ "parent1gender", TMDbDataType::INTEGER, 4, 1 }, // 396, 10ac, 1 (m=0, f=1)
		{ "parent1occupationtype", TMDbDataType::INTEGER, 4, 1 }, // 400, 10b0, 291019  todo: occupation type
		{ "parent1hasdata", TMDbDataType::INTEGER, 4, 1 }, // 404, 10b4, 1 // todo: not sure but seems to be a 1 for records that actually have data
		{ "parent1emailreports1", TMDbDataType::INTEGER, 4, 1 }, // 408, 10b8, 1
		{ "parent1emailreports2", TMDbDataType::INTEGER, 4, 1 }, // 412, 10bc, 1 
		{ "parent1linktoadult", TMDbDataType::INTEGER, 4, 1 }, // 416, 10c0, ! // todo: occupations type? was text but changed to integer maybe link
		{ "parent1linktopoc", TMDbDataType::INTEGER, 4, 1 }, // 420, 10c4, 5 // todo: Maybe a link just guessing
		{ "parent1linktombc", TMDbDataType::INTEGER, 4, 1 }, // 424, 10c8, ���� // todo: probably the links?
		{ "column36", TMDbDataType::TEXT, 261, 1 }, // 
		
		// parent2
		{ "parent2firstname", TMDbDataType::TEXT, 32, 1}, // 9, f29, parentfirst
		{ "parent2initial", TMDbDataType::TEXT, 2, 1}, // 41, f49, F
		{ "parent2lastname", TMDbDataType::TEXT, 20, 1}, // 43, f4b, parentLast
		{ "parent2suffix", TMDbDataType::TEXT, 5, 1}, // 63, f5f, s
		{ "parent2nickname", TMDbDataType::TEXT, 16, 1}, // 68, f64, Cal
		{ "parent2phone1type", TMDbDataType::SMALLINT, 2, 1}, // 84, f74, 1
		{ "parent2phone1areacode", TMDbDataType::TEXT, 5, 1}, // 86, f76, 804
		{ "parent2phone1number", TMDbDataType::TEXT, 14, 1}, // 91, f7b, 580-0334
		{ "parent2phone1extension", TMDbDataType::TEXT, 7, 1}, // 105, f89, 1
		{ "parent2phone2type", TMDbDataType::SMALLINT, 2, 1}, // 112, f90, 3
		{ "parent2phone2areacode", TMDbDataType::TEXT, 5, 1}, // 114, f92, 804
		{ "parent2phone2number", TMDbDataType::TEXT, 14, 1}, // 119, f97, 255-9011
		{ "parent2phone2extension", TMDbDataType::TEXT, 7, 1}, // 133, fa5, 2
		{ "parent2phone3type", TMDbDataType::SMALLINT, 2, 1}, // 140, fac, 4
		{ "parent2phone3areacode", TMDbDataType::TEXT, 5, 1}, // 142, fae, 222
		{ "parent2phone3number", TMDbDataType::TEXT, 14, 1}, // 147, fb3, 234-2922
		{ "parent2phone3extension", TMDbDataType::TEXT, 7, 1}, // 161, fc1, 3
		{ "parent2email1", TMDbDataType::TEXT, 60, 1}, // 168, fc8, CalsCamping@outdoors.com
		{ "parent2email2", TMDbDataType::TEXT, 60, 1}, // 228, 1004, parent2email2@somefile.com
		{ "parent2socialsecurity", TMDbDataType::TEXT, 12, 1}, // 288, 1040, 123-45-6789
		{ "parent2dateofbirth", TMDbDataType::TEXT, 9, 1}, // 300, 104c, 19740914
		{ "parent2driverslicense", TMDbDataType::TEXT, 20, 1}, // 309, 1055, 457558888
		{ "parent2driverslicensestate", TMDbDataType::TEXT, 3, 1}, // 329, 1069, VA
		{ "parent2occupation", TMDbDataType::TEXT, 30, 1}, // 332, 106c, Bull Rider
		{ "parent2employer", TMDbDataType::TEXT, 30, 1 }, // 362, 108a, Bulldozers (this should be 30)
		{ "column40", TMDbDataType::INTEGER, 4, 1 }, // 373, 1095,todo: seems unused
		{ "parent2gender", TMDbDataType::INTEGER, 4, 1 }, // 396, 10ac, 1 (m=0, f=1)
		{ "parent2occupationtype", TMDbDataType::INTEGER, 4, 1 }, // 400, 10b0, 291019  todo: occupation type
		{ "parent2hasdata", TMDbDataType::INTEGER, 4, 1 }, // 404, 10b4, 1 // todo: not sure but seems to be a 1 for records that actually have data
		{ "parent2emailreports1", TMDbDataType::INTEGER, 4, 1 }, // 408, 10b8, 1
		{ "parent2emailreports2", TMDbDataType::INTEGER, 4, 1 }, // 412, 10bc, 1 
		{ "parent2linktoadult", TMDbDataType::INTEGER, 4, 1 }, // 416, 10c0, ! // todo: occupations type? was text but changed to integer maybe link
		{ "parent2linktopoc", TMDbDataType::INTEGER, 4, 1 }, // 420, 10c4, 5 // todo: Maybe a link just guessing
		{ "parent2linktombc", TMDbDataType::INTEGER, 4, 1 }, // 424, 10c8, ���� // todo: probably the links?
		{ "column56", TMDbDataType::TEXT, 256, 1 }, // 

		// home phone
		{ "homephonetype", TMDbDataType::SMALLINT, 2, 1 }, // 1364, 1474, 7, I think always 7 family honme phone
		{ "homephoneareacode", TMDbDataType::TEXT, 5, 1 }, // 1366, 1476, 804
		{ "homephonenumber", TMDbDataType::TEXT, 14, 1 }, // 1371, 147b, 281-0001
		{ "phomephoneextension", TMDbDataType::TEXT, 7, 1 }, // 1385, 1489, 1

		// family vehicles
		{ "familyvehicle1seatbelts", TMDbDataType::INTEGER, 4, 1 }, // 1392, 1490, 5
		{ "familyvehicle1trailerhit", TMDbDataType::INTEGER, 4, 1 }, // 1396, 1494, 1
		{ "familyvehicle1year", TMDbDataType::TEXT, 3, 1 }, // 1400, 1498, 96
		{ "familyvehicle1make", TMDbDataType::TEXT, 16, 1 }, // 1403, 149b, GEO
		{ "familyvehicle1model", TMDbDataType::TEXT, 16, 1 }, // 1419, 14ab, Prizm
		{ "familyvehicle1plate", TMDbDataType::TEXT, 16, 1 }, // 1435, 14bb, BL 5662
		{ "familyvehicle1perperson", TMDbDataType::TEXT, 5, 1 }, // 1451, 14cb, 1001 
		{ "familyvehicle1peraccident", TMDbDataType::TEXT, 5, 1 }, // 1456, 14d0, 200
		{ "familyvehicle1perproperty", TMDbDataType::TEXT, 7, 1 }, // 1461, 14d5, 50
		{ "familyvehicle2seatbelts", TMDbDataType::INTEGER, 4, 1 }, // 1392, 1490, 5
		{ "familyvehicle2trailerhit", TMDbDataType::INTEGER, 4, 1 }, // 1396, 1494, 1
		{ "familyvehicle2year", TMDbDataType::TEXT, 3, 1 }, // 1400, 1498, 96
		{ "familyvehicle2make", TMDbDataType::TEXT, 16, 1 }, // 1403, 149b, GEO
		{ "familyvehicle2model", TMDbDataType::TEXT, 16, 1 }, // 1419, 14ab, Prizm
		{ "familyvehicle2plate", TMDbDataType::TEXT, 16, 1 }, // 1435, 14bb, BL 5662
		{ "familyvehicle2perperson", TMDbDataType::TEXT, 5, 1 }, // 1451, 14cb, 1001 
		{ "familyvehicle2peraccident", TMDbDataType::TEXT, 5, 1 }, // 1456, 14d0, 200
		{ "familyvehicle2perproperty", TMDbDataType::TEXT, 7, 1 }, // 1461, 14d5, 50
		{ "linktovehiclesadultrecord", TMDbDataType::INTEGER, 4, 1 }, // 1537, 1521, 50

		// Home addresses
		{ "homeaddressline1", TMDbDataType::TEXT, 30, 1 }, // 1548, 152c, address1asdfsadfasfsafd
		{ "homeaddressline2", TMDbDataType::TEXT, 30, 1 }, // 1578, 154a, address2asdfasdfsadf
		{ "homeaddresscity", TMDbDataType::TEXT, 20, 1 }, // 1608, 1568, homecityadsfsafdasf
		{ "homeaddressstate", TMDbDataType::TEXT, 3, 1 }, // 1628, 157c, VA
		{ "homeaddresszipcode", TMDbDataType::TEXT, 11, 1 }, // 1631, 157f, 99352
		{ "homeaddresscountry", TMDbDataType::TEXT, 3, 1 }, // 1642, 158a, US
		{ "mailingaddressline1", TMDbDataType::TEXT, 30, 1 }, // 1645, 158d, mailingaddress1
		{ "mailingaddressline2", TMDbDataType::TEXT, 30, 1 }, // 1675, 15ab, mailingaddress1
		{ "mailingaddresscity", TMDbDataType::TEXT, 20, 1 }, // 1705, 15c9, mailingcity
		{ "mailingaddressstate", TMDbDataType::TEXT, 3, 1 }, // 1725, 15dd, WA
		{ "mailingaddresszip", TMDbDataType::TEXT, 11, 1 }, // 1728, 15e0, 98072
		{ "mailingaddresscountry", TMDbDataType::TEXT, 10, 1 }, // 1739, 15eb, US

		// Alternate relative
		{ "altrelfirstname", TMDbDataType::TEXT, 32, 1}, // 9, f29, parentfirst
		{ "altrelinitial", TMDbDataType::TEXT, 2, 1}, // 41, f49, F
		{ "altrellastname", TMDbDataType::TEXT, 20, 1}, // 43, f4b, parentLast
		{ "altrelsuffix", TMDbDataType::TEXT, 5, 1}, // 63, f5f, s
		{ "altrelnickname", TMDbDataType::TEXT, 16, 1}, // 68, f64, Cal
		{ "altrelphone1type", TMDbDataType::SMALLINT, 2, 1}, // 84, f74, 1
		{ "altrelphone1areacode", TMDbDataType::TEXT, 5, 1}, // 86, f76, 804
		{ "altrelphone1number", TMDbDataType::TEXT, 14, 1}, // 91, f7b, 580-0334
		{ "altrelphone1extension", TMDbDataType::TEXT, 7, 1}, // 105, f89, 1
		{ "altrelphone2type", TMDbDataType::SMALLINT, 2, 1}, // 112, f90, 3
		{ "altrelphone2areacode", TMDbDataType::TEXT, 5, 1}, // 114, f92, 804
		{ "altrelphone2number", TMDbDataType::TEXT, 14, 1}, // 119, f97, 255-9011
		{ "altrelphone2extension", TMDbDataType::TEXT, 7, 1}, // 133, fa5, 2
		{ "altrelphone3type", TMDbDataType::SMALLINT, 2, 1}, // 140, fac, 4
		{ "altrelphone3areacode", TMDbDataType::TEXT, 5, 1}, // 142, fae, 222
		{ "altrelphone3number", TMDbDataType::TEXT, 14, 1}, // 147, fb3, 234-2922
		{ "altrelphone3extension", TMDbDataType::TEXT, 7, 1}, // 161, fc1, 3
		{ "altrelemail1", TMDbDataType::TEXT, 60, 1}, // 168, fc8, CalsCamping@outdoors.com
		{ "altrelemail2", TMDbDataType::TEXT, 60, 1}, // 228, 1004, altrelemail2@somefile.com
		{ "altrelsocialsecurity", TMDbDataType::TEXT, 12, 1}, // 288, 1040, 123-45-6789
		{ "altreldateofbirth", TMDbDataType::TEXT, 9, 1}, // 300, 104c, 19740914
		{ "altreldriverslicense", TMDbDataType::TEXT, 20, 1}, // 309, 1055, 457558888
		{ "altreldriverslicensestate", TMDbDataType::TEXT, 3, 1}, // 329, 1069, VA
		{ "altreloccupation", TMDbDataType::TEXT, 30, 1}, // 332, 106c, Bull Rider
		{ "altrelemployer", TMDbDataType::TEXT, 30, 1 }, // 362, 108a, Bulldozers (this should be 30)
		{ "altrelcolumn40", TMDbDataType::INTEGER, 4, 1 }, // 373, 1095,todo: seems unused
		{ "altrelgender", TMDbDataType::INTEGER, 4, 1 }, // 396, 10ac, 1 (m=0, f=1)
		{ "altreloccupationtype", TMDbDataType::INTEGER, 4, 1 }, // 400, 10b0, 291019  todo: occupation type
		{ "altrelhasdata", TMDbDataType::INTEGER, 4, 1 }, // 404, 10b4, 1 // todo: not sure but seems to be a 1 for records that actually have data
		{ "altrelemailreports1", TMDbDataType::INTEGER, 4, 1 }, // 408, 10b8, 1
		{ "altrelemailreports2", TMDbDataType::INTEGER, 4, 1 }, // 412, 10bc, 1 
		{ "altrellinktoadult", TMDbDataType::INTEGER, 4, 1 }, // 416, 10c0, ! // todo: occupations type? was text but changed to integer maybe link
		{ "altrellinktopoc", TMDbDataType::INTEGER, 4, 1 }, // 420, 10c4, 5 // todo: Maybe a link just guessing
		{ "altrellinktombc", TMDbDataType::INTEGER, 4, 1 }, // 424, 10c8, ���� // todo: probably the links?
		{ "altrelcolumn56", TMDbDataType::TEXT, 256, 1 }, // 
		{ "altrelhomephonetype", TMDbDataType::SMALLINT, 2, 1 }, // 2424, 1898, 7 // alt home phone
		{ "altrelhomephoneareacode", TMDbDataType::TEXT, 5, 1 }, // 2426, 189a, 425
		{ "altrelhomephonenumber", TMDbDataType::TEXT, 14, 1 }, // 2431, 189f, 444-0001
		{ "altrelhomephoneextension", TMDbDataType::TEXT, 7, 1 }, // 2445, 18ad, 1
		{ "altrelspouse", TMDbDataType::TEXT, 16, 1 }, // 2452, 18b4, spoute
		{ "altrelremarks", TMDbDataType::TEXT, 80, 1 }, // 2468, 18c4, remakrs
		{ "altreladdressline1", TMDbDataType::TEXT, 30, 1 }, // 2548, 1914, altaddress1
		{ "altreladdressline2", TMDbDataType::TEXT, 30, 1 }, // 2578, 1932, altaddress2
		{ "altrelcity", TMDbDataType::TEXT, 20, 1 }, // 2608, 1950, altcity
		{ "altrelstate", TMDbDataType::TEXT, 3, 1 }, // 2628, 1964, WA
		{ "altrelzip", TMDbDataType::TEXT, 11, 1 }, // 2631, 1967, 98072
		{ "altrelcountry", TMDbDataType::TEXT, 3, 1 }, // 2642, 1972, AD
		{ "altrelmailingaddressline1", TMDbDataType::TEXT, 30, 1 }, // 2645, 1975, altmailingaddress1
		{ "altrelmailingaddressline2", TMDbDataType::TEXT, 30, 1 }, // 2675, 1993, altmailingaddress2
		{ "altrelmailingcity", TMDbDataType::TEXT, 20, 1 }, // 2705, 19b1, altmcitasdy
		{ "altrelmailingstate", TMDbDataType::TEXT, 3, 1 }, // 2725, 19c5, WA
		{ "altrelmailingzip", TMDbDataType::TEXT, 11, 1 }, // 2728, 19c8, 98072
		{ "altrelmailingcountry", TMDbDataType::TEXT, 5, 1 }, // 2739, 19d3, TR
		{ "altrelvehiclebels", TMDbDataType::INTEGER, 4, 1 }, // 2744, 19d8, 	
		{ "altrelvehiclehitch", TMDbDataType::INTEGER, 4, 1 }, // 2744, 19d8, 	
		{ "altrelvehicleyear", TMDbDataType::TEXT, 3, 1 }, // 2752, 19e0, 67
		{ "altrelvehiclemake", TMDbDataType::TEXT, 16, 1 }, // 2755, 19e3, volks // family vehicle
		{ "altrelvehiclemodel", TMDbDataType::TEXT, 16, 1 }, // 2771, 19f3, bug
		{ "altrelvehicleplate", TMDbDataType::TEXT, 16, 1 }, // 2787, 1a03, herby
		{ "altrelvehicleperperson", TMDbDataType::TEXT, 5, 1 }, // 2803, 1a13, 1111
		{ "altrelvehicleperaccident", TMDbDataType::TEXT, 5, 1 }, // 2808, 1a18, 2222
		{ "altrelvehicleperproperty", TMDbDataType::TEXT, 11, 1 }, // 2813, 1a1d, 3333
		{ "column156", TMDbDataType::INTEGER, 4, 1 }, // 2824, 1a28, 
		{ "column157", TMDbDataType::INTEGER, 4, 1 }, // 2824, 1a28,  

		// seems to be a start of a bunch of -1s
		{ "column158", TMDbDataType::INTEGER, 4, 1 }, // 2832, 1a30, ������������������������������������ 
		{ "column159", TMDbDataType::INTEGER, 4, 1 }, // 2832, 1a30, ������������������������������������ 
		{ "column160", TMDbDataType::INTEGER, 4, 1 }, // 2832, 1a30, ������������������������������������ 
		{ "column161", TMDbDataType::TEXT, 1028, 1 }, // 2832, 1a30, ������������������������������������ 
	};

	// todo: consider a total columns entry so don't have to calculate as well as in debug
	// can verify the entries match expected.
	const TableDataRecordOriented TableDataRecordOrientedParent =
	{
		"parentdata",
		0xf20, // record size. 
		true, //  isEncrypted
		sizeof(TableDataRecordOrientedColumnParent) / sizeof(*TableDataRecordOrientedColumnParent), // number of  columns
		TableDataRecordOrientedColumnParent, // column metadata
	};

// todo: review if want any type/schema information on this
typedef struct tag_TableDataColumnOrientedColumn {
	const char *name; // column Name
	const TMDbDataType dataType;
	unsigned int size; // size of a record in the column
	const unsigned long fileOffset; // Offset from start of file to the column Data
} TableDataColumnOrientedColumn;

// Column oriented are stored data tables as sections of columns of data (Activity)
typedef struct tag_TableDataColumnOriented
{
	const char *tableName;
	const unsigned long totalNumberOfRecords; // total number of Records that can be stored.
	const unsigned int numColumns;
	const TableDataColumnOrientedColumn *pmetaData;

} TableDataColumnOriented;


// todo: on the file offset if know the total records and order of columns should be 
// able to calculate the file Offset. Level starts at 0x04 though
// todo: for "columns" possible off by a few bytes based on diff.
const TableDataColumnOrientedColumn TableDataColumnOrientedColumnActvity[] =
{
	// first four bytes of file uknonw 
	// name, size, fileOffset
	{ "Level", TMDbDataType::TEXT, 10, 0x0004 },
	{ "ActivityType", TMDbDataType::TEXT, 10, 0x004e24 },
	{ "Date", TMDbDataType::TEXT, 9, 0x009c44 },
	{ "Location", TMDbDataType::TEXT, 30, 0x01ccf4 },
	{ "Remarks", TMDbDataType::TEXT, 35, 0x02b754 },
	{ "column6", TMDbDataType::INTEGER, 4, 0x03c8c4 }, // todo: Always zero on test data

	{ "starttime ", TMDbDataType::TEXT, 30, 0x059d84 }, // could be 59d83 (diff if kind of weac
	{ "endtime ", TMDbDataType::TEXT, 30, 0x05eba4 }, // could be 59d83 (diff if kind of weac

	//  amount is in decimal so 50 is 5.0 unit depends on the ActivityType outing=hours, Hiking=miles, camping=nights
	// serviceproject=hours, etc. so need to check. Can find mapping in "Define Activities" dialog. 
	{ "amount ", TMDbDataType::INTEGER, 4, 0x639c4 }, 

	{ "includeoncalendar", TMDbDataType::INTEGER, 4, 0x65904 }, 
	{ "cabincamp", TMDbDataType::INTEGER, 4, 0x67844 }, 
	{ "summercamp", TMDbDataType::INTEGER, 4, 0x69784 }, 
	{ "credittowards1st2ndclass", TMDbDataType::INTEGER, 4, 0x6b6c4 }, 
	{ "backpacking", TMDbDataType::INTEGER, 4, 0x6d604 },

	// todo: some value, seems to do something with attendance but (scouts + adults) don't match but could be because some
	// scouts remove or archived since outing entered.
	{ "column18", TMDbDataType::INTEGER, 4, 0x6f544 }, 
};

const TableDataColumnOriented TableDataColumnOrientedActivity =
{
	"Activity", // table name
	2000, // total number of records
	sizeof(TableDataColumnOrientedColumnActvity) / sizeof(*TableDataColumnOrientedColumnActvity), // number of  columns
	TableDataColumnOrientedColumnActvity, // column metadata
};


// todo: need to determine if going to be zero Index'd or 1 for sparce items like activity.
// if 1 based then could possibly also collapse repeating type records in the ScoutData.

// todo: I don't see the training stuff here.
// todo: sibling link
// todo: parent record index;

// Description of a ScoutData record
// todo: these don't match the order or is a direct field map to what is in the Troopmaster 
//  the SOAR export format also doesn't match but does seem to Match the Troopmaster export (which would make sense)
//  possibly want some type of virtual table like I think you'd want for Outing attendance.

// Home address and stuff must come from the parentRecordIndex.

const TableDataRecordOrientedColumn TableDataRecordOrientedColumnScoutData[] =
{ { "column1", TMDbDataType::TEXT, 9, 1 }, // 0, 0, 
{ "firstname", TMDbDataType::TEXT, 16, 1 }, // 9, 9, firstname
{ "middlename", TMDbDataType::TEXT, 16, 1 }, // 25, 19, middle
{ "middleinitial", TMDbDataType::TEXT, 2, 1 }, // 41, 29, 109
{ "lastname", TMDbDataType::TEXT, 20, 1 }, // 43, 2b, lastname
{ "suffix", TMDbDataType::TEXT, 5, 1 }, // 63, 3f, suff
{ "nickname", TMDbDataType::TEXT, 16, 1 }, // 68, 44, nickname
{ "bsaid", TMDbDataType::TEXT, 20, 1 }, // 84, 54, bsaid
{ "phonetype2", TMDbDataType::SMALLINT, 2, 1 }, // 104, 68, 3 // todo: not sure where phone 1 is
{ "phoneareacode2", TMDbDataType::TEXT, 5, 1 }, // 106, 6a, 804
{ "phonenumber2", TMDbDataType::TEXT, 14, 1 }, // 111, 6f, 281-0002
{ "phoneextension2", TMDbDataType::TEXT, 7, 1 }, // 125, 7d, 2
{ "phonetype3", TMDbDataType::SMALLINT, 2, 1 }, // 132, 84, 4
{ "phoneareacode3", TMDbDataType::TEXT, 5, 1 }, // 134, 86, 804
{ "phonenumber3", TMDbDataType::TEXT, 14, 1 }, // 139, 8b, 281-0003
{ "phoneextension3", TMDbDataType::TEXT, 7, 1 }, // 153, 99, 3
{ "email1", TMDbDataType::TEXT, 60, 1 }, // 160, a0, JJCab44@delta.com
{ "email2", TMDbDataType::TEXT, 60, 1 }, // 220, dc, email2@email.com
{ "emailreports1", TMDbDataType::INTEGER, 4, 1 }, // 280, 118, 1
{ "emailreports2", TMDbDataType::INTEGER, 4, 1 }, // 284, 11c, 1
{ "dateofbirth", TMDbDataType::TEXT, 9, 1 }, // 288, 120, 19670404
{ "socialsecurity", TMDbDataType::TEXT, 12, 1 }, // 297, 129, 123-45-6789
{ "grade", TMDbDataType::TEXT, 3, 1 }, // 309, 135, 12
{ "school", TMDbDataType::TEXT, 30, 1 }, // 312, 138, Alma Middle School
{ "church", TMDbDataType::TEXT, 30, 1 }, // 342, 156, Churchsdfsdfasdf
{ "cubscoutfrom", TMDbDataType::TEXT, 9, 1 }, // 372, 174, 20020202
{ "cubscoutto", TMDbDataType::TEXT, 9, 1 }, // 381, 17d, 20030303
{ "driverslicense", TMDbDataType::TEXT, 20, 1 }, // 390, 186, driverslicens
{ "driverslicensestate", TMDbDataType::TEXT, 6, 1 }, // 410, 19a, WA
{ "parentrecordIndex", TMDbDataType::INTEGER, 4, 1 }, // 416, 1a0, 1 // parentrecordIndex??
{ "column31", TMDbDataType::INTEGER, 4, 1 }, // 420, 1a4, 1
{ "column32", TMDbDataType::INTEGER, 4, 1 }, // 424, 1a8, 2
{ "column33", TMDbDataType::INTEGER, 4, 1 }, // 428, 1ac, 4
{ "column34", TMDbDataType::INTEGER, 4, 1 }, // 432, 1b0, 1
{ "column35", TMDbDataType::INTEGER, 4, 1 }, // 436, 1b4, 1
{ "column36", TMDbDataType::INTEGER, 4, 1 }, // 440, 1b8, 1
{ "column37", TMDbDataType::INTEGER, 4, 1 }, // 444, 1bc, 1
{ "column38", TMDbDataType::INTEGER, 4, 1 }, // 448, 1c0, 2
{ "column39", TMDbDataType::INTEGER, 4, 1 }, // 452, 1c4, 1
{ "column40", TMDbDataType::INTEGER, 4, 1 }, // 456, 1c8, 1
{ "column41", TMDbDataType::INTEGER, 4, 1 }, // 460, 1cc, 1
{ "column42", TMDbDataType::TEXT, 12, 1 }, // 464, 1d0, Complete // todo: three completes? or status?
{ "column43", TMDbDataType::TEXT, 24, 1 }, // 476, 1dc, 
{ "column44", TMDbDataType::TEXT, 9, 1 }, // 500, 1f4, 20010101 
{ "priorservicefrom", TMDbDataType::TEXT, 9, 1 }, // 509, 1fd, 20010101
{ "column46", TMDbDataType::TEXT, 27, 1 }, // 518, 206, 20030303
{ "priorserviceto", TMDbDataType::TEXT, 9, 4 }, // 545, 221, 20020202
{ "priorserviceunit", TMDbDataType::TEXT, 5, 4 }, // 581, 245, 520
{ "prioriservicelevel", TMDbDataType::TEXT, 9, 4 }, // 601, 259, Troop
{ "priorservicecounsel", TMDbDataType::TEXT, 4, 4 }, // 637, 27d, 1
{ "column54", TMDbDataType::TEXT, 3, 1 }, // // todo: took column54 data to make prioerserveicecounsel.
{ "column55", TMDbDataType::INTEGER, 4, 1 }, // 656, 290, 3
{ "column56", TMDbDataType::INTEGER, 4, 1 }, // 660, 294, 12
{ "column57", TMDbDataType::INTEGER, 4, 1 }, // 664, 298, 1
{ "column58", TMDbDataType::INTEGER, 4, 1 }, // 668, 29c, 1
{ "currentposition", TMDbDataType::TEXT, 20, 2 }, // 672, 2a0, Junior Asst SM
{ "currentpositiondate", TMDbDataType::TEXT, 9, 2 }, // 712, 2c8, 20010101
{ "column62", TMDbDataType::TEXT, 2, 1 }, // 721, 2d1, //  todo: not sure what this is.
{ "leaserhiphistorycredit", TMDbDataType::INTEGER, 4, 15 }, // 732, 2dc, 1 
{ "leadershiphistoryposition", TMDbDataType::TEXT, 20, 15 }, // 792, 318, Historian
{ "leadershiphistoryFrom", TMDbDataType::TEXT, 9, 15 }, // 1092, 444, 20121128
{ "leadershiphistoryTo", TMDbDataType::TEXT, 9, 15 }, // 1227, 4cb, 20130220
{ "column78", TMDbDataType::TEXT, 2, 1 }, // 1254, 4e6, 20030303 // todo: Unknown/
{ "emergencycontact1", TMDbDataType::TEXT, 30, 1 }, // 1364, 554, Emercencycontact2
{ "emergencycontact2", TMDbDataType::TEXT, 32, 1 }, // 1394, 572, emergencycontact2
{ "emergencycontactareacode1", TMDbDataType::TEXT, 5, 1 }, // 1426, 592, 804
{ "emergencycontactnumber1", TMDbDataType::TEXT, 14, 1 }, // 1431, 597, 665-1232
{ "emergencycontactextension1", TMDbDataType::TEXT, 9, 1 }, // 1445, 5a5, 1
{ "emergencycontactareacode2", TMDbDataType::TEXT, 5, 1 }, // 1454, 5ae, 804
{ "emergencycontactnumber2", TMDbDataType::TEXT, 14, 1 }, // 1459, 5b3, 555-1212
{ "emergencycontactextension2", TMDbDataType::TEXT, 7, 1 }, // 1473, 5c1, 2
{ "doctor", TMDbDataType::TEXT, 32, 1 }, // 1480, 5c8, Dr William Fox
{ "doctorareacode", TMDbDataType::TEXT, 5, 1 }, // 1512, 5e8, 804
{ "doctornumber", TMDbDataType::TEXT, 14, 1 }, // 1517, 5ed, 839-8747
{ "doctorextension", TMDbDataType::TEXT, 7, 1 }, // 1531, 5fb, 3
{ "insurancecompany", TMDbDataType::TEXT, 32, 1 }, // 1538, 602, insurancecompan
{ "insurancecompanyareacode", TMDbDataType::TEXT, 5, 1 }, // 1570, 622, 555
{ "insurancecompanynumber", TMDbDataType::TEXT, 14, 1 }, // 1575, 627, 121-1212
{ "insuranccompanyextension", TMDbDataType::TEXT, 7, 1 }, // 1589, 635, 4
{ "policynumber", TMDbDataType::TEXT, 20, 1 }, // 1596, 63c, policynumber
{ "policygroup", TMDbDataType::TEXT, 20, 1 }, // 1616, 650, poicygroup
{ "medicalmedications", TMDbDataType::TEXT, 80, 1 }, // 1636, 664, medications
{ "medicalallergies", TMDbDataType::TEXT, 80, 1 }, // 1716, 6b4, allergies
{ "medicalother", TMDbDataType::TEXT, 150, 1 }, // 1796, 704, other
{ "medicalparta", TMDbDataType::TEXT, 9, 1 }, // 1946, 79a, 20011101
{ "mericalpartb", TMDbDataType::TEXT, 9, 1 }, // 1955, 7a3, 20021218
{ "medicalpartc", TMDbDataType::TEXT, 9, 1 }, // 1964, 7ac, 20031218
{ "tetnusshot", TMDbDataType::TEXT, 9, 1 }, // 1973, 7b5, 20050505 // todo lots of space
{ "column105", TMDbDataType::TEXT, 1000, 1 }, // 1973, 7b5, 20050505 // todo lots of space, maybe reserved?
{ "medicalpartd", TMDbDataType::TEXT, 502, 1 }, // 2982, ba6, 20041218 // todo: space
{ "column106", TMDbDataType::INTEGER, 4, 1 }, // 3484, d9c, 1 //??
{ "column107", TMDbDataType::INTEGER, 4, 1 }, // 3488, da0, 5 //??
{ "label1", TMDbDataType::TEXT, 30, 1 }, // 3492, da4, label1texgt
{ "label2", TMDbDataType::TEXT, 30, 1 }, // 3522, dc2, label2text
{ "label3", TMDbDataType::TEXT, 30, 1 }, // 3552, de0, label3text
{ "label4", TMDbDataType::TEXT, 30, 1 }, // 3582, dfe, label4text
{ "checkbox1", TMDbDataType::INTEGER, 4, 1 }, // 3612, e1c, 1
{ "checkbox2", TMDbDataType::INTEGER, 4, 1 }, // 3616, e20, 1
{ "checkbox3", TMDbDataType::INTEGER, 4, 1 }, // 3620, e24, 1
{ "checkbox4", TMDbDataType::INTEGER, 4, 1 }, // 3624, e28, 1
{ "buttonlink1", TMDbDataType::TEXT, 260, 1 }, // 3628, e2c, button1linktofile
{ "buttonlink2", TMDbDataType::TEXT, 260, 1 }, // 3888, f30, button2linktofile
{ "buttonlink3", TMDbDataType::TEXT, 260, 1 }, // 4148, 1034, button3linktofile
{ "buttonlink4", TMDbDataType::TEXT, 268, 1 }, // 4408, 1138, button4linktofile
{ "column121", TMDbDataType::TEXT, 8, 1 }, // 4676, 1244, 
{ "remarks", TMDbDataType::TEXT, 1400, 1 }, // 4684, 124c, remarks // todo: not sure on size. 
{ "column123", TMDbDataType::TEXT, 3, 1 }, // 6084, 17c4, 3
{ "column124", TMDbDataType::TEXT, 17, 1 }, // 6087, 17c7, 40326
};

const TableDataRecordOriented TableDataRecordOrientedScoutData =
{
	"ScoutData",
	0x17d8, // record size.
	true, //  isEncrypted
	sizeof(TableDataRecordOrientedColumnScoutData) / sizeof(*TableDataRecordOrientedColumnScoutData), // number of  columns
	TableDataRecordOrientedColumnScoutData, // column metadata
};

// todo: is there a c_ convention or anything for this.
const TableDataRecordOriented tmDataRecordOrientedTables[] = 
{ 
	TableDataRecordOrientedScoutData,
	TableDataRecordOrientedAttend,
	TableDataRecordOrientedParent,
	TableDataRecordOrientedAdult,
	TableDataRecordOrientedMBCData,
	TableDataRecordOrientedAdvancement,
	TableDataRecordOrientedPOC,
};

const TableDataColumnOriented tmDataColumnOrientedTables[1] = 
{ 
	TableDataColumnOrientedActivity 
};

// represents a table in a TroopMaster database.
// todo: finalize names.
class TMDbTable
{
public:
	virtual bool ReadNextRecord() = 0; // Must call ReadNextRecord() before reading the first record.
	virtual unsigned int GetNumFields() = 0;
	virtual const char* GetFieldName(unsigned int index) = 0;
	virtual const char* GetFieldValue(unsigned int columnIndex) = 0;
};

// Table class for a Column Oriented persisted Table
class TMDbTableColumnOriented : public TMDbTable
{
	// todo: Stashing currently value on column data but if wanted to build a static cache
	// of columnData so don't have to rebuild with every table creation (aka store it on the db object)
	// would not want it.
	typedef struct tag_ColumnData
	{
		char name[255]; // name of the column, todo: just set at 255 for simplicity review if sqlite or should just make dynamic
		TMDbDataType dataType;
		unsigned long length; // todo: some conventions on names and types would be good.
		unsigned long fileOffset;
		const char *pCurrentRecordValue;
	} ColumnData;

	unsigned long m_totalNumberOfRecords;
	unsigned long m_numColumns;
	ColumnData *m_pcolumnData;

	CFile *m_ptableFile;
	unsigned long m_recordIndex;
	char *m_pData;

public:


	TMDbTableColumnOriented(const unsigned int tmTableDataColumnOrientedIndex,
		CFile *pdataFile)
	{
		const TableDataColumnOriented *m_pmetaData; // todo rename since now a temporary
		m_pmetaData = (const TableDataColumnOriented *)&tmDataColumnOrientedTables[0];

		m_totalNumberOfRecords = m_pmetaData->totalNumberOfRecords;

		m_ptableFile = pdataFile;
		m_recordIndex = -1;

		// interpret the metadata
		m_numColumns = m_pmetaData->numColumns; // match for now but if add repeat count won't
		m_pcolumnData = new ColumnData[m_numColumns];

		// okay now loop through again filling in all the data.
		ColumnData *pCurColumnData = m_pcolumnData;
		for (unsigned long index = 0; index < m_pmetaData->numColumns; index++) // todo: rename numColumsn to entries or something.
		{
			TableDataColumnOrientedColumn *pMetaData = (TableDataColumnOrientedColumn *)m_pmetaData->pmetaData + index;
		// currently no repeat count.
		//	for (unsigned long repeat = 1; repeat <= pMetaData->repeatCount; repeat++)
			{
				strcpy_s(pCurColumnData->name, pMetaData->name); // copy over, bug bug, needs a max and set.
				pCurColumnData->dataType = pMetaData->dataType;
				pCurColumnData->length = pMetaData->size;
				pCurColumnData->fileOffset = pMetaData->fileOffset;
				pCurColumnData->pCurrentRecordValue = NULL;

				// update the vars
				++pCurColumnData;
			}
		}
	}

	~TMDbTableColumnOriented()
	{
		if (NULL != m_ptableFile)
		{
			delete m_ptableFile;
			m_ptableFile = NULL;
		}

		if (NULL != m_pData)
		{
			delete m_pData;
			m_pData = NULL;
		}

		ClearRecordValueCache();
		if (NULL != m_pcolumnData)
		{
			delete m_pcolumnData;
			m_pcolumnData = NULL;
		}
	}

	bool ReadNextRecord()
	{
		ClearRecordValueCache();

		// Need to seek all over to read a record so just remember the record index we
		// are on
		++m_recordIndex;
		if (m_recordIndex >= m_totalNumberOfRecords)
		{
			return false;
		}

		return true;
	}

	unsigned int GetNumFields()
	{
		return m_numColumns;
	}

	const char* GetFieldName(unsigned int index)
	{
		// todo: consider helper for finding column data. currently only used in 
		// two places so not a big deal.
		const ColumnData *pColumnData = m_pcolumnData + index;
		return pColumnData->name; 
	}

	const char* GetFieldValue(unsigned int columnIndex)
	{
		// need to seek to the column Index offset + recordIndex*sizeof(columnIndex).
		ColumnData *pColumnData = m_pcolumnData + columnIndex;

		if (NULL == pColumnData->pCurrentRecordValue)
		{
			unsigned long fileOffset = (pColumnData->fileOffset + (pColumnData->length*m_recordIndex));

			char *tempBuf = new char[pColumnData->length]; // todo: consider a buffer on class big enough for typical case.
			m_ptableFile->SeekPosition(fileOffset); // todo: bug bug, what if seek fails/
			m_ptableFile->Read(tempBuf, pColumnData->length); // I think here is where you'd want a schema reader.

			const char *pConverted = TMDbDataTypeConverter::Convert(
				pColumnData->dataType, pColumnData->length, tempBuf);
			pColumnData->pCurrentRecordValue = pConverted;

			delete tempBuf;
		}

		// return NULL if an empty String
		if (0 == pColumnData->pCurrentRecordValue[0])
		{
			return NULL;
		}
		else
		{
			return pColumnData->pCurrentRecordValue;
		}
	}

	// clears out any cached record Values.
	void ClearRecordValueCache()
	{
		ColumnData *pCurColumnData = m_pcolumnData;
		for (unsigned long index = 0; index < m_numColumns; index++)
		{
			if (NULL != pCurColumnData->pCurrentRecordValue)
			{
				delete pCurColumnData->pCurrentRecordValue;
				pCurColumnData->pCurrentRecordValue = NULL;
			}

			++pCurColumnData;
		}
	}

};

// Table class for a Record Oriented persisted Table
class TMDbTableRecordOriented: public TMDbTable
{
private:
	typedef struct tag_ColumnData
	{
		char name[255]; // name of the column, todo: just set at 255 for simplicity review if sqlite or should just make dynamic
		TMDbDataType dataType;
		unsigned long length; // todo: some conventions on names and types would be good.
		unsigned long recordOffset;
		const char *pCurrentRecordValue;
	} ColumnData;

	unsigned long m_recordSize;
	unsigned long m_numColumns;
	ColumnData *m_pcolumnData;	
	CFile *m_ptableFile;
	char *m_pData; // todo: consider some sort of smartPtr class but easy enough to delete in destuctor vs. overhead.

public:

	// todo: pasing in inex to TMTableMetadaIndex to be clear this is const data that 
	// shouldn't be deleted. not sure how scales when get multiple tables or a table
	// factory. See what makes sense when get a couple of tables. 
	// table take ownership of the CFile.
	TMDbTableRecordOriented(const unsigned int tableMetadaIndex, CFile *pdataFile)
	{
		const TableDataRecordOriented *m_pmetaData; // todo: rename to not be a methoer.

		m_pmetaData = (const TableDataRecordOriented *)&tmDataRecordOrientedTables[tableMetadaIndex];

		m_recordSize = m_pmetaData->recordSize;

		// for simplicity allocate a buffer big enough to read in the entire record.
		m_pData = new char[m_pmetaData->recordSize];

		// wrap file in a Decrypt file if this is encrypted
		if (m_pmetaData->isEncrypted)
		{
			m_ptableFile = new CFileTMRecEncrypt(pdataFile, m_pmetaData->recordSize);
		}
		else
		{
			m_ptableFile = pdataFile;
		}

		// setup the metadata to use for the column information. 

		// step through metadata to determine the number of columns the table is 
		// going to have.
		m_numColumns = 0;
		for (unsigned long index = 0; index < m_pmetaData->numColumns; index++) // todo: rename numColumsn to entries or something.
		{
			TableDataRecordOrientedColumn *pMetaData = (TableDataRecordOrientedColumn *)m_pmetaData->pmetaData + index;
			m_numColumns += pMetaData->repeatCount;
		}

		m_pcolumnData = new ColumnData[m_numColumns];

		// okay now loop through again filling in all the data.
		ColumnData *pCurColumnData = m_pcolumnData;
		unsigned long recordOffset = 0;
		for (unsigned long index = 0; index < m_pmetaData->numColumns; index++) // todo: rename numColumsn to entries or something.
		{
			TableDataRecordOrientedColumn *pMetaData = (TableDataRecordOrientedColumn *)m_pmetaData->pmetaData + index;
			for (unsigned long repeat = 1; repeat <= pMetaData->repeatCount; repeat++)
			{
				// if repeat count is > 1 we append the repeat num onto the column.
				if (pMetaData->repeatCount > 1)
				{
					CString pColumnName;
					pColumnName += pMetaData->name;

					char *pOutput = new char[500]; // todo: obviously this is too large but need to figure out how to set.
					_itoa_s(repeat, pOutput, 500, 10);
					pColumnName += pOutput;
					delete pOutput;
					strcpy_s(pCurColumnData->name, pColumnName);
				}
				else
				{
					strcpy_s(pCurColumnData->name, pMetaData->name); // copy over, bug bug, needs a max and set.
				}
				pCurColumnData->dataType = pMetaData->dataType;
				pCurColumnData->length = pMetaData->size;
				pCurColumnData->recordOffset = recordOffset;
				pCurColumnData->pCurrentRecordValue = NULL;

				// update the vars
				recordOffset += pMetaData->size;
				++pCurColumnData;
			}
		}
	}

	~TMDbTableRecordOriented()
	{
		if (NULL != m_ptableFile)
		{
			delete m_ptableFile;
			m_ptableFile = NULL;
		}

		if (NULL != m_pData)
		{
			delete m_pData;
			m_pData = NULL;
		}

		ClearRecordValueCache();
		if (NULL != m_pcolumnData)
		{
			delete m_pcolumnData;
			m_pcolumnData = NULL;
		}
	}

	int recordsRead = 0;

	// move ahead to the next record.
	// return false if at end of records.
	bool ReadNextRecord()
	{
		++recordsRead; // just for debugging to see what record at.

		ClearRecordValueCache();
		unsigned int sizeRead = m_ptableFile->Read(m_pData, m_recordSize);
		if (sizeRead < m_recordSize) 
		{
			return false;
		}

		return true;
	}


	// number of fields,
	// todo: rename these to match more of the output.
	unsigned int GetNumFields()
	{
		return m_numColumns;
	}

	const char* GetFieldName(unsigned int index)
	{
		ColumnData *pColumnData = m_pcolumnData + index;
		return pColumnData->name;
	}

	const char* GetFieldValue(unsigned int columnIndex)
	{
		ColumnData *pColumnData = m_pcolumnData + columnIndex;

		// if already have some data just return it.
		if (NULL == pColumnData->pCurrentRecordValue)
		{
			const char *pValue = (m_pData + pColumnData->recordOffset);
			const char *pConverted = TMDbDataTypeConverter::Convert(
				pColumnData->dataType,
				pColumnData->length,
				pValue);

			pColumnData->pCurrentRecordValue = pConverted;
		}

		// return NULL if an empty String
		if (0 == pColumnData->pCurrentRecordValue[0])
		{
			return NULL;
		}
		else
		{
			return pColumnData->pCurrentRecordValue;
		}

		return pColumnData->pCurrentRecordValue;
	}

	// clears out any cached record Values.
	void ClearRecordValueCache()
	{
		ColumnData *pCurColumnData = m_pcolumnData;
		for (unsigned long index = 0; index < m_numColumns; index++)
		{
			if (NULL != pCurColumnData->pCurrentRecordValue)
			{
				delete pCurColumnData->pCurrentRecordValue;
				pCurColumnData->pCurrentRecordValue = NULL;
			}

			++pCurColumnData;
		}
	}
};

int CTMDb::Exec(const char *sql,                           /* SQL to be evaluated */
	int(*callback)(void*, int, const char**, const char**),  /* Callback function */
	void *pBlob,                                    /* 1st argument to callback */
	char **errmsg                              /* Error msg written here */
	)
{
	// errmsg currently isn't implement so just NULL out
	*errmsg = NULL;

	// Make sure connected
	if (!m_fConnected)
	{
		// TODO: should check sqllite behavior.
		return TM_CANTOPEN;
	}

	CString tableName;

	// hack for now always just treat as "select * from [table]";
	// and only a subet of Tables and column data

	// parse the select to get the table name, just walk backwards from
	// end until hit a space
	// todo: need to get stdio proper out of here but since a hack to start with
	// don't worry. 
	int len = strlen(sql);
	char* pspLoc = NULL;
	char *pCurLoc = ((char *) sql) + len;
	while (pCurLoc >= sql)
	{
		if (*pCurLoc == ' ')
		{
			pspLoc = pCurLoc;
			break;
		}

		--pCurLoc;
	}

	if (NULL != pCurLoc)
	{
		++pCurLoc;
		tableName += pCurLoc;
	}


//	tableName = "ScoutData"; // todo: should get table name from query.
//  tableName = "Activity"; // todo: should get table name from query.
//	tableName = "Attend"; // todo: should get table name from query.

	// hack for now always just treat as "select * from scoutdata";
	// todo: check if connected 
	CString filePath(260 /* CStdio::MAX_PATH */ ); // todo: choking on const/static for max path
	filePath = m_tmDirectoryPath; // todo: consider adding a Path.Combine type method
	filePath += "\\";
	filePath += tableName;
	filePath += ".tm4";

#if adv
	if (0 == strcmp("advancement", tableName))
	{
		DatabaseMetaDataDecoder *pDecode = new DatabaseMetaDataDecoder();
		pDecode->DecodeRecordFile(filePath, false, 0xab28, 0);  // todo: had a bug the size as 0xab20. See if docs correct 
		delete pDecode;
	}
#endif

#if poc
	if (0 == strcmp("pocdata", tableName))
	{
		DatabaseMetaDataDecoder *pDecode = new DatabaseMetaDataDecoder();
		pDecode->DecodeRecordFile(filePath, true, 0xA3C, 1 /* advance once record */); 
		delete pDecode;
	}
#endif

#if parentdata
	// just a hack location for putting the decrypt stuf
	if (0 == strcmp("parentdata", tableName))
	{
		DatabaseMetaDataDecoder *pDecode = new DatabaseMetaDataDecoder();
		pDecode->DecodeRecordFile(filePath, true, 0xf20, 1 /* advance once record */); 
		delete pDecode;
	}
#endif

#if scoutdata
	// just a hack location for putting the decrypt stuf
	if (0 == strcmp("scoutdata", tableName))
	{
		DatabaseMetaDataDecoder *pDecode = new DatabaseMetaDataDecoder();
		pDecode->DecodeRecordFile(filePath, true, 0x17d8, 0);
		delete pDecode;
	}
#endif


#if aduldata
	// just a hack location for putting the decrypt stuf
	// see what is in mbcdata
	if (0 == strcmp("adultdata", tableName))
	{
		DatabaseMetaDataDecoder *pDecode = new DatabaseMetaDataDecoder();
		pDecode->DecodeRecordFile(filePath, true, 0x3264, 0);
		delete pDecode;
	}
#endif

#if mbcData
	// just a hack location for putting the decrypt stuf
	// see what is in mbcdata
	if (0 == strcmp("mbcdata", tableName))
	{
		DatabaseMetaDataDecoder *pDecode = new DatabaseMetaDataDecoder();
		pDecode->DecodeRecordFile(filePath, true, 0x874, 5);
		delete pDecode;
	}
#endif

	CFileDisk *pfile = new CFileDisk();
	int result = pfile->Open(filePath, "rb"); // !!!! import to pass in the b for "binary". If don't ascii open will not read all of the file in some cases.
	if (0 == result)
	{
		TMDbTable *pdbTable = NULL; // if don't recognize the table then crash.
		// figure out which table to use.
		if (0 == strcmp("scoutdata", tableName))
		{
			pdbTable = new TMDbTableRecordOriented(0, pfile);
		}
		else if (0 == strcmp("activity", tableName))
		{
			pdbTable = new TMDbTableColumnOriented(0, pfile);
		}
		else if (0 == strcmp("attend", tableName))
		{
			pdbTable = new TMDbTableRecordOriented(1, pfile);
		}
		else if (0 == strcmp("parentdata", tableName))
		{
			pdbTable = new TMDbTableRecordOriented(2, pfile);
		}
		else if (0 == strcmp("adultdata", tableName))
		{
			pdbTable = new TMDbTableRecordOriented(3, pfile);
		}
		else if (0 == strcmp("mbcdata", tableName))
		{
			pdbTable = new TMDbTableRecordOriented(4, pfile);
		}
		else if (0 == strcmp("advancement", tableName))
		{
			pdbTable = new TMDbTableRecordOriented(5, pfile);
		}
		else if (0 == strcmp("pocdata", tableName))
		{
			pdbTable = new TMDbTableRecordOriented(6, pfile);
		}

		unsigned int numFields = pdbTable->GetNumFields();
		char** columnValues = new char*[numFields]; 
		char** columnNames = new char*[numFields];

		while (pdbTable->ReadNextRecord())
		{
			for (unsigned int index = 0; index < numFields; index++)
			{
				char **pcolumnName = columnNames + index;
				char **pcolumnValue = columnValues + index;
				// todo: use ptr arithmetic to avoid overflow
				*pcolumnName = (char*)pdbTable->GetFieldName(index);
				*pcolumnValue = (char*)pdbTable->GetFieldValue(index);
			}

			callback(pBlob, numFields, (const char**)columnValues, (const char**)columnNames);
		}

		delete columnValues;
		delete columnNames;
		delete pdbTable;
	}
	else
	{
		return TM_CANTOPEN;
	}

	return TM_OK;
}

#if later
int CTMDb::GetTable(
	const char *zSql,     /* SQL to be evaluated */
	char ***pazResult,    /* Results of the query */
	int *pnRow,           /* Number of result rows written here */
	int *pnColumn,        /* Number of result columns written here */
	char **pzErrmsg       /* Error msg written here */
	)
{

	return TM_OK;
}
#endif 
