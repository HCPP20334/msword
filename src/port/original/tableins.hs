#pragma once

/* Compile contract reconstructed from Opus/dlg/tableins.des. */
#define tmcColumns        (tmcUserMin + 0)
#define tmcRows           (tmcUserMin + 1)
#define tmcInitColWidth   (tmcUserMin + 2)
#define tmcConvertFrom    (tmcUserMin + 3)
#define tmcFormatPara     (tmcUserMin + 4)
#define tmcFormatTabs     (tmcUserMin + 5)
#define tmcFormatComma    (tmcUserMin + 6)
#define tmcTunnelFormat   (tmcUserMin + 7)

typedef struct _CABINSERTTABLE
{
	CABH cabh;
	WORD sab;
	int cColumns;
	int cRows;
	int dxaWidth;
	int tblfmt;
} CABINSERTTABLE;

typedef CABINSERTTABLE **HCABINSERTTABLE;
#define cabiCABINSERTTABLE Cabi((sizeof(CABINSERTTABLE) + sizeof(WORD) - 1) / sizeof(WORD), 0)
