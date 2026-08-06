#pragma once

/* Compile contract generated from Opus/dlg/apprun.des. */
typedef struct _CABAPPRUN
{
	CABH cabh;
	WORD sab;
	int iApp;
} CABAPPRUN;

typedef CABAPPRUN **HCABAPPRUN;
#define cabiCABAPPRUN Cabi((sizeof(CABAPPRUN) + sizeof(WORD) - 1) / sizeof(WORD), 0)
