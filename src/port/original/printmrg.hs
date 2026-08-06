#pragma once

/* Compile contract reconstructed from Opus/dlg/printmrg.des. */
#define tmcPMMergeRec     (tmcUserMin + 0)
#define tmcPMAll          (tmcUserMin + 1)
#define tmcPMRange        (tmcUserMin + 2)
#define tmcPMFrom         (tmcUserMin + 3)
#define tmcPMTo           (tmcUserMin + 4)
#define tmcPMPrint        (tmcUserMin + 5)
#define tmcPMNewDoc       (tmcUserMin + 6)

#pragma pack(push, 2)
typedef struct _CABPRINTMERGE
{
	CABH cabh;
	WORD sab;
	int iMergeRec;
	int iRecFrom;
	int iRecTo;
} CABPRINTMERGE;
#pragma pack(pop)

typedef CABPRINTMERGE **HCABPRINTMERGE;
#define cabiCABPRINTMERGE Cabi((sizeof(CABPRINTMERGE) + sizeof(WORD) - 1) / sizeof(WORD), 0)
