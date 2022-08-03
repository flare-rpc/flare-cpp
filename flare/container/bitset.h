
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_CONTAINER_BITSET_H_
#define FLARE_CONTAINER_BITSET_H_

#include <cstddef>
#include <cstring>
#include "flare/base/profile.h"
#include "flare/base/math.h"
#include "flare/log/logging.h"

namespace flare {

    /// BITSET_WORD_COUNT
    ///
    /// Defines the number of words we use, based on the number of bits.
    /// nBitCount refers to the number of bits in a bitset.
    /// WordType refers to the type of integer word which stores bitet data. By default it is BitsetWordType.
    ///
#define BITSET_WORD_COUNT(nBitCount, WordType) (nBitCount == 0 ? 1 : ((nBitCount - 1) / (8 * sizeof(WordType)) + 1))


    /// bitset_base
    ///
    /// This is a default implementation that works for any number of words.
    ///
    template<size_t NW, typename WordType> // Templated on the number of words used to hold the bitset and the word type.
    struct bitset_base {
        typedef WordType word_type;
        typedef bitset_base<NW, WordType> this_type;
        typedef size_t size_type;

        enum {
            kBitsPerWord = (8 * sizeof(word_type)),
            kBitsPerWordMask = (kBitsPerWord - 1),
            kBitsPerWordShift = ((kBitsPerWord == 8) ? 3 : ((kBitsPerWord == 16) ? 4 : ((kBitsPerWord == 32) ? 5
                                                                                                             : (((kBitsPerWord ==
                                                                                                                  64)
                                                                                                                 ? 6
                                                                                                                 : 7)))))
        };

    public:
        word_type mWord[NW];

    public:
        bitset_base();

        bitset_base(
                uint32_t value); // This exists only for compatibility with std::bitset, which has a 'long' constructor.
        //bitset_base(uint64_t value); // Disabled because it causes conflicts with the 32 bit version with existing user code. Use from_uint64 to init from a uint64_t instead.

        void operator&=(const this_type &x);

        void operator|=(const this_type &x);

        void operator^=(const this_type &x);

        void operator<<=(size_type n);

        void operator>>=(size_type n);

        void flip();

        void set();

        void set(size_type i, bool value);

        void reset();

        bool operator==(const this_type &x) const;

        bool any() const;

        size_type count() const;

        word_type &DoGetWord(size_type i);

        word_type DoGetWord(size_type i) const;

        size_type DoFindFirst() const;

        size_type DoFindNext(size_type last_find) const;

        size_type
        DoFindLast() const;                       // Returns NW * kBitsPerWord (the bit count) if no bits are set.
        size_type
        DoFindPrev(size_type last_find) const;    // Returns NW * kBitsPerWord (the bit count) if no bits are set.

    }; // class bitset_base



    /// bitset_base<1, WordType>
    ///
    /// This is a specialization for a bitset that fits within one word.
    ///
    template<typename WordType>
    struct bitset_base<1, WordType> {
        typedef WordType word_type;
        typedef bitset_base<1, WordType> this_type;
        typedef size_t size_type;

        enum {
            kBitsPerWord = (8 * sizeof(word_type)),
            kBitsPerWordMask = (kBitsPerWord - 1),
            kBitsPerWordShift = ((kBitsPerWord == 8) ? 3 : ((kBitsPerWord == 16) ? 4 : ((kBitsPerWord == 32) ? 5
                                                                                                             : (((kBitsPerWord ==
                                                                                                                  64)
                                                                                                                 ? 6
                                                                                                                 : 7)))))
        };

    public:
        word_type mWord[1]; // Defined as an array of 1 so that bitset can treat this bitset_base like others.

    public:
        bitset_base();

        bitset_base(uint32_t value);
        //bitset_base(uint64_t value); // Disabled because it causes conflicts with the 32 bit version with existing user code. Use from_uint64 instead.

        void operator&=(const this_type &x);

        void operator|=(const this_type &x);

        void operator^=(const this_type &x);

        void operator<<=(size_type n);

        void operator>>=(size_type n);

        void flip();

        void set();

        void set(size_type i, bool value);

        void reset();

        bool operator==(const this_type &x) const;

        bool any() const;

        size_type count() const;

        word_type &DoGetWord(size_type);

        word_type DoGetWord(size_type) const;

        size_type DoFindFirst() const;

        size_type DoFindNext(size_type last_find) const;

        size_type
        DoFindLast() const;                       // Returns 1 * kBitsPerWord (the bit count) if no bits are set.
        size_type
        DoFindPrev(size_type last_find) const;    // Returns 1 * kBitsPerWord (the bit count) if no bits are set.

    }; // bitset_base<1, WordType>



    /// bitset_base<2, WordType>
    ///
    /// This is a specialization for a bitset that fits within two words.
    /// The difference here is that we avoid branching (ifs and loops).
    ///
    template<typename WordType>
    struct bitset_base<2, WordType> {
        typedef WordType word_type;
        typedef bitset_base<2, WordType> this_type;
        typedef size_t size_type;

        enum {
            kBitsPerWord = (8 * sizeof(word_type)),
            kBitsPerWordMask = (kBitsPerWord - 1),
            kBitsPerWordShift = ((kBitsPerWord == 8) ? 3 : ((kBitsPerWord == 16) ? 4 : ((kBitsPerWord == 32) ? 5
                                                                                                             : (((kBitsPerWord ==
                                                                                                                  64)
                                                                                                                 ? 6
                                                                                                                 : 7)))))
        };

    public:
        word_type mWord[2];

    public:
        bitset_base();

        bitset_base(uint32_t value);
        //bitset_base(uint64_t value); // Disabled because it causes conflicts with the 32 bit version with existing user code. Use from_uint64 instead.

        void operator&=(const this_type &x);

        void operator|=(const this_type &x);

        void operator^=(const this_type &x);

        void operator<<=(size_type n);

        void operator>>=(size_type n);

        void flip();

        void set();

        void set(size_type i, bool value);

        void reset();

        bool operator==(const this_type &x) const;

        bool any() const;

        size_type count() const;

        word_type &DoGetWord(size_type);

        word_type DoGetWord(size_type) const;

        size_type DoFindFirst() const;

        size_type DoFindNext(size_type last_find) const;

        size_type
        DoFindLast() const;                       // Returns 2 * kBitsPerWord (the bit count) if no bits are set.
        size_type
        DoFindPrev(size_type last_find) const;    // Returns 2 * kBitsPerWord (the bit count) if no bits are set.

    }; // bitset_base<2, WordType>




    /// bitset
    ///
    /// Implements a bitset much like the C++ std::bitset.
    ///
    /// As of this writing we don't implement a specialization of bitset<0>,
    /// as it is deemed an academic exercise that nobody would actually
    /// use and it would increase code space and provide little practical
    /// benefit. Note that this doesn't mean bitset<0> isn't supported;
    /// it means that our version of it isn't as efficient as it would be
    /// if a specialization was made for it.
    ///
    /// - N can be any unsigned (non-zero) value, though memory usage is
    ///   linear with respect to N, so large values of N use large amounts of memory.
    /// - WordType must be one of [uint16_t, uint32_t, uint64_t, uint128_t]
    ///   and the compiler must support the type. By default the WordType is
    ///   the largest native register type that the target platform supports.
    ///
    template<size_t N, typename WordType = FLARE_BITSET_WORD_TYPE_DEFAULT>
    class bitset : private bitset_base<BITSET_WORD_COUNT(N, WordType), WordType> {
    public:
        typedef bitset_base<BITSET_WORD_COUNT(N, WordType), WordType> base_type;
        typedef bitset<N, WordType> this_type;
        typedef WordType word_type;
        typedef typename base_type::size_type size_type;

        enum {
            kBitsPerWord = (8 * sizeof(word_type)),
            kBitsPerWordMask = (kBitsPerWord - 1),
            kBitsPerWordShift = ((kBitsPerWord == 8) ? 3 : ((kBitsPerWord == 16) ? 4 : ((kBitsPerWord == 32) ? 5
                                                                                                             : (((kBitsPerWord ==
                                                                                                                  64)
                                                                                                                 ? 6
                                                                                                                 : 7))))),
            kSize = N,                               // The number of bits the bitset holds
            kWordSize = sizeof(word_type),               // The size of individual words the bitset uses to hold the bits.
            kWordCount = BITSET_WORD_COUNT(N,
                                           WordType)   // The number of words the bitset uses to hold the bits. sizeof(bitset<N, WordType>) == kWordSize * kWordCount.
        };

        using base_type::mWord;
        using base_type::DoGetWord;
        using base_type::DoFindFirst;
        using base_type::DoFindNext;
        using base_type::DoFindLast;
        using base_type::DoFindPrev;
        using base_type::count;
        using base_type::any;

    public:
        /// reference
        ///
        /// A reference is a reference to a specific bit in the bitset.
        /// The C++ standard specifies that this be a nested class,
        /// though it is not clear if a non-nested reference implementation
        /// would be non-conforming.
        ///
        class reference {
        protected:

            friend class bitset<N, WordType>;

            word_type *mpBitWord;
            size_type mnBitIndex;

            reference() {} // The C++ standard specifies that this is private.

        public:
            reference(const bitset &x, size_type i);

            reference &operator=(bool value);

            reference &operator=(const reference &x);

            bool operator~() const;

            operator bool() const // Defined inline because CodeWarrior fails to be able to compile it outside.
            { return (*mpBitWord & (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask))) != 0; }

            reference &flip();
        };

    public:
        friend class reference;

        bitset();

        bitset(uint32_t value);
        //bitset(uint64_t value); // Disabled because it causes conflicts with the 32 bit version with existing user code. Use from_uint64 instead.

        // We don't define copy constructor and operator= because
        // the compiler-generated versions will suffice.

        this_type &operator&=(const this_type &x);

        this_type &operator|=(const this_type &x);

        this_type &operator^=(const this_type &x);

        this_type &operator<<=(size_type n);

        this_type &operator>>=(size_type n);

        this_type &set();

        this_type &set(size_type i, bool value = true);

        this_type &reset();

        this_type &reset(size_type i);

        this_type &flip();

        this_type &flip(size_type i);

        this_type operator~() const;

        reference operator[](size_type i);

        bool operator[](size_type i) const;

        const word_type *data() const;

        word_type *data();

        //size_type count() const;            // We inherit this from the base class.
        size_type size() const;

        bool operator==(const this_type &x) const;


        bool operator!=(const this_type &x) const;

        bool test(size_type i) const;

        //bool any() const;                   // We inherit this from the base class.
        bool all() const;

        bool none() const;

        this_type operator<<(size_type n) const;

        this_type operator>>(size_type n) const;

        // Finds the index of the first "on" bit, returns kSize if none are set.
        size_type find_first() const;

        // Finds the index of the next "on" bit after last_find, returns kSize if none are set.
        size_type find_next(size_type last_find) const;

        // Finds the index of the last "on" bit, returns kSize if none are set.
        size_type find_last() const;

        // Finds the index of the last "on" bit before last_find, returns kSize if none are set.
        size_type find_prev(size_type last_find) const;

    }; // bitset







    /// BitsetCountBits
    ///
    /// This is a fast trick way to count bits without branches nor memory accesses.
    ///
    inline uint32_t BitsetCountBits(uint64_t x) {
        x = x - ((x >> 1) & UINT64_C(0x5555555555555555));
        x = (x & UINT64_C(0x3333333333333333)) + ((x >> 2) & UINT64_C(0x3333333333333333));
        x = (x + (x >> 4)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
        return (uint32_t) ((x * UINT64_C(0x0101010101010101)) >> 56);
    }

    inline uint32_t BitsetCountBits(uint32_t x) {
        x = x - ((x >> 1) & 0x55555555);
        x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
        x = (x + (x >> 4)) & 0x0F0F0F0F;
        return (uint32_t) ((x * 0x01010101) >> 24);
    }

    inline uint32_t BitsetCountBits(uint16_t x) {
        return BitsetCountBits((uint32_t) x);
    }

    inline uint32_t BitsetCountBits(uint8_t x) {
        return BitsetCountBits((uint32_t) x);
    }

    inline uint32_t GetFirstBit(uint8_t x) {
        if (x) {
            uint32_t n = 1;

            if ((x & 0x0000000F) == 0) {
                n += 4;
                x >>= 4;
            }
            if ((x & 0x00000003) == 0) {
                n += 2;
                x >>= 2;
            }

            return (uint32_t) (n - (x & 1));
        }

        return 8;
    }

    inline uint32_t GetFirstBit(
            uint16_t x) // To do: Update this to use VC++ _BitScanForward, _BitScanForward64; GCC __builtin_ctz, __builtin_ctzl. VC++ __lzcnt16, __lzcnt, __lzcnt64 requires recent CPUs (2013+) and probably can't be used. http://en.wikipedia.org/wiki/Haswell_%28microarchitecture%29#New_features
    {
        if (x) {
            uint32_t n = 1;

            if ((x & 0x000000FF) == 0) {
                n += 8;
                x >>= 8;
            }
            if ((x & 0x0000000F) == 0) {
                n += 4;
                x >>= 4;
            }
            if ((x & 0x00000003) == 0) {
                n += 2;
                x >>= 2;
            }

            return (uint32_t) (n - (x & 1));
        }

        return 16;
    }

    inline uint32_t GetFirstBit(uint32_t x) {
        if (x) {
            uint32_t n = 1;

            if ((x & 0x0000FFFF) == 0) {
                n += 16;
                x >>= 16;
            }
            if ((x & 0x000000FF) == 0) {
                n += 8;
                x >>= 8;
            }
            if ((x & 0x0000000F) == 0) {
                n += 4;
                x >>= 4;
            }
            if ((x & 0x00000003) == 0) {
                n += 2;
                x >>= 2;
            }

            return (n - (x & 1));
        }

        return 32;
    }

    inline uint32_t GetFirstBit(uint64_t x) {
        if (x) {
            uint32_t n = 1;

            if ((x & 0xFFFFFFFF) == 0) {
                n += 32;
                x >>= 32;
            }
            if ((x & 0x0000FFFF) == 0) {
                n += 16;
                x >>= 16;
            }
            if ((x & 0x000000FF) == 0) {
                n += 8;
                x >>= 8;
            }
            if ((x & 0x0000000F) == 0) {
                n += 4;
                x >>= 4;
            }
            if ((x & 0x00000003) == 0) {
                n += 2;
                x >>= 2;
            }

            return (n - ((uint32_t) x & 1));
        }

        return 64;
    }


    inline uint32_t GetFirstBit(uint128 x) {
        if (x) {
            uint32_t n = 1;

            if ((x & UINT64_C(0xFFFFFFFFFFFFFFFF)) == 0) {
                n += 64;
                x >>= 64;
            }
            if ((x & 0xFFFFFFFF) == 0) {
                n += 32;
                x >>= 32;
            }
            if ((x & 0x0000FFFF) == 0) {
                n += 16;
                x >>= 16;
            }
            if ((x & 0x000000FF) == 0) {
                n += 8;
                x >>= 8;
            }
            if ((x & 0x0000000F) == 0) {
                n += 4;
                x >>= 4;
            }
            if ((x & 0x00000003) == 0) {
                n += 2;
                x >>= 2;
            }

            return (n - ((uint32_t) x & 1));
        }

        return 128;
    }

    inline uint32_t GetLastBit(uint8_t x) {
        if (x) {
            uint32_t n = 0;

            if (x & 0xFFF0) {
                n += 4;
                x >>= 4;
            }
            if (x & 0xFFFC) {
                n += 2;
                x >>= 2;
            }
            if (x & 0xFFFE) { n += 1; }

            return n;
        }

        return 8;
    }

    inline uint32_t GetLastBit(uint16_t x) {
        if (x) {
            uint32_t n = 0;

            if (x & 0xFF00) {
                n += 8;
                x >>= 8;
            }
            if (x & 0xFFF0) {
                n += 4;
                x >>= 4;
            }
            if (x & 0xFFFC) {
                n += 2;
                x >>= 2;
            }
            if (x & 0xFFFE) { n += 1; }

            return n;
        }

        return 16;
    }

    inline uint32_t GetLastBit(uint32_t x) {
        if (x) {
            uint32_t n = 0;

            if (x & 0xFFFF0000) {
                n += 16;
                x >>= 16;
            }
            if (x & 0xFFFFFF00) {
                n += 8;
                x >>= 8;
            }
            if (x & 0xFFFFFFF0) {
                n += 4;
                x >>= 4;
            }
            if (x & 0xFFFFFFFC) {
                n += 2;
                x >>= 2;
            }
            if (x & 0xFFFFFFFE) { n += 1; }

            return n;
        }

        return 32;
    }

    inline uint32_t GetLastBit(uint64_t x) {
        if (x) {
            uint32_t n = 0;

            if (x & UINT64_C(0xFFFFFFFF00000000)) {
                n += 32;
                x >>= 32;
            }
            if (x & 0xFFFF0000) {
                n += 16;
                x >>= 16;
            }
            if (x & 0xFFFFFF00) {
                n += 8;
                x >>= 8;
            }
            if (x & 0xFFFFFFF0) {
                n += 4;
                x >>= 4;
            }
            if (x & 0xFFFFFFFC) {
                n += 2;
                x >>= 2;
            }
            if (x & 0xFFFFFFFE) { n += 1; }

            return n;
        }

        return 64;
    }

    inline uint32_t GetLastBit(uint128 x) {
        if (x) {
            uint32_t n = 0;

            uint128 mask(UINT64_C(0xFFFFFFFF00000000));
            mask <<= 64;

            if (x & mask) {
                n += 64;
                x >>= 64;
            }
            if (x & UINT64_C(0xFFFFFFFF00000000)) {
                n += 32;
                x >>= 32;
            }
            if (x & UINT64_C(0x00000000FFFF0000)) {
                n += 16;
                x >>= 16;
            }
            if (x & UINT64_C(0x00000000FFFFFF00)) {
                n += 8;
                x >>= 8;
            }
            if (x & UINT64_C(0x00000000FFFFFFF0)) {
                n += 4;
                x >>= 4;
            }
            if (x & UINT64_C(0x00000000FFFFFFFC)) {
                n += 2;
                x >>= 2;
            }
            if (x & UINT64_C(0x00000000FFFFFFFE)) { n += 1; }

            return n;
        }

        return 128;
    }




    ///////////////////////////////////////////////////////////////////////////
    // bitset_base
    //
    // We tried two forms of array access here:
    //     for(word_type *pWord(mWord), *pWordEnd(mWord + NW); pWord < pWordEnd; ++pWord)
    //         *pWord = ...
    // and
    //     for(size_t i = 0; i < NW; i++)
    //         mWord[i] = ...
    //
    // For our tests (~NW < 16), the latter (using []) access resulted in faster code.
    ///////////////////////////////////////////////////////////////////////////

    template<size_t NW, typename WordType>
    inline bitset_base<NW, WordType>::bitset_base() {
        reset();
    }


    template<size_t NW, typename WordType>
    inline bitset_base<NW, WordType>::bitset_base(uint32_t value) {
        reset();
        mWord[0] = static_cast<word_type>(value);
    }


    template<size_t NW, typename WordType>
    inline void bitset_base<NW, WordType>::operator&=(const this_type &x) {
        for (size_t i = 0; i < NW; i++)
            mWord[i] &= x.mWord[i];
    }


    template<size_t NW, typename WordType>
    inline void bitset_base<NW, WordType>::operator|=(const this_type &x) {
        for (size_t i = 0; i < NW; i++)
            mWord[i] |= x.mWord[i];
    }


    template<size_t NW, typename WordType>
    inline void bitset_base<NW, WordType>::operator^=(const this_type &x) {
        for (size_t i = 0; i < NW; i++)
            mWord[i] ^= x.mWord[i];
    }


    template<size_t NW, typename WordType>
    inline void bitset_base<NW, WordType>::operator<<=(size_type n) {
        const size_type nWordShift = (size_type) (n >> kBitsPerWordShift);

        if (nWordShift) {
            for (int i = (int) (NW - 1); i >= 0; --i)
                mWord[i] = (nWordShift <= (size_type) i) ? mWord[i - nWordShift] : (word_type) 0;
        }

        if (n &= kBitsPerWordMask) {
            for (size_t i = (NW - 1); i > 0; --i)
                mWord[i] = (word_type) ((mWord[i] << n) | (mWord[i - 1] >> (kBitsPerWord - n)));
            mWord[0] <<= n;
        }

        // We let the parent class turn off any upper bits.
    }


    template<size_t NW, typename WordType>
    inline void bitset_base<NW, WordType>::operator>>=(size_type n) {
        const size_type nWordShift = (size_type) (n >> kBitsPerWordShift);

        if (nWordShift) {
            for (size_t i = 0; i < NW; ++i)
                mWord[i] = ((nWordShift < (NW - i)) ? mWord[i + nWordShift] : (word_type) 0);
        }

        if (n &= kBitsPerWordMask) {
            for (size_t i = 0; i < (NW - 1); ++i)
                mWord[i] = (word_type) ((mWord[i] >> n) | (mWord[i + 1] << (kBitsPerWord - n)));
            mWord[NW - 1] >>= n;
        }
    }


    template<size_t NW, typename WordType>
    inline void bitset_base<NW, WordType>::flip() {
        for (size_t i = 0; i < NW; i++)
            mWord[i] = ~mWord[i];
        // We let the parent class turn off any upper bits.
    }


    template<size_t NW, typename WordType>
    inline void bitset_base<NW, WordType>::set() {
        for (size_t i = 0; i < NW; i++)
            mWord[i] = static_cast<word_type>(~static_cast<word_type>(0));
        // We let the parent class turn off any upper bits.
    }


    template<size_t NW, typename WordType>
    inline void bitset_base<NW, WordType>::set(size_type i, bool value) {
        if (value)
            mWord[i >> kBitsPerWordShift] |= (static_cast<word_type>(1) << (i & kBitsPerWordMask));
        else
            mWord[i >> kBitsPerWordShift] &= ~(static_cast<word_type>(1) << (i & kBitsPerWordMask));
    }


    template<size_t NW, typename WordType>
    inline void bitset_base<NW, WordType>::reset() {
        if (NW > 16) // This is a constant expression and should be optimized away.
        {
            // This will be fastest if compiler intrinsic function optimizations are enabled.
            memset(mWord, 0, sizeof(mWord));
        } else {
            for (size_t i = 0; i < NW; i++)
                mWord[i] = 0;
        }
    }


    template<size_t NW, typename WordType>
    inline bool bitset_base<NW, WordType>::operator==(const this_type &x) const {
        for (size_t i = 0; i < NW; i++) {
            if (mWord[i] != x.mWord[i])
                return false;
        }
        return true;
    }


    template<size_t NW, typename WordType>
    inline bool bitset_base<NW, WordType>::any() const {
        for (size_t i = 0; i < NW; i++) {
            if (mWord[i])
                return true;
        }
        return false;
    }


    template<size_t NW, typename WordType>
    inline typename bitset_base<NW, WordType>::size_type
    bitset_base<NW, WordType>::count() const {
        size_type n = 0;

        for (size_t i = 0; i < NW; i++) {
            n += (size_type) flare::base::popcount(mWord[i]);
        }
        return n;
    }

    template<size_t NW, typename WordType>
    inline typename bitset_base<NW, WordType>::word_type &
    bitset_base<NW, WordType>::DoGetWord(size_type i) {
        return mWord[i >> kBitsPerWordShift];
    }


    template<size_t NW, typename WordType>
    inline typename bitset_base<NW, WordType>::word_type
    bitset_base<NW, WordType>::DoGetWord(size_type i) const {
        return mWord[i >> kBitsPerWordShift];
    }


    template<size_t NW, typename WordType>
    inline typename bitset_base<NW, WordType>::size_type
    bitset_base<NW, WordType>::DoFindFirst() const {
        for (size_type word_index = 0; word_index < NW; ++word_index) {
            const size_type fbiw = GetFirstBit(mWord[word_index]);

            if (fbiw != kBitsPerWord)
                return (word_index * kBitsPerWord) + fbiw;
        }

        return (size_type) NW * kBitsPerWord;
    }


    template<size_t NW, typename WordType>
    inline typename bitset_base<NW, WordType>::size_type
    bitset_base<NW, WordType>::DoFindNext(size_type last_find) const {
        // Start looking from the next bit.
        ++last_find;

        // Set initial state based on last find.
        size_type word_index = static_cast<size_type>(last_find >> kBitsPerWordShift);
        size_type bit_index = static_cast<size_type>(last_find & kBitsPerWordMask);

        // To do: There probably is a more elegant way to write looping below.
        if (word_index < NW) {
            // Mask off previous bits of the word so our search becomes a "find first".
            word_type this_word = mWord[word_index] & (~static_cast<word_type>(0) << bit_index);

            for (;;) {
                const size_type fbiw = GetFirstBit(this_word);

                if (fbiw != kBitsPerWord)
                    return (word_index * kBitsPerWord) + fbiw;

                if (++word_index < NW)
                    this_word = mWord[word_index];
                else
                    break;
            }
        }

        return (size_type) NW * kBitsPerWord;
    }


    template<size_t NW, typename WordType>
    inline typename bitset_base<NW, WordType>::size_type
    bitset_base<NW, WordType>::DoFindLast() const {
        for (size_type word_index = (size_type) NW; word_index > 0; --word_index) {
            const size_type lbiw = GetLastBit(mWord[word_index - 1]);

            if (lbiw != kBitsPerWord)
                return ((word_index - 1) * kBitsPerWord) + lbiw;
        }

        return (size_type) NW * kBitsPerWord;
    }


    template<size_t NW, typename WordType>
    inline typename bitset_base<NW, WordType>::size_type
    bitset_base<NW, WordType>::DoFindPrev(size_type last_find) const {
        if (last_find > 0) {
            // Set initial state based on last find.
            size_type word_index = static_cast<size_type>(last_find >> kBitsPerWordShift);
            size_type bit_index = static_cast<size_type>(last_find & kBitsPerWordMask);

            // Mask off subsequent bits of the word so our search becomes a "find last".
            word_type mask = (~static_cast<word_type>(0) >> (kBitsPerWord - 1 - bit_index))
                    >> 1; // We do two shifts here because many CPUs ignore requests to shift 32 bit integers by 32 bits, which could be the case above.
            word_type this_word = mWord[word_index] & mask;

            for (;;) {
                const size_type lbiw = GetLastBit(this_word);

                if (lbiw != kBitsPerWord)
                    return (word_index * kBitsPerWord) + lbiw;

                if (word_index > 0)
                    this_word = mWord[--word_index];
                else
                    break;
            }
        }

        return (size_type) NW * kBitsPerWord;
    }



    ///////////////////////////////////////////////////////////////////////////
    // bitset_base<1, WordType>
    ///////////////////////////////////////////////////////////////////////////

    template<typename WordType>
    inline bitset_base<1, WordType>::bitset_base() {
        mWord[0] = 0;
    }


    template<typename WordType>
    inline bitset_base<1, WordType>::bitset_base(uint32_t value) {
        mWord[0] = static_cast<word_type>(value);
    }

    template<typename WordType>
    inline void bitset_base<1, WordType>::operator&=(const this_type &x) {
        mWord[0] &= x.mWord[0];
    }


    template<typename WordType>
    inline void bitset_base<1, WordType>::operator|=(const this_type &x) {
        mWord[0] |= x.mWord[0];
    }


    template<typename WordType>
    inline void bitset_base<1, WordType>::operator^=(const this_type &x) {
        mWord[0] ^= x.mWord[0];
    }


    template<typename WordType>
    inline void bitset_base<1, WordType>::operator<<=(size_type n) {
        mWord[0] <<= n;
        // We let the parent class turn off any upper bits.
    }


    template<typename WordType>
    inline void bitset_base<1, WordType>::operator>>=(size_type n) {
        mWord[0] >>= n;
    }


    template<typename WordType>
    inline void bitset_base<1, WordType>::flip() {
        mWord[0] = ~mWord[0];
        // We let the parent class turn off any upper bits.
    }


    template<typename WordType>
    inline void bitset_base<1, WordType>::set() {
        mWord[0] = static_cast<word_type>(~static_cast<word_type>(0));
        // We let the parent class turn off any upper bits.
    }


    template<typename WordType>
    inline void bitset_base<1, WordType>::set(size_type i, bool value) {
        if (value)
            mWord[0] |= (static_cast<word_type>(1) << i);
        else
            mWord[0] &= ~(static_cast<word_type>(1) << i);
    }


    template<typename WordType>
    inline void bitset_base<1, WordType>::reset() {
        mWord[0] = 0;
    }


    template<typename WordType>
    inline bool bitset_base<1, WordType>::operator==(const this_type &x) const {
        return mWord[0] == x.mWord[0];
    }


    template<typename WordType>
    inline bool bitset_base<1, WordType>::any() const {
        return mWord[0] != 0;
    }


    template<typename WordType>
    inline typename bitset_base<1, WordType>::size_type
    bitset_base<1, WordType>::count() const {
        return (size_type) flare::base::popcount(mWord[0]);
    }

    template<typename WordType>
    inline typename bitset_base<1, WordType>::word_type &
    bitset_base<1, WordType>::DoGetWord(size_type) {
        return mWord[0];
    }


    template<typename WordType>
    inline typename bitset_base<1, WordType>::word_type
    bitset_base<1, WordType>::DoGetWord(size_type) const {
        return mWord[0];
    }


    template<typename WordType>
    inline typename bitset_base<1, WordType>::size_type
    bitset_base<1, WordType>::DoFindFirst() const {
        return GetFirstBit(mWord[0]);
    }


    template<typename WordType>
    inline typename bitset_base<1, WordType>::size_type
    bitset_base<1, WordType>::DoFindNext(size_type last_find) const {
        if (++last_find < kBitsPerWord) {
            // Mask off previous bits of word so our search becomes a "find first".
            const word_type this_word = mWord[0] & ((~static_cast<word_type>(0)) << last_find);

            return GetFirstBit(this_word);
        }

        return kBitsPerWord;
    }


    template<typename WordType>
    inline typename bitset_base<1, WordType>::size_type
    bitset_base<1, WordType>::DoFindLast() const {
        return GetLastBit(mWord[0]);
    }


    template<typename WordType>
    inline typename bitset_base<1, WordType>::size_type
    bitset_base<1, WordType>::DoFindPrev(size_type last_find) const {
        if (last_find > 0) {
            // Mask off previous bits of word so our search becomes a "find first".
            const word_type this_word = mWord[0] & ((~static_cast<word_type>(0)) >> (kBitsPerWord - last_find));

            return GetLastBit(this_word);
        }

        return kBitsPerWord;
    }




    ///////////////////////////////////////////////////////////////////////////
    // bitset_base<2, WordType>
    ///////////////////////////////////////////////////////////////////////////

    template<typename WordType>
    inline bitset_base<2, WordType>::bitset_base() {
        mWord[0] = 0;
        mWord[1] = 0;
    }


    template<typename WordType>
    inline bitset_base<2, WordType>::bitset_base(uint32_t value) {
        mWord[0] = static_cast<word_type>(value);
        mWord[1] = 0;
    }

    template<typename WordType>
    inline void bitset_base<2, WordType>::operator&=(const this_type &x) {
        mWord[0] &= x.mWord[0];
        mWord[1] &= x.mWord[1];
    }


    template<typename WordType>
    inline void bitset_base<2, WordType>::operator|=(const this_type &x) {
        mWord[0] |= x.mWord[0];
        mWord[1] |= x.mWord[1];
    }


    template<typename WordType>
    inline void bitset_base<2, WordType>::operator^=(const this_type &x) {
        mWord[0] ^= x.mWord[0];
        mWord[1] ^= x.mWord[1];
    }


    template<typename WordType>
    inline void bitset_base<2, WordType>::operator<<=(size_type n) {
        if (n) // to avoid a shift by kBitsPerWord, which is undefined
        {
            if (FLARE_UNLIKELY(n >= kBitsPerWord))   // parent expected to handle high bits and n >= 64
            {
                mWord[1] = mWord[0];
                mWord[0] = 0;
                n -= kBitsPerWord;
            }

            mWord[1] = (mWord[1] << n) | (mWord[0] >> (kBitsPerWord - n)); // Intentionally use | instead of +.
            mWord[0] <<= n;
            // We let the parent class turn off any upper bits.
        }
    }


    template<typename WordType>
    inline void bitset_base<2, WordType>::operator>>=(size_type n) {
        if (n) // to avoid a shift by kBitsPerWord, which is undefined
        {
            if (FLARE_UNLIKELY(n >= kBitsPerWord))   // parent expected to handle n >= 64
            {
                mWord[0] = mWord[1];
                mWord[1] = 0;
                n -= kBitsPerWord;
            }

            mWord[0] = (mWord[0] >> n) | (mWord[1] << (kBitsPerWord - n)); // Intentionally use | instead of +.
            mWord[1] >>= n;
        }
    }


    template<typename WordType>
    inline void bitset_base<2, WordType>::flip() {
        mWord[0] = ~mWord[0];
        mWord[1] = ~mWord[1];
        // We let the parent class turn off any upper bits.
    }


    template<typename WordType>
    inline void bitset_base<2, WordType>::set() {
        mWord[0] = ~static_cast<word_type>(0);
        mWord[1] = ~static_cast<word_type>(0);
        // We let the parent class turn off any upper bits.
    }


    template<typename WordType>
    inline void bitset_base<2, WordType>::set(size_type i, bool value) {
        if (value)
            mWord[i >> kBitsPerWordShift] |= (static_cast<word_type>(1) << (i & kBitsPerWordMask));
        else
            mWord[i >> kBitsPerWordShift] &= ~(static_cast<word_type>(1) << (i & kBitsPerWordMask));
    }


    template<typename WordType>
    inline void bitset_base<2, WordType>::reset() {
        mWord[0] = 0;
        mWord[1] = 0;
    }


    template<typename WordType>
    inline bool bitset_base<2, WordType>::operator==(const this_type &x) const {
        return (mWord[0] == x.mWord[0]) && (mWord[1] == x.mWord[1]);
    }


    template<typename WordType>
    inline bool bitset_base<2, WordType>::any() const {
        // Or with two branches: { return (mWord[0] != 0) || (mWord[1] != 0); }
        return (mWord[0] | mWord[1]) != 0;
    }

    template<typename WordType>
    inline typename bitset_base<2, WordType>::size_type
    bitset_base<2, WordType>::count() const {
        return (size_type) flare::base::popcount(mWord[0]) + (size_type) __builtin_popcountll(mWord[1]);
    }


    template<typename WordType>
    inline typename bitset_base<2, WordType>::word_type &
    bitset_base<2, WordType>::DoGetWord(size_type i) {
        return mWord[i >> kBitsPerWordShift];
    }


    template<typename WordType>
    inline typename bitset_base<2, WordType>::word_type
    bitset_base<2, WordType>::DoGetWord(size_type i) const {
        return mWord[i >> kBitsPerWordShift];
    }


    template<typename WordType>
    inline typename bitset_base<2, WordType>::size_type
    bitset_base<2, WordType>::DoFindFirst() const {
        size_type fbiw = GetFirstBit(mWord[0]);

        if (fbiw != kBitsPerWord)
            return fbiw;

        fbiw = GetFirstBit(mWord[1]);

        if (fbiw != kBitsPerWord)
            return kBitsPerWord + fbiw;

        return 2 * kBitsPerWord;
    }


    template<typename WordType>
    inline typename bitset_base<2, WordType>::size_type
    bitset_base<2, WordType>::DoFindNext(size_type last_find) const {
        // If the last find was in the first word, we must check it and then possibly the second.
        if (++last_find < (size_type) kBitsPerWord) {
            // Mask off previous bits of word so our search becomes a "find first".
            word_type this_word = mWord[0] & ((~static_cast<word_type>(0)) << last_find);

            // Step through words.
            size_type fbiw = GetFirstBit(this_word);

            if (fbiw != kBitsPerWord)
                return fbiw;

            fbiw = GetFirstBit(mWord[1]);

            if (fbiw != kBitsPerWord)
                return kBitsPerWord + fbiw;
        } else if (last_find < (size_type) (2 * kBitsPerWord)) {
            // The last find was in the second word, remove the bit count of the first word from the find.
            last_find -= kBitsPerWord;

            // Mask off previous bits of word so our search becomes a "find first".
            word_type this_word = mWord[1] & ((~static_cast<word_type>(0)) << last_find);

            const size_type fbiw = GetFirstBit(this_word);

            if (fbiw != kBitsPerWord)
                return kBitsPerWord + fbiw;
        }

        return 2 * kBitsPerWord;
    }


    template<typename WordType>
    inline typename bitset_base<2, WordType>::size_type
    bitset_base<2, WordType>::DoFindLast() const {
        size_type lbiw = GetLastBit(mWord[1]);

        if (lbiw != kBitsPerWord)
            return kBitsPerWord + lbiw;

        lbiw = GetLastBit(mWord[0]);

        if (lbiw != kBitsPerWord)
            return lbiw;

        return 2 * kBitsPerWord;
    }


    template<typename WordType>
    inline typename bitset_base<2, WordType>::size_type
    bitset_base<2, WordType>::DoFindPrev(size_type last_find) const {
        // If the last find was in the second word, we must check it and then possibly the first.
        if (last_find > (size_type) kBitsPerWord) {
            // This has the same effect as last_find %= kBitsPerWord in our case.
            last_find -= kBitsPerWord;

            // Mask off previous bits of word so our search becomes a "find first".
            word_type this_word = mWord[1] & ((~static_cast<word_type>(0)) >> (kBitsPerWord - last_find));

            // Step through words.
            size_type lbiw = GetLastBit(this_word);

            if (lbiw != kBitsPerWord)
                return kBitsPerWord + lbiw;

            lbiw = GetLastBit(mWord[0]);

            if (lbiw != kBitsPerWord)
                return lbiw;
        } else if (last_find != 0) {
            // Mask off previous bits of word so our search becomes a "find first".
            word_type this_word = mWord[0] & ((~static_cast<word_type>(0)) >> (kBitsPerWord - last_find));

            const size_type lbiw = GetLastBit(this_word);

            if (lbiw != kBitsPerWord)
                return lbiw;
        }

        return 2 * kBitsPerWord;
    }



    ///////////////////////////////////////////////////////////////////////////
    // bitset::reference
    ///////////////////////////////////////////////////////////////////////////

    template<size_t N, typename WordType>
    inline bitset<N, WordType>::reference::reference(const bitset &x, size_type i)
            : mpBitWord(&const_cast<bitset &>(x).DoGetWord(i)),
              mnBitIndex(i &
                         kBitsPerWordMask) {   // We have an issue here because the above is casting away the const-ness of the source bitset.
        // Empty
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::reference &
    bitset<N, WordType>::reference::operator=(bool value) {
        if (value)
            *mpBitWord |= (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        else
            *mpBitWord &= ~(static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::reference &
    bitset<N, WordType>::reference::operator=(const reference &x) {
        if (*x.mpBitWord & (static_cast<word_type>(1) << (x.mnBitIndex & kBitsPerWordMask)))
            *mpBitWord |= (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        else
            *mpBitWord &= ~(static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        return *this;
    }


    template<size_t N, typename WordType>
    inline bool bitset<N, WordType>::reference::operator~() const {
        return (*mpBitWord & (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask))) == 0;
    }


    //Defined inline in the class because Metrowerks fails to be able to compile it here.
    //template <size_t N, typename WordType>
    //inline bitset<N, WordType>::reference::operator bool() const
    //{
    //    return (*mpBitWord & (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask))) != 0;
    //}


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::reference &
    bitset<N, WordType>::reference::flip() {
        *mpBitWord ^= static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask);
        return *this;
    }




    ///////////////////////////////////////////////////////////////////////////
    // bitset
    ///////////////////////////////////////////////////////////////////////////

    template<size_t N, typename WordType>
    inline bitset<N, WordType>::bitset()
            : base_type() {
        // Empty. The base class will set all bits to zero.
    }

    template<size_t N, typename WordType>
    inline bitset<N, WordType>::bitset(uint32_t value)
            : base_type(value) {
        if ((N & kBitsPerWordMask) || (N ==
                                       0)) // If there are any high bits to clear... (If we didn't have this check, then the code below would do the wrong thing when N == 32.
            mWord[kWordCount - 1] &= ~(static_cast<word_type>(~static_cast<word_type>(0))
                    << (N & kBitsPerWordMask)); // This clears any high unused bits.
    }

    /*
    template <size_t N, typename WordType>
    inline bitset<N, WordType>::bitset(uint64_t value)
        : base_type(value)
    {
        if((N & kBitsPerWordMask) || (N == 0)) // If there are any high bits to clear...
            mWord[kWordCount - 1] &= ~(~static_cast<word_type>(0) << (N & kBitsPerWordMask)); // This clears any high unused bits.
    }
    */


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type &
    bitset<N, WordType>::operator&=(const this_type &x) {
        base_type::operator&=(x);
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type &
    bitset<N, WordType>::operator|=(const this_type &x) {
        base_type::operator|=(x);
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type &
    bitset<N, WordType>::operator^=(const this_type &x) {
        base_type::operator^=(x);
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type &
    bitset<N, WordType>::operator<<=(size_type n) {
        if (FLARE_LIKELY((intptr_t) n < (intptr_t) N)) {
            base_type::operator<<=(n);
            if ((N & kBitsPerWordMask) || (N ==
                                           0)) // If there are any high bits to clear... (If we didn't have this check, then the code below would do the wrong thing when N == 32.
                mWord[kWordCount - 1] &= ~(static_cast<word_type>(~static_cast<word_type>(0)) << (N &
                                                                                                  kBitsPerWordMask)); // This clears any high unused bits. We need to do this so that shift operations proceed correctly.
        } else
            base_type::reset();
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type &
    bitset<N, WordType>::operator>>=(size_type n) {
        if (FLARE_LIKELY(n < N))
            base_type::operator>>=(n);
        else
            base_type::reset();
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type &
    bitset<N, WordType>::set() {
        base_type::set(); // This sets all bits.
        if ((N & kBitsPerWordMask) || (N ==
                                       0)) // If there are any high bits to clear... (If we didn't have this check, then the code below would do the wrong thing when N == 32.
            mWord[kWordCount - 1] &= ~(static_cast<word_type>(~static_cast<word_type>(0)) << (N &
                                                                                              kBitsPerWordMask)); // This clears any high unused bits. We need to do this so that shift operations proceed correctly.
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type &
    bitset<N, WordType>::set(size_type i, bool value) {
        FLARE_CHECK(i < N) << "bitset::set -- out of range";
        base_type::set(i, value);
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type &
    bitset<N, WordType>::reset() {
        base_type::reset();
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type &
    bitset<N, WordType>::reset(size_type i) {
        FLARE_CHECK(i < N) << "bitset::reset -- out of range";
        DoGetWord(i) &= ~(static_cast<word_type>(1) << (i & kBitsPerWordMask));
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type &
    bitset<N, WordType>::flip() {
        base_type::flip();
        if ((N & kBitsPerWordMask) || (N ==
                                       0)) // If there are any high bits to clear... (If we didn't have this check, then the code below would do the wrong thing when N == 32.
            mWord[kWordCount - 1] &= ~(static_cast<word_type>(~static_cast<word_type>(0)) << (N &
                                                                                              kBitsPerWordMask)); // This clears any high unused bits. We need to do this so that shift operations proceed correctly.
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type &
    bitset<N, WordType>::flip(size_type i) {
        if (FLARE_LIKELY(i < N)) {
            DoGetWord(i) ^= (static_cast<word_type>(1) << (i & kBitsPerWordMask));
        } else {
            FLARE_CHECK(false) << "bitset::reset -- out of range";
        }
        return *this;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type
    bitset<N, WordType>::operator~() const {
        return this_type(*this).flip();
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::reference
    bitset<N, WordType>::operator[](size_type i) {
        FLARE_CHECK(FLARE_UNLIKELY(i < N)) << "bitset::operator[] -- out of range";
        return reference(*this, i);
    }


    template<size_t N, typename WordType>
    inline bool bitset<N, WordType>::operator[](size_type i) const {
        FLARE_CHECK(FLARE_UNLIKELY(i < N)) << "bitset::operator[] -- out of range";
        return (DoGetWord(i) & (static_cast<word_type>(1) << (i & kBitsPerWordMask))) != 0;
    }


    template<size_t N, typename WordType>
    inline const typename bitset<N, WordType>::word_type *bitset<N, WordType>::data() const {
        return base_type::mWord;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::word_type *bitset<N, WordType>::data() {
        return base_type::mWord;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::size_type
    bitset<N, WordType>::size() const {
        return (size_type) N;
    }


    template<size_t N, typename WordType>
    inline bool bitset<N, WordType>::operator==(const this_type &x) const {
        return base_type::operator==(x);
    }

    template<size_t N, typename WordType>
    inline bool bitset<N, WordType>::operator!=(const this_type &x) const {
        return !base_type::operator==(x);
    }


    template<size_t N, typename WordType>
    inline bool bitset<N, WordType>::test(size_type i) const {
        FLARE_CHECK(FLARE_UNLIKELY(i < N)) << "bitset::test -- out of range";
        return (DoGetWord(i) & (static_cast<word_type>(1) << (i & kBitsPerWordMask))) != 0;
    }


    // template <size_t N, typename WordType>
    // inline bool bitset<N, WordType>::any() const
    // {
    //     return base_type::any();
    // }


    template<size_t N, typename WordType>
    inline bool bitset<N, WordType>::all() const {
        return count() == size();
    }


    template<size_t N, typename WordType>
    inline bool bitset<N, WordType>::none() const {
        return !base_type::any();
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type
    bitset<N, WordType>::operator<<(size_type n) const {
        return this_type(*this).operator<<=(n);
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::this_type
    bitset<N, WordType>::operator>>(size_type n) const {
        return this_type(*this).operator>>=(n);
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::size_type
    bitset<N, WordType>::find_first() const {
        const size_type i = base_type::DoFindFirst();

        if (i < kSize)
            return i;
        // Else i could be the base type bit count, so we clamp it to our size.

        return kSize;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::size_type
    bitset<N, WordType>::find_next(size_type last_find) const {
        const size_type i = base_type::DoFindNext(last_find);

        if (i < kSize)
            return i;
        // Else i could be the base type bit count, so we clamp it to our size.

        return kSize;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::size_type
    bitset<N, WordType>::find_last() const {
        const size_type i = base_type::DoFindLast();

        if (i < kSize)
            return i;
        // Else i could be the base type bit count, so we clamp it to our size.

        return kSize;
    }


    template<size_t N, typename WordType>
    inline typename bitset<N, WordType>::size_type
    bitset<N, WordType>::find_prev(size_type last_find) const {
        const size_type i = base_type::DoFindPrev(last_find);

        if (i < kSize)
            return i;
        // Else i could be the base type bit count, so we clamp it to our size.

        return kSize;
    }



    ///////////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////////

    template<size_t N, typename WordType>
    inline bitset<N, WordType> operator&(const bitset<N, WordType> &a, const bitset<N, WordType> &b) {
        // We get betting inlining when we don't declare temporary variables.
        return bitset<N, WordType>(a).operator&=(b);
    }


    template<size_t N, typename WordType>
    inline bitset<N, WordType> operator|(const bitset<N, WordType> &a, const bitset<N, WordType> &b) {
        return bitset<N, WordType>(a).operator|=(b);
    }


    template<size_t N, typename WordType>
    inline bitset<N, WordType> operator^(const bitset<N, WordType> &a, const bitset<N, WordType> &b) {
        return bitset<N, WordType>(a).operator^=(b);
    }

}  // namespace flare

#endif  // FLARE_CONTAINER_BITSET_H_
