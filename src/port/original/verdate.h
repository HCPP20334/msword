#pragma once

/*
 * The historical makefile generated this header from SLMVER.H and
 * resource/vertmpl.txt.  The generated file was not present in the source
 * archive, so keep an equivalent deterministic x64-port build identity.
 */
#ifdef RSH
#define szVersionDef "Research Version 1.1a"
#elif defined(DEMO)
#define szVersionDef "Demonstration Version 1.1a"
#elif defined(DEBUG)
#define szVersionDef "Debug Version 1.1a (2401)"
#elif defined(HYBRID)
#define szVersionDef "Hybrid Version 1.1a (2401)"
#elif defined(SHIP)
#define szVersionDef "Version 1.1a"
#else
#define szVersionDef "Preview Version 1.1a (2401)"
#endif

#define nRevProduct 24
#define nIncrProduct 1
#define nRelProduct 2401
#define szDisableWarningDef "WarnVersion2401"

#define szDateBuiltDef "8/5/2026"
#define nMonthBuilt 8
#define nYearBuilt 2026
#define nDayBuilt 5
#define szVerDateDef "August 5, 2026"
