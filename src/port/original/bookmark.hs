#pragma once

/* Compile contract reconstructed from Opus/dlg/bookmark.des. */
#define tmcBkmkName   ((tmcUserMin + 0) | ftmcGrouped)
#define tmcDeleteBkmk (tmcUserMin + 2)

typedef struct _CABINSBOOKMARK
{
	CABH cabh;
	CHAR **hszBkmkName;
} CABINSBOOKMARK;

typedef CABINSBOOKMARK **HCABINSBOOKMARK;
#define cabiCABINSBOOKMARK Cabi((sizeof(CABINSBOOKMARK) + sizeof(WORD) - 1) / sizeof(WORD), 1)
