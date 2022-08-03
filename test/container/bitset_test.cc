
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "flare/container/bitset.h"
#include "testing/gtest_wrap.h"


using namespace flare;


// Template instantations.
// These tell the compiler to compile all the functions for the given class.

#if (FLARE_BITSET_WORD_SIZE_DEFAULT != 1)

template
class flare::bitset<1, uint8_t>;

template
class flare::bitset<33, uint8_t>;

template
class flare::bitset<65, uint8_t>;

template
class flare::bitset<129, uint8_t>;

#endif

#if (FLARE_BITSET_WORD_SIZE_DEFAULT != 2)

template
class flare::bitset<1, uint16_t>;

template
class flare::bitset<33, uint16_t>;

template
class flare::bitset<65, uint16_t>;

template
class flare::bitset<129, uint16_t>;

#endif

#if (FLARE_BITSET_WORD_SIZE_DEFAULT != 4) // If not already represented

template
class flare::bitset<1, uint32_t>;

template
class flare::bitset<33, uint32_t>;

template
class flare::bitset<65, uint32_t>;

template
class flare::bitset<129, uint32_t>;

#endif

#if (FLARE_BITSET_WORD_SIZE_DEFAULT != 8)
template class flare::bitset<1,   uint64_t>;
template class flare::bitset<33,  uint64_t>;
template class flare::bitset<65,  uint64_t>;
template class flare::bitset<129, uint64_t>;
#endif

#if (FLARE_BITSET_WORD_SIZE_DEFAULT != 16)
#if EASTL_INT128_SUPPORTED
template class flare::bitset<1,   eastl_uint128_t>;
        template class flare::bitset<33,  eastl_uint128_t>;
        template class flare::bitset<65,  eastl_uint128_t>;
        template class flare::bitset<129, eastl_uint128_t>;
#endif
#endif


TEST(bitmap, all) {


    // bitset<0> tests
    bitset<0> b0(0x10101010);
    EXPECT_TRUE(b0.count() == 0);
    b0.flip();
    EXPECT_TRUE(b0.count() == 0);

    b0 <<= 1;
    EXPECT_TRUE(b0.count() == 0);

    // bitset<8> tests
    bitset<8> b8(0x10101010);
    EXPECT_TRUE(b8.count() == 1);

    b8.flip();
    EXPECT_TRUE(b8.count() == 7);

    b8 <<= 1;
    EXPECT_TRUE(b8.count() == 6);

    b8.reset();
    b8.flip();
    b8 >>= 33;
    EXPECT_TRUE(b8.count() == 0);

    b8.reset();
    b8.flip();
    b8 >>= 65;
    EXPECT_TRUE(b8.count() == 0);




    // bitset<16> tests
    bitset<16> b16(0x10101010);
    EXPECT_TRUE(b16.count() == 2);

    b16.flip();
    EXPECT_TRUE(b16.count() == 14);

    b16 <<= 1;
    EXPECT_TRUE(b16.count() == 13);

    b16.reset();
    b16.flip();
    b16 >>= 33;
    EXPECT_TRUE(b16.count() == 0);

    b16.reset();
    b16.flip();
    b16 >>= 65;
    EXPECT_TRUE(b16.count() == 0);

}

TEST(bitmap, s1) {
    bitset<1> b1;
    bitset<1> b1A(1);

    EXPECT_TRUE(b1.size() == 1);
    EXPECT_TRUE(b1.any() == false);
    EXPECT_TRUE(b1.all() == false);
    EXPECT_TRUE(b1.none() == true);
    EXPECT_TRUE(b1A.any() == true);
    EXPECT_TRUE(b1A.all() == true);
    EXPECT_TRUE(b1A.none() == false);


    bitset<33> b33;
    bitset<33> b33A(1);

    EXPECT_TRUE(b33.size() == 33);
    EXPECT_TRUE(b33.any() == false);
    EXPECT_TRUE(b33.all() == false);
    EXPECT_TRUE(b33.none() == true);
    EXPECT_TRUE(b33A.any() == true);
    EXPECT_TRUE(b33A.all() == false);
    EXPECT_TRUE(b33A.none() == false);


    bitset<65> b65;
    bitset<65> b65A(1);

    EXPECT_TRUE(b65.size() == 65);
    EXPECT_TRUE(b65.any() == false);
    EXPECT_TRUE(b65.all() == false);
    EXPECT_TRUE(b65.none() == true);
    EXPECT_TRUE(b65A.any() == true);
    EXPECT_TRUE(b65A.all() == false);
    EXPECT_TRUE(b65A.none() == false);


    bitset<129> b129;
    bitset<129> b129A(1);

    EXPECT_TRUE(b129.size() == 129);
    EXPECT_TRUE(b129.any() == false);
    EXPECT_TRUE(b129.all() == false);
    EXPECT_TRUE(b129.none() == true);
    EXPECT_TRUE(b129A.any() == true);
    EXPECT_TRUE(b129A.all() == false);
    EXPECT_TRUE(b129A.none() == false);
    b1[0] = true;
    EXPECT_TRUE(b1.test(0) == true);
    EXPECT_TRUE(b1.count() == 1);

    b33[0] = true;
    b33[32] = true;
    EXPECT_TRUE(b33.test(0) == true);
    EXPECT_TRUE(b33.test(15) == false);
    EXPECT_TRUE(b33.test(32) == true);
    EXPECT_TRUE(b33.count() == 2);

    b65[0] = true;
    b65[32] = true;
    b65[64] = true;
    EXPECT_TRUE(b65.test(0) == true);
    EXPECT_TRUE(b65.test(15) == false);
    EXPECT_TRUE(b65.test(32) == true);
    EXPECT_TRUE(b65.test(47) == false);
    EXPECT_TRUE(b65.test(64) == true);
    EXPECT_TRUE(b65.count() == 3);

    b129[0] = true;
    b129[32] = true;
    b129[64] = true;
    b129[128] = true;
    EXPECT_TRUE(b129.test(0) == true);
    EXPECT_TRUE(b129.test(15) == false);
    EXPECT_TRUE(b129.test(32) == true);
    EXPECT_TRUE(b129.test(47) == false);
    EXPECT_TRUE(b129.test(64) == true);
    EXPECT_TRUE(b129.test(91) == false);
    EXPECT_TRUE(b129.test(128) == true);
    EXPECT_TRUE(b129.count() == 4);

    bitset<1>::word_type *pWordArray;

    pWordArray = b1.data();
    EXPECT_TRUE(pWordArray != NULL);
    pWordArray = b33.data();
    EXPECT_TRUE(pWordArray != NULL);
    pWordArray = b65.data();
    EXPECT_TRUE(pWordArray != NULL);
    pWordArray = b129.data();
    EXPECT_TRUE(pWordArray != NULL);


    // bitset<1> set, reset, flip, ~
    b1.reset();
    EXPECT_TRUE(b1.count() == 0);

    b1.set();
    EXPECT_TRUE(b1.count() == b1.size());
    EXPECT_TRUE(b1.all());

    b1.flip();
    EXPECT_TRUE(b1.count() == 0);
    EXPECT_TRUE(!b1.all());
    EXPECT_TRUE(b1.none());

    b1.set(0, true);
    EXPECT_TRUE(b1[0] == true);

    b1.reset(0);
    EXPECT_TRUE(b1[0] == false);

    b1.flip(0);
    EXPECT_TRUE(b1[0] == true);

    bitset<1> b1Not = ~b1;
    EXPECT_TRUE(b1[0] == true);
    EXPECT_TRUE(b1Not[0] == false);


    // bitset<33> set, reset, flip, ~
    b33.reset();
    EXPECT_TRUE(b33.count() == 0);

    b33.set();
    EXPECT_TRUE(b33.count() == b33.size());
    EXPECT_TRUE(b33.all());


    b33.flip();
    EXPECT_TRUE(b33.count() == 0);
    EXPECT_TRUE(!b33.all());

    b33.set(0, true);
    b33.set(32, true);
    EXPECT_TRUE(b33[0] == true);
    EXPECT_TRUE(b33[15] == false);
    EXPECT_TRUE(b33[32] == true);

    b33.reset(0);
    b33.reset(32);
    EXPECT_TRUE(b33[0] == false);
    EXPECT_TRUE(b33[32] == false);

    b33.flip(0);
    b33.flip(32);
    EXPECT_TRUE(b33[0] == true);
    EXPECT_TRUE(b33[32] == true);

    bitset<33> b33Not(~b33);
    EXPECT_TRUE(b33[0] == true);
    EXPECT_TRUE(b33[32] == true);
    EXPECT_TRUE(b33Not[0] == false);
    EXPECT_TRUE(b33Not[32] == false);


    // bitset<65> set, reset, flip, ~
    b65.reset();
    EXPECT_TRUE(b65.count() == 0);
    EXPECT_TRUE(!b65.all());
    EXPECT_TRUE(b65.none());

    b65.set();
    EXPECT_TRUE(b65.count() == b65.size());
    EXPECT_TRUE(b65.all());
    EXPECT_TRUE(!b65.none());

    b65.flip();
    EXPECT_TRUE(b65.count() == 0);
    EXPECT_TRUE(!b65.all());
    EXPECT_TRUE(b65.none());


    b65.set(0, true);
    b65.set(32, true);
    b65.set(64, true);
    EXPECT_TRUE(b65[0] == true);
    EXPECT_TRUE(b65[15] == false);
    EXPECT_TRUE(b65[32] == true);
    EXPECT_TRUE(b65[50] == false);
    EXPECT_TRUE(b65[64] == true);

    b65.reset(0);
    b65.reset(32);
    b65.reset(64);
    EXPECT_TRUE(b65[0] == false);
    EXPECT_TRUE(b65[32] == false);
    EXPECT_TRUE(b65[64] == false);

    b65.flip(0);
    b65.flip(32);
    b65.flip(64);
    EXPECT_TRUE(b65[0] == true);
    EXPECT_TRUE(b65[32] == true);
    EXPECT_TRUE(b65[64] == true);

    bitset<65> b65Not(~b65);
    EXPECT_TRUE(b65[0] == true);
    EXPECT_TRUE(b65[32] == true);
    EXPECT_TRUE(b65[64] == true);
    EXPECT_TRUE(b65Not[0] == false);
    EXPECT_TRUE(b65Not[32] == false);
    EXPECT_TRUE(b65Not[64] == false);


    // bitset<65> set, reset, flip, ~
    b129.reset();
    EXPECT_TRUE(b129.count() == 0);

    b129.set();
    EXPECT_TRUE(b129.count() == b129.size());
    EXPECT_TRUE(b129.all());

    b129.flip();
    EXPECT_TRUE(b129.count() == 0);
    EXPECT_TRUE(!b129.all());
    EXPECT_TRUE(b129.none());

    b129.set(0, true);
    b129.set(32, true);
    b129.set(64, true);
    b129.set(128, true);
    EXPECT_TRUE(b129[0] == true);
    EXPECT_TRUE(b129[15] == false);
    EXPECT_TRUE(b129[32] == true);
    EXPECT_TRUE(b129[50] == false);
    EXPECT_TRUE(b129[64] == true);
    EXPECT_TRUE(b129[90] == false);
    EXPECT_TRUE(b129[128] == true);

    b129.reset(0);
    b129.reset(32);
    b129.reset(64);
    b129.reset(128);
    EXPECT_TRUE(b129[0] == false);
    EXPECT_TRUE(b129[32] == false);
    EXPECT_TRUE(b129[64] == false);
    EXPECT_TRUE(b129[128] == false);

    b129.flip(0);
    b129.flip(32);
    b129.flip(64);
    b129.flip(128);
    EXPECT_TRUE(b129[0] == true);
    EXPECT_TRUE(b129[32] == true);
    EXPECT_TRUE(b129[64] == true);
    EXPECT_TRUE(b129[128] == true);

    bitset<129> b129Not(~b129);
    EXPECT_TRUE(b129[0] == true);
    EXPECT_TRUE(b129[32] == true);
    EXPECT_TRUE(b129[64] == true);
    EXPECT_TRUE(b129[128] == true);
    EXPECT_TRUE(b129Not[0] == false);
    EXPECT_TRUE(b129Not[32] == false);
    EXPECT_TRUE(b129Not[64] == false);
    EXPECT_TRUE(b129Not[128] == false);


    // operator ==, !=
    bitset<1> b1Equal(b1);
    EXPECT_TRUE(b1Equal == b1);
    EXPECT_TRUE(b1Equal != b1Not);

    bitset<33> b33Equal(b33);
    EXPECT_TRUE(b33Equal == b33);
    EXPECT_TRUE(b33Equal != b33Not);

    bitset<65> b65Equal(b65);
    EXPECT_TRUE(b65Equal == b65);
    EXPECT_TRUE(b65Equal != b65Not);

    bitset<129> b129Equal(b129);
    EXPECT_TRUE(b129Equal == b129);
    EXPECT_TRUE(b129Equal != b129Not);


    // bitset<1> operator<<=, operator>>=, operator<<, operator>>
    b1.reset();

    b1[0] = true;
    b1 >>= 0;
    EXPECT_TRUE(b1[0] == true);
    b1 >>= 1;
    EXPECT_TRUE(b1[0] == false);

    b1[0] = true;
    b1 <<= 0;
    EXPECT_TRUE(b1[0] == true);
    b1 <<= 1;
    EXPECT_TRUE(b1[0] == false);

    b1[0] = true;
    b1Equal = b1 >> 0;
    EXPECT_TRUE(b1Equal == b1);
    b1Equal = b1 >> 1;
    EXPECT_TRUE(b1Equal[0] == false);

    b1[0] = true;
    b1Equal = b1 << 0;
    EXPECT_TRUE(b1Equal[0] == true);
    b1Equal = b1 << 1;
    EXPECT_TRUE(b1Equal[0] == false);

    b1.reset();
    b1.flip();
    b1 >>= 33;
    EXPECT_TRUE(b1.count() == 0);
    EXPECT_TRUE(!b1.all());
    EXPECT_TRUE(b1.none());

    b1.reset();
    b1.flip();
    b1 <<= 33;
    EXPECT_TRUE(b1.count() == 0);
    EXPECT_TRUE(!b1.all());
    EXPECT_TRUE(b1.none());

    b1.reset();
    b1.flip();
    b1 >>= 65;
    EXPECT_TRUE(b1.count() == 0);
    EXPECT_TRUE(!b1.all());
    EXPECT_TRUE(b1.none());

    b1.reset();
    b1.flip();
    b1 <<= 65;
    EXPECT_TRUE(b1.count() == 0);
    EXPECT_TRUE(!b1.all());
    EXPECT_TRUE(b1.none());


    // bitset<33> operator<<=, operator>>=, operator<<, operator>>
    b33.reset();

    b33[0] = true;
    b33[32] = true;
    b33 >>= 0;
    EXPECT_TRUE(b33[0] == true);
    EXPECT_TRUE(b33[32] == true);
    b33 >>= 10;
    EXPECT_TRUE(b33[22] == true);

    b33.reset();
    b33[0] = true;
    b33[32] = true;
    b33 <<= 0;
    EXPECT_TRUE(b33[0] == true);
    EXPECT_TRUE(b33[32] == true);
    b33 <<= 10;
    EXPECT_TRUE(b33[10] == true);

    b33.reset();
    b33[0] = true;
    b33[32] = true;
    b33Equal = b33 >> 0;
    EXPECT_TRUE(b33Equal == b33);
    b33Equal = b33 >> 10;
    EXPECT_TRUE(b33Equal[22] == true);

    b33.reset();
    b33[0] = true;
    b33[32] = true;
    b33Equal = b33 << 10;
    EXPECT_TRUE(b33Equal[10] == true);

    b33.reset();
    b33.flip();
    b33 >>= 33;
    EXPECT_TRUE(b33.count() == 0);
    EXPECT_TRUE(!b33.all());
    EXPECT_TRUE(b33.none());

    b33.reset();
    b33.flip();
    b33 <<= 33;
    EXPECT_TRUE(b33.count() == 0);
    EXPECT_TRUE(!b33.all());
    EXPECT_TRUE(b33.none());

    b33.reset();
    b33.flip();
    b33 >>= 65;
    EXPECT_TRUE(b33.count() == 0);
    EXPECT_TRUE(!b33.all());
    EXPECT_TRUE(b33.none());

    b33.reset();
    b33.flip();
    b33 <<= 65;
    EXPECT_TRUE(b33.count() == 0);
    EXPECT_TRUE(!b33.all());
    EXPECT_TRUE(b33.none());


    // bitset<65> operator<<=, operator>>=, operator<<, operator>>
    b65.reset();

    b65[0] = true;
    b65[32] = true;
    b65[64] = true;
    b65 >>= 0;
    EXPECT_TRUE(b65[0] == true);
    EXPECT_TRUE(b65[32] == true);
    EXPECT_TRUE(b65[64] == true);
    b65 >>= 10;
    EXPECT_TRUE(b65[22] == true);
    EXPECT_TRUE(b65[54] == true);

    b65.reset();
    b65[0] = true;
    b65[32] = true;
    b65[64] = true;
    b65 <<= 0;
    EXPECT_TRUE(b65[0] == true);
    EXPECT_TRUE(b65[32] == true);
    EXPECT_TRUE(b65[64] == true);
    b65 <<= 10;
    EXPECT_TRUE(b65[10] == true);
    EXPECT_TRUE(b65[42] == true);

    b65.reset();
    b65[0] = true;
    b65[32] = true;
    b65[64] = true;
    b65Equal = b65 >> 0;
    EXPECT_TRUE(b65Equal == b65);
    b65Equal = b65 >> 10;
    EXPECT_TRUE(b65Equal[22] == true);
    EXPECT_TRUE(b65Equal[54] == true);

    b65.reset();
    b65[0] = true;
    b65[32] = true;
    b65[64] = true;
    b65Equal = b65 << 10;
    EXPECT_TRUE(b65Equal[10] == true);
    EXPECT_TRUE(b65Equal[42] == true);

    b65.reset();
    b65.flip();
    b65 >>= 33;
    EXPECT_TRUE(b65.count() == 32);

    b65.reset();
    b65.flip();
    b65 <<= 33;
    EXPECT_TRUE(b65.count() == 32);

    b65.reset();
    b65.flip();
    b65 >>= 65;
    EXPECT_TRUE(b65.count() == 0);

    b65.reset();
    b65.flip();
    b65 <<= 65;
    EXPECT_TRUE(b65.count() == 0);


    // bitset<129> operator<<=, operator>>=, operator<<, operator>>
    b129.reset();

    b129[0] = true;
    b129[32] = true;
    b129[64] = true;
    b129[128] = true;
    b129 >>= 0;
    EXPECT_TRUE(b129[0] == true);
    EXPECT_TRUE(b129[32] == true);
    EXPECT_TRUE(b129[64] == true);
    EXPECT_TRUE(b129[128] == true);
    b129 >>= 10;
    EXPECT_TRUE(b129[22] == true);
    EXPECT_TRUE(b129[54] == true);
    EXPECT_TRUE(b129[118] == true);

    b129.reset();
    b129[0] = true;
    b129[32] = true;
    b129[64] = true;
    b129[128] = true;
    b129 <<= 0;
    EXPECT_TRUE(b129[0] == true);
    EXPECT_TRUE(b129[32] == true);
    EXPECT_TRUE(b129[64] == true);
    EXPECT_TRUE(b129[128] == true);
    b129 <<= 10;
    EXPECT_TRUE(b129[10] == true);
    EXPECT_TRUE(b129[42] == true);
    EXPECT_TRUE(b129[74] == true);

    b129.reset();
    b129[0] = true;
    b129[32] = true;
    b129[64] = true;
    b129[128] = true;
    b129Equal = b129 >> 0;
    EXPECT_TRUE(b129Equal == b129);
    b129Equal = b129 >> 10;
    EXPECT_TRUE(b129Equal[22] == true);
    EXPECT_TRUE(b129Equal[54] == true);
    EXPECT_TRUE(b129Equal[118] == true);

    b129.reset();
    b129[0] = true;
    b129[32] = true;
    b129[64] = true;
    b129[128] = true;
    b129Equal = b129 << 10;
    EXPECT_TRUE(b129Equal[10] == true);
    EXPECT_TRUE(b129Equal[42] == true);
    EXPECT_TRUE(b129Equal[74] == true);

    b129.reset();
    b129.flip();
    b129 >>= 33;
    EXPECT_TRUE(b129.count() == 96);

    b129.reset();
    b129.flip();
    b129 <<= 33;
    EXPECT_TRUE(b129.count() == 96);

    b129.reset();
    b129.flip();
    b129 >>= 65;
    EXPECT_TRUE(b129.count() == 64);

    b129.reset();
    b129.flip();
    b129 <<= 65;
    EXPECT_TRUE(b129.count() == 64);


    // operator&=(const this_type& x), operator|=(const this_type& x), operator^=(const this_type& x)
    b1.set();
    b1[0] = false;
    b1A[0] = true;
    b1 &= b1A;
    EXPECT_TRUE(b1[0] == false);
    b1 |= b1A;
    EXPECT_TRUE(b1[0] == true);
    b1 ^= b1A;
    EXPECT_TRUE(b1[0] == false);
    b1 |= b1A;
    EXPECT_TRUE(b1[0] == true);

    b33.set();
    b33[0] = false;
    b33[32] = false;
    b33A[0] = true;
    b33A[32] = true;
    b33 &= b33A;
    EXPECT_TRUE((b33[0] == false) && (b33[32] == false));
    b33 |= b33A;
    EXPECT_TRUE((b33[0] == true) && (b33[32] == true));
    b33 ^= b33A;
    EXPECT_TRUE((b33[0] == false) && (b33[32] == false));
    b33 |= b33A;
    EXPECT_TRUE((b33[0] == true) && (b33[32] == true));

    b65.set();
    b65[0] = false;
    b65[32] = false;
    b65[64] = false;
    b65A[0] = true;
    b65A[32] = true;
    b65A[64] = true;
    b65 &= b65A;
    EXPECT_TRUE((b65[0] == false) && (b65[32] == false) && (b65[64] == false));
    b65 |= b65A;
    EXPECT_TRUE((b65[0] == true) && (b65[32] == true) && (b65[64] == true));
    b65 ^= b65A;
    EXPECT_TRUE((b65[0] == false) && (b65[32] == false) && (b65[64] == false));
    b65 |= b65A;
    EXPECT_TRUE((b65[0] == true) && (b65[32] == true) && (b65[64] == true));

    b129.set();
    b129[0] = false;
    b129[32] = false;
    b129[64] = false;
    b129[128] = false;
    b129A[0] = true;
    b129A[32] = true;
    b129A[64] = true;
    b129A[128] = true;
    b129 &= b129A;
    EXPECT_TRUE((b129[0] == false) && (b129[32] == false) && (b129[64] == false) && (b129[128] == false));
    b129 |= b129A;
    EXPECT_TRUE((b129[0] == true) && (b129[32] == true) && (b129[64] == true) && (b129[128] == true));
    b129 ^= b129A;
    EXPECT_TRUE((b129[0] == false) && (b129[32] == false) && (b129[64] == false) && (b129[128] == false));
    b129 |= b129A;
    EXPECT_TRUE((b129[0] == true) && (b129[32] == true) && (b129[64] == true) && (b129[128] == true));
}

TEST(bitmap, s3) { // Test bitset::reference
    bitset<65> b65;
    bitset<65>::reference r = b65[33];

    r = true;
    EXPECT_TRUE(r == true);
}

TEST(bitmap, s4) { // Test find_first, find_next
    size_t i, j;

    // bitset<1>
    bitset<1> b1;

    i = b1.find_first();
    EXPECT_TRUE(i == b1.kSize);
    b1.set(0, true);
    i = b1.find_first();
    EXPECT_TRUE(i == 0);
    i = b1.find_next(i);
    EXPECT_TRUE(i == b1.kSize);

    b1.

            set();

    for (
            i = 0, j = b1.find_first();
            j != b1.
                    kSize;
            j = b1.find_next(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 1);

// bitset<7>
    bitset<7> b7;

    i = b7.find_first();
    EXPECT_TRUE(i == b7.kSize);
    b7.set(0, true);
    b7.set(5, true);
    i = b7.find_first();
    EXPECT_TRUE(i == 0);
    i = b7.find_next(i);
    EXPECT_TRUE(i == 5);
    i = b7.find_next(i);
    EXPECT_TRUE(i == b7.kSize);

    b7.

            set();

    for (
            i = 0, j = b7.find_first();
            j != b7.
                    kSize;
            j = b7.find_next(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 7);

// bitset<32>
    bitset<32> b32;

    i = b32.find_first();
    EXPECT_TRUE(i == b32.kSize);
    b32.set(0, true);
    b32.set(27, true);
    i = b32.find_first();
    EXPECT_TRUE(i == 0);
    i = b32.find_next(i);
    EXPECT_TRUE(i == 27);
    i = b32.find_next(i);
    EXPECT_TRUE(i == b32.kSize);

    b32.

            set();

    for (
            i = 0, j = b32.find_first();
            j != b32.
                    kSize;
            j = b32.find_next(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 32);

// bitset<41>
    bitset<41> b41;

    i = b41.find_first();
    EXPECT_TRUE(i == b41.kSize);
    b41.set(0, true);
    b41.set(27, true);
    b41.set(37, true);
    i = b41.find_first();
    EXPECT_TRUE(i == 0);
    i = b41.find_next(i);
    EXPECT_TRUE(i == 27);
    i = b41.find_next(i);
    EXPECT_TRUE(i == 37);
    i = b41.find_next(i);
    EXPECT_TRUE(i == b41.kSize);

    b41.

            set();

    for (
            i = 0, j = b41.find_first();
            j != b41.
                    kSize;
            j = b41.find_next(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 41);

// bitset<64>
    bitset<64> b64;

    i = b64.find_first();
    EXPECT_TRUE(i == b64.kSize);
    b64.set(0, true);
    b64.set(27, true);
    b64.set(37, true);
    i = b64.find_first();
    EXPECT_TRUE(i == 0);
    i = b64.find_next(i);
    EXPECT_TRUE(i == 27);
    i = b64.find_next(i);
    EXPECT_TRUE(i == 37);
    i = b64.find_next(i);
    EXPECT_TRUE(i == b64.kSize);

    b64.

            set();

    for (
            i = 0, j = b64.find_first();
            j != b64.
                    kSize;
            j = b64.find_next(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 64);

// bitset<79>
    bitset<79> b79;

    i = b79.find_first();
    EXPECT_TRUE(i == b79.kSize);
    b79.set(0, true);
    b79.set(27, true);
    b79.set(37, true);
    i = b79.find_first();
    EXPECT_TRUE(i == 0);
    i = b79.find_next(i);
    EXPECT_TRUE(i == 27);
    i = b79.find_next(i);
    EXPECT_TRUE(i == 37);
    i = b79.find_next(i);
    EXPECT_TRUE(i == b79.kSize);

    b79.

            set();

    for (
            i = 0, j = b79.find_first();
            j != b79.
                    kSize;
            j = b79.find_next(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 79);

// bitset<128>
    bitset<128> b128;

    i = b128.find_first();
    EXPECT_TRUE(i == b128.kSize);
    b128.set(0, true);
    b128.set(27, true);
    b128.set(37, true);
    b128.set(77, true);
    i = b128.find_first();
    EXPECT_TRUE(i == 0);
    i = b128.find_next(i);
    EXPECT_TRUE(i == 27);
    i = b128.find_next(i);
    EXPECT_TRUE(i == 37);
    i = b128.find_next(i);
    EXPECT_TRUE(i == 77);
    i = b128.find_next(i);
    EXPECT_TRUE(i == b128.kSize);

    b128.

            set();

    for (
            i = 0, j = b128.find_first();
            j != b128.
                    kSize;
            j = b128.find_next(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 128);

// bitset<137>
    bitset<137> b137;

    i = b137.find_first();
    EXPECT_TRUE(i == b137.kSize);
    b137.set(0, true);
    b137.set(27, true);
    b137.set(37, true);
    b137.set(77, true);
    b137.set(99, true);
    b137.set(136, true);
    i = b137.find_first();
    EXPECT_TRUE(i == 0);
    i = b137.find_next(i);
    EXPECT_TRUE(i == 27);
    i = b137.find_next(i);
    EXPECT_TRUE(i == 37);
    i = b137.find_next(i);
    EXPECT_TRUE(i == 77);
    i = b137.find_next(i);
    EXPECT_TRUE(i == 99);
    i = b137.find_next(i);
    EXPECT_TRUE(i == 136);
    i = b137.find_next(i);
    EXPECT_TRUE(i == b137.kSize);

    b137.

            set();

    for (
            i = 0, j = b137.find_first();
            j != b137.
                    kSize;
            j = b137.find_next(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 137);
}

TEST(bitmap, s5) { // Test find_last, find_prev
    size_t i, j;

// bitset<1>
    bitset<1> b1;

    i = b1.find_last();
    EXPECT_TRUE(i == b1.kSize);
    b1.set(0, true);
    i = b1.find_last();
    EXPECT_TRUE(i == 0);
    i = b1.find_prev(i);
    EXPECT_TRUE(i == b1.kSize);

    b1.

            set();

    for (
            i = 0, j = b1.find_last();
            j != b1.
                    kSize;
            j = b1.find_prev(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 1);

// bitset<7>
    bitset<7> b7;

    i = b7.find_last();
    EXPECT_TRUE(i == b7.kSize);
    b7.set(0, true);
    b7.set(5, true);
    i = b7.find_last();
    EXPECT_TRUE(i == 5);
    i = b7.find_prev(i);
    EXPECT_TRUE(i == 0);
    i = b7.find_prev(i);
    EXPECT_TRUE(i == b7.kSize);

    b7.

            set();

    for (
            i = 0, j = b7.find_last();
            j != b7.
                    kSize;
            j = b7.find_prev(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 7);

// bitset<32>
    bitset<32> b32;

    i = b32.find_last();
    EXPECT_TRUE(i == b32.kSize);
    b32.set(0, true);
    b32.set(27, true);
    i = b32.find_last();
    EXPECT_TRUE(i == 27);
    i = b32.find_prev(i);
    EXPECT_TRUE(i == 0);
    i = b32.find_prev(i);
    EXPECT_TRUE(i == b32.kSize);

    b32.

            set();

    for (
            i = 0, j = b32.find_last();
            j != b32.
                    kSize;
            j = b32.find_prev(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 32);

// bitset<41>
    bitset<41> b41;

    i = b41.find_last();
    EXPECT_TRUE(i == b41.kSize);
    b41.set(0, true);
    b41.set(27, true);
    b41.set(37, true);
    i = b41.find_last();
    EXPECT_TRUE(i == 37);
    i = b41.find_prev(i);
    EXPECT_TRUE(i == 27);
    i = b41.find_prev(i);
    EXPECT_TRUE(i == 0);
    i = b41.find_prev(i);
    EXPECT_TRUE(i == b41.kSize);

    b41.

            set();

    for (
            i = 0, j = b41.find_last();
            j != b41.
                    kSize;
            j = b41.find_prev(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 41);

// bitset<64>
    bitset<64> b64;

    i = b64.find_last();
    EXPECT_TRUE(i == b64.kSize);
    b64.set(0, true);
    b64.set(27, true);
    b64.set(37, true);
    i = b64.find_last();
    EXPECT_TRUE(i == 37);
    i = b64.find_prev(i);
    EXPECT_TRUE(i == 27);
    i = b64.find_prev(i);
    EXPECT_TRUE(i == 0);
    i = b64.find_prev(i);
    EXPECT_TRUE(i == b64.kSize);

    b64.

            set();

    for (
            i = 0, j = b64.find_last();
            j != b64.
                    kSize;
            j = b64.find_prev(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 64);

// bitset<79>
    bitset<79> b79;

    i = b79.find_last();
    EXPECT_TRUE(i == b79.kSize);
    b79.set(0, true);
    b79.set(27, true);
    b79.set(37, true);
    i = b79.find_last();
    EXPECT_TRUE(i == 37);
    i = b79.find_prev(i);
    EXPECT_TRUE(i == 27);
    i = b79.find_prev(i);
    EXPECT_TRUE(i == 0);
    i = b79.find_prev(i);
    EXPECT_TRUE(i == b79.kSize);

    b79.

            set();

    for (
            i = 0, j = b79.find_last();
            j != b79.
                    kSize;
            j = b79.find_prev(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 79);

// bitset<128>
    bitset<128> b128;

    i = b128.find_last();
    EXPECT_TRUE(i == b128.kSize);
    b128.set(0, true);
    b128.set(27, true);
    b128.set(37, true);
    b128.set(77, true);
    i = b128.find_last();
    EXPECT_TRUE(i == 77);
    i = b128.find_prev(i);
    EXPECT_TRUE(i == 37);
    i = b128.find_prev(i);
    EXPECT_TRUE(i == 27);
    i = b128.find_prev(i);
    EXPECT_TRUE(i == 0);
    i = b128.find_prev(i);
    EXPECT_TRUE(i == b128.kSize);

    b128.

            set();

    for (
            i = 0, j = b128.find_last();
            j != b128.
                    kSize;
            j = b128.find_prev(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 128);

// bitset<137>
    bitset<137> b137;

    i = b137.find_last();
    EXPECT_TRUE(i == b137.kSize);
    b137.set(0, true);
    b137.set(27, true);
    b137.set(37, true);
    b137.set(77, true);
    b137.set(99, true);
    b137.set(136, true);
    i = b137.find_last();
    EXPECT_TRUE(i == 136);
    i = b137.find_prev(i);
    EXPECT_TRUE(i == 99);
    i = b137.find_prev(i);
    EXPECT_TRUE(i == 77);
    i = b137.find_prev(i);
    EXPECT_TRUE(i == 37);
    i = b137.find_prev(i);
    EXPECT_TRUE(i == 27);
    i = b137.find_prev(i);
    EXPECT_TRUE(i == 0);
    i = b137.find_prev(i);
    EXPECT_TRUE(i == b137.kSize);

    b137.

            set();

    for (
            i = 0, j = b137.find_last();
            j != b137.
                    kSize;
            j = b137.find_prev(j)
            )
        ++
                i;
    EXPECT_TRUE(i == 137);
}

// test BITSET_WORD_COUNT macro
TEST(bitmap, s6) {
    {
        typedef flare::bitset<32, char> bitset_t;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
    {
        typedef flare::bitset<32, int> bitset_t;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
    {
        typedef flare::bitset<32, int16_t> bitset_t;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
    {
        typedef flare::bitset<32, int32_t> bitset_t;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
    {
        typedef flare::bitset<128, int64_t> bitset_t;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
    {
        typedef flare::bitset<256, int64_t> bitset_t;
        static_assert(bitset_t::kWordCount == BITSET_WORD_COUNT(bitset_t::kSize, bitset_t::word_type),
                      "bitset failure");
    }
}




















