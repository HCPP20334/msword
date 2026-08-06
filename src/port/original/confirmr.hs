#pragma once

/* Compile contract reconstructed from Opus/dlg/confirmr.des. */
#define tmcReplText    (tmcUserMin + 0)
#define tmcReplYes     (tmcUserMin + 1)
#define tmcReplNo      (tmcUserMin + 2)
#define tmcReplConfirm (tmcUserMin + 3)

typedef struct _CABCONFIRMREPL
{
	CABH cabh;
	WORD sab;
	BOOL fConfirm;
} CABCONFIRMREPL;

typedef CABCONFIRMREPL **HCABCONFIRMREPL;
#define cabiCABCONFIRMREPL Cabi((sizeof(CABCONFIRMREPL) + sizeof(WORD) - 1) / sizeof(WORD), 0)
