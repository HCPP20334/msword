#pragma once

/* Compile contract reconstructed from Opus/dlg/username.des. */
#define tmcUserName    (tmcUserMin + 0)
#define tmcUserInitial (tmcUserMin + 1)

typedef struct _CABUSERNAME
{
	CABH cabh;
	WORD sab;
	CHAR **hszName;
	CHAR **hszInitials;
} CABUSERNAME;

typedef CABUSERNAME **HCABUSERNAME;
#define cabiCABUSERNAME Cabi((sizeof(CABUSERNAME) + sizeof(WORD) - 1) / sizeof(WORD), 2)
