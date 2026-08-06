#pragma once

/* Win3 contract from the checked-in Opus/dlg/ribbon3.des description. */

#ifndef tmcFont
#define tmcFont   (tmcUserMin + 0)
#endif
#ifndef tmcPoints
#define tmcPoints (tmcUserMin + 1)
#endif

typedef struct _CABRIBBON3
{
	CABH cabh;
	WORD sab;
	int ftc;
	int hps;
} CABRIBBON3;

typedef CABRIBBON3 **HCABRIBBON3;

#define cabiCABRIBBON3 Cabi((sizeof(CABRIBBON3) + sizeof(WORD) - 1) / sizeof(WORD), 0)
