
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef FLARE_BASE_INTERNAL_CONFIG_H_
#define FLARE_BASE_INTERNAL_CONFIG_H_

#include "flare/base/profile/compiler.h"

#if defined(_MSC_VER)
#if defined(FLARE_BUILD_DLL)
#define FLARE_EXPORT __declspec(dllexport)
#elif defined(FLARE_CONSUME_DLL)
#define FLARE_EXPORT __declspec(dllimport)
#else
#define FLARE_EXPORT
#endif
#else
#define FLARE_EXPORT
#endif  // defined(_MSC_VER)

#if !defined(FLARE_BITSET_WORD_TYPE_DEFAULT)
    #if defined(FLARE_BITSET_WORD_SIZE)         // FLARE_BITSET_WORD_SIZE is deprecated, but we temporarily support the ability for the user to specify it. Use FLARE_BITSET_WORD_SIZE instead.
        #if (FLARE_BITSET_WORD_SIZE == 4)
			#define FLARE_BITSET_WORD_TYPE_DEFAULT uint32_t
			#define FLARE_BITSET_WORD_SIZE_DEFAULT 4
		#else
			#define FLARE_BITSET_WORD_TYPE_DEFAULT uint64_t
			#define FLARE_BITSET_WORD_SIZE_DEFAULT 8
		#endif
    #elif (FLARE_PLATFORM_WORD_SIZE == 16)                     // FLARE_PLATFORM_WORD_SIZE is defined in platform.h.
        #define FLARE_BITSET_WORD_TYPE_DEFAULT uint128_t
        #define FLARE_BITSET_WORD_SIZE_DEFAULT 16
    #elif (FLARE_PLATFORM_WORD_SIZE == 8)
        #define FLARE_BITSET_WORD_TYPE_DEFAULT uint64_t
        #define FLARE_BITSET_WORD_SIZE_DEFAULT 8
    #elif (FLARE_PLATFORM_WORD_SIZE == 4)
        #define FLARE_BITSET_WORD_TYPE_DEFAULT uint32_t
        #define FLARE_BITSET_WORD_SIZE_DEFAULT 4
    #else
        #define FLARE_BITSET_WORD_TYPE_DEFAULT uint16_t
        #define FLARE_BITSET_WORD_SIZE_DEFAULT 2
    #endif
#endif

#endif  // FLARE_BASE_INTERNAL_CONFIG_H_
