#pragma once
#define tmcMSFile (tmcUserMin + 0)
#define tmcMSDot  (tmcUserMin + 1)
#define tmcMSFrom (tmcUserMin + 2)
#define tmcMSTo   (tmcUserMin + 3)
typedef struct _CABMERGESTYLE
{
	CABH cabh;
	WORD sab;
	CHAR **hszMergeStyle;
	int iDirList;
} CABMERGESTYLE;
typedef CABMERGESTYLE **HCABMERGESTYLE;
#define cabiCABMERGESTYLE Cabi((sizeof(CABMERGESTYLE) + sizeof(WORD) - 1) / sizeof(WORD), 1)
