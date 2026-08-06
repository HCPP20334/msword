#pragma once

/*
 * Compile-time contract emitted from Opus/dlg/ruler.des by Microsoft's
 * historical dialog compiler.  The hszStyle argument is its single CAB
 * handle and tmcStyle is the only named control in the source description.
 */

#ifndef tmcStyle
#define tmcStyle (tmcUserMin + 0)
#endif

typedef struct _CABRULER
{
	CABH cabh;
	CHAR **hszStyle;
} CABRULER;

typedef CABRULER **HCABRULER;

#define cabiCABRULER Cabi((sizeof(CABRULER) + sizeof(WORD) - 1) / sizeof(WORD), 1)
