/* * Include file for SetJmp / DoJmp */


#ifdef OPUS_X64
#include <setjmp.h>
typedef struct
	{
	/* Retain the fields inspected by the original EL error machinery. */
	char fPcodeEnv;
	char snEnv;
	unsigned bpcEnv;
	int cbEnv;
	int *fEnv;
	jmp_buf nativeEnv;
	} ENV;

static __inline ENV *OpusPrepareEnv(ENV *penv)
	{
	penv->fPcodeEnv = 1;
	penv->snEnv = 0;
	penv->bpcEnv = 0;
	penv->cbEnv = 0;
	penv->fEnv = (int *)penv;
	return penv;
	}
#else
typedef struct
	{
	char fPcodeEnv;
	char snEnv;
	unsigned bpcEnv;
	int cbEnv;
	int *fEnv;
	} ENV;
#endif

/* Use this to indicate an invalid ENV: fPcodeEnv=snEnv=0. */
#define InvalidateEnv(penv)	(*(int *)(penv)=0)

#ifdef OPUS_X64
#define SetJmp(penv) setjmp(OpusPrepareEnv((ENV *)(penv))->nativeEnv)
#define DoJmp(penv,rv) longjmp(((ENV *)(penv))->nativeEnv, \
		((int)(rv) == 0 ? 1 : (int)(rv)))

#elif defined(CC)	/* Cmerge */
void far pascal DoJmp(ENV near *, int);

#else		/* Cs */
sys int WSYS_SETJMP();
#define SetJmp(penv) WSYS_SETJMP(3, (ENV *) penv)
sys void VSYS_SETJMP();
#define DoJmp(penv,rv) VSYS_SETJMP(6, (ENV *) penv, (int) rv)

#endif
