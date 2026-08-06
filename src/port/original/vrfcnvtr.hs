#pragma once

/*
 * Compile-time CAB contract emitted from Opus/dlg/vrfcnvtr.des by the
 * historical dialog compiler.  Keep the original single iFmt value here
 * until that compiler/template generator is ported.
 */
typedef struct _CABEXCR
{
	CABH cabh;
	WORD sab;
	int iFmt;
} CABEXCR;

typedef CABEXCR **HCABEXCR;

#define cabiCABEXCR Cabi((sizeof(CABEXCR) + sizeof(WORD) - 1) / sizeof(WORD), 0)
