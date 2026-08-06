#pragma once

/* Compile-time CAB contract from Opus/dlg/password.des. */
#define tmcPswd (tmcUserMin + 0)

typedef struct _CABFILEPSWD
{
	CABH cabh;
	CHAR **hszPswd;
} CABFILEPSWD;

typedef CABFILEPSWD **HCABFILEPSWD;

#define cabiCABFILEPSWD Cabi((sizeof(CABFILEPSWD) + sizeof(WORD) - 1) / sizeof(WORD), 1)
