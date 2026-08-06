#pragma once

/* Compile contract generated from Opus/dlg/edmacro.des. */
#define sabRename      0x0001
#define tmcEdMacroList ((tmcUserMin + 2) | ftmcGrouped)
#define tmcEMContext   (tmcUserMin + 4)
#define tmcEMGlobal    (tmcUserMin + 5)
#define tmcEMDocType   (tmcUserMin + 6)
#define tmcEMShowAll   (tmcUserMin + 7)
#define tmcDescHdr     (tmcUserMin + 8)
#define tmcDelete      (tmcUserMin + 9)
#define tmcRename      (tmcUserMin + 10)
#define tmcNewName     (tmcUserMin + 11)

typedef struct _CABEDMACRO
{
	CABH cabh;
	WORD sab;
	CHAR **hszName;
	int iContext;
	BOOL fShowAll;
	CHAR **hszDesc;
	CHAR **hszNewName;
} CABEDMACRO;

typedef CABEDMACRO **HCABEDMACRO;
#define cabiCABEDMACRO Cabi((sizeof(CABEDMACRO) + sizeof(WORD) - 1) / sizeof(WORD), 3)
