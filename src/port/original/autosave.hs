#pragma once

/* Compile contract generated from Opus/dlg/autosave.des. */
#define tmcASGroup    (tmcUserMin + 0)
#define tmcASYes      (tmcUserMin + 1)
#define tmcASPostpone (tmcUserMin + 2)
#define tmcASDMinute  (tmcUserMin + 3)

typedef struct _CABAUTOSAVE
{
	CABH cabh;
	WORD sab;
	BOOL fPostpone;
	int dMinutePostpone;
} CABAUTOSAVE;

typedef CABAUTOSAVE **HCABAUTOSAVE;
#define cabiCABAUTOSAVE Cabi((sizeof(CABAUTOSAVE) + sizeof(WORD) - 1) / sizeof(WORD), 0)
