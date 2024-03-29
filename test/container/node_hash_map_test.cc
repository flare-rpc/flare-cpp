
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef THIS_HASH_MAP
#define THIS_HASH_MAP   node_hash_map
#define THIS_TEST_NAME  NodeHashMap
#endif

#include "flare/container/node_hash_map.h"
#include "flare/container/parallel_node_hash_map.h"
#include "tracked.h"
#include "unordered_map_constructor_test.h"
#include "unordered_map_lookup_test.h"
#include "unordered_map_members_test.h"
#include "unordered_map_modifiers_test.h"

namespace flare {
    namespace priv {
        namespace {

            using ::testing::Field;
            using ::testing::Pair;
            using ::testing::UnorderedElementsAre;

            using MapTypes = ::testing::Types<
                    flare::THIS_HASH_MAP<int, int, StatefulTestingHash, StatefulTestingEqual,
                            Alloc<std::pair<const int, int>>>,
                    flare::THIS_HASH_MAP<std::string, std::string, StatefulTestingHash,
                            StatefulTestingEqual,
                            Alloc<std::pair<const std::string, std::string>>>>;

            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, ConstructorTest, MapTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, LookupTest, MapTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, MembersTest, MapTypes);
            INSTANTIATE_TYPED_TEST_SUITE_P(THIS_TEST_NAME, ModifiersTest, MapTypes);

            using M = flare::THIS_HASH_MAP<std::string, Tracked<int>>;

            TEST(THIS_TEST_NAME, Emplace) {
                M m;
                Tracked<int> t(53);
                m.emplace("a", t);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(1, t.num_copies());

                m.emplace(std::string("a"), t);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(1, t.num_copies());

                std::string a("a");
                m.emplace(a, t);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(1, t.num_copies());

                const std::string ca("a");
                m.emplace(a, t);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(1, t.num_copies());

                m.emplace(std::make_pair("a", t));
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(2, t.num_copies());

                m.emplace(std::make_pair(std::string("a"), t));
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(3, t.num_copies());

                std::pair<std::string, Tracked<int>> p("a", t);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(4, t.num_copies());
                m.emplace(p);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(4, t.num_copies());

                const std::pair<std::string, Tracked<int>> cp("a", t);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(5, t.num_copies());
                m.emplace(cp);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(5, t.num_copies());

                std::pair<const std::string, Tracked<int>> pc("a", t);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(6, t.num_copies());
                m.emplace(pc);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(6, t.num_copies());

                const std::pair<const std::string, Tracked<int>> cpc("a", t);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(7, t.num_copies());
                m.emplace(cpc);
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(7, t.num_copies());

                m.emplace(std::piecewise_construct, std::forward_as_tuple("a"),
                          std::forward_as_tuple(t));
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(7, t.num_copies());

                m.emplace(std::piecewise_construct, std::forward_as_tuple(std::string("a")),
                          std::forward_as_tuple(t));
                ASSERT_EQ(0, t.num_moves());
                ASSERT_EQ(7, t.num_copies());
            }

            TEST(THIS_TEST_NAME, AssignRecursive) {
                struct Tree {
                    // Verify that unordered_map<K, IncompleteType> can be instantiated.
                    flare::THIS_HASH_MAP<int, Tree> children;
                };
                Tree root;
                const Tree &child = root.children.emplace().first->second;
                // Verify that `lhs = rhs` doesn't read rhs after clearing lhs.
                root = child;
            }

            TEST(FlatHashMap, MoveOnlyKey) {
                struct Key {
                    Key() = default;

                    Key(Key &&) = default;

                    Key &operator=(Key &&) = default;
                };
                struct Eq {
                    bool operator()(const Key &, const Key &) const { return true; }
                };
                struct hash {
                    size_t operator()(const Key &) const { return 0; }
                };
                flare::THIS_HASH_MAP<Key, int, hash, Eq> m;
                m[Key()];
            }

            struct NonMovableKey {
                explicit NonMovableKey(int i_) : i(i_) {}

                NonMovableKey(NonMovableKey &&) = delete;

                int i;
            };

            struct NonMovableKeyHash {
                using is_transparent = void;

                size_t operator()(const NonMovableKey &k) const { return k.i; }

                size_t operator()(int k) const { return k; }
            };

            struct NonMovableKeyEq {
                using is_transparent = void;

                bool operator()(const NonMovableKey &a, const NonMovableKey &b) const {
                    return a.i == b.i;
                }

                bool operator()(const NonMovableKey &a, int b) const { return a.i == b; }
            };

            TEST(THIS_TEST_NAME, MergeExtractInsert) {
                flare::THIS_HASH_MAP<NonMovableKey, int, NonMovableKeyHash, NonMovableKeyEq>
                        set1, set2;
                set1.emplace(std::piecewise_construct, std::make_tuple(7),
                             std::make_tuple(-7));
                set1.emplace(std::piecewise_construct, std::make_tuple(17),
                             std::make_tuple(-17));

                set2.emplace(std::piecewise_construct, std::make_tuple(7),
                             std::make_tuple(-70));
                set2.emplace(std::piecewise_construct, std::make_tuple(19),
                             std::make_tuple(-190));

                auto Elem = [](int key, int value) {
                    return Pair(Field(&NonMovableKey::i, key), value);
                };

                EXPECT_THAT(set1, UnorderedElementsAre(Elem(7, -7), Elem(17, -17)));
                EXPECT_THAT(set2, UnorderedElementsAre(Elem(7, -70), Elem(19, -190)));

                // NonMovableKey is neither copyable nor movable. We should still be able to
                // move nodes around.
                static_assert(!std::is_move_constructible<NonMovableKey>::value, "");
                set1.merge(set2);

                EXPECT_THAT(set1,
                            UnorderedElementsAre(Elem(7, -7), Elem(17, -17), Elem(19, -190)));
                EXPECT_THAT(set2, UnorderedElementsAre(Elem(7, -70)));

                auto node = set1.extract(7);
                EXPECT_TRUE(node);
                EXPECT_EQ(node.key().i, 7);
                EXPECT_EQ(node.mapped(), -7);
                EXPECT_THAT(set1, UnorderedElementsAre(Elem(17, -17), Elem(19, -190)));

                auto insert_result = set2.insert(std::move(node));
                EXPECT_FALSE(node);
                EXPECT_FALSE(insert_result.inserted);
                EXPECT_TRUE(insert_result.node);
                EXPECT_EQ(insert_result.node.key().i, 7);
                EXPECT_EQ(insert_result.node.mapped(), -7);
                EXPECT_THAT(*insert_result.position, Elem(7, -70));
                EXPECT_THAT(set2, UnorderedElementsAre(Elem(7, -70)));

                node = set1.extract(17);
                EXPECT_TRUE(node);
                EXPECT_EQ(node.key().i, 17);
                EXPECT_EQ(node.mapped(), -17);
                EXPECT_THAT(set1, UnorderedElementsAre(Elem(19, -190)));

                node.mapped() = 23;

                insert_result = set2.insert(std::move(node));
                EXPECT_FALSE(node);
                EXPECT_TRUE(insert_result.inserted);
                EXPECT_FALSE(insert_result.node);
                EXPECT_THAT(*insert_result.position, Elem(17, 23));
                EXPECT_THAT(set2, UnorderedElementsAre(Elem(7, -70), Elem(17, 23)));
            }

        }  // namespace
    }  // namespace priv
}  // namespace flare
