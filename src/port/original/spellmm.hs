#pragma once

/* Compile contract reconstructed from Opus/dlg/spellmm.des. */
#define tmcSplMMErr        (tmcUserMin + 0)
#define tmcSplMMWord       (tmcUserMin + 1)
#define tmcSplMMChangeTo   (tmcUserMin + 2)
#define tmcSplMMSuggList   (tmcUserMin + 3)
#define tmcSplMMIgnore     (tmcUserMin + 4)
#define tmcSplMMSugg       (tmcUserMin + 5)
#define tmcSplMMChange     (tmcUserMin + 6)
#define tmcSplMMAdd        (tmcUserMin + 7)
#define tmcSplMMAddToBox   (tmcUserMin + 8)
#define tmcSplMMIgnoreCaps (tmcUserMin + 9)
#define tmcSplMMAutoSugg   (tmcUserMin + 10)

typedef struct _CABSPELLERMM
{
	CABH cabh;
	WORD sab;
	CHAR **hszSplMMChangeTo;
	unsigned uSplMMSuggList;
	int iSplMMAddToBox;
	BOOL fSplMMIgnoreCaps;
	BOOL fSplMMAutoSugg;
} CABSPELLERMM;

typedef CABSPELLERMM **HCABSPELLERMM;
#define cabiCABSPELLERMM Cabi((sizeof(CABSPELLERMM) + sizeof(WORD) - 1) / sizeof(WORD), 1)
