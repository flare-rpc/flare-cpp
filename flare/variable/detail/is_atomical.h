// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef FLARE_VARIABLE_DETAIL_IS_ATOMICAL_H_
#define FLARE_VARIABLE_DETAIL_IS_ATOMICAL_H_

#include "flare/base/type_traits.h"

namespace flare::variable {
namespace detail {
template <class T> struct is_atomical
: std::integral_constant<bool, (std::is_integral<T>::value ||
                                 std::is_floating_point<T>::value)> {};
template <class T> struct is_atomical<const T> : is_atomical<T> { };
template <class T> struct is_atomical<volatile T> : is_atomical<T> { };
template <class T> struct is_atomical<const volatile T> : is_atomical<T> { };

}  // namespace detail
}  // namespace flare::variable

#endif
