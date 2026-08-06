#pragma once

/* Compile contract reconstructed from Opus/dlg/index.des. */
#ifndef sabElOnly
#define sabElOnly          0x0001
#endif

#define tmcNestedIndex     (tmcUserMin + 0)
#define tmcRunInIndex      (tmcUserMin + 1)
#define tmcIndexNone       (tmcUserMin + 2)
#define tmcIndexBlank      (tmcUserMin + 3)
#define tmcIndexLetter     (tmcUserMin + 4)

typedef struct _CABINDEX
{
	CABH cabh;
	WORD sab;
	int iIndexType;
	int iHeading;
	BOOL fReplace;
} CABINDEX;

typedef CABINDEX **HCABINDEX;
#define cabiCABINDEX Cabi((sizeof(CABINDEX) + sizeof(WORD) - 1) / sizeof(WORD), 0)
