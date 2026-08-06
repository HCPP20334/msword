#pragma once

/* Compile contract reconstructed from Opus/dlg/indexent.des. */
#define tmcIndexEntry      (tmcUserMin + 0)
#define tmcXeRange         (tmcUserMin + 1)
#define tmcXeBold          (tmcUserMin + 3)
#define tmcXeItalic        (tmcUserMin + 4)

typedef struct _CABINDEXENTRY
{
	CABH cabh;
	WORD sab;
	CHAR **hstIndexEntry;
	CHAR **hstXeRange;
	BOOL fBold;
	BOOL fItalic;
} CABINDEXENTRY;

typedef CABINDEXENTRY **HCABINDEXENTRY;
#define cabiCABINDEXENTRY Cabi((sizeof(CABINDEXENTRY) + sizeof(WORD) - 1) / sizeof(WORD), 2)
