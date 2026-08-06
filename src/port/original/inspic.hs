#pragma once

/* Compile contract reconstructed from Opus/dlg/inspic.des. */
#define tmcInsPic  ((tmcUserMin + 0) | ftmcGrouped)
#define tmcIPList  (tmcUserMin + 1)
#define tmcIPDir   (tmcUserMin + 2)
#define tmcNewPic  (tmcUserMin + 3)

#pragma pack(push, 2)
typedef struct _CABINSPIC
{
	CABH cabh;
	CHAR **hszFile;
	int iDirectory;
} CABINSPIC;
#pragma pack(pop)

typedef CABINSPIC **HCABINSPIC;
#define cabiCABINSPIC Cabi((sizeof(CABINSPIC) + sizeof(WORD) - 1) / sizeof(WORD), 1)
