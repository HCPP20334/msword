#pragma once

/* Compile contract generated from Opus/dlg/style.des. */
#define sabFSCreate 0x0001
#define tmcASStyle  ((tmcUserMin + 0) | ftmcGrouped)
#define tmcASDefine (tmcUserMin + 2)
#define tmcASBanter (tmcUserMin + 3)

typedef struct _CABAPPLYSTYLE
{
	CABH cabh;
	WORD sab;
	CHAR **hszASStyle;
	BOOL fCreate;
} CABAPPLYSTYLE;

typedef CABAPPLYSTYLE **HCABAPPLYSTYLE;
#define cabiCABAPPLYSTYLE Cabi((sizeof(CABAPPLYSTYLE) + sizeof(WORD) - 1) / sizeof(WORD), 1)
