//
// Created by jeff.li.
//
#include "flare/memory/resident.h"
#include "gtest/gtest.h"

namespace flare::memory {

    struct C {
        C() { ++instances; }

        ~C() { --instances; }

        inline static std::size_t instances{};
    };

    struct D {
        void foo() {
            [[maybe_unused]] static resident_singleton<D> test_compilation2;
        }
    };

    resident<int> test_compilation2;

    TEST(resident, All) {
        ASSERT_EQ(0, C::instances);
        {
            C c1;
            ASSERT_EQ(1, C::instances);
            [[maybe_unused]] resident<C> c2;
            ASSERT_EQ(2, C::instances);
        }
// Not 0, as `resident<C>` is not destroyed.
        ASSERT_EQ(1, C::instances);
    }

}  // namespace flare
