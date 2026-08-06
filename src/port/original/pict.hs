#pragma once

/* CAB and named-control contract transcribed from Opus/dlg/pict.des. */
#define tmcPicBrcl    (tmcUserMin + 0)
#define tmcScaleMy    (tmcUserMin + 1)
#define tmcScaleMx    (tmcUserMin + 2)
#define tmcCropTop    (tmcUserMin + 3)
#define tmcCropLeft   (tmcUserMin + 4)
#define tmcCropBottom (tmcUserMin + 5)
#define tmcCropRight  (tmcUserMin + 6)

#pragma pack(push, 2)
typedef struct _CABFORMATPIC
{
	CABH cabh;
	CHAR **hszOrigSize;
	int iPicBrcl;
	int wScaleMy;
	int wScaleMx;
	int wCropTop;
	int wCropLeft;
	int wCropBottom;
	int wCropRight;
} CABFORMATPIC;
#pragma pack(pop)

typedef CABFORMATPIC **HCABFORMATPIC;

#define cabiCABFORMATPIC \
	Cabi((sizeof(CABFORMATPIC) + sizeof(WORD) - 1) / sizeof(WORD), 1)
