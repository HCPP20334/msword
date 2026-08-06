#pragma once

/* Compile contract reconstructed from Opus/dlg/toc.des. */
#define sabElOnly        0x0001

#define tmcTocOutline   (tmcUserMin + 0)
#define tmcTocFields    (tmcUserMin + 1)
#define tmcOutlineLevels (tmcUserMin + 2)
#define tmcTocAll       (tmcUserMin + 3)
#define tmcTocFromTo    (tmcUserMin + 4)
#define tmcTocFrom      (tmcUserMin + 5)
#define tmcTocTo        (tmcUserMin + 6)

typedef struct _CABTOC
{
	CABH cabh;
	WORD sab;
	int iTocType;
	int iOutlineLevels;
	unsigned wTocFrom;
	unsigned wTocTo;
	BOOL fReplace;
} CABTOC;

typedef CABTOC **HCABTOC;
#define cabiCABTOC Cabi((sizeof(CABTOC) + sizeof(WORD) - 1) / sizeof(WORD), 0)
