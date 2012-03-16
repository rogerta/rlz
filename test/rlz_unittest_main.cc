// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Main entry point for all unit tests.

#include "base/at_exit.h"
#include "base/command_line.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_WIN)
// TODO(thakis): Add this once more stuff is ported.
#include "rlz/win/lib/rlz_lib.h"
#endif

int main(int argc, char **argv) {
  base::AtExitManager at_exit;
  CommandLine::Init(argc, argv);

  testing::InitGoogleMock(&argc, argv);
  testing::InitGoogleTest(&argc, argv);

  int ret = RUN_ALL_TESTS();
#if defined(OS_WIN)
  // TODO(thakis): Add this once more stuff is ported.
  if (ret == 0) {
    // Now re-run all the tests using a supplementary brand code.  This brand
    // code will remain in effect for the lifetime of the branding object.
    rlz_lib::SupplementaryBranding branding("TEST");
    ret = RUN_ALL_TESTS();
  }
#endif

  return ret;
}
