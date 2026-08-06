#pragma once

/* Compile contract reconstructed from Opus/dlg/docsum.des. */
#define sabStats     0x0001
#define tmcDSTitle   (tmcUserMin + 0)
#define tmcDSComments (tmcUserMin + 1)
#define tmcDSDir     (tmcUserMin + 2)
#define tmcDSStat    (tmcUserMin + 3)
#define tmcDSUpdate  (tmcUserMin + 4)
#define tmcDSFName   (tmcUserMin + 5)

typedef struct _CABDOCSUM
{
	CABH cabh;
	WORD sab;
	CHAR **hszTitle;
	CHAR **hszSubject;
	CHAR **hszAuthor;
	CHAR **hszKeyWords;
	CHAR **hszComments;
	CHAR **hszDirSum;
	CHAR **hszFName;
	CHAR **hszDir;
	CHAR **hszDOT;
	CHAR **hszTitleStat;
	CHAR **hszCDate;
	CHAR **hszRDate;
	CHAR **hszLRevBy;
	CHAR **hszRevNum;
	CHAR **hszEdMin;
	CHAR **hszLPDate;
	CHAR **hszCPg;
	CHAR **hszCWords;
	CHAR **hszCch;
	CHAR **hszFNameSum;
} CABDOCSUM;

typedef CABDOCSUM **HCABDOCSUM;
#define cabiCABDOCSUM Cabi((sizeof(CABDOCSUM) + sizeof(WORD) - 1) / sizeof(WORD), 20)
