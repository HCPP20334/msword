#pragma once

/* Compile contract reconstructed from Opus/dlg/chgpr.des. */
#define tmcChgPrLb    (tmcUserMin + 0)
#define tmcChgPrSetup (tmcUserMin + 1)

typedef struct _CABCHGPR
{
	CABH cabh;
	CHAR **hszLb;
} CABCHGPR;

typedef CABCHGPR **HCABCHGPR;
#define cabiCABCHGPR Cabi((sizeof(CABCHGPR) + sizeof(WORD) - 1) / sizeof(WORD), 1)
