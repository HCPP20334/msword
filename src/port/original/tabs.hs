#pragma once
#define tmcTabsPos    (tmcUserMin + 0)
#define tmcAlignment  (tmcUserMin + 1)
#define tmcTabsLeft   (tmcUserMin + 2)
#define tmcTabsCenter (tmcUserMin + 3)
#define tmcTabsRight  (tmcUserMin + 4)
#define tmcTabsDecimal (tmcUserMin + 5)
#define tmcLeader     (tmcUserMin + 6)
#define tmcTabsNoFill (tmcUserMin + 7)
#define tmcTabsDot    (tmcUserMin + 8)
#define tmcTabsMinus  (tmcUserMin + 9)
#define tmcTabsUScore (tmcUserMin + 10)
#define tmcTabsClList (tmcUserMin + 11)
#define tmcTabsSet    (tmcUserMin + 12)
#define tmcTabsClear  (tmcUserMin + 13)
#define tmcTabsReset  (tmcUserMin + 14)
typedef struct _CABTABS
{
	CABH cabh;
	WORD sab;
	CHAR **hszTabsPos;
	int iAlignment;
	int iLeader;
} CABTABS;
typedef CABTABS **HCABTABS;
#define cabiCABTABS Cabi((sizeof(CABTABS) + sizeof(WORD) - 1) / sizeof(WORD), 1)
