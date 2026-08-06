#pragma once

/* Compile-time CAB/control contract reconstructed from Opus/dlg/runmacro.des.
 * The original dialog compiler is absent from this source archive. */
#define tmcRMCombo   ((tmcUserMin + 2) | ftmcGrouped)
#define tmcRMList    (tmcUserMin + 3)
#define tmcRMShowAll (tmcUserMin + 4)

typedef struct _CABRUNMACRO
{
	CABH cabh;
	CHAR **hszName;
	BOOL fShowAll;
} CABRUNMACRO;

typedef CABRUNMACRO **HCABRUNMACRO;
#define cabiCABRUNMACRO Cabi((sizeof(CABRUNMACRO) + sizeof(WORD) - 1) / sizeof(WORD), 1)
