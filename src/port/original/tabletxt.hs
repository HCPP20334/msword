#pragma once

/* Compile contract reconstructed from Opus/dlg/tabletxt.des. */
#define tmcConvertTo      (tmcUserMin + 0)
#define tmcParagraphs     (tmcUserMin + 1)
#define tmcCommaDelim     (tmcUserMin + 2)
#define tmcTabDelim       (tmcUserMin + 3)

#pragma pack(push, 2)
typedef struct _CABTABLETOTEXT
{
	CABH cabh;
	WORD sab;
	int tblfmt;
} CABTABLETOTEXT;
#pragma pack(pop)

typedef CABTABLETOTEXT **HCABTABLETOTEXT;
#define cabiCABTABLETOTEXT Cabi((sizeof(CABTABLETOTEXT) + sizeof(WORD) - 1) / sizeof(WORD), 0)
