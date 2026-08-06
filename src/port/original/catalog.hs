#pragma once

/* Compile contract reconstructed from Opus/dlg/catalog.des. */
#define sabCatSrch   0x0001

#define tmcCatSortBy  ((tmcUserMin + 0) | ftmcGrouped)
#define tmcCatSort    (tmcUserMin + 1)
#define tmcCatLB      (tmcUserMin + 2)
#define tmcCatBanter  (tmcUserMin + 3)
#define tmcCatOpen    (tmcUserMin + 4)
#define tmcCatSearch  (tmcUserMin + 5)
#define tmcCatPrint   (tmcUserMin + 6)
#define tmcCatDelete  (tmcUserMin + 7)
#define tmcCatSummary (tmcUserMin + 8)

typedef struct _CABCATALOG
{
	CABH cabh;
	WORD sab;
	int **hrgCatLBIndex;
	CHAR **hszCatSrhList;
	CHAR **hszCatSrhTitle;
	CHAR **hszCatSrhSubject;
	CHAR **hszCatSrhAuthor;
	CHAR **hszCatSrhKeyword;
	CHAR **hszCatSrhRevisor;
	CHAR **hszCatSrhText;
	int iCatSortBy;
	long hCatSrhDttmCreateFrom;
	long hCatSrhDttmCreateTo;
	long hCatSrhDttmReviseFrom;
	long hCatSrhDttmReviseTo;
	BOOL fCatSrhMatchCase;
	BOOL fCatSrhRedo;
} CABCATALOG;

typedef CABCATALOG **HCABCATALOG;
#define cabiCABCATALOG Cabi((sizeof(CABCATALOG) + sizeof(WORD) - 1) / sizeof(WORD), 8)
