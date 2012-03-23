// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Macros specific to the RLZ library.

#include "rlz/lib/assert.h"

namespace rlz_lib {

#ifdef MUTE_EXPECTED_ASSERTS
std::string expected_assertion_;
#endif

}  // namespace rlz_lib
