//
// Created by liyinbin on 2022/1/22.
//

#ifndef FLARE_BASE_INTERNAL_CONFIG_H_
#define FLARE_BASE_INTERNAL_CONFIG_H_

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

#endif  // FLARE_BASE_INTERNAL_CONFIG_H_
