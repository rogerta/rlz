// Copyright 2011 Google Inc. All Rights Reserved.
// Use of this source code is governed by an Apache-style license that can be
// found in the COPYING file.
//
// A library to manage RLZ information for access-points shared
// across different client applications.
//
// All functions return true on success and false on error.
// This implemenation is thread safe.
//
// Each prototype mentions the registry access requirements:
//
// HKLM read:  Will work from any process and at any privilege level on Vista.
// HKCU read:  Calls made from the SYSTEM account must pass the current user's
//             SID as the optional 'sid' param. Can be called from low integrity
//             process on Vista.
// HKCU write: Calls made from the SYSTEM account must pass the current user's
//             SID as the optional 'sid' param. Calls require at least medium
//             integrity on Vista (e.g. Toolbar will need to use their broker)
// HKLM write: Calls must be made from an account with admin rights. No SID
//             need be passed when running as SYSTEM.
// Functions which do not access registry will be marked with "no restrictions".

#ifndef RLZ_WIN_LIB_RLZ_LIB_H_
#define RLZ_WIN_LIB_RLZ_LIB_H_

#include "rlz/lib/rlz_lib.h"

#if defined(OS_WIN)
#include "base/win/registry.h"
#endif

namespace rlz_lib {

// The length of the Machine unique ID in WCHARs, excluding the NULL terminator.
const int kMachineIdLength = 50;


// TODO(thakis): Port these functions.
#if defined(OS_WIN)

// Clears all product-specifc state from the RLZ registry.
// Should be called during product uninstallation.
// This removes outstanding product events, product financial ping times,
// the product RLS argument (if any), and any RLZ's for access points being
// uninstalled with the product.
// access_points is an array terminated with NO_ACCESS_POINT.
// IMPORTANT: These are the access_points the product is removing as part
// of the uninstallation, not necessarily all the access points passed to
// SendFinancialPing() and GetPingParams().
// access_points can be NULL if no points are being uninstalled.
// No return value - this is best effort. Will assert in debug mode on
// failed attempts.
// Access: HKCU write.
void RLZ_LIB_API ClearProductState(Product product,
                                   const AccessPoint* access_points);

// Gets the unique ID for the machine used for RLZ tracking purposes. This ID
// is derived from the Windows machine SID, and is the string representation of
// a 20 byte hash + a 1 byte checksum.
// Included in financial pings with events, unless explicitly forbidden by the
// calling application.
// Access: HKLM read.
bool GetMachineId(char* buffer, int buffer_size);


// Initialize temporary HKLM/HKCU registry hives used for testing.
// Testing RLZ requires reading and writing to the Windows registry.  To keep
// the tests isolated from the machine's state, as well as to prevent the tests
// from causing side effects in the registry, HKCU and HKLM are overridden for
// the duration of the tests. RLZ tests don't expect the HKCU and KHLM hives to
// be empty though, and this function initializes the minimum value needed so
// that the test will run successfully.
//
// The two arguments to this function should be the keys that will represent
// the HKLM and HKCU registry hives during the tests.  This function should be
// called *before* the hives are overridden.
void InitializeTempHivesForTesting(const base::win::RegKey& temp_hklm_key,
                                   const base::win::RegKey& temp_hkcu_key);
#endif  // defined(OS_WIN)

}  // namespace rlz_lib

#endif  // RLZ_WIN_LIB_RLZ_LIB_H_
