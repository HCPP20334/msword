#pragma once

/* Compile-time contract reconstructed from Opus/dlg/edstyle.des. */
#define tmcDSStyle    ((tmcUserMin + 0) | ftmcGrouped)
#define tmcDSChars    (tmcUserMin + 2)
#define tmcDSParas    (tmcUserMin + 3)
#define tmcDSTabs     (tmcUserMin + 4)
#define tmcDSPosition (tmcUserMin + 5)
#define tmcDSOptions  (tmcUserMin + 6)
#define tmcDSBanter   (tmcUserMin + 7)
#define tmcDSBasedOn  ((tmcUserMin + 8) | ftmcGrouped)
#define tmcDSNext     ((tmcUserMin + 10) | ftmcGrouped)
#define tmcDSTemplate (tmcUserMin + 12)
#define tmcDSDefine   (tmcUserMin + 13)
#define tmcDSDelete   (tmcUserMin + 14)
#define tmcDSRename   (tmcUserMin + 15)
#define tmcDSMerge    (tmcUserMin + 16)

#define sabDSRename  0x0001
#define sabDSOptions 0x0002

typedef struct _CABDEFINESTYLE
{
	CABH cabh;
	WORD sab;
	CHAR **hszDSStyle;
	CHAR **hszDSBasedOn;
	CHAR **hszDSNext;
	CHAR **hszDSNewName;
	CHAR **hszMergeStyle;
	BOOL fTemplate;
	unsigned uSource;
} CABDEFINESTYLE;

typedef CABDEFINESTYLE **HCABDEFINESTYLE;

#define cabiCABDEFINESTYLE Cabi((sizeof(CABDEFINESTYLE) + sizeof(WORD) - 1) / sizeof(WORD), 5)
