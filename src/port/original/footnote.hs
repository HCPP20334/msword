#pragma once

/* Compile contract generated from Opus/dlg/footnote.des. */
#define tmcFAuto           (tmcUserMin + 0)
#define tmcRefMark         (tmcUserMin + 1)
#define tmcSeparator       (tmcUserMin + 2)
#define tmcContSeparator   (tmcUserMin + 3)
#define tmcNoticeSeparator (tmcUserMin + 4)

typedef struct _CABINSERTFTN
{
	CABH cabh;
	WORD sab;
	BOOL fAuto;
	CHAR **hstRef;
} CABINSERTFTN;

typedef CABINSERTFTN **HCABINSERTFTN;
#define cabiCABINSERTFTN Cabi((sizeof(CABINSERTFTN) + sizeof(WORD) - 1) / sizeof(WORD), 1)
