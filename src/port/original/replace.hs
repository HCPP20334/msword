#pragma once

/* Compile contract reconstructed from Opus/dlg/replace.des. */
#define sabRFmt    0x0001
#define tmcRSearch (tmcUserMin + 4)
#define tmcRFmt    (tmcUserMin + 5)

typedef struct _CABREPLACE
{
	CABH cabh;
	WORD sab;
	CHAR **hszSearch;
	CHAR **hszReplace;
	BOOL fWholeWord;
	BOOL fMatchUpLow;
	BOOL fConfirm;
	BOOL fFormatted;
} CABREPLACE;

typedef CABREPLACE **HCABREPLACE;
#define cabiCABREPLACE Cabi((sizeof(CABREPLACE) + sizeof(WORD) - 1) / sizeof(WORD), 2)
