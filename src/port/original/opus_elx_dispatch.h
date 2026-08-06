#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum OPUS_ELX_NATIVE_ARG_KIND
	{
	opusElxArgInt = 1,
	opusElxArgPointer = 2,
	opusElxArgNum = 3
	};

typedef struct OPUS_ELX_NATIVE_ARG
	{
	int kind;
	union
		{
		int integer;
		void *pointer;
		unsigned char num[8];
		};
	} OPUS_ELX_NATIVE_ARG;

/* Invoke an original EL entry using the native AMD64 calling convention.
 * return_kind uses the original elrVoid/elrNum/elrInt/elrSd values. */
int OpusInvokeElx(void *pfn, int return_kind, int argument_count,
		const OPUS_ELX_NATIVE_ARG *arguments, void *result);

#ifdef __cplusplus
}
#endif

