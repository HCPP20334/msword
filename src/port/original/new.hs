#pragma once

/* Compile contract reconstructed from Opus/dlg/new.des. */
#define tmcSummary     (tmcUserMin + 0)
#define tmcNewDot      (tmcUserMin + 1)
#define tmcRNewDoc     (tmcUserMin + 2)
#define tmcRNewDot     (tmcUserMin + 3)
#define tmcNewType     ((tmcUserMin + 4) | ftmcGrouped)
#define tmcNewTypeList (tmcUserMin + 5)

typedef struct _CABNEWDOC
{
	CABH cabh;
	WORD sab;
	CHAR **hszNewType;
	BOOL fNewDot;
} CABNEWDOC;

typedef CABNEWDOC **HCABNEWDOC;
#define cabiCABNEWDOC Cabi((sizeof(CABNEWDOC) + sizeof(WORD) - 1) / sizeof(WORD), 1)
