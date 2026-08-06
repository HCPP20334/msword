#pragma once

/* Compile contract reconstructed from Opus/dlg/open.des. */
#define tmcOpenFileName ((tmcUserMin + 0) | ftmcGrouped)
#define tmcOpenFileList (tmcUserMin + 1)
#define tmcOpenFileDir  (tmcUserMin + 2)
#define tmcOpenCatalog  (tmcUserMin + 3)
#define tmcOpenReadOnly (tmcUserMin + 4)

typedef struct _CABOPEN
{
	CABH cabh;
	WORD sab;
	CHAR **hszFile;
	int iDirectory;
	BOOL fReadOnly;
} CABOPEN;

typedef CABOPEN **HCABOPEN;
#define cabiCABOPEN Cabi((sizeof(CABOPEN) + sizeof(WORD) - 1) / sizeof(WORD), 1)
