#pragma once

/* Compile-time CAB/control contract reconstructed from Opus/dlg/search.des. */
#define tmcSearch       (tmcUserMin + 0)
#define tmcSearchBanter (tmcUserMin + 1)
#define tmcWholeWord    (tmcUserMin + 2)
#define tmcSFmt         (tmcUserMin + 3)
#define sabSFmt         0

typedef struct _CABSEARCH
{
	CABH cabh;
	WORD sab;
	CHAR **hszSearch;
	BOOL fWholeWord;
	BOOL fMatchUpLow;
	int iDirection;
	BOOL fFormatted;
} CABSEARCH;

typedef CABSEARCH **HCABSEARCH;
#define cabiCABSEARCH Cabi((sizeof(CABSEARCH) + sizeof(WORD) - 1) / sizeof(WORD), 1)
