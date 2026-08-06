#pragma once

/* Compile contract reconstructed from Opus/dlg/renum.des. */
#define tmcRPAction      (tmcUserMin + 0)
#define tmcRPAll         (tmcUserMin + 1)
#define tmcRPNumbered    (tmcUserMin + 2)
#define tmcRPRemove      (tmcUserMin + 3)
#define tmcRPMode        (tmcUserMin + 4)
#define tmcRPAutomatic   (tmcUserMin + 5)
#define tmcRPManual      (tmcUserMin + 6)
#define tmcRPStartAt     (tmcUserMin + 7)
#define tmcRPAllLevels   (tmcUserMin + 8)
#define tmcRPFormat      ((tmcUserMin + 9) | ftmcGrouped)

#pragma pack(push, 2)
typedef struct _CABRENUMPARAS
{
	CABH cabh;
	WORD sab;
	CHAR **hszStartAt;
	CHAR **hszFormat;
	int rpp;
	BOOL fManual;
	BOOL fShowAllLev;
} CABRENUMPARAS;
#pragma pack(pop)

typedef CABRENUMPARAS **HCABRENUMPARAS;
#define cabiCABRENUMPARAS Cabi((sizeof(CABRENUMPARAS) + sizeof(WORD) - 1) / sizeof(WORD), 2)
