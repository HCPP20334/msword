#pragma once

/* Compile contract generated from Opus/dlg/sect.des. */
#define tmcSecColNum          (tmcUserMin + 0)
#define tmcSecColSpacing      (tmcUserMin + 1)
#define tmcSecColLBetween     (tmcUserMin + 2)
#define tmcSecStart           (tmcUserMin + 3)
#define tmcSecFtn             (tmcUserMin + 4)
#define tmcSecLineNum         (tmcUserMin + 5)
#define tmcSecLNStartAt       (tmcUserMin + 6)
#define tmcSecLNFrom          (tmcUserMin + 7)
#define tmcSecLNCountBy       (tmcUserMin + 8)
#define tmcSecLNGroup         (tmcUserMin + 9)
#define tmcSecLNPerPage       (tmcUserMin + 10)
#define tmcSecLNPerSec        (tmcUserMin + 11)
#define tmcSecLNContinue      (tmcUserMin + 12)
#define tmcSecVAlignGroup     (tmcUserMin + 13)
#define tmcSecVAlignRadGroup  (tmcUserMin + 14)
#define tmcSecVAlignTop       (tmcUserMin + 15)
#define tmcSecVAlignCenter    (tmcUserMin + 16)
#define tmcSecVAlignJustify   (tmcUserMin + 17)

typedef struct _CABSECTION
{
	CABH cabh;
	WORD sab;
	int cColumns;
	int dxaColumns;
	BOOL fColLBetween;
	int bkc;
	BOOL fIncFtn;
	BOOL fLnn;
	int lnnMin;
	int dxaLnn;
	int nLnnMod;
	int lnc;
	int vjc;
} CABSECTION;

typedef CABSECTION **HCABSECTION;
#define cabiCABSECTION Cabi((sizeof(CABSECTION) + sizeof(WORD) - 1) / sizeof(WORD), 0)
