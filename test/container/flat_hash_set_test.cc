
#ifndef THIS_HASH_SET
#define THIS_HASH_SET   flat_hash_set
#define THIS_TEST_NAME  FlatHashSet
#endif

#include "flare/container/flat_hash_set.h"
#include "flare/container/parallel_flat_hash_set.h"
#include "flare/container/parallel_node_hash_set.h"
#include <vector>

#include "hash_generator_testing.h"
#include "unordered_set_constructor_test.h"
#include "unordered_set_lookup_test.h"
#include "unordered_set_members_test.h"
#include "unordered_set_modifiers_test.h"

namespace flare {
    namespace priv {
        namespace {

            using ::flare::priv::hash_internal::Enum;
            using ::flare::priv::hash_internal::EnumClass;
            using ::testing::Pointee;
            using ::testing::UnorderedElementsAre;
            using ::testing::UnorderedElementsAreArray;

            template<class T>
            using Set =
            flare::THIS_HASH_SET<T, StatefulTestingHash, StatefulTestingEqual, Alloc<T>>;

            using SetTypes =
            ::testing::Types<Set<int>, Set<std::string>, Set<Enum>, Set<EnumClass>>;

            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, ConstructorTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, LookupTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, MembersTest, SetTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, ModifiersTest, SetTypes);

            TEST(THIS_TEST_NAME, EmplaceString) {
                std::vector<std::string> v = {"a", "b"};
                flare::THIS_HASH_SET<std::string_view> hs(v.begin(), v.end());
                //EXPECT_THAT(hs, UnorderedElementsAreArray(v));
            }

            TEST(THIS_TEST_NAME, BitfieldArgument) {
                union {
                    int n: 1;
                };
                n = 0;
                flare::THIS_HASH_SET<int> s = {n};
                s.insert(n);
                s.insert(s.end(), n);
                s.insert({n});
                s.erase(n);
                s.count(n);
                s.prefetch(n);
                s.find(n);
                s.contains(n);
                s.equal_range(n);
            }

            TEST(THIS_TEST_NAME, MergeExtractInsert) {
                struct hash {
                    size_t operator()(const std::unique_ptr<int> &p) const { return *p; }
                };
                struct Eq {
                    bool operator()(const std::unique_ptr<int> &a,
                                    const std::unique_ptr<int> &b) const {
                        return *a == *b;
                    }
                };
                flare::THIS_HASH_SET<std::unique_ptr<int>, hash, Eq> set1, set2;
                set1.insert(flare::make_unique<int>(7));
                set1.insert(flare::make_unique<int>(17));

                set2.insert(flare::make_unique<int>(7));
                set2.insert(flare::make_unique<int>(19));

                EXPECT_THAT(set1, UnorderedElementsAre(Pointee(7), Pointee(17)));
                EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7), Pointee(19)));

                set1.merge(set2);

                EXPECT_THAT(set1, UnorderedElementsAre(Pointee(7), Pointee(17), Pointee(19)));
                EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7)));

                auto node = set1.extract(flare::make_unique<int>(7));
                EXPECT_TRUE(node);
                EXPECT_THAT(node.value(), Pointee(7));
                EXPECT_THAT(set1, UnorderedElementsAre(Pointee(17), Pointee(19)));

                auto insert_result = set2.insert(std::move(node));
                EXPECT_FALSE(node);
                EXPECT_FALSE(insert_result.inserted);
                EXPECT_TRUE(insert_result.node);
                EXPECT_THAT(insert_result.node.value(), Pointee(7));
                EXPECT_EQ(**insert_result.position, 7);
                EXPECT_NE(insert_result.position->get(), insert_result.node.value().get());
                EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7)));

                node = set1.extract(flare::make_unique<int>(17));
                EXPECT_TRUE(node);
                EXPECT_THAT(node.value(), Pointee(17));
                EXPECT_THAT(set1, UnorderedElementsAre(Pointee(19)));

                node.value() = flare::make_unique<int>(23);

                insert_result = set2.insert(std::move(node));
                EXPECT_FALSE(node);
                EXPECT_TRUE(insert_result.inserted);
                EXPECT_FALSE(insert_result.node);
                EXPECT_EQ(**insert_result.position, 23);
                EXPECT_THAT(set2, UnorderedElementsAre(Pointee(7), Pointee(23)));
            }

        }  // namespace
    }  // namespace priv
}  // namespace flare
