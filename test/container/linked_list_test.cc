// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "flare/container/linked_list.h"
#include "testing/gtest_wrap.h"

namespace flare::container {
    namespace {

        class Node : public link_node<Node> {
        public:
            explicit Node(int id) : id_(id) {}

            int id() const { return id_; }

        private:
            int id_;
        };

        class MultipleInheritanceNodeBase {
        public:
            MultipleInheritanceNodeBase() : field_taking_up_space_(0) {}

            int field_taking_up_space_;
        };

        class MultipleInheritanceNode : public MultipleInheritanceNodeBase,
                                        public link_node<MultipleInheritanceNode> {
        public:
            MultipleInheritanceNode() {}
        };

// Checks that when iterating |list| (either from head to tail, or from
// tail to head, as determined by |forward|), we get back |node_ids|,
// which is an array of size |num_nodes|.
        void ExpectListContentsForDirection(const linked_list<Node> &list,
                                            int num_nodes, const int *node_ids, bool forward) {
            int i = 0;
            for (const link_node<Node> *node = (forward ? list.head() : list.tail());
                 node != list.end();
                 node = (forward ? node->next() : node->previous())) {
                ASSERT_LT(i, num_nodes);
                int index_of_id = forward ? i : num_nodes - i - 1;
                EXPECT_EQ(node_ids[index_of_id], node->value()->id());
                ++i;
            }
            EXPECT_EQ(num_nodes, i);
        }

        void ExpectListContents(const linked_list<Node> &list,
                                int num_nodes,
                                const int *node_ids) {
            {
                SCOPED_TRACE("Iterating forward (from head to tail)");
                ExpectListContentsForDirection(list, num_nodes, node_ids, true);
            }
            {
                SCOPED_TRACE("Iterating backward (from tail to head)");
                ExpectListContentsForDirection(list, num_nodes, node_ids, false);
            }
        }

        TEST(linked_list, Empty) {
            linked_list<Node> list;
            EXPECT_EQ(list.end(), list.head());
            EXPECT_EQ(list.end(), list.tail());
            ExpectListContents(list, 0, NULL);
        }

        TEST(linked_list, append) {
            linked_list<Node> list;
            ExpectListContents(list, 0, NULL);

            Node n1(1);
            list.append(&n1);

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n1, list.tail());
            {
                const int expected[] = {1};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }

            Node n2(2);
            list.append(&n2);

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n2, list.tail());
            {
                const int expected[] = {1, 2};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }

            Node n3(3);
            list.append(&n3);

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n3, list.tail());
            {
                const int expected[] = {1, 2, 3};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }
        }

        TEST(linked_list, remove_from_list) {
            linked_list<Node> list;

            Node n1(1);
            Node n2(2);
            Node n3(3);
            Node n4(4);
            Node n5(5);

            list.append(&n1);
            list.append(&n2);
            list.append(&n3);
            list.append(&n4);
            list.append(&n5);

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n5, list.tail());
            {
                const int expected[] = {1, 2, 3, 4, 5};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }

            // Remove from the middle.
            n3.remove_from_list();

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n5, list.tail());
            {
                const int expected[] = {1, 2, 4, 5};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }

            // Remove from the tail.
            n5.remove_from_list();

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n4, list.tail());
            {
                const int expected[] = {1, 2, 4};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }

            // Remove from the head.
            n1.remove_from_list();

            EXPECT_EQ(&n2, list.head());
            EXPECT_EQ(&n4, list.tail());
            {
                const int expected[] = {2, 4};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }

            // Empty the list.
            n2.remove_from_list();
            n4.remove_from_list();

            ExpectListContents(list, 0, NULL);
            EXPECT_EQ(list.end(), list.head());
            EXPECT_EQ(list.end(), list.tail());

            // Fill the list once again.
            list.append(&n1);
            list.append(&n2);
            list.append(&n3);
            list.append(&n4);
            list.append(&n5);

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n5, list.tail());
            {
                const int expected[] = {1, 2, 3, 4, 5};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }
        }

        TEST(linked_list, insert_before) {
            linked_list<Node> list;

            Node n1(1);
            Node n2(2);
            Node n3(3);
            Node n4(4);

            list.append(&n1);
            list.append(&n2);

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n2, list.tail());
            {
                const int expected[] = {1, 2};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }

            n3.insert_before(&n2);

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n2, list.tail());
            {
                const int expected[] = {1, 3, 2};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }

            n4.insert_before(&n1);

            EXPECT_EQ(&n4, list.head());
            EXPECT_EQ(&n2, list.tail());
            {
                const int expected[] = {4, 1, 3, 2};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }
        }

        TEST(linked_list, insert_after) {
            linked_list<Node> list;

            Node n1(1);
            Node n2(2);
            Node n3(3);
            Node n4(4);

            list.append(&n1);
            list.append(&n2);

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n2, list.tail());
            {
                const int expected[] = {1, 2};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }

            n3.insert_after(&n2);

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n3, list.tail());
            {
                const int expected[] = {1, 2, 3};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }

            n4.insert_after(&n1);

            EXPECT_EQ(&n1, list.head());
            EXPECT_EQ(&n3, list.tail());
            {
                const int expected[] = {1, 4, 2, 3};
                ExpectListContents(list, FLARE_ARRAY_SIZE(expected), expected);
            }
        }

        TEST(linked_list, MultipleInheritanceNode) {
            MultipleInheritanceNode node;
            EXPECT_EQ(&node, node.value());
        }

        TEST(linked_list, EmptyListIsEmpty) {
            linked_list<Node> list;
            EXPECT_TRUE(list.empty());
        }

        TEST(linked_list, NonEmptyListIsNotEmpty) {
            linked_list<Node> list;

            Node n(1);
            list.append(&n);

            EXPECT_FALSE(list.empty());
        }

        TEST(linked_list, EmptiedListIsEmptyAgain) {
            linked_list<Node> list;

            Node n(1);
            list.append(&n);
            n.remove_from_list();

            EXPECT_TRUE(list.empty());
        }

        TEST(linked_list, NodesCanBeReused) {
            linked_list<Node> list1;
            linked_list<Node> list2;

            Node n(1);
            list1.append(&n);
            n.remove_from_list();
            list2.append(&n);

            EXPECT_EQ(list2.head()->value(), &n);
        }

        TEST(linked_list, RemovedNodeHasNullNextPrevious) {
            linked_list<Node> list;

            Node n(1);
            list.append(&n);
            n.remove_from_list();

            EXPECT_EQ(&n, n.next());
            EXPECT_EQ(&n, n.previous());
        }

    }  // namespace
}  // namespace flare::container
