#pragma once

/*
 * Compile-time contract emitted from Opus/dlg/saveas.des by Microsoft's
 * historical dialog compiler.  The compiler is not present in this source
 * drop, so retain the original CAB fields and named-control ordering here.
 */

#define tmcSavFile       (tmcUserMin + 0)
#define tmcSavOptions    (tmcUserMin + 1)
#define tmcSavDirText    (tmcUserMin + 2)
#define tmcSavDirList    (tmcUserMin + 3)
#define tmcSavDir        (tmcUserMin + 4)
#define tmcSavDfmtPrompt (tmcUserMin + 5)
#define tmcSavIfmt       (tmcUserMin + 6)
#define tmcSavQuick      (tmcUserMin + 7)
#define tmcSavBackup     (tmcUserMin + 8)
#define tmcSavLockAnnot  (tmcUserMin + 9)
#define sabSAOptions     0

typedef struct _CABSAVE
{
	CABH cabh;
	WORD sab;
	CHAR **hszFile;
	int iDirList;
	int iFmt;
	BOOL fQuicksave;
	BOOL fBackup;
	BOOL fLockAnnot;
	BOOL fOptions;
} CABSAVE;

typedef CABSAVE **HCABSAVE;

/* One CAB handle (hszFile); all remaining arguments are simple values. */
#define cabiCABSAVE Cabi((sizeof(CABSAVE) + sizeof(WORD) - 1) / sizeof(WORD), 1)
