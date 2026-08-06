#pragma once

/* Win3 contract from the checked-in Opus/dlg/ruler3.des description. */

#ifndef tmcStyle
#define tmcStyle (tmcUserMin + 0)
#endif

typedef struct _CABRULER3
{
	CABH cabh;
	WORD sab;
	CHAR **hszStyle;
} CABRULER3;

typedef CABRULER3 **HCABRULER3;

#define cabiCABRULER3 Cabi((sizeof(CABRULER3) + sizeof(WORD) - 1) / sizeof(WORD), 1)
