
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_CONTAINER_SINGLE_THREADED_POOL_H_
#define FLARE_CONTAINER_SINGLE_THREADED_POOL_H_

#include <cstdlib>   // malloc & free

namespace flare::container {

    // A single-threaded pool for very efficient allocations of same-sized items.
    // Example:
    //   SingleThreadedPool<16, 512> pool;
    //   void* mem = pool.get();
    //   pool.back(mem);

    template<size_t ITEM_SIZE_IN,   // size of an item
            size_t BLOCK_SIZE_IN,  // suggested size of a block
            size_t MIN_NITEM = 1>  // minimum number of items in one block
    class SingleThreadedPool {
    public:
        // Note: this is a union. The next pointer is set iff when spaces is free,
        // ok to be overlapped.
        union Node {
            Node *next;
            char spaces[ITEM_SIZE_IN];
        };
        struct Block {
            static const size_t INUSE_SIZE =
                    BLOCK_SIZE_IN - sizeof(void *) - sizeof(size_t);
            static const size_t NITEM = (sizeof(Node) <= INUSE_SIZE ?
                                         (INUSE_SIZE / sizeof(Node)) : MIN_NITEM);
            size_t nalloc;
            Block *next;
            Node nodes[NITEM];
        };
        static const size_t BLOCK_SIZE = sizeof(Block);
        static const size_t NITEM = Block::NITEM;
        static const size_t ITEM_SIZE = ITEM_SIZE_IN;

        SingleThreadedPool() : _free_nodes(nullptr), _blocks(nullptr) {}

        ~SingleThreadedPool() { reset(); }

        void swap(SingleThreadedPool &other) {
            std::swap(_free_nodes, other._free_nodes);
            std::swap(_blocks, other._blocks);
        }

        // Get space of an item. The space is as long as ITEM_SIZE.
        // Returns nullptr on out of memory
        void *get() {
            if (_free_nodes) {
                void *spaces = _free_nodes->spaces;
                _free_nodes = _free_nodes->next;
                return spaces;
            }
            if (_blocks == nullptr || _blocks->nalloc >= Block::NITEM) {
                Block *new_block = (Block *) malloc(sizeof(Block));
                if (new_block == nullptr) {
                    return nullptr;
                }
                new_block->nalloc = 0;
                new_block->next = _blocks;
                _blocks = new_block;
            }
            return _blocks->nodes[_blocks->nalloc++].spaces;
        }

        // Return a space allocated by get() before.
        // Do nothing for nullptr.
        void back(void *p) {
            if (nullptr != p) {
                Node *node = (Node *) ((char *) p - offsetof(Node, spaces));
                node->next = _free_nodes;
                _free_nodes = node;
            }
        }

        // Remove all allocated spaces. Spaces that are not back()-ed yet become
        // invalid as well.
        void reset() {
            _free_nodes = nullptr;
            while (_blocks) {
                Block *next = _blocks->next;
                free(_blocks);
                _blocks = next;
            }
        }

        // Count number of allocated/free/actively-used items.
        // Notice that these functions walk through all free nodes or blocks and
        // are not O(1).
        size_t count_allocated() const {
            size_t n = 0;
            for (Block *p = _blocks; p; p = p->next) {
                n += p->nalloc;
            }
            return n;
        }

        size_t count_free() const {
            size_t n = 0;
            for (Node *p = _free_nodes; p; p = p->next, ++n) {}
            return n;
        }

        size_t count_active() const {
            return count_allocated() - count_free();
        }

    private:
        // You should not copy a pool.
        SingleThreadedPool(const SingleThreadedPool &);

        void operator=(const SingleThreadedPool &);

        Node *_free_nodes;
        Block *_blocks;
    };

}  // namespace flare::container

#endif  // FLARE_CONTAINER_SINGLE_THREADED_POOL_H_
