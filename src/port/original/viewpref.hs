#pragma once

/* Compile contract generated from Opus/dlg/viewpref.des. */
#define tmcVPTabs       (tmcUserMin + 0)
#define tmcVPNmAreaSize (tmcUserMin + 1)

typedef struct _CABVIEWPREF
{
	CABH cabh;
	WORD sab;
	BOOL fvisiTabs;
	BOOL fvisiSpaces;
	BOOL fvisiParaMarks;
	BOOL fvisiCondHyp;
	BOOL fSeeHidden;
	BOOL fvisiShowAll;
	BOOL fDispAsPrint;
	BOOL fShowPictures;
	BOOL fTextBound;
	BOOL fHorzScroll;
	BOOL fVertScroll;
	BOOL fTblGridlines;
	unsigned uNmAreaSize;
} CABVIEWPREF;

typedef CABVIEWPREF **HCABVIEWPREF;
#define cabiCABVIEWPREF Cabi((sizeof(CABVIEWPREF) + sizeof(WORD) - 1) / sizeof(WORD), 0)
