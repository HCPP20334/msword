#pragma once

/* Compile contract reconstructed from Opus/dlg/usrdlg.des. */
#pragma pack(push, 2)
typedef struct _CABUSRDLG
{
	CABH cabh;
	CHAR **foo;
} CABUSRDLG;
#pragma pack(pop)

typedef CABUSRDLG **HCABUSRDLG;
#define cabiCABUSRDLG Cabi((sizeof(CABUSRDLG) + sizeof(WORD) - 1) / sizeof(WORD), 1)
