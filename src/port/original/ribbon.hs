#pragma once

/*
 * Compile-time contract emitted from Opus/dlg/ribbon.des by Microsoft's
 * historical dialog compiler.  The compiler is not present in this source
 * drop, so retain the original CAB fields and named-control ordering here.
 */

#ifndef tmcFont
#define tmcFont   (tmcUserMin + 0)
#endif
#ifndef tmcPoints
#define tmcPoints (tmcUserMin + 1)
#endif

typedef struct _CABRIBBON
{
	CABH cabh;
	int ftc;
	int hps;
} CABRIBBON;

typedef CABRIBBON **HCABRIBBON;

#define cabiCABRIBBON Cabi((sizeof(CABRIBBON) + sizeof(WORD) - 1) / sizeof(WORD), 0)
