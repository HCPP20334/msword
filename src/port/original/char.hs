#pragma once

/* Compile contract generated from Opus/dlg/char.des. */
#define tmcCharName         ((tmcUserMin + 0) | ftmcGrouped)
#define tmcCharSize         ((tmcUserMin + 2) | ftmcGrouped)
#define tmcCharBold         (tmcUserMin + 5)
#define tmcCharItalic       (tmcUserMin + 6)
#define tmcCharSmCaps       (tmcUserMin + 7)
#define tmcCharHidden       (tmcUserMin + 8)
#define tmcCharULine        (tmcUserMin + 9)
#define tmcCharWordUL       (tmcUserMin + 10)
#define tmcCharDUL          (tmcUserMin + 11)
#define tmcCharPos          (tmcUserMin + 12)
#define tmcCharPosNormal    (tmcUserMin + 13)
#define tmcCharPosSuper     (tmcUserMin + 14)
#define tmcCharPosSub       (tmcUserMin + 15)
#define tmcCharHpsPos       (tmcUserMin + 16)
#define tmcCharIS           (tmcUserMin + 17)
#define tmcCharISNormal     (tmcUserMin + 18)
#define tmcCharISExpanded   (tmcUserMin + 19)
#define tmcCharISCompressed (tmcUserMin + 20)
#define tmcCharQpsSpacing   (tmcUserMin + 21)

typedef struct _CABCHARACTER
{
	CABH cabh;
	WORD sab;
	int ftc;
	int hps;
	int color;
	BOOL fCharBold;
	BOOL fCharItalic;
	BOOL fCharSmCaps;
	BOOL fCharHidden;
	BOOL fCharULine;
	BOOL fCharWordUL;
	BOOL fCharDUL;
	int iCharPos;
	int wCharHpsPos;
	int iCharIS;
	int wCharQpsSpacing;
} CABCHARACTER;

typedef CABCHARACTER **HCABCHARACTER;
#define cabiCABCHARACTER Cabi((sizeof(CABCHARACTER) + sizeof(WORD) - 1) / sizeof(WORD), 0)
