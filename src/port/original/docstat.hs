#pragma once

/* Compile contract reconstructed from Opus/dlg/docstat.des. */
#define tmcSTFName  (tmcUserMin + 0)
#define tmcSTDir    (tmcUserMin + 1)
#define tmcSTDOT    (tmcUserMin + 2)
#define tmcSTTitle  (tmcUserMin + 3)
#define tmcSTCDate  (tmcUserMin + 4)
#define tmcSTRDate  (tmcUserMin + 5)
#define tmcSTLRevBy (tmcUserMin + 6)
#define tmcSTRevNum (tmcUserMin + 7)
#define tmcSTEdMin  (tmcUserMin + 8)
#define tmcSTLPDate (tmcUserMin + 9)
#define tmcSTCPg    (tmcUserMin + 10)
#define tmcSTCWords (tmcUserMin + 11)
#define tmcSTCch    (tmcUserMin + 12)
#define tmcSTUpdate (tmcUserMin + 13)

typedef struct _CABDOCSTAT
{
	CABH cabh;
	CHAR **hszFName;
	CHAR **hszDir;
	CHAR **hszDOT;
	CHAR **hszTitle;
	CHAR **hszCDate;
	CHAR **hszRDate;
	CHAR **hszLRevBy;
	CHAR **hszRevNum;
	CHAR **hszEdMin;
	CHAR **hszLPDate;
	CHAR **hszCPg;
	CHAR **hszCWords;
	CHAR **hszCch;
} CABDOCSTAT;

typedef CABDOCSTAT **HCABDOCSTAT;
#define cabiCABDOCSTAT Cabi((sizeof(CABDOCSTAT) + sizeof(WORD) - 1) / sizeof(WORD), 13)
