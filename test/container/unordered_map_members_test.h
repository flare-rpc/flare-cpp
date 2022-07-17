

#ifndef UNORDERED_MAP_MEMBERS_TEST_H_
#define UNORDERED_MAP_MEMBERS_TEST_H_

#include <type_traits>

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif


#include "testing/gtest_wrap.h"
#include "hash_generator_testing.h"
#include "hash_policy_testing.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace flare {
    namespace priv {

        template<class UnordMap>
        class MembersTest : public ::testing::Test {
        };

        TYPED_TEST_SUITE_P(MembersTest);

        template<typename T>
        void UseType() {}

        TYPED_TEST_P(MembersTest, Typedefs) {
            EXPECT_TRUE((std::is_same<std::pair<const typename TypeParam::key_type,
                    typename TypeParam::mapped_type>,
                    typename TypeParam::value_type>()));
            EXPECT_TRUE((flare::conjunction<
                    flare::negation<std::is_signed<typename TypeParam::size_type>>,
                    std::is_integral<typename TypeParam::size_type>>()));
            EXPECT_TRUE((flare::conjunction<
                    std::is_signed<typename TypeParam::difference_type>,
                    std::is_integral<typename TypeParam::difference_type>>()));
            EXPECT_TRUE((std::is_convertible<
                    decltype(std::declval<const typename TypeParam::hasher &>()(
                            std::declval<const typename TypeParam::key_type &>())),
                    size_t>()));
            EXPECT_TRUE((std::is_convertible<
                    decltype(std::declval<const typename TypeParam::key_equal &>()(
                            std::declval<const typename TypeParam::key_type &>(),
                            std::declval<const typename TypeParam::key_type &>())),
                    bool>()));
            EXPECT_TRUE((std::is_same<typename TypeParam::allocator_type::value_type,
                    typename TypeParam::value_type>()));
            EXPECT_TRUE((std::is_same<typename TypeParam::value_type &,
                    typename TypeParam::reference>()));
            EXPECT_TRUE((std::is_same<const typename TypeParam::value_type &,
                    typename TypeParam::const_reference>()));
            EXPECT_TRUE((std::is_same<typename std::allocator_traits<
                    typename TypeParam::allocator_type>::pointer,
                    typename TypeParam::pointer>()));
            EXPECT_TRUE(
                    (std::is_same<typename std::allocator_traits<
                            typename TypeParam::allocator_type>::const_pointer,
                            typename TypeParam::const_pointer>()));
        }

        TYPED_TEST_P(MembersTest, SimpleFunctions) {
            EXPECT_GT(TypeParam().max_size(), 0ul);
        }

        TYPED_TEST_P(MembersTest, BeginEnd) {
            TypeParam t = {typename TypeParam::value_type{}};
            EXPECT_EQ(t.begin(), t.cbegin());
            EXPECT_EQ(t.end(), t.cend());
            EXPECT_NE(t.begin(), t.end());
            EXPECT_NE(t.cbegin(), t.cend());
        }

        REGISTER_TYPED_TEST_SUITE_P(MembersTest, Typedefs, SimpleFunctions, BeginEnd);

    }  // namespace priv
}  // namespace flare

#endif  // UNORDERED_MAP_MEMBERS_TEST_H_
