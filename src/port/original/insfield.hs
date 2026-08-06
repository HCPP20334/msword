#pragma once

/* Compile contract reconstructed from Opus/dlg/insfield.des. */
#define tmcIFldFlt       (tmcUserMin + 0)
#define tmcIFldInst      (tmcUserMin + 1)
#define tmcIFldSyntax    (tmcUserMin + 2)
#define tmcIFldTxt       (tmcUserMin + 3)
#define tmcIFldHelp      (tmcUserMin + 4)
#define tmcIFldAdd       (tmcUserMin + 5)

#pragma pack(push, 2)
typedef struct _CABINSFIELD
{
	CABH cabh;
	WORD sab;
	CHAR **hszFld;
	unsigned uFlt;
	unsigned uInst;
} CABINSFIELD;
#pragma pack(pop)

typedef CABINSFIELD **HCABINSFIELD;
#define cabiCABINSFIELD Cabi((sizeof(CABINSFIELD) + sizeof(WORD) - 1) / sizeof(WORD), 1)
