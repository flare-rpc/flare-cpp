//
// Created by liyinbin on 2022/1/23.
//

#ifndef FLARE_BASE_PROFILE_MACROS_H_
#define FLARE_BASE_PROFILE_MACROS_H_

// Concatenate numbers in c/c++ macros.
#ifndef FLARE_CONCAT
# define FLARE_CONCAT(a, b) FLARE_CONCAT_HELPER(a, b)
# define FLARE_CONCAT_HELPER(a, b) a##b
#endif

#define FLARE_DELETE_FUNCTION(decl) decl = delete

#define FLARE_DISALLOW_COPY_AND_ASSIGN(TypeName)                      \
    FLARE_DELETE_FUNCTION(TypeName(const TypeName&));            \
    FLARE_DELETE_FUNCTION(void operator=(const TypeName&))

#endif  // FLARE_BASE_PROFILE_MACROS_H_
