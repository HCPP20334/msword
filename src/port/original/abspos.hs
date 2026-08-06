#pragma once

/* Compile contract generated from Opus/dlg/abspos.des. */
#define tmcDxaHorz     ((tmcUserMin + 0) | ftmcGrouped)
#define tmcHorzList    (tmcUserMin + 1)
#define tmcPcHorz      (tmcUserMin + 2)
#define tmcPcHorzM     (tmcUserMin + 3)
#define tmcPcHorzP     (tmcUserMin + 4)
#define tmcPcHorzC     (tmcUserMin + 5)
#define tmcDyaVert     ((tmcUserMin + 6) | ftmcGrouped)
#define tmcVertList    (tmcUserMin + 7)
#define tmcPcVert      (tmcUserMin + 8)
#define tmcDistText    (tmcUserMin + 11)
#define tmcPosFromText (tmcUserMin + 12)
#define tmcPosWidth    (tmcUserMin + 13)
#define tmcPosReset    (tmcUserMin + 14)
#define tmcPosPreview  (tmcUserMin + 15)

typedef struct _CABABSPOS
{
	CABH cabh;
	WORD sab;
	int dxaHorz;
	int pcHorz;
	int dyaVert;
	int pcVert;
	int dxaFromText;
	int dxaWidth;
} CABABSPOS;

typedef CABABSPOS **HCABABSPOS;
#define cabiCABABSPOS Cabi((sizeof(CABABSPOS) + sizeof(WORD) - 1) / sizeof(WORD), 0)
