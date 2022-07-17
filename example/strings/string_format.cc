
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <iostream>
#include "flare/strings/str_format.h"
#include <set>
#include <vector>

void print(const std::string &str) {
    std::cout << str << std::endl;
}

void simple_ex() {

    auto str = flare::string_format("hello {}", "world");
    print(str);
    str = flare::string_format("hello {}", 999);
    print(str);
}

void simple_align() {

    auto str = flare::string_format("hello {:02d}", 1);
    print(str);
    str = flare::string_format("{:<30}", "left aligned");
    print(str);
    str = flare::string_format("{:>30}", "right aligned");
    print(str);
    str = flare::string_format("{:^30}", "centered");
    print(str);
    // use '*' as a fill char
    str = flare::string_format("{:*^30}", "centered");
    print(str);
    str = flare::string_format("┌{0:─^20}┐\n"
                               "│{1: ^20}│\n"
                               "└{0:─^20}┘\n", "", "Hello, world!");
    print(str);
}

void simple_join() {

    std::set s{"1", "2", "3"};
    auto str = flare::string_format("hello {}", fmt::join(s, ","));
    print(str);
}

void simple_hex() {

    std::set s{"1", "2", "3"};
    auto str = flare::string_format("int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    print(str);
    str = flare::string_format("int: {0:d};  hex: {0:#x};  oct: {0:#o};  bin: {0:#b}", 42);
    print(str);
    str = flare::string_format("int: {0:d};  hex: {0:#X};  oct: {0:#o};  bin: {0:#b}", 42);
    print(str);
    int a;
    str = flare::string_format("addr: {}", fmt::ptr(&a));
    print(str);
}

int main() {
    simple_ex();
    simple_align();
    simple_join();
    simple_hex();
    return 0;
}
