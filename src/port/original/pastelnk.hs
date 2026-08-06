#pragma once
#define tmcLinkTo (tmcUserMin + 0)
typedef struct _CABPASTELINK
{
	CABH cabh;
	WORD sab;
	BOOL fHot;
	CHAR **hszLink;
} CABPASTELINK;
typedef CABPASTELINK **HCABPASTELINK;
#define cabiCABPASTELINK Cabi((sizeof(CABPASTELINK) + sizeof(WORD) - 1) / sizeof(WORD), 1)
