#pragma once

/* Compile contract reconstructed from Opus/dlg/goto.des. */
#define tmcGoto ((tmcUserMin + 0) | ftmcGrouped)

typedef struct _CABGOTO
{
	CABH cabh;
	WORD sab;
	CHAR **hszGoto;
} CABGOTO;

typedef CABGOTO **HCABGOTO;
#define cabiCABGOTO Cabi((sizeof(CABGOTO) + sizeof(WORD) - 1) / sizeof(WORD), 1)
