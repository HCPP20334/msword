#include "opus_elx_dispatch.h"

#include <cstring>
#include <type_traits>

namespace
{
struct NativeNum
	{
	union
		{
		unsigned char rgb[8];
		double d;
		} num;
	};

static_assert(sizeof(NativeNum) == 8);

NativeNum ReadNum(const OPUS_ELX_NATIVE_ARG& argument)
	{
	NativeNum value{};
	std::memcpy(&value, argument.num, sizeof(value));
	return value;
	}

template <int Remaining, typename Return, typename... Arguments>
bool InvokeValue(void *pfn, const OPUS_ELX_NATIVE_ARG *arguments,
		int count, Return *result, Arguments... values)
	{
	if (count == 0)
		{
		using Function = Return (*)(Arguments...);
		*result = reinterpret_cast<Function>(pfn)(values...);
		return true;
		}

	if constexpr (Remaining == 0)
		return false;
	else
		{
		switch (arguments->kind)
			{
			case opusElxArgInt:
				return InvokeValue<Remaining - 1>(pfn, arguments + 1,
						count - 1, result, values..., arguments->integer);
			case opusElxArgPointer:
				return InvokeValue<Remaining - 1>(pfn, arguments + 1,
						count - 1, result, values..., arguments->pointer);
			case opusElxArgNum:
				return InvokeValue<Remaining - 1>(pfn, arguments + 1,
						count - 1, result, values..., ReadNum(*arguments));
			default:
				return false;
			}
		}
	}

template <int Remaining, typename... Arguments>
bool InvokeVoid(void *pfn, const OPUS_ELX_NATIVE_ARG *arguments,
		int count, Arguments... values)
	{
	if (count == 0)
		{
		using Function = void (*)(Arguments...);
		reinterpret_cast<Function>(pfn)(values...);
		return true;
		}

	if constexpr (Remaining == 0)
		return false;
	else
		{
		switch (arguments->kind)
			{
			case opusElxArgInt:
				return InvokeVoid<Remaining - 1>(pfn, arguments + 1,
						count - 1, values..., arguments->integer);
			case opusElxArgPointer:
				return InvokeVoid<Remaining - 1>(pfn, arguments + 1,
						count - 1, values..., arguments->pointer);
			case opusElxArgNum:
				return InvokeVoid<Remaining - 1>(pfn, arguments + 1,
						count - 1, values..., ReadNum(*arguments));
			default:
				return false;
			}
		}
	}
}

extern "C" int OpusInvokeElx(void *pfn, int return_kind, int argument_count,
		const OPUS_ELX_NATIVE_ARG *arguments, void *result)
	{
	if (pfn == nullptr || argument_count < 0 || argument_count > 6 ||
			(argument_count != 0 && arguments == nullptr))
		return 0;

	switch (return_kind)
		{
		case 0: /* elrVoid */
			return InvokeVoid<6>(pfn, arguments, argument_count) ? 1 : 0;

		case 1: /* elrNum */
			{
			NativeNum value{};
			if (!InvokeValue<6>(pfn, arguments, argument_count, &value))
				return 0;
			std::memcpy(result, &value, sizeof(value));
			return 1;
			}

		case 2: /* elrInt */
			{
			int value = 0;
			if (!InvokeValue<6>(pfn, arguments, argument_count, &value))
				return 0;
			*static_cast<int *>(result) = value;
			return 1;
			}

		case 3: /* elrSd */
			{
			uintptr_t value = 0;
			if (!InvokeValue<6>(pfn, arguments, argument_count, &value))
				return 0;
			*static_cast<uintptr_t *>(result) = value;
			return 1;
			}

		default:
			return 0;
		}
	}

