#pragma once

/* Compile contract generated from Opus/dlg/renmacro.des. */
#ifndef tmcName
#define tmcName      (tmcUserMin + 0)
#endif
#define tmcRenPrompt (tmcUserMin + 1)

typedef struct _CABRENMACRO
{
	CABH cabh;
	WORD sab;
	CHAR **hszName;
} CABRENMACRO;

typedef CABRENMACRO **HCABRENMACRO;
#define cabiCABRENMACRO Cabi((sizeof(CABRENMACRO) + sizeof(WORD) - 1) / sizeof(WORD), 1)
