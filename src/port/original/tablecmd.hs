#pragma once

/* Compile contract reconstructed from Opus/dlg/tablecmd.des. */
#define sabHack           0x0001
#define tmcCmdType        (tmcUserMin + 0)
#define tmcRow            (tmcUserMin + 1)
#define tmcColumn         (tmcUserMin + 2)
#define tmcSelection      (tmcUserMin + 3)
#define tmcShiftText      (tmcUserMin + 4)
#define tmcShiftDirection (tmcUserMin + 5)
#define tmcShiftHorz      (tmcUserMin + 6)
#define tmcShiftVert      (tmcUserMin + 7)
#define tmcTblInsert      (tmcUserMin + 8)
#define tmcTblDelete      (tmcUserMin + 9)
#define tmcMergeCells     (tmcUserMin + 10)
#define tmcSplitCells     (tmcUserMin + 11)

typedef struct _CABEDITTABLE
{
	CABH cabh;
	WORD sab;
	int cmdtype;
	BOOL fShiftVert;
} CABEDITTABLE;

typedef CABEDITTABLE **HCABEDITTABLE;
#define cabiCABEDITTABLE Cabi((sizeof(CABEDITTABLE) + sizeof(WORD) - 1) / sizeof(WORD), 0)
