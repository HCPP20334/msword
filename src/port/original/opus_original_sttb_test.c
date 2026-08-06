#include "word.h"
DEBUGASSERTSZ
#include "heap.h"
#include "doc.h"
#include "file.h"

#include <string.h>

int vfInCommit = 0;
CHAR stEmpty[] = {0};
struct DOD** mpdochdod[docMax];
struct FCB** mpfnhfcb[fnMax];
struct WWD** mpwwhwwd[wwMax];
struct MWD** mpmwhmwd[mwMax];

struct STTB** HsttbInit(int cwEstimate, int fExternal);
int IbstAddStToSttb(struct STTB** hsttb, char* pst);
int FChangeStInSttb(struct STTB** hsttb, int ibst, char* pst);
int DeleteIFromSttb(struct STTB** hsttb, int ibst);
void FreeHsttb(struct STTB** hsttb);
CHAR* HpstFromSttb(struct STTB** hsttb, int ibst);
int GetStFromSttb(struct STTB** hsttb, int ibst, CHAR* st);
int IbstFindSt(struct STTB** hsttb, const CHAR* st);

static int FSameSt(const CHAR* first, const CHAR* second)
{
	return first[0] == second[0] &&
		memcmp(first + 1, second + 1, first[0]) == 0;
}

static int FExerciseSttb(int fExternal)
{
	CHAR stAlpha[] = {5, 'a', 'l', 'p', 'h', 'a'};
	CHAR stBeta[] = {4, 'b', 'e', 't', 'a'};
	CHAR stGamma[] = {5, 'g', 'a', 'm', 'm', 'a'};
	CHAR stCopy[16];
	struct STTB** hsttb = HsttbInit(2, fExternal);
	int result = 0;

	if (hsttb == 0)
		return 1;
	if (IbstAddStToSttb(hsttb, stAlpha) != 0 ||
		IbstAddStToSttb(hsttb, stBeta) != 1)
		result = 2;
	else if ((**hsttb).ibstMac != 2 ||
		!FSameSt(HpstFromSttb(hsttb, 0), stAlpha) ||
		!FSameSt(HpstFromSttb(hsttb, 1), stBeta))
		result = 3;
	else if (IbstFindSt(hsttb, stAlpha) != 0 ||
		IbstFindSt(hsttb, stGamma) != -1)
		result = 4;
	else if (!FChangeStInSttb(hsttb, 1, stGamma) ||
		!FSameSt(HpstFromSttb(hsttb, 1), stGamma))
		result = 5;
	else
		{
		memset(stCopy, 0, sizeof(stCopy));
		GetStFromSttb(hsttb, 1, stCopy);
		if (!FSameSt(stCopy, stGamma))
			result = 6;
		}
	if (result == 0 && (DeleteIFromSttb(hsttb, 0),
		((**hsttb).ibstMac != 1 ||
		 !FSameSt(HpstFromSttb(hsttb, 0), stGamma))))
		result = 7;
	FreeHsttb(hsttb);
	return result;
}

int main(void)
{
	int result = FExerciseSttb(fFalse);
	if (result != 0)
		return result;
	result = FExerciseSttb(fTrue);
	return result == 0 ? 0 : result + 10;
}
