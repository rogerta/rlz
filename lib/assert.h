// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Macros specific to the RLZ library.

#ifndef RLZ_LIB_ASSERT_H_
#define RLZ_LIB_ASSERT_H_

#include <string>
#include "base/logging.h"

// An assertion macro.
// Can mute expected assertions in debug mode.

#ifndef ASSERT_STRING
  #ifndef MUTE_EXPECTED_ASSERTS
    #define ASSERT_STRING(expr) LOG_IF(FATAL, false) << (expr)
  #else
    #define ASSERT_STRING(expr) \
      do { \
        std::string expr_string(expr); \
        if (rlz_lib::expected_assertion_ != expr_string) { \
          LOG_IF(FATAL, FALSE) << (expr); \
        } \
      } while (0)
  #endif
#endif


#ifndef VERIFY
  #ifdef _DEBUG
    #define VERIFY(expr) LOG_IF(FATAL, !(expr)) << #expr
  #else
    #define VERIFY(expr) (void)(expr)
  #endif
#endif

namespace rlz_lib {
  extern std::string expected_assertion_;
};

#endif  // RLZ_LIB_ASSERT_H_
