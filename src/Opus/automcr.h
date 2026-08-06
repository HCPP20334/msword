/* automcr.h -- include file for invoking AutoMacros */

#define atmExec		0
#define atmNew		1
#define atmOpen		2
#define atmClose	3
#define atmExit		4

#define atmMax		5


#ifdef MPATMST

#ifdef OPUS_X64
/* Counted-string representation emitted by the historical StringMap pass. */
static csconst char mpatmst [][6] =
	{
	"\004Exec",
	"\003New",
	"\004Open",
	"\005Close",
	"\004Exit"
	};
static csconst char stAuto [] = "\004Auto";
#else
static csconst char mpatmst [][] =
	{
	StKey("Exec", AutoMcrExec),
	StKey("New", AutoMcrNew),
	StKey("Open", AutoMcrOpen),
	StKey("Close", AutoMcrClose),
	StKey("Exit", AutoMcrExit)
	};
static csconst char stAuto [] = StKey("Auto", Auto);
#endif

/* 1 for cch, 5 for "Auto", 5 for "Close" (longest in mpatmst) */
#define ichMaxAtm (1 + 5 + 5)

#endif
