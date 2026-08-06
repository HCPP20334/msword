#pragma once

/* Compile contract reconstructed from Opus/dlg/newopen.des. */
typedef struct _CABNEWOPEN
{
	CABH cabh;
	CHAR **hszFileName;
} CABNEWOPEN;

typedef CABNEWOPEN **HCABNEWOPEN;
#define cabiCABNEWOPEN Cabi((sizeof(CABNEWOPEN) + sizeof(WORD) - 1) / sizeof(WORD), 1)
