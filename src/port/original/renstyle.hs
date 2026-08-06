#pragma once
#define tmcRSRenameStyle (tmcUserMin + 0)
typedef struct _CABRENAMESTYLE
{
	CABH cabh;
	WORD sab;
	CHAR **hszRenameStyle;
} CABRENAMESTYLE;
typedef CABRENAMESTYLE **HCABRENAMESTYLE;
#define cabiCABRENAMESTYLE Cabi((sizeof(CABRENAMESTYLE) + sizeof(WORD) - 1) / sizeof(WORD), 1)
