#pragma once

/* Compile contract reconstructed from Opus/dlg/insfile.des. */
#define tmcIFName ((tmcUserMin + 0) | ftmcGrouped)
#define tmcIFList (tmcUserMin + 1)
#define tmcIFDir  (tmcUserMin + 2)
#define tmcIFLink (tmcUserMin + 4)

typedef struct _CABINSFILE
{
	CABH cabh;
	CHAR **hszFile;
	int iDirectory;
	CHAR **hszRange;
	BOOL fLink;
} CABINSFILE;

typedef CABINSFILE **HCABINSFILE;
#define cabiCABINSFILE Cabi((sizeof(CABINSFILE) + sizeof(WORD) - 1) / sizeof(WORD), 2)
