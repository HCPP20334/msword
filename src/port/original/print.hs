#pragma once

/* Compile contract generated from Opus/dlg/print.des. */
#define sabPRTOptions 0x0001
#define sabPRTEl      0x0002

#define tmcPrPrinter     (tmcUserMin + 0)
#define tmcPrSrc         (tmcUserMin + 1)
#define tmcPrCopies      (tmcUserMin + 2)
#define tmcPrRange       (tmcUserMin + 3)
#define tmcPrAll         (tmcUserMin + 4)
#define tmcPrSel         (tmcUserMin + 5)
#define tmcPrPages       (tmcUserMin + 6)
#define tmcPrFrom        (tmcUserMin + 7)
#define tmcPrTo          (tmcUserMin + 8)
#define tmcPrOptions     (tmcUserMin + 9)
#define tmcPrReverse     (tmcUserMin + 10)
#define tmcPrDraft       (tmcUserMin + 11)
#define tmcPrRefresh     (tmcUserMin + 12)
#define tmcPrFeedTxt     (tmcUserMin + 13)
#define tmcPrFeed        (tmcUserMin + 14)
#define tmcPrSummary     (tmcUserMin + 15)
#define tmcPrAnnotations (tmcUserMin + 16)
#define tmcPrSeeHidden   (tmcUserMin + 17)
#define tmcPrFields      (tmcUserMin + 18)

typedef struct _CABPRINT
{
	CABH cabh;
	WORD sab;
	CHAR **hszPrPrinter;
	CHAR **hszPrFrom;
	CHAR **hszPrTo;
	CHAR **hszFileName;
	int istPrSrc;
	int cCopies;
	int prng;
	BOOL fReverse;
	BOOL fDraft;
	BOOL fRefresh;
	int istPrFeed;
	BOOL fSummary;
	BOOL fAnnotations;
	BOOL fSeeHidden;
	BOOL fShowInst;
} CABPRINT;

typedef CABPRINT **HCABPRINT;
#define cabiCABPRINT Cabi((sizeof(CABPRINT) + sizeof(WORD) - 1) / sizeof(WORD), 4)
