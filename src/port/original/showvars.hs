#pragma once

/* Compile contract generated from Opus/dlg/showvars.des. */
#define tmcVarList (tmcUserMin + 0)
#define tmcSet     (tmcUserMin + 1)

typedef struct _CABSHOWVARS
{
	CABH cabh;
	WORD sab;
	int iVar;
} CABSHOWVARS;

typedef CABSHOWVARS **HCABSHOWVARS;
#define cabiCABSHOWVARS Cabi((sizeof(CABSHOWVARS) + sizeof(WORD) - 1) / sizeof(WORD), 0)
