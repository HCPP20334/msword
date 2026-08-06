#pragma once

/* Compile contract generated from Opus/dlg/cust.des. */
#define sabElOnly       0x0001
#define tmcCustAS       (tmcUserMin + 0)
#define tmcCustASHigh   (tmcUserMin + 1)
#define tmcCustASMedium (tmcUserMin + 2)
#define tmcCustASLow    (tmcUserMin + 3)
#define tmcCustASNever  (tmcUserMin + 4)
#define tmcCustUt       (tmcUserMin + 5)
#define tmcCustUtInch   (tmcUserMin + 6)
#define tmcCustUtCm     (tmcUserMin + 7)
#define tmcCustUtPoint  (tmcUserMin + 8)
#define tmcCustUtPicas  (tmcUserMin + 9)
#define tmcCustBkPag    (tmcUserMin + 10)
#define tmcCustSI       (tmcUserMin + 11)
#define tmcCustAutoDel  (tmcUserMin + 12)
#define tmcCustName     (tmcUserMin + 13)
#define tmcCustInitials (tmcUserMin + 14)

typedef struct _CABCUSTOMIZE
{
	CABH cabh;
	WORD sab;
	int iAS;
	int iUnit;
	BOOL fBkgrndPag;
	BOOL fPromptSI;
	BOOL fAutoDelete;
	CHAR **hszName;
	CHAR **hszInitials;
	int cBtnFldClicks;
} CABCUSTOMIZE;

typedef CABCUSTOMIZE **HCABCUSTOMIZE;
#define cabiCABCUSTOMIZE Cabi((sizeof(CABCUSTOMIZE) + sizeof(WORD) - 1) / sizeof(WORD), 2)
