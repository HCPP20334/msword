/* Standalone x64 build of the original ChUpperLookup routine from res.c.
 * Keeping it in its own archive member lets the low-level translated runtime
 * use the original algorithm without pulling the entire resource module. */

#include "word.h"
#include "inter.h"

extern struct ITR vitr;

EXPORT ChUpperLookup(ch)
int ch;
{
	BOOL fFrnT = vitr.fFrench;
	vitr.fFrench = fTrue;
	ch = ChUpper(ch);
	vitr.fFrench = fFrnT;
	return (ch);
}
