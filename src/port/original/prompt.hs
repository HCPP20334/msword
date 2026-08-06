#pragma once

/*
 * Compile-time CAB/control contract reconstructed from Opus/dlg/prompt.des.
 * The checked-in source tree does not contain the historical dialog
 * compiler's generated prompt.hs output.
 */

#define tmcPrompt (tmcUserMin + 0)
#define tmcInput  (tmcUserMin + 1)

typedef struct _CABPROMPT
{
	CABH cabh;
	WORD sab;
	CHAR **hszPrompt;
	CHAR **hszInput;
} CABPROMPT;

typedef CABPROMPT **HCABPROMPT;

#define cabiCABPROMPT Cabi((sizeof(CABPROMPT) + sizeof(WORD) - 1) / sizeof(WORD), 2)
