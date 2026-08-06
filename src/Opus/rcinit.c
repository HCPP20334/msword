/* R C I N I T . C */
/*  CSCONST resources used during initialization
*/

#include "word.h"
DEBUGASSERTSZ            /* WIN - bogus macro for assert string */
#include "heap.h"
#include "resource.h"
#include "debug.h"
#include "error.h"

#ifdef OPUS_X64
extern HANDLE vhInstance;
#endif

#ifdef OPUS_X64
static csconst struct RCDS rgrcds[] =
#else
csconst struct RCDS rgrcds[] =
#endif
{
#ifdef DEBUG
#include "mopus.hi"
#else
#include "mword.hi"
#endif /* DEBUG */

#include "mwhires.hc"
#include "split.hc"
#include "column.hc"
};


/* H  L O A D  O U R   R E S O U R C E */
/* %%Function:HLoadRes0  %%Owner:peterj */
HANDLE HLoadRes0(i)
int i;
{
#ifdef OPUS_X64
	/* BITAPP preserved these resources as the original Win16 RCI header
	 * followed by 32x32 monochrome AND and XOR masks.  A Win64 global-memory
	 * handle is not a cursor/icon handle, so construct the corresponding USER
	 * object directly from Microsoft's checked-in mask data. */
	const BYTE *pb;
	const BYTE *pbAnd;
	const BYTE *pbXor;
	int xHotspot;
	int yHotspot;
	int cx;
	int cy;
	int cbRow;
	int cbMask;

	Assert(i < ircdsNonCoreMin);
	if (i < 0 || i >= ircdsNonCoreMin || rgrcds[i].cb < 12)
		return NULL;

	pb = (const BYTE *)rgrcds[i].rgchBits;
	xHotspot = pb[0] | ((int)pb[1] << 8);
	yHotspot = pb[2] | ((int)pb[3] << 8);
	cx = pb[4] | ((int)pb[5] << 8);
	cy = pb[6] | ((int)pb[7] << 8);
	cbRow = pb[8] | ((int)pb[9] << 8);
	cbMask = cy * cbRow;
	if (cx <= 0 || cy <= 0 || cbRow <= 0 || cbMask > (rgrcds[i].cb - 12) / 2)
		return NULL;

	pbAnd = pb + 12;
	pbXor = pbAnd + cbMask;
	if (i == ircdsWordIcon)
		return (HANDLE)CreateIcon(vhInstance, cx, cy, 1, 1, pbAnd, pbXor);

	return (HANDLE)CreateCursor(vhInstance, xHotspot, yHotspot,
			cx, cy, pbAnd, pbXor);
#else
	HANDLE h;
	LPSTR lpch;

	Assert(i < ircdsNonCoreMin);

	if ((h = OurGlobalAlloc(GMEM_MOVEABLE, (LONG)rgrcds[i].cb)) == NULL)
		return NULL;

	lpch = GlobalLock(h);
	bltbx((LPSTR)rgrcds[i].rgchBits, lpch, rgrcds[i].cb);
	GlobalUnlock(h);
	return h;
#endif /* OPUS_X64 */
}



#ifdef WIN23
/* REVIEW DIB? */
static csconst struct BMDS rgbmds[] =
{
#include "showvis.hb"
};
#else
static csconst struct BMDS rgbmds[] =
{
#include "showvis.hb"
};
#endif /* WIN23 */


/* H  F R O M  I B M D S  0 */
/* %%Function:HFromIbmds0  %%Owner:peterj */
HANDLE HFromIbmds0(i)
int i;
{
	int cb = rgbmds[i].cb;
	HANDLE hbmp = NULL;
	struct BMDS FAR *pbmds;
	BYTE *pb;

#ifdef DCSRC
	CommSzNum(SzShared("HFromIbmds0: loading: "), i);
#endif /* DCSRC */

/* Allocate a local frame variable of size cb */
	ReturnOnNoStack(cb, NULL, fFalse); /* assure there is room */
	pb = OurAllocA(cb);

	pbmds = &rgbmds[i]; /* pointer into CS space! */
	bltbx(pbmds->rgchBits, (LPSTR)pb, cb);

	hbmp = CreateBitmap (pbmds->bm.bmWidth, pbmds->bm.bmHeight,
			pbmds->bm.bmPlanes, pbmds->bm.bmBitsPixel, (LPSTR)pb);
	LogGdiHandle(hbmp, 1037);

	return hbmp;
}


