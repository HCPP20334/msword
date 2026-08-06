#pragma once

/* Compile contract generated from Opus/dlg/para.des. */
#define tmcParLeft      (tmcUserMin + 0)
#define tmcParCenter    (tmcUserMin + 1)
#define tmcParRight     (tmcUserMin + 2)
#define tmcParJustified (tmcUserMin + 3)
#define tmcParLeftIn    (tmcUserMin + 4)
#define tmcParRightIn   (tmcUserMin + 5)
#define tmcParFirstIn   (tmcUserMin + 6)
#define tmcParSpaceBf   (tmcUserMin + 7)
#define tmcParSpaceAf   (tmcUserMin + 8)
#define tmcParIntLSp    (tmcUserMin + 9)
#define tmcParStyle     (tmcUserMin + 10)
#define tmcParKeepLT    (tmcUserMin + 11)
#define tmcParKeepNP    (tmcUserMin + 12)
#define tmcParBrcp      (tmcUserMin + 13)
#define tmcParBrcl      (tmcUserMin + 14)
#define tmcParNewPg     (tmcUserMin + 15)
#define tmcParNoLNum    (tmcUserMin + 16)
#define tmcParTabs      (tmcUserMin + 17)

typedef struct _CABPARALOOKS
{
	CABH cabh;
	WORD sab;
	int iJustify;
	int wParLeftIn;
	int wParRightIn;
	int wParFirstIn;
	int wParDyaBefore;
	int wParDyaAfter;
	int wParDyaLine;
	CHAR **hszStyle;
	BOOL fParKeepLT;
	BOOL fParKeepNP;
	int iBrcp;
	int iBrcl;
	BOOL fParNewPg;
	BOOL fParNoLNum;
} CABPARALOOKS;

typedef CABPARALOOKS **HCABPARALOOKS;
#define cabiCABPARALOOKS Cabi((sizeof(CABPARALOOKS) + sizeof(WORD) - 1) / sizeof(WORD), 1)
