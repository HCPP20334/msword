#pragma once

/* Compile contract reconstructed from Opus/dlg/tablefmt.des. */
#define sabFmtTblEl       0x0001
#define tmcFTElFromCol    (tmcUserMin + 0)
#define tmcFTElCol        (tmcUserMin + 1)
#define tmcWidthText      (tmcUserMin + 2)
#define tmcColumnWidth    (tmcUserMin + 3)
#define tmcGapSpace       (tmcUserMin + 4)
#define tmcIndent         (tmcUserMin + 5)
#define tmcRowHeight      (tmcUserMin + 6)
#define tmcOutline        (tmcUserMin + 7)
#define tmcTop            (tmcUserMin + 8)
#define tmcBottom         (tmcUserMin + 9)
#define tmcInside         (tmcUserMin + 10)
#define tmcLeft           (tmcUserMin + 11)
#define tmcRight          (tmcUserMin + 12)
#define tmcTFAlign        (tmcUserMin + 13)
#define tmcAlignLeft      (tmcUserMin + 14)
#define tmcAlignCenter    (tmcUserMin + 15)
#define tmcAlignRIght     (tmcUserMin + 16)
#define tmcApplyTo        (tmcUserMin + 17)
#define tmcApplySel       (tmcUserMin + 18)
#define tmcWholeTable     (tmcUserMin + 19)
#define tmcNextColumn     (tmcUserMin + 20)
#define tmcPrevColumn     (tmcUserMin + 21)

#pragma pack(push, 2)
typedef struct _CABFORMATTABLE
{
	CABH cabh;
	WORD sab;
	CHAR **hszColumns;
	int wFromCol;
	int wCol;
	int dxaWidth;
	int dxaGap;
	int dxaLeft;
	int dyaRowHeight;
	int ibrclOutline;
	int ibrclTop;
	int ibrclBottom;
	int ibrclInside;
	int ibrclLeft;
	int ibrclRight;
	int jc;
	BOOL fWholeTable;
} CABFORMATTABLE;
#pragma pack(pop)

typedef CABFORMATTABLE **HCABFORMATTABLE;
#define cabiCABFORMATTABLE Cabi((sizeof(CABFORMATTABLE) + sizeof(WORD) - 1) / sizeof(WORD), 1)
