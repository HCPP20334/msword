#pragma once

/* Compile contract reconstructed from Opus/dlg/usrdlg.des. */
typedef struct _CABUSRDLG
{
	CABH cabh;
	WORD sab;
	CHAR **foo;
} CABUSRDLG;

typedef CABUSRDLG **HCABUSRDLG;
#define cabiCABUSRDLG Cabi((sizeof(CABUSRDLG) + sizeof(WORD) - 1) / sizeof(WORD), 1)
