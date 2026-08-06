#pragma once

/* Compile contract reconstructed from Opus/dlg/catprog.des. */
#define tmcCatSrhMText     (tmcUserMin + 0)
#define tmcCatSrhMatched   (tmcUserMin + 1)
#define tmcCatSrhSearched  (tmcUserMin + 2)
#define tmcCatSrhFileInUse (tmcUserMin + 3)

#pragma pack(push, 2)
typedef struct _CABCATSRHPROG
{
	CABH cabh;
	WORD sab;
} CABCATSRHPROG;
#pragma pack(pop)

typedef CABCATSRHPROG **HCABCATSRHPROG;
#define cabiCABCATSRHPROG Cabi((sizeof(CABCATSRHPROG) + sizeof(WORD) - 1) / sizeof(WORD), 0)
