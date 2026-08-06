#pragma once

/* Compile contract generated from Opus/dlg/glsy.des. */
#define tmcGlossary    (tmcUserMin + 0)
#define tmcGlsyContext (tmcUserMin + 1)
#define tmcGlobal      (tmcUserMin + 2)
#define tmcTemplate    (tmcUserMin + 3)
#define tmcInsert      (tmcUserMin + 4)
#define tmcDefine      (tmcUserMin + 5)
#define tmcUndefine    (tmcUserMin + 6)
#define tmcGlsyName    (tmcUserMin + 7)
#define tmcGlsyText    (tmcUserMin + 8)

typedef struct _CABGLOSSARY
{
	CABH cabh;
	WORD sab;
	CHAR **hszGlossary;
	BOOL fLocal;
} CABGLOSSARY;

typedef CABGLOSSARY **HCABGLOSSARY;
#define cabiCABGLOSSARY Cabi((sizeof(CABGLOSSARY) + sizeof(WORD) - 1) / sizeof(WORD), 1)
