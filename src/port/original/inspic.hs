#pragma once

/* Compile contract reconstructed from Opus/dlg/inspic.des. */
#define tmcInsPic  ((tmcUserMin + 0) | ftmcGrouped)
#define tmcIPList  (tmcUserMin + 1)
#define tmcIPDir   (tmcUserMin + 2)
#define tmcNewPic  (tmcUserMin + 3)

typedef struct _CABINSPIC
{
	CABH cabh;
	WORD sab;
	CHAR **hszFile;
	int iDirectory;
} CABINSPIC;

typedef CABINSPIC **HCABINSPIC;
#define cabiCABINSPIC Cabi((sizeof(CABINSPIC) + sizeof(WORD) - 1) / sizeof(WORD), 1)
