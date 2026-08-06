#pragma once
#define sabHDROptions 0x0001
#define tmcHdrLB         (tmcUserMin + 0)
#define tmcHdrOption     (tmcUserMin + 1)
#define tmcHdrPgStartAt  (tmcUserMin + 2)
#define tmcHdrPgFormat   (tmcUserMin + 3)
#define tmcHdrFromTop    (tmcUserMin + 4)
#define tmcHdrFromBottom (tmcUserMin + 5)
#define tmcHdrFirstPage  (tmcUserMin + 6)
#define tmcHdrFacingPage (tmcUserMin + 7)
typedef struct _CABHEADER
{
	CABH cabh;
	WORD sab;
	unsigned uHdrLB;
	unsigned uHdrPgStartAt;
	int iHdrPgFormat;
	unsigned uHdrFromTop;
	unsigned uHdrFromBottom;
	BOOL fHdrFirstPage;
	BOOL fHdrFacingPage;
} CABHEADER;
typedef CABHEADER **HCABHEADER;
#define cabiCABHEADER Cabi((sizeof(CABHEADER) + sizeof(WORD) - 1) / sizeof(WORD), 0)
