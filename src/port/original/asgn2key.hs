#pragma once

/* Compile contract reconstructed from Opus/dlg/asgn2key.des. */
#define tmcMacroList   (tmcUserMin + 0)
#define tmcKeyList     (tmcUserMin + 1)
#define tmcContextKey  (tmcUserMin + 2)
#define tmcGlobalKey   (tmcUserMin + 3)
#define tmcTemplateKey (tmcUserMin + 4)
#define tmcReset       (tmcUserMin + 5)
#define tmcKey         (tmcUserMin + 6)
#define tmcCurrent     (tmcUserMin + 7)
#define tmcAssignKey   (tmcUserMin + 8)
#define tmcUnassignKey (tmcUserMin + 9)
#define tmcDescKey     (tmcUserMin + 10)

#pragma pack(push, 2)
typedef struct _CABCHANGEKEYS
{
	CABH cabh;
	WORD sab;
	CHAR **hszMacroList;
	unsigned uKeyList;
	int iContext;
} CABCHANGEKEYS;
#pragma pack(pop)

typedef CABCHANGEKEYS **HCABCHANGEKEYS;
#define cabiCABCHANGEKEYS Cabi((sizeof(CABCHANGEKEYS) + sizeof(WORD) - 1) / sizeof(WORD), 1)
