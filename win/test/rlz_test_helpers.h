// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Helper functions used by the tests.

#ifndef RLZ_WIN_TEST_RLZ_TEST_HELPERS_H
#define RLZ_WIN_TEST_RLZ_TEST_HELPERS_H

#include "testing/gtest/include/gtest/gtest.h"

class RlzLibTestNoMachineState : public ::testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
};

class RlzLibTestBase : public RlzLibTestNoMachineState {
  virtual void SetUp();
};


#endif  // RLZ_WIN_TEST_RLZ_TEST_HELPERS_H
