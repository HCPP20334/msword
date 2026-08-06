#pragma once

/* Compile contract generated from Opus/dlg/spell.des. */
#define sabSPLOptions   0x0001
#define tmcSplStart     (tmcUserMin + 0)
#define tmcSplOptions   (tmcUserMin + 1)
#define tmcSplWord      (tmcUserMin + 2)
#define tmcSplCheck     (tmcUserMin + 3)
#define tmcSplRemove    (tmcUserMin + 4)
#define tmcSplMDictBox  (tmcUserMin + 5)
#define tmcSplADictBox  ((tmcUserMin + 6) | ftmcGrouped)
#define tmcSplIgnoreCaps (tmcUserMin + 8)
#define tmcSplAutoSugg  (tmcUserMin + 9)

typedef struct _CABSPELLER
{
	CABH cabh;
	WORD sab;
	CHAR **hszSplWord;
	CHAR **hszSplMDictBox;
	CHAR **hszSplADictBox;
	BOOL fSplIgnoreCaps;
	BOOL fSplAutoSugg;
} CABSPELLER;

typedef CABSPELLER **HCABSPELLER;
#define cabiCABSPELLER Cabi((sizeof(CABSPELLER) + sizeof(WORD) - 1) / sizeof(WORD), 3)
