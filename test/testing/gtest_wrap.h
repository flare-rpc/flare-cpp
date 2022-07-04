//
// Created by liyinbin on 2022/5/24.
//

#ifndef FLARE_GTEST_WRAP_H
#define FLARE_GTEST_WRAP_H

#ifdef private
#undef private
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#define private public
#else
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#endif

#endif //FLARE_GTEST_WRAP_H
