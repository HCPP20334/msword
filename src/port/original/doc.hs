#pragma once

/* Compile contract reconstructed from Opus/dlg/doc.des. */
#define tmcDocPageWidth     (tmcUserMin + 0)
#define tmcDocPageHeight    (tmcUserMin + 1)
#define tmcDocDefTabs       (tmcUserMin + 2)
#define tmcDocTMargin       (tmcUserMin + 3)
#define tmcDocBMargin       (tmcUserMin + 4)
#define tmcDocLMText        (tmcUserMin + 5)
#define tmcDocLMargin       (tmcUserMin + 6)
#define tmcDocRMText        (tmcUserMin + 7)
#define tmcDocRMargin       (tmcUserMin + 8)
#define tmcDocGutter        (tmcUserMin + 9)
#define tmcDocMirror        (tmcUserMin + 10)
#define tmcDocFNPrintAt     (tmcUserMin + 11)
#define tmcDocFNStartAt     (tmcUserMin + 12)
#define tmcDocFNRestartEach (tmcUserMin + 13)
#define tmcDocTemplate      ((tmcUserMin + 14) | ftmcGrouped)
#define tmcDocTypeList      (tmcUserMin + 15)
#define tmcDocWidow         (tmcUserMin + 16)
#define tmcDocSetDef        (tmcUserMin + 17)

typedef struct _CABDOCUMENT
{
	CABH cabh;
	unsigned uDocPageWidth;
	unsigned uDocPageHeight;
	unsigned uDocDefTabs;
	unsigned uDocTMargin;
	unsigned uDocBMargin;
	unsigned uDocLMargin;
	unsigned uDocRMargin;
	unsigned uDocGutter;
	BOOL fMirrorMargins;
	int iFN;
	int iFNStartAt;
	BOOL fFNRestart;
	CHAR **hszTemplate;
	BOOL fWidowControl;
} CABDOCUMENT;

typedef CABDOCUMENT **HCABDOCUMENT;
#define cabiCABDOCUMENT Cabi((sizeof(CABDOCUMENT) + sizeof(WORD) - 1) / sizeof(WORD), 1)
