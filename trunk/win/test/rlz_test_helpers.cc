// Copyright 2010 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// Main entry point for all unit tests.

#include "rlz_test_helpers.h"

#include <shlwapi.h>

#include "base/win/registry.h"
#include "rlz/win/lib/rlz_lib.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const wchar_t* kHKCUReplacement = L"Software\\Google\\RlzUtilUnittest\\HKCU";
const wchar_t* kHKLMReplacement = L"Software\\Google\\RlzUtilUnittest\\HKLM";

// Helper functions to create and destroy dummy registry hives for tests.
// This is needed so that the tests don't need to run with admin privileges.

void OverrideRegistryHives() {
  // Wipe the keys we redirect to.
  // This gives us a stable run, even in the presence of previous
  // crashes or failures.
  LSTATUS err = SHDeleteKey(HKEY_CURRENT_USER, kHKCUReplacement);
  EXPECT_TRUE(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND);
  err = SHDeleteKey(HKEY_CURRENT_USER, kHKLMReplacement);
  EXPECT_TRUE(err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND);

  // Create the keys we're redirecting HKCU and HKLM to.
  base::win::RegKey hkcu;
  base::win::RegKey hklm;
  ASSERT_TRUE(hkcu.Create(HKEY_CURRENT_USER, kHKCUReplacement, KEY_READ));
  ASSERT_TRUE(hklm.Create(HKEY_CURRENT_USER, kHKLMReplacement, KEY_READ));

  // And do the switcharoo.
  ASSERT_EQ(ERROR_SUCCESS,
    ::RegOverridePredefKey(HKEY_CURRENT_USER, hkcu.Handle()));
  ASSERT_EQ(ERROR_SUCCESS,
    ::RegOverridePredefKey(HKEY_LOCAL_MACHINE, hklm.Handle()));
}

void UndoOverrideRegistryHives() {
  // Undo the redirection.
  EXPECT_EQ(ERROR_SUCCESS, ::RegOverridePredefKey(HKEY_CURRENT_USER, NULL));
  EXPECT_EQ(ERROR_SUCCESS, ::RegOverridePredefKey(HKEY_LOCAL_MACHINE, NULL));
}

}  // namespace anonymous


void RlzLibTestNoMachineState::SetUp() {
  OverrideRegistryHives();
}

void RlzLibTestNoMachineState::TearDown() {
  UndoOverrideRegistryHives();
}

void RlzLibTestBase::SetUp() {
  RlzLibTestNoMachineState::SetUp();
  rlz_lib::CreateMachineState();
}
