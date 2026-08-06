#pragma once

/* Compile contract reconstructed from Opus/dlg/thesaur.des. */
#define tmcLookup          (tmcUserMin + 0)
#define tmcSynList         (tmcUserMin + 1)
#define tmcDefList         (tmcUserMin + 2)
#define tmcReplace         (tmcUserMin + 3)
#define tmcSynonym         (tmcUserMin + 4)
#define tmcOriginal        (tmcUserMin + 5)
#define tmcPartOfSpeech    (tmcUserMin + 6)
#define tmcDefinition      (tmcUserMin + 7)

#pragma pack(push, 2)
typedef struct _CABTHESAURUS
{
	CABH cabh;
	WORD sab;
	CHAR **hszLookup;
	int uSynList;
	int uDefList;
} CABTHESAURUS;
#pragma pack(pop)

typedef CABTHESAURUS **HCABTHESAURUS;
#define cabiCABTHESAURUS Cabi((sizeof(CABTHESAURUS) + sizeof(WORD) - 1) / sizeof(WORD), 1)
