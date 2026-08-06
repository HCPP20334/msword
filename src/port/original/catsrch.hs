#pragma once

/* Compile contract reconstructed from Opus/dlg/catsrch.des. */
#define tmcCatSrhList       (tmcUserMin + 0)
#define tmcCatSrhTitle      (tmcUserMin + 1)
#define tmcCatSrhSubject    (tmcUserMin + 2)
#define tmcCatSrhAuthor     (tmcUserMin + 3)
#define tmcCatSrhKeyword    (tmcUserMin + 4)
#define tmcCatSrhRevisor    (tmcUserMin + 5)
#define tmcCatSrhText       (tmcUserMin + 6)
#define tmcCatSrhCreateFrom (tmcUserMin + 7)
#define tmcCatSrhCreateTo   (tmcUserMin + 8)
#define tmcCatSrhReviseFrom (tmcUserMin + 9)
#define tmcCatSrhReviseTo   (tmcUserMin + 10)
#define tmcCatSrhMatchCase  (tmcUserMin + 11)
#define tmcCatSrhRedo       (tmcUserMin + 12)

#pragma pack(push, 2)
typedef struct _CABCATSEARCH
{
	CABH cabh;
	WORD sab;
	CHAR **hszCatSrhList;
	CHAR **hszCatSrhTitle;
	CHAR **hszCatSrhSubject;
	CHAR **hszCatSrhAuthor;
	CHAR **hszCatSrhKeyword;
	CHAR **hszCatSrhRevisor;
	CHAR **hszCatSrhText;
	long hCatSrhDttmCreateFrom;
	long hCatSrhDttmCreateTo;
	long hCatSrhDttmReviseFrom;
	long hCatSrhDttmReviseTo;
	BOOL fCatSrhMatchCase;
	BOOL fCatSrhRedo;
} CABCATSEARCH;
#pragma pack(pop)

typedef CABCATSEARCH **HCABCATSEARCH;
#define cabiCABCATSEARCH Cabi((sizeof(CABCATSEARCH) + sizeof(WORD) - 1) / sizeof(WORD), 7)
