#pragma once

/* Compile-time CAB contract from the checked-in Opus/dlg/about.des file. */
typedef struct _CABABOUT
{
	CABH cabh;
	WORD sab;
	CHAR **hszAboutApp;
	CHAR **hszAboutVersion;
	CHAR **hszAboutCopyright;
	CHAR **hszAboutMem;
	CHAR **hszAboutEmem;
	CHAR **hszAboutMath;
	CHAR **hszAboutDisk;
} CABABOUT;

typedef CABABOUT **HCABOUT;

#define cabiCABABOUT Cabi((sizeof(CABABOUT) + sizeof(WORD) - 1) / sizeof(WORD), 7)
