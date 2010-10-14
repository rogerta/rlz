// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Main entry point for all unit tests.

#include <shlwapi.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

int main(int argc, char **argv) {
  base::AtExitManager at_exit;
  CommandLine::Init(argc, argv);

  testing::InitGoogleMock(&argc, argv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
