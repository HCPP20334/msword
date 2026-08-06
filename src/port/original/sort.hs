#pragma once

/* Compile contract reconstructed from Opus/dlg/sort.des. */
#define tmcSortAsc       (tmcUserMin + 0)
#define tmcSortDes       (tmcUserMin + 1)
#define tmcSortKeyType   (tmcUserMin + 2)
#define tmcSortSep       (tmcUserMin + 3)
#define tmcSortComma     (tmcUserMin + 4)
#define tmcSortTab       (tmcUserMin + 5)
#define tmcSortFieldNum  (tmcUserMin + 6)
#define tmcSortCol       (tmcUserMin + 7)
#define tmcSortCase      (tmcUserMin + 8)

#pragma pack(push, 2)
typedef struct _CABSORT
{
	CABH cabh;
	WORD sab;
	int iSortOrder;
	unsigned skt;
	int iSortSep;
	unsigned uFieldNum;
	BOOL fSortCol;
	BOOL fCase;
} CABSORT;
#pragma pack(pop)

typedef CABSORT **HCABSORT;
#define cabiCABSORT Cabi((sizeof(CABSORT) + sizeof(WORD) - 1) / sizeof(WORD), 0)
