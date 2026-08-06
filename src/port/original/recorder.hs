#pragma once

/* Compile contract generated from Opus/dlg/recorder.des. */
#define tmcName    (tmcUserMin + 0)
#define tmcContext (tmcUserMin + 2)
#define tmcDocType (tmcUserMin + 4)

typedef struct _CABRECORDER
{
	CABH cabh;
	WORD sab;
	CHAR **pszName;
	CHAR **pszDesc;
	int iContext;
} CABRECORDER;

typedef CABRECORDER **HCABRECORDER;
#define cabiCABRECORDER Cabi((sizeof(CABRECORDER) + sizeof(WORD) - 1) / sizeof(WORD), 2)
