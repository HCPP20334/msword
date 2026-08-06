#pragma once

/* Compile contract reconstructed from Opus/dlg/asgn2mnu.des. */
#define tmcMnuMacroList (tmcUserMin + 0)
#define tmcMenuList     (tmcUserMin + 1)
#define tmcTitle        ((tmcUserMin + 2) | ftmcGrouped)
#define tmcTitleList    (tmcUserMin + 3)
#define tmcMnuContext   (tmcUserMin + 4)
#define tmcGlobalMenu   (tmcUserMin + 5)
#define tmcTemplateMenu (tmcUserMin + 6)
#define tmcAssignMenu   (tmcUserMin + 7)
#define tmcUnassignMenu (tmcUserMin + 8)
#define tmcResetMenu    (tmcUserMin + 9)
#define tmcDescMenu     (tmcUserMin + 10)

#pragma pack(push, 2)
typedef struct _CABASSIGNTOMENU
{
	CABH cabh;
	WORD sab;
	CHAR **hszMacroList;
	CHAR **hszMenuList;
	CHAR **hszTitle;
	int iContext;
} CABASSIGNTOMENU;
#pragma pack(pop)

typedef CABASSIGNTOMENU **HCABASSIGNTOMENU;
#define cabiCABASSIGNTOMENU Cabi((sizeof(CABASSIGNTOMENU) + sizeof(WORD) - 1) / sizeof(WORD), 3)
