#pragma once

/* Compile contract reconstructed from Opus/dlg/hyphen.des. */
#define tmcHypAt          (tmcUserMin + 0)
#define tmcHypText        (tmcUserMin + 1)
#define tmcHypCaps        (tmcUserMin + 2)
#define tmcHypConfirm     (tmcUserMin + 3)
#define tmcHypHotZ        (tmcUserMin + 4)
#define tmcHypChange      (tmcUserMin + 5)
#define tmcHypNoChange    (tmcUserMin + 6)

#pragma pack(push, 2)
typedef struct _CABHYPHEN
{
	CABH cabh;
	WORD sab;
	CHAR **hszHypHotZ;
	BOOL fHypCaps;
	BOOL fHypConfirm;
} CABHYPHEN;
#pragma pack(pop)

typedef CABHYPHEN **HCABHYPHEN;
#define cabiCABHYPHEN Cabi((sizeof(CABHYPHEN) + sizeof(WORD) - 1) / sizeof(WORD), 1)
