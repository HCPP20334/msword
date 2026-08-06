#pragma once
#define tmcIPNType   (tmcUserMin + 0)
#define tmcIPNTop    (tmcUserMin + 1)
#define tmcIPNBottom (tmcUserMin + 2)
#define tmcIPNPos    (tmcUserMin + 3)
#define tmcIPNLeft   (tmcUserMin + 4)
#define tmcIPNCenter (tmcUserMin + 5)
#define tmcIPNRight  (tmcUserMin + 6)
typedef struct _CABINSPGNUM
{
	CABH cabh;
	WORD sab;
	int iType;
	int iPos;
} CABINSPGNUM;
typedef CABINSPGNUM **HCABINSPGNUM;
#define cabiCABINSPGNUM Cabi((sizeof(CABINSPGNUM) + sizeof(WORD) - 1) / sizeof(WORD), 0)
