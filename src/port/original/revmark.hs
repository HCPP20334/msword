#pragma once

/* Compile contract generated from Opus/dlg/revmark.des. */
#define tmcRMSearch (tmcUserMin + 0)
#define tmcRMRemove (tmcUserMin + 1)
#define tmcRMUndo   (tmcUserMin + 2)

typedef struct _CABREVMARKING
{
	CABH cabh;
	WORD sab;
	BOOL fRevMarking;
	int iRMBar;
	int iRMProps;
} CABREVMARKING;

typedef CABREVMARKING **HCABREVMARKING;
#define cabiCABREVMARKING Cabi((sizeof(CABREVMARKING) + sizeof(WORD) - 1) / sizeof(WORD), 0)
