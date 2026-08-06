#pragma once

/* Compile contract reconstructed from Opus/dlg/insbreak.des. */
#define tmcIPBPage   (tmcUserMin + 0)
#define tmcIPBColumn (tmcUserMin + 1)
#define tmcIPBNext   (tmcUserMin + 2)
#define tmcIPBCont   (tmcUserMin + 3)
#define tmcIPBEven   (tmcUserMin + 4)
#define tmcIPBOdd    (tmcUserMin + 5)

typedef struct _CABINSBREAK
{
	CABH cabh;
	int iType;
} CABINSBREAK;

typedef CABINSBREAK **HCABINSBREAK;
#define cabiCABINSBREAK Cabi((sizeof(CABINSBREAK) + sizeof(WORD) - 1) / sizeof(WORD), 0)
