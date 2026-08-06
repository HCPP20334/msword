#pragma once

/* Compile contract generated from Opus/dlg/cmpfile.des. */
#define tmcCmpFile (tmcUserMin + 0)
#define tmcCmpList (tmcUserMin + 1)
#define tmcCmpDir  (tmcUserMin + 2)

typedef struct _CABCMPFILE
{
	CABH cabh;
	WORD sab;
	CHAR **hszFile;
	int iDirectory;
} CABCMPFILE;

typedef CABCMPFILE **HCABCMPFILE;
#define cabiCABCMPFILE Cabi((sizeof(CABCMPFILE) + sizeof(WORD) - 1) / sizeof(WORD), 1)
